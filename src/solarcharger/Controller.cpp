// SPDX-License-Identifier: GPL-2.0-or-later
#include <Configuration.h>
#include <MessageOutput.h>
#include <MqttSettings.h>
#include <solarcharger/Controller.h>
#include <solarcharger/DummyStats.h>
#include <solarcharger/victron/Provider.h>
#include <solarcharger/mqtt/Provider.h>
#include <solarcharger/integrated/Provider.h>

SolarChargers::Controller SolarCharger;

namespace SolarChargers {

void Controller::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&Controller::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    this->updateSettings();
}

void Controller::updateSettings()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_upProvider) {
        _upProvider->deinit();
        _upProvider = nullptr;
    }

    auto const& config = Configuration.get();
    if (!config.SolarCharger.Enabled) { return; }

    bool verboseLogging = config.SolarCharger.VerboseLogging;

    switch (config.SolarCharger.Provider) {
        case SolarChargerProviderType::VEDIRECT:
            _upProvider = std::make_unique<::SolarChargers::Victron::Provider>();
            break;
        case SolarChargerProviderType::MQTT:
            _upProvider = std::make_unique<::SolarChargers::Mqtt::Provider>();
            break;
        case SolarChargerProviderType::Integrated:
            _upProvider = std::make_unique<::SolarChargers::Integrated::Provider>();
            break;
        default:
            MessageOutput.printf("[SolarCharger] Unknown provider: %d\r\n", config.SolarCharger.Provider);
            return;
    }

    if (!_upProvider->init(verboseLogging)) { _upProvider = nullptr; }

    _forcePublishSensors = true;
}

std::shared_ptr<Stats const> Controller::getStats() const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) {
        static auto sspDummyStats = std::make_shared<DummyStats>();
        return sspDummyStats;
    }

    return _upProvider->getStats();
}

std::shared_ptr<Integrated::Stats> Controller::getIntegratedStats()
{
    auto const& config = Configuration.get();

    std::lock_guard<std::mutex> lock(_mutex);

    if (millis() < 20 * 1000) {
        MessageOutput.printf("[SolarCharger] Startup Delay\r\n");
        return nullptr;
    }

    if (!_upProvider || config.SolarCharger.Provider != SolarChargerProviderType::Integrated) {
        return nullptr;
    }

    MessageOutput.printf("[SolarCharger] getIntegratedStats() => SUCCESS\r\n");

    return std::reinterpret_pointer_cast<Integrated::Stats>(_upProvider->getStats());
}

void Controller::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_upProvider) { return; }

    _upProvider->loop();

    // TODO(schlimmchen): this cannot make sure that transient
    // connection problems are actually always noticed.
    if (!MqttSettings.getConnected()) {
        _forcePublishSensors = true;
        return;
    }

    _upProvider->getStats()->mqttLoop();

    auto const& config = Configuration.get();
    if (!config.Mqtt.Hass.Enabled) { return; }

    _upProvider->getStats()->mqttPublishSensors(_forcePublishSensors);

    _forcePublishSensors = false;
}

} // namespace SolarChargers
