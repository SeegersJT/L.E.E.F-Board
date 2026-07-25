#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include <Arduino.h>
#include "cloud/firebase_service.h"
#include <vector>
#include <map>
#include "hardware/device_wrapper.h"

class CommandManager
{
public:
    enum class CommandResult
    {
        PENDING,
        COMPLETED,
        FAILED
    };

    static void begin(DeviceWrapper<RelayDevice> &relay);
    static void maintain();

    static String enqueueSystemCommand();
    static CommandResult resultOf(const String &commandId);

    static String relayState();
    static const String &lastRelayTimestamp();

    static bool consumeRelayChanged();

private:
    struct QueuedCommand
    {
        String id;
        bool isSystem;
        String uid;
        String requestedAtLiteral;
        unsigned long enqueuedAtMillis;
    };

    static DeviceWrapper<RelayDevice> *relay;
    static bool streamStarted;

    static std::vector<QueuedCommand> queue;
    static std::map<String, CommandResult> results;

    static String activeCommandId;
    static bool activeIsSystem;
    static String activeUid;
    static String activeRequestedAtLiteral;
    static unsigned long activeStartedAt;

    static unsigned long cooldownUntil;
    static String lastRelayTimestampValue;

    static bool relayChanged;

    static void startStream();
    static void streamCallback(AsyncResult &aResult);
    static void reconcileCommands();
    static void considerCommand(const String &commandId, const String &commandJson);
    static void claimIncomingCommand(const String &commandId, const String &uid, const String &requestedAtLiteral);

    static void enqueue(const String &id, bool isSystem, const String &uid, const String &requestedAtLiteral);
    static void tryStartNext();
    static void startExecuting(const String &id, bool isSystem, const String &uid, const String &requestedAtLiteral);
    static void finishExecuting();

    static void writeStatus(const String &commandId, bool isSystem, const String &uid, const String &requestedAtLiteral, const String &status);

    static bool extractJsonValue(const String &json, const char *fieldName, String &out);
    static bool extractJsonNumber(const String &json, const char *fieldName, String &out);
    static int findMatchingBrace(const String &text, int openIndex);
};

#endif // COMMAND_MANAGER_H