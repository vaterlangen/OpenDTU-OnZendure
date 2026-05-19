// SPDX-License-Identifier: GPL-2.0-or-later
#include <battery/Controller.h>
#include <battery/jbdbms/Provider.h>
#include <battery/jkbms/Provider.h>
#include <battery/mqtt/Provider.h>
#include <battery/pylontech/Provider.h>
#include <battery/pytes/Provider.h>
#include <battery/sbs/Provider.h>
#include <battery/victronsmartshunt/Provider.h>
#include <battery/zendure/LocalMqttProvider.h>
#include <battery/zendure/ZendureMqttProvider.h>
#include <Configuration.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "battery";
static const char* SUBTAG = "Controller";

Batteries::Controller Battery;

namespace Batteries {

std::shared_ptr<Stats const> Controller::getStats() const
{
    //DTU_LOGW("Useing deprectaed function Batteries::Controller::getStats()!");
    std::lock_guard<std::mutex> lock(_mutex);

    if (_batteries.empty()) {
        static auto sspDummyStats = std::make_shared<Stats>();
        return sspDummyStats;
    }

    return _batteries.front()->getStats();
}

std::shared_ptr<Stats const> Controller::getStatsByUid(const uint32_t uid) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_batteries.empty()) {
        for (const auto& battery : _batteries) {
            if (battery->getStats()->getBatteryUid() == uid) {
                return battery->getStats();
            }
        }
    }

    return nullptr;
}

void Controller::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.setCallback(std::bind(&Controller::loop, this));
    _loopTask.setIterations(TASK_FOREVER);
    _loopTask.enable();

    this->updateSettings();
}

void Controller::removeByUid(const uint32_t uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_batteries.empty()) { return; }

    for (auto& battery : _batteries) {
        if (battery->getStats()->getBatteryUid() != uid) { continue; }

        battery->deinit();
        _batteries.erase(std::remove(_batteries.begin(), _batteries.end(), battery), _batteries.end());

        DTU_LOGD("Battery with UID 0x%" PRIX32 " removed", uid);
        return;
    }
}

bool Controller::updateSettings(const uint32_t uid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (!_batteries.empty()) {
        for (auto& battery : _batteries) {
            if (battery->getStats()->getBatteryUid() != uid) { continue; }

            auto bat_cfg = battery->getStats()->getConfig();
            battery->deinit();

            if (bat_cfg.Enabled) {
                return battery->init();
            }

            _batteries.erase(std::remove(_batteries.begin(), _batteries.end(), battery), _batteries.end());
            return true;
        }
    }
    DTU_LOGE("Battery with UID 0x%" PRIX32 " not found", uid);
    return false;
}

void Controller::updateSettings()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (!_batteries.empty()) {
        for (auto& battery : _batteries) {
            battery->deinit();
        }
        _batteries.clear();
    }

    // Configure batteries
    for (uint8_t i = 0; i < BAT_MAX_COUNT; i++) {
        DTU_LOGD("Processing battery slot #%d", i);
        const auto& bat_cfg = Configuration.get().Batteries[i];
        if (!bat_cfg.Enabled || bat_cfg.Uid == 0U) { continue; }

        std::shared_ptr<Provider> _upProvider = nullptr;

        switch (bat_cfg.Provider) {
            case 0:
                _upProvider = std::make_shared<Pylontech::Provider>();
                break;
            case 1:
                _upProvider = std::make_shared<JkBms::Provider>();
                break;
            case 2:
                _upProvider = std::make_shared<Mqtt::Provider>();
                break;
            case 3:
                _upProvider = std::make_shared<VictronSmartShunt::Provider>();
                break;
            case 4:
                _upProvider = std::make_shared<Pytes::Provider>();
                break;
            case 5:
                _upProvider = std::make_shared<SBS::Provider>();
                break;
            case 6:
                _upProvider = std::make_shared<JbdBms::Provider>();
                break;
            case 7:
                DTU_LOGD("Enabling ZENDURE battery on slot #%d", i);
                switch (bat_cfg.Zendure->ConnectionType) {
                    case BatteryZendureConfig::ConnectionType::LocalMqtt:
                        _upProvider = std::make_shared<Zendure::LocalMqttProvider>();
                        break;
                    case BatteryZendureConfig::ConnectionType::ZendureMqtt:
                        _upProvider = std::make_shared<Zendure::ZendureMqttProvider>();
                        break;
                    default:
                        DTU_LOGE("Unknown Zendure connection type: %d", bat_cfg.Zendure->ConnectionType);
                        break;
                }
                break;
            default:
                DTU_LOGE("Unknown provider: %d", bat_cfg.Provider);
                break;
        }

        if (!_upProvider) { continue; }

        // assign battery config to provider stats
        //MUST be done before calling INIT!
        _upProvider->getStats()->setBatteryIndex(i);
        _upProvider->getStats()->setBatteryUid(bat_cfg.Uid);

        if (_upProvider->init()) {
            _batteries.push_back(_upProvider);
            DTU_LOGI("Added battery UID 0x%" PRIX32 " (slot #%d) with provider %d to controller", bat_cfg.Uid, i, bat_cfg.Provider);
        } else {
            DTU_LOGE("Failed to initialize battery provider %d", bat_cfg.Provider);
        }

        delay(500);
    }
}

