#include "cloud/status_reporter.h"
#include "cloud/firebase_service.h"
#include "cloud/ota_manager.h"
#include "core/globals.h"
#include "core/time_utils.h"
#include "core/logger.h"
#include <WiFi.h>

unsigned long StatusReporter::lastPush = 0;

void StatusReporter::pushStatus(
    int moisturePercentage,
    const String &moistureTimestamp,
    const String &relayState,
    const String &relayTimestamp,
    bool force)
{
    if (WiFi.status() != WL_CONNECTED || !FirebaseService::ready())
        return;

    unsigned long currentMillis = millis();

    if (!force && lastPush != 0 && currentMillis - lastPush < (unsigned long)config["FIREBASE_PUSH_INTERVAL"])
    {
        return;
    }

    lastPush = currentMillis;

    String payload = buildStatusPayload(moisturePercentage, moistureTimestamp, relayState, relayTimestamp);
    String path = "/devices/" + FirebaseService::deviceId() + "/status";

    if (FirebaseService::put(path, payload))
    {
        Logger::log(LogCategory::LOG_FIREBASE, "Push OK");
    }
}

void StatusReporter::logMoistureReading(int moisturePercentage, const String &timestamp)
{
    if (timestamp.length() == 0 || WiFi.status() != WL_CONNECTED || !FirebaseService::ready())
        return;

    String payload = "{";
    payload += "\"reading\":" + String(moisturePercentage) + ",";
    payload += "\"unit\":\"percent\"";
    payload += "}";

    putHistoryEntry("MOISTURE_SENSOR_PIN_MM01", timestamp, payload);
}

void StatusReporter::logRelayEvent(const String &state, const String &timestamp, bool isSystemSource, const String &uid, const String &commandId)
{
    if (timestamp.length() == 0 || WiFi.status() != WL_CONNECTED || !FirebaseService::ready())
    {
        return;
    }

    String payload = "{";
    payload += "\"state\":\"" + state + "\",";
    payload += "\"source\":{";
    payload += "\"type\":\"" + String(isSystemSource ? "auto" : "user") + "\"";

    if (!isSystemSource)
    {
        payload += ",\"uid\":\"" + uid + "\"";
    }

    payload += ",\"commandId\":\"" + commandId + "\"";
    payload += "}";
    payload += "}";

    putHistoryEntry("RELAY_PIN_R01", timestamp, payload);
}

String StatusReporter::buildStatusPayload(int moisturePercentage, const String &moistureTimestamp, const String &relayState, const String &relayTimestamp)
{
    String payload = "{";

    payload += "\"MOISTURE_SENSOR_PIN_MM01\":{";
    payload += "\"label\":\"MOISTURE SENSOR 01\",";
    payload += "\"pin\":" + String((int)config["MOISTURE_SENSOR_PIN_MM01"]) + ",";
    payload += "\"reading\":" + String(moisturePercentage) + ",";
    payload += "\"unit\":\"percent\",";
    payload += "\"timestamp\":\"" + moistureTimestamp + "\",";
    payload += "\"intervalMinutes\":" + String((int)config["MOISTURE_SENSORS_INTERVAL_MINUTES"]);
    payload += "},";

    payload += "\"RELAY_PIN_R01\":{";
    payload += "\"label\":\"RELAY 01\",";
    payload += "\"pin\":" + String((int)config["RELAY_PIN_R01"]) + ",";
    payload += "\"state\":\"" + relayState + "\",";
    payload += "\"timestamp\":\"" + relayTimestamp + "\",";
    payload += "\"onDurationMs\":" + String((int)config["RELAY_ON_DURATION"]);
    payload += "},";

    payload += "\"appliedFirmwareDate\":\"" + config.str["APPLIED_FIRMWARE_DATE"] + "\",";
    payload += "\"lastOtaResult\":\"" + OtaManager::lastResult() + "\",";
    payload += "\"lastOtaCheckTime\":\"" + OtaManager::lastCheckTime() + "\",";
    payload += "\"lastSeen\":\"" + currentIsoTimestamp() + "\"";
    payload += "}";

    return payload;
}

void StatusReporter::putHistoryEntry(const String &deviceKey, const String &timestamp, const String &payload)
{
    String path = "/devices/" + FirebaseService::deviceId() + "/history/" + deviceKey + "/" + timestamp;

    if (FirebaseService::put(path, payload))
    {
        Logger::log(LogCategory::LOG_FIREBASE, "History entry logged: " + deviceKey);
    }
}