#ifndef WATERING_CONTROLLER_H
#define WATERING_CONTROLLER_H

#include <Arduino.h>
#include "hardware/device_wrapper.h"

class WateringController
{
public:
    explicit WateringController(DeviceWrapper<MoistureDevice> &moisture);

    void tick();

    int lastMoisturePercentage() const;
    const String &lastMoistureTimestamp() const;

    bool consumeMoistureChanged();

private:
    DeviceWrapper<MoistureDevice> &moisture;

    WateringState state;
    int pulseCount;
    int lastMoisture;
    String moistureTimestampValue;

    bool moistureChanged;

    String activeCommandId;
    unsigned long settleStartedAt;

    void handleIdle(unsigned long currentMillis);
    void handlePulseOn();
    void handlePulseSettle(unsigned long currentMillis);

    void takeReading();
};

#endif // WATERING_CONTROLLER_H