void Controller::loop()
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_batteries.empty()) { return; }

    for (auto& battery : _batteries) {
        battery->loop();
        battery->getStats()->mqttLoop();

        auto spHassIntegration = battery->getHassIntegration();
        if (spHassIntegration) { spHassIntegration->hassLoop(); }
    }
}

void Controller::getLiveViewData(JsonVariant& root) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_batteries.empty()) { return; }

    auto array = root.to<JsonArray>();
    for (auto& battery : _batteries) {
        const auto& stats = battery->getStats();
        if (!stats || stats->getBatteryUid() == 0U) { continue; }

        JsonVariant bat = array.add<JsonObject>();
        stats->getLiveViewData(bat);
    }
}

bool Controller::updateAvailable(uint32_t since) const
{
    if (_batteries.empty()) { return false; }

    for (auto& battery : _batteries) {
        if (battery->getStats()->updateAvailable(since)) {
            return true;
        }
    }

    return false;
}

float Controller::getDischargeCurrentLimitByUid(const uint32_t uid)
{
    auto const& config = Configuration.get();
    auto batteryConfig = Configuration.getBatteryConfig(uid);
    auto spStats = getStatsByUid(uid);

    if (!batteryConfig || !spStats || !batteryConfig->EnableDischargeCurrentLimit) { return FLT_MAX; }

    /**
     * we are looking at two limits: (1) the static discharge current limit
     * setup by the user as part of the configuration, which is effective below
     * a (SoC or voltage) threshold, and (2) the dynamic discharge current
     * limit reported by the BMS.
     *
     * for both types of limits, we will determine its value, then test a bunch
     * of excuses why the limit might not be applicable.
     *
     * the smaller limit will be enforced, i.e., returned here.
     */


    auto getConfiguredLimit = [&batteryConfig, &config, &spStats]() -> float {
        auto configuredLimit = batteryConfig->DischargeCurrentLimit;
        if (configuredLimit <= 0.0f) { return FLT_MAX; } // invalid setting

        bool useSoC = spStats->getSoCAgeSeconds() <= 60 && !config.PowerLimiter.IgnoreSoc;

        if (useSoC) {
            auto threshold = batteryConfig->DischargeCurrentLimitBelowSoc;
            if (spStats->getSoC() >= threshold) { return FLT_MAX; }

            return configuredLimit;
        }

        bool voltageValid = spStats->getVoltageAgeSeconds() <= 60;
        if (voltageValid) {
            auto threshold = batteryConfig->DischargeCurrentLimitBelowVoltage;
            if (spStats->getVoltage() >= threshold) { return FLT_MAX; }
        }

        return configuredLimit;
    };

    auto getBatteryLimit = [&batteryConfig, &spStats]() -> float {
        if (!batteryConfig->UseBatteryReportedDischargeCurrentLimit) { return FLT_MAX; }

        if (spStats->getDischargeCurrentLimitAgeSeconds() > 60) { return FLT_MAX; } // unusable

        return spStats->getDischargeCurrentLimit();
    };

    return std::min(getConfiguredLimit(), getBatteryLimit());
}

} // namespace Batteries
