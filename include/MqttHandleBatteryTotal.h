// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <TaskSchedulerDeclarations.h>

class MqttHandleBatteryTotalClass {
public:
    MqttHandleBatteryTotalClass();
    void init(Scheduler& scheduler);

private:
    void loop();

    Task _loopTask;
};

extern MqttHandleBatteryTotalClass MqttHandleBatteryTotal;
