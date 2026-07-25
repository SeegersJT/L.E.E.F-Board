#ifndef STATUS_REPORTER_H
#define STATUS_REPORTER_H

#include <Arduino.h>

class StatusReporter
{
public:
    static void pushStatus(
        int moisturePercentage,
        const String &moistureTimestamp,
        const String &relayState,
        const String &relayTimestamp,
        bool force = false);

    static void logMoistureReading(
        int moisturePercentage,
        const String &timestamp);

    static void logRelayEvent(
        const String &state,
        const String &timestamp,
        bool isSystemSource,
        const String &uid,
        const String &commandId);

private:
    static unsigned long lastPush;

    static String buildStatusPayload(
        int moisturePercentage,
        const String &moistureTimestamp,
        const String &relayState,
        const String &relayTimestamp);

    static void putHistoryEntry(
        const String &deviceKey,
        const String &timestamp,
        const String &payload);
};

#endif // STATUS_REPORTER_H