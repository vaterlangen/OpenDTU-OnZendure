// SPDX-License-Identifier: GPL-2.0-or-later
#include <limits>
#include <battery/Stats.h>
#include <Configuration.h>
#include <MqttSettings.h>

namespace Batteries {

void Stats::setManufacturer(const String& m)
{
    String sanitized(m);
    for (int i = 0; i < sanitized.length(); i++) {
        char c = sanitized[i];
        if (c < 0x20 || c >= 0x80) {
            sanitized.remove(i); // Truncate string
            break;
        }
    }
    _oManufacturer = std::move(sanitized);
}

bool Stats::updateAvailable(uint32_t since) const
{
    if (_lastUpdate == 0) { return false; } // no data at all processed yet

    auto constexpr halfOfAllMillis = std::numeric_limits<uint32_t>::max() / 2;
    return (_lastUpdate - since) < halfOfAllMillis;
}

void Stats::getLiveViewData(JsonVariant& root) const
{
    root["manufacturer"] = _oManufacturer.value_or("unknown");
    if (_serial.has_value()) {
        root["serial"] = *_serial;
    }
    if (!_fwversion.isEmpty()) {
        root["fwversion"] = _fwversion;
    }
    if (!_hwversion.isEmpty()) {
        root["hwversion"] = _hwversion;
    }

    root["data_age_ms"] = getAgeMilliSeconds();
    root["max_age"] = 20;
    root["enabled"] = true;
    root["poll_enabled"] = isReachable() || !isSleeping();
    root["reachable"] = isReachable();
    root["producing"] = isProducing();
    root["limit_absolute"] = getLimit();

    if (isSoCValid()) {
        addLiveViewValue(root, "SoC", _soc, "%", _socPrecision);
    }

    if (isVoltageValid()) {
        addLiveViewValue(root, "voltage", _voltage, "V", 2);
    }

    if (isCurrentValid()) {
        addLiveViewValue(root, "current", _current, "A", _currentPrecision);
    }

    if (isPowerValid()) {
        addLiveViewValue(root, "power", getPower(), "W", 2);
    }

    if (isDischargeCurrentLimitValid()) {
        addLiveViewValue(root, "dischargeCurrentLimitation", _dischargeCurrentLimit, "A", 1);
    }

    if (isChargeCurrentLimitValid()) {
        addLiveViewValue(root, "chargeCurrentLimitation", _chargeCurrentLimit, "A", 1);
    }

    String name = getName();
    if (name.length() > 0) {
        root["name"] = name;
    }

    root["id"] = getBatteryIndex();
    root["uid"] = getBatteryUid();

    root["showIssues"] = supportsAlarmsAndWarnings();
}

void Stats::mqttLoop()
{
    auto& config = Configuration.get();

    if (!MqttSettings.getConnected()
            || (millis() - _lastMqttPublish) < (config.Mqtt.PublishInterval * 1000)) {
        return;
    }

    mqttPublish();

    _lastMqttPublish = millis();
}

uint32_t Stats::getMqttFullPublishIntervalMs() const
{
    auto& config = Configuration.get();

    // this is the default interval, see mqttLoop(). mqttPublish()
    // implementations in derived classes may choose to publish some values
    // with a lower frequency and hence implement this method with a different
    // return value.
    return config.Mqtt.PublishInterval * 1000;
}

void Stats::mqttPublish() const
{
    if (_oManufacturer.has_value()) {
        publish("manufacturer", *_oManufacturer);
    }

    String name = getName();
    if (name.length() > 0) {
        publish("name", name);
    }

    publish("dataAge", String(getAgeSeconds()));

    if (isSoCValid()) {
        publish("stateOfCharge", String(_soc));
    }

    if (isVoltageValid()) {
        publish("voltage", String(_voltage));
    }

    if (isCurrentValid()) {
        publish("current", String(_current));
    }

    if (isDischargeCurrentLimitValid()) {
        publish("settings/dischargeCurrentLimitation", String(_dischargeCurrentLimit));
    }

    if (isChargeCurrentLimitValid()) {
        publish("settings/chargeCurrentLimitation", String(_chargeCurrentLimit));
    }
}

} // namespace Batteries
