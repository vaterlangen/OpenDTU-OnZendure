// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <battery/Stats.h>
#include <memory>

namespace Batteries {
    static inline const String HASS_SENSOR_PREFIX = "sensor";
    static inline const String HASS_BINARY_SENSOR_PREFIX = "binary_sensor";

class HassIntegration {
public:
    explicit HassIntegration(std::shared_ptr<Stats> spStats);

    void hassLoop();

protected:
    void publish(const String& subtopic, const String& payload) const;
    void publishBinarySensor(const char* caption,
            const char* icon, const char* subTopic,
            const char* payload_on, const char* payload_off,
            const bool enabled = true) const;
    void publishSensor(const char* caption, const char* icon,
            const char* subTopic, const char* deviceClass = nullptr,
            const char* stateClass = nullptr,
            const char* unitOfMeasurement = nullptr,
            const bool enabled = true) const;
    void createDeviceInfo(JsonObject& object) const;

    void removeSensor(const char* caption) const {
        remove(caption, HASS_SENSOR_PREFIX);
    };
    void removeBinarySensor(const char* caption) const {
        remove(caption, HASS_BINARY_SENSOR_PREFIX);
    };

    virtual void publishSensors() const;

private:
    static String sanitizeUniqueId(const char* value);
    String createConfigTopic(const String& sensorId, const String& type) const {
        return type + "/dtu_battery_" + _serial + "/" + sensorId + "/config";
    };
    void remove(const char* caption, const String& type) const {
        String configTopic = createConfigTopic(sanitizeUniqueId(caption), type);
        publish(configTopic, "");
    };

    String _serial = "0001"; // pseudo-serial, can be replaced in future with real serialnumber
    std::shared_ptr<Stats> _spStats = nullptr;

    bool _publishSensors = true;
};

} // namespace Batteries
