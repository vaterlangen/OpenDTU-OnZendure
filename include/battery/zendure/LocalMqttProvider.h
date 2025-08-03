// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <battery/zendure/Stats.h>
#include <battery/zendure/Provider.h>

namespace Batteries::Zendure {

class LocalMqttProvider : public ::Batteries::Zendure::Provider {
public:
    LocalMqttProvider();
    bool init() final;
    void deinit() final;

private:
    void shutdown() const final;
    void timesync() final;
    void writeSettings() final;
    void processPackDataJson(JsonVariantConst& packDataJson, const String& serial, const uint64_t timestamp) final;

    void onMqttMessageReport(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len);

    void onMqttMessageLog(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len);

    void onMqttMessageTimesync(espMqttClientTypes::MessageProperties const& properties,
            char const* topic, uint8_t const* payload, size_t len);

};

} // namespace Batteries::Zendure
