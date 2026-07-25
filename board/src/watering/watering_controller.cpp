#include "watering/watering_controller.h"
#include "cloud/status_reporter.h"
#include "cloud/command_manager.h"
#include "core/globals.h"
#include "core/time_utils.h"

WateringController::WateringController(DeviceWrapper<MoistureDevice> &moisture)
    : moisture(moisture),
      state(WateringState::IDLE),
      pulseCount(0),
      lastMoisture(-1),
      settleStartedAt(0),
      moistureChanged(false) {}
int WateringController::lastMoisturePercentage() const
{
    return lastMoisture;
}

const String &WateringController::lastMoistureTimestamp() const
{
    return moistureTimestampValue;
}

void WateringController::tick()
{
    unsigned long currentMillis = millis();

    switch (state)
    {
    case WateringState::IDLE:
        handleIdle(currentMillis);
        break;
    case WateringState::PULSE_ON:
        handlePulseOn();
        break;
    case WateringState::PULSE_SETTLE:
        handlePulseSettle(currentMillis);
        break;
    }
}

void WateringController::takeReading()
{
    int moisturePercentage = moisture.getMoisturePercentage();

    lastMoisture = moisturePercentage;
    moistureTimestampValue = currentIsoTimestamp();
    moistureChanged = true;

    StatusReporter::logMoistureReading(moisturePercentage, moistureTimestampValue);

    display("Moisture: " + String(moisturePercentage) + "%").clear().print();
}

void WateringController::handleIdle(unsigned long currentMillis)
{
    if (currentMillis - moisture.getTimestamp() < (unsigned long)(config["MOISTURE_SENSORS_INTERVAL_MINUTES"] * 60 * 1000))
    {
        return;
    }

    takeReading();

    if (lastMoisture < config["ACTIVATE_RELAY_THRESHOLD"])
    {
        pulseCount = 0;
        activeCommandId = CommandManager::enqueueSystemCommand();

        if (activeCommandId.length() > 0)
        {
            display("Watering... (1)").bottom().print();
            state = WateringState::PULSE_ON;
        }
    }
    else
    {
        display("Moisture OK").bottom().print();
    }
}

void WateringController::handlePulseOn()
{
    CommandManager::CommandResult result = CommandManager::resultOf(activeCommandId);

    if (result == CommandManager::CommandResult::PENDING)
    {
        return;
    }

    activeCommandId = "";

    if (result == CommandManager::CommandResult::COMPLETED)
    {
        pulseCount++;
        settleStartedAt = millis();
        state = WateringState::PULSE_SETTLE;
    }
    else
    {
        state = WateringState::IDLE;
    }
}

void WateringController::handlePulseSettle(unsigned long currentMillis)
{
    if (currentMillis - settleStartedAt < (unsigned long)config["PULSE_RECHECK_DELAY"])
    {
        return;
    }

    takeReading();

    if (lastMoisture >= config["ACTIVATE_RELAY_THRESHOLD"])
    {
        display("Watering done").bottom().print();
        state = WateringState::IDLE;
    }
    else if (pulseCount >= config["MAX_WATERING_PULSES"])
    {
        display("Pulse limit hit").bottom().print();
        state = WateringState::IDLE;
    }
    else
    {
        activeCommandId = CommandManager::enqueueSystemCommand();

        if (activeCommandId.length() > 0)
        {
            display("Watering... (" + String(pulseCount + 1) + ")").bottom().print();
            state = WateringState::PULSE_ON;
        }
        else
        {
            state = WateringState::IDLE;
        }
    }
}

bool WateringController::consumeMoistureChanged()
{
    bool value = moistureChanged;
    moistureChanged = false;
    return value;
}