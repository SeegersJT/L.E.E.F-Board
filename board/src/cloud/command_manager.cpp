#include "cloud/command_manager.h"
#include "cloud/firebase_service.h"
#include "cloud/status_reporter.h"
#include "core/globals.h"
#include "core/time_utils.h"
#include "core/logger.h"
#include <WiFi.h>

DeviceWrapper<RelayDevice> *CommandManager::relay = nullptr;
bool CommandManager::streamStarted = false;

std::vector<CommandManager::QueuedCommand> CommandManager::queue;
std::map<String, CommandManager::CommandResult> CommandManager::results;

String CommandManager::activeCommandId = "";
bool CommandManager::activeIsSystem = false;
String CommandManager::activeUid = "";
String CommandManager::activeRequestedAtLiteral = "";
unsigned long CommandManager::activeStartedAt = 0;

unsigned long CommandManager::cooldownUntil = 0;
String CommandManager::lastRelayTimestampValue = "";

bool CommandManager::relayChanged = false;

void CommandManager::begin(DeviceWrapper<RelayDevice> &relayIn)
{
    relay = &relayIn;
}

String CommandManager::relayState()
{
    return relay ? relay->getState() : "OFF";
}

const String &CommandManager::lastRelayTimestamp()
{
    return lastRelayTimestampValue;
}

CommandManager::CommandResult CommandManager::resultOf(const String &commandId)
{
    auto it = results.find(commandId);

    if (it == results.end())
    {
        return CommandResult::PENDING;
    }

    return it->second;
}

String CommandManager::enqueueSystemCommand()
{
    if (!FirebaseService::ready())
    {
        return "";
    }

    String commandId = currentIsoTimestamp();

    if (commandId.length() == 0)
    {
        return "";
    }

    if (activeCommandId.length() > 0 || millis() < cooldownUntil)
    {
        enqueue(commandId, true, "", "");
        writeStatus(commandId, true, "", "", "queued");
    }
    else
    {
        startExecuting(commandId, true, "", "");
    }

    return commandId;
}

void CommandManager::maintain()
{
    if (WiFi.status() != WL_CONNECTED || !FirebaseService::ready())
    {
        return;
    }

    if (!streamStarted)
    {
        startStream();
        return;
    }

    unsigned long currentMillis = millis();

    if (activeCommandId.length() > 0)
    {
        if (currentMillis - activeStartedAt >= (unsigned long)config["RELAY_ON_DURATION"])
        {
            finishExecuting();
        }

        return;
    }

    if (currentMillis < cooldownUntil)
    {
        return;
    }

    tryStartNext();
}

void CommandManager::startStream()
{
    String path = "/devices/" + FirebaseService::deviceId() + "/commands";

    FirebaseService::database().setSSEFilters("put,patch");
    FirebaseService::database().get(FirebaseService::streamClient(), path, streamCallback, true, "commandStream");

    streamStarted = true;

    Logger::log(LogCategory::LOG_FIREBASE, "Command stream started");
}

void CommandManager::streamCallback(AsyncResult &aResult)
{
    if (!aResult.isResult())
    {
        return;
    }

    if (aResult.isError())
    {
        Logger::log(LogCategory::LOG_FIREBASE, "Command stream error: " + aResult.error().message());
        return;
    }

    if (aResult.available())
    {
        reconcileCommands();
    }
}

void CommandManager::reconcileCommands()
{
    String response;
    String path = "/devices/" + FirebaseService::deviceId() + "/commands";

    if (!FirebaseService::get(path, response))
    {
        return;
    }

    response.trim();
    if (response == "null" || response.length() == 0)
    {
        return;
    }

    int pos = 0;

    while (true)
    {
        int keyStart = response.indexOf("\"", pos);

        if (keyStart == -1)
        {
            break;
        }

        int keyEnd = response.indexOf("\"", keyStart + 1);

        if (keyEnd == -1)
        {
            break;
        }

        String commandId = response.substring(keyStart + 1, keyEnd);

        int objStart = response.indexOf("{", keyEnd);

        if (objStart == -1)
        {
            break;
        }

        int objEnd = findMatchingBrace(response, objStart);

        if (objEnd == -1)
        {
            break;
        }

        String commandJson = response.substring(objStart, objEnd + 1);
        considerCommand(commandId, commandJson);

        pos = objEnd + 1;
    }
}

void CommandManager::considerCommand(const String &commandId, const String &commandJson)
{
    String status;
    if (!extractJsonValue(commandJson, "status", status))
    {
        return;
    }

    if (status != "pending")
    {
        return;
    }

    String kind, uid, requestedAtLiteral;
    extractJsonValue(commandJson, "kind", kind);
    extractJsonValue(commandJson, "uid", uid);
    extractJsonNumber(commandJson, "requestedAt", requestedAtLiteral);

    if (kind != "user")
    {
        return;
    }

    claimIncomingCommand(commandId, uid, requestedAtLiteral);
}

