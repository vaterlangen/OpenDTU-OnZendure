// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */
#include "MqttHandleBatteryTotal.h"
#include "Configuration.h"
#include "Datastore.h"
#include "MqttSettings.h"
#include <Hoymiles.h>

MqttHandleBatteryTotalClass MqttHandleBatteryTotal;

MqttHandleBatteryTotalClass::MqttHandleBatteryTotalClass()
    : _loopTask(TASK_IMMEDIATE, TASK_FOREVER, std::bind(&MqttHandleBatteryTotalClass::loop, this))
{
}

void MqttHandleBatteryTotalClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);
    _loopTask.enable();
}

void MqttHandleBatteryTotalClass::loop()
{
    // Update interval from config
    _loopTask.setInterval(Configuration.get().Mqtt.PublishInterval * TASK_SECOND);

    if (!MqttSettings.getConnected() || !Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
        return;
    }

    MqttSettings.publish("bat/stateOfCharge", String(Datastore.getTotalBatteryStateOfCharge(), Datastore.getTotalBatteryStateOfChargeDigits()));
    MqttSettings.publish("bat/stateOfUse", String(Datastore.getTotalBatteryUseableStateOfCharge(), Datastore.getTotalBatteryUseableStateOfChargeDigits()));
    MqttSettings.publish("bat/installedCapacity", String(Datastore.getTotalBatteryInstalledCapacity()));
    MqttSettings.publish("bat/availableCapacity", String(Datastore.getTotalBatteryAvailableCapacity()));
    MqttSettings.publish("bat/useableCapacity", String(Datastore.getTotalBatteryUsableCapacity()));
    MqttSettings.publish("bat/storedEnergy", String(Datastore.getTotalBatteryStoredEnergy()));
    MqttSettings.publish("bat/power", String(Datastore.getTotalBatteryPower(), Datastore.getTotalBatteryPowerDigits()));
}
