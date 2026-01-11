// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <ESPAsyncWebServer.h>
#include <TaskSchedulerDeclarations.h>

class WebApiBatteryClass {
public:
    void init(AsyncWebServer& server, Scheduler& scheduler);

private:
    void onStatus(AsyncWebServerRequest* request);

    void onBatteryList(AsyncWebServerRequest* request);
    void onBatteryAdd(AsyncWebServerRequest* request);
    void onBatteryEdit(AsyncWebServerRequest* request);
    void onBatteryDelete(AsyncWebServerRequest* request);
    void onBatteryOrder(AsyncWebServerRequest* request);

    AsyncWebServer* _server;
};