void CommandManager::claimIncomingCommand(const String &commandId, const String &uid, const String &requestedAtLiteral)
{
    writeStatus(commandId, false, uid, requestedAtLiteral, "received");

    if (activeCommandId.length() > 0 || millis() < cooldownUntil)
    {
        enqueue(commandId, false, uid, requestedAtLiteral);
        writeStatus(commandId, false, uid, requestedAtLiteral, "queued");
    }
    else
    {
        startExecuting(commandId, false, uid, requestedAtLiteral);
    }
}

void CommandManager::enqueue(const String &id, bool isSystem, const String &uid, const String &requestedAtLiteral)
{
    QueuedCommand q;

    q.id = id;
    q.isSystem = isSystem;
    q.uid = uid;
    q.requestedAtLiteral = requestedAtLiteral;
    q.enqueuedAtMillis = millis();

    queue.push_back(q);
}

void CommandManager::tryStartNext()
{
    while (!queue.empty())
    {
        QueuedCommand next = queue.front();
        queue.erase(queue.begin());

        unsigned long age = millis() - next.enqueuedAtMillis;
        if (age > (unsigned long)config["COMMAND_EXPIRY_MS"])
        {
            writeStatus(next.id, next.isSystem, next.uid, next.requestedAtLiteral, "expired");
            results[next.id] = CommandResult::FAILED;
            continue;
        }

        startExecuting(next.id, next.isSystem, next.uid, next.requestedAtLiteral);
        return;
    }
}

void CommandManager::startExecuting(const String &id, bool isSystem, const String &uid, const String &requestedAtLiteral)
{
    activeCommandId = id;
    activeIsSystem = isSystem;
    activeUid = uid;
    activeRequestedAtLiteral = requestedAtLiteral;
    activeStartedAt = millis();

    relay->setState("ON");
    String timestamp = currentIsoTimestamp();
    lastRelayTimestampValue = timestamp;
    relayChanged = true;

    StatusReporter::logRelayEvent("ON", timestamp, isSystem, uid, id);
    writeStatus(id, isSystem, uid, requestedAtLiteral, "executing");

    if (!isSystem)
    {
        display("Watering").clear().print();
        display("requested in app").bottom().print();
    }
}

void CommandManager::finishExecuting()
{
    relay->setState("OFF");
    String timestamp = currentIsoTimestamp();
    lastRelayTimestampValue = timestamp;
    relayChanged = true;

    StatusReporter::logRelayEvent("OFF", timestamp, activeIsSystem, activeUid, activeCommandId);
    writeStatus(activeCommandId, activeIsSystem, activeUid, activeRequestedAtLiteral, "completed");

    results[activeCommandId] = CommandResult::COMPLETED;

    activeCommandId = "";
    cooldownUntil = millis() + (unsigned long)config["COMMAND_COOLDOWN"];
}

void CommandManager::writeStatus(const String &commandId, bool isSystem, const String &uid, const String &requestedAtLiteral, const String &status)
{
    String payload = "{";
    payload += "\"type\":\"WATER_NOW\",";
    payload += "\"requestedBy\":{";
    payload += "\"kind\":\"" + String(isSystem ? "system" : "user") + "\",";
    payload += isSystem ? "\"uid\":null" : ("\"uid\":\"" + uid + "\"");
    payload += "},";
    payload += "\"requestedAt\":" + (requestedAtLiteral.length() > 0 ? requestedAtLiteral : String("{\".sv\":\"timestamp\"}")) + ",";
    payload += "\"status\":\"" + status + "\",";
    payload += "\"statusUpdatedAt\":{\".sv\":\"timestamp\"}";
    payload += "}";

    String path = "/devices/" + FirebaseService::deviceId() + "/commands/" + commandId;
    FirebaseService::put(path, payload);
}

bool CommandManager::extractJsonValue(const String &json, const char *fieldName, String &out)
{
    String searchKey = String("\"") + fieldName + "\"";

    int start = json.indexOf(searchKey);

    if (start == -1)
    {
        return false;
    }

    start += searchKey.length();

    while (start < (int)json.length() && (json[start] == ' ' || json[start] == ':'))
    {
        start++;
    }

    if (start >= (int)json.length() || json[start] != '"')
    {
        return false;
    }

    start++;

    int end = json.indexOf('"', start);

    if (end == -1)
    {
        return false;
    }

    out = json.substring(start, end);

    return true;
}

bool CommandManager::extractJsonNumber(const String &json, const char *fieldName, String &out)
{
    String searchKey = String("\"") + fieldName + "\"";

    int start = json.indexOf(searchKey);

    if (start == -1)
    {
        return false;
    }

    start += searchKey.length();

    while (start < (int)json.length() && (json[start] == ' ' || json[start] == ':'))
    {
        start++;
    }

    int end = start;

    while (end < (int)json.length() && (isDigit(json[end]) || json[end] == '-'))
    {
        end++;
    }

    if (end == start)
    {
        return false;
    }

    out = json.substring(start, end);

    return true;
}

int CommandManager::findMatchingBrace(const String &text, int openIndex)
{
    int depth = 0;
    for (int i = openIndex; i < (int)text.length(); i++)
    {
        if (text[i] == '{')
        {
            depth++;
        }
        else if (text[i] == '}')
        {
            depth--;
            if (depth == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

bool CommandManager::consumeRelayChanged()
{
    bool value = relayChanged;
    relayChanged = false;
    return value;
}