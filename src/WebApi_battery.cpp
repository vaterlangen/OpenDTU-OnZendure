// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022-2024 Thomas Basler and others
 */

#include "ArduinoJson.h"
#include "AsyncJson.h"
#include <battery/Controller.h>
#include "Configuration.h"
#include "MqttHandleHass.h"
#include "MqttHandlePowerLimiterHass.h"
#include "WebApi.h"
#include "WebApi_battery.h"
#include "WebApi_errors.h"
#include "helper.h"

void WebApiBatteryClass::init(AsyncWebServer& server, Scheduler& scheduler)
{
    using std::placeholders::_1;

    _server = &server;

    _server->on("/api/battery/status", HTTP_GET, std::bind(&WebApiBatteryClass::onStatus, this, _1));

    _server->on("/api/battery/list", HTTP_GET, std::bind(&WebApiBatteryClass::onBatteryList, this, _1));
    _server->on("/api/battery/add", HTTP_POST, std::bind(&WebApiBatteryClass::onBatteryAdd, this, _1));
    _server->on("/api/battery/edit", HTTP_POST, std::bind(&WebApiBatteryClass::onBatteryEdit, this, _1));
    _server->on("/api/battery/del", HTTP_POST, std::bind(&WebApiBatteryClass::onBatteryDelete, this, _1));
    _server->on("/api/battery/order", HTTP_POST, std::bind(&WebApiBatteryClass::onBatteryOrder, this, _1));
}

void WebApiBatteryClass::onBatteryList(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
            return;
        }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto& root = response->getRoot();
    JsonArray data = root["battery"].to<JsonArray>();

    const CONFIG_T& config = Configuration.get();

    for (uint8_t i = 0; i < BAT_MAX_COUNT; i++) {
        auto const& battery = config.Batteries[i];
        if (battery.Uid == 0) { continue; }
        JsonObject obj = data.add<JsonObject>();
        obj["id"] = i;

        ConfigurationClass::serializeBatteryConfig(battery, obj);
    }

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiBatteryClass::onBatteryAdd(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonDocument root;
    if (!WebApi.parseRequestData(request, response, root)) {
        return;
    }

    auto& retMsg = response->getRoot();

    if (!(root["provider"].is<uint8_t>() && root["name"].is<String>() && root["uid"].is<uint32_t>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["provider"].as<uint8_t>() > BAT_PROVIDER_MAX) {
        retMsg["message"] = "Invalid provider!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    auto uid = root["uid"].as<uint32_t>();
    if (uid == 0U || uid > 0x7FFFFFFFU) {
        retMsg["message"] = "UID must be a number between 1 and 0x7FFFFFFF!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["name"].as<String>().length() == 0 || root["name"].as<String>().length() > BAT_MAX_NAME_STRLEN) {
        retMsg["message"] = "Name must between 1 and " STR(BAT_MAX_NAME_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        retMsg["param"]["max"] = BAT_MAX_NAME_STRLEN;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (Configuration.getBatteryConfig(uid)) {
        retMsg["message"] = "UID already in use!";
        retMsg["code"] = WebApiError::GenericInternalServerError;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    auto battery = Configuration.getFreeBatterySlot();

    if (!battery) {
        retMsg["message"] = "Only " STR(BAT_MAX_COUNT) " batteries are supported!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        retMsg["param"]["max"] = BAT_MAX_COUNT;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    ConfigurationClass::deserializeBatteryConfig(root.as<JsonObject>(), *battery);

    WebApi.writeConfig(retMsg, WebApiError::GenericSuccess, "Battery created!");

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    MqttHandleHass.forceUpdate();
}

void WebApiBatteryClass::onBatteryEdit(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonDocument root;
    if (!WebApi.parseRequestData(request, response, root)) {
        return;
    }

    auto& retMsg = response->getRoot();

    if (!(root["id"].is<uint8_t>()
            && root["name"].is<String>()
            && root["provider"].is<uint8_t>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["id"].as<uint8_t>() > BAT_MAX_COUNT - 1) {
        retMsg["message"] = "Invalid ID specified!";
        retMsg["code"] = WebApiError::GenericNoValueFound;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["name"].as<String>().length() == 0 || root["name"].as<String>().length() > BAT_MAX_NAME_STRLEN) {
        retMsg["message"] = "Name must between 1 and " STR(BAT_MAX_NAME_STRLEN) " characters long!";
        retMsg["code"] = WebApiError::GenericDataTooLarge;
        retMsg["param"]["max"] = BAT_MAX_NAME_STRLEN;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    auto uid = 0U;
    {
        auto guard = Configuration.getWriteGuard();
        auto& config = guard.getConfig();

        auto& battery = config.Batteries[root["id"].as<uint8_t>()];
        uid = battery.Uid;

        ConfigurationClass::deserializeBatteryConfig(root.as<JsonObject>(), battery);

        // force UID to remain unchanged
        battery.Uid = uid;
    }

    // force complete update of battery in controller if partial update fails
    if (!Battery.updateSettings(uid)) {
        Battery.updateSettings();
    }

    WebApi.writeConfig(retMsg, WebApiError::GenericSuccess, "Battery updated!");
    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);


    // potentially make SoC thresholds auto-discoverable
    MqttHandlePowerLimiterHass.forceUpdate();
}

void WebApiBatteryClass::onBatteryDelete(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonDocument root;
    if (!WebApi.parseRequestData(request, response, root)) {
        return;
    }

    auto& retMsg = response->getRoot();

    if (!(root["id"].is<uint8_t>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    if (root["id"].as<uint8_t>() > BAT_MAX_COUNT - 1) {
        retMsg["message"] = "Invalid ID specified!";
        retMsg["code"] = WebApiError::GenericParseError;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    auto const battery_id = root["id"].as<uint8_t>();
    auto const battery_uid = Configuration.get().Batteries[battery_id].Uid;

    Configuration.deleteBatteryById(battery_id);
    Battery.removeByUid(battery_uid);

    WebApi.writeConfig(retMsg, WebApiError::GenericSuccess, "Battery deleted!");

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);

    MqttHandleHass.forceUpdate();
}

void WebApiBatteryClass::onBatteryOrder(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentials(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    JsonDocument root;
    if (!WebApi.parseRequestData(request, response, root)) {
        return;
    }

    auto& retMsg = response->getRoot();

    if (!(root["order"].is<JsonArray>())) {
        retMsg["message"] = "Values are missing!";
        retMsg["code"] = WebApiError::GenericValueMissing;
        WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
        return;
    }

    // The order array contains list or id in the right order
    JsonArray orderArray = root["order"].as<JsonArray>();
    uint8_t order = 0;
    {
        auto guard = Configuration.getWriteGuard();
        auto& config = guard.getConfig();

        for (JsonVariant id : orderArray) {
            uint8_t battery_id = id.as<uint8_t>();
            if (battery_id < BAT_MAX_COUNT) {
                auto& battery = config.Batteries[battery_id];
                battery.Order = order;
            }
            order++;
        }
    }

    WebApi.writeConfig(retMsg, WebApiError::GenericSuccess, "Battery order saved!");

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}

void WebApiBatteryClass::onStatus(AsyncWebServerRequest* request)
{
    if (!WebApi.checkCredentialsReadonly(request)) {
        return;
    }

    AsyncJsonResponse* response = new AsyncJsonResponse();
    auto root = response->getRoot().as<JsonObject>();
    auto const& config = Configuration.get();

    ConfigurationClass::serializeBatteryConfig(*(config.Battery), root);

    WebApi.sendJsonResponse(request, response, __FUNCTION__, __LINE__);
}
