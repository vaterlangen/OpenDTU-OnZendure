// SPDX-License-Identifier: GPL-2.0-or-later
#include <MqttSettings.h>
#include <battery/zendure/Stats.h>
#include <Utils.h>

namespace Batteries::Zendure {

void Stats::getLiveViewData(JsonVariant& root) const
{
    ::Batteries::Stats::getLiveViewData(root);

    auto const& config = getConfig();

    // set maximum age for aging within UI
    root["max_age"] = 90;

    // values go into the "Status" card of the web application
    std::string section("status");
    addLiveViewInSection(root, section, "totalInputPower", getInputPower().value_or(0), "W", 0);
    addLiveViewInSection(root, section, "chargePower", _charge_power, "W", 0);
    addLiveViewInSection(root, section, "dischargePower", _discharge_power, "W", 0);
    addLiveViewInSection(root, section, "totalOutputPower", _output_power, "W", 0);
    addLiveViewInSection(root, section, "outputVoltage", _output_voltage, "V", 2);
    addLiveViewInSection(root, section, "efficiency", _efficiency, "%", 3);
    addLiveViewInSection(root, section, "batteries", _num_batteries, "", 0);
    addLiveViewInSection(root, section, "capacity", _capacity, "Wh", 0, false);
    addLiveViewInSection(root, section, "availableCapacity", _capacity_avail, "Wh", 0, true);
    addLiveViewInSection(root, section, "useableCapacity", getUseableCapacity(), "Wh", 0, false);
    addLiveViewTextInSection(root, section, "zendure.mainState", "zendure.mainStates." + std::string(stateToString(_state)));
    addLiveViewTextInSection(root, section, "zendure.controlState", "zendure.controlStates." + std::string(controlStateToString(_controlState)));
    addLiveViewBooleanInSection(root, section, "heatState", _heat_state);
    addLiveViewBooleanInSection(root, section, "bypassState", _bypass_state);
    addLiveViewTextInSection(root, section, "zendure.chargeThroughState", "zendure.chargeThroughStates." + std::string(chargeThroughStateToString(_charge_through_state)));
    addLiveViewInSection(root, section, "lastFullCharge", _last_full_hours, "h", 0);
    addLiveViewInSection(root, section, "lastEmptyCharge", _last_empty_hours, "h", 0);
    addLiveViewInSection(root, section, "remainOutTime", _remain_out_time, "min", 0);
    addLiveViewInSection(root, section, "remainInTime", _remain_in_time, "min", 0);

    // values go into the "Settings" card of the web application
    section = "settings";
    if (config.Zendure.ConnectionType != BatteryZendureConfig::ConnectionType::ZendureMqtt) {
        addLiveViewTextInSection(root, section, "controlMode", std::string(controlModeToString(config.Zendure.ControlMode)));
        addLiveViewBooleanInSection(root, section, "zendure.batteryProtection", config.Zendure.BatteryProtectionEnable);
        addLiveViewInSection(root, section, "zendure.batteryProtectionHysteresis", config.Zendure.MinSoCHysteresis, "%", 1);
    }
    addLiveViewInSection(root, section, "maxInversePower", _inverse_max, "W", 0);
    addLiveViewInSection(root, section, "outputLimit", _output_limit, "W", 0);
    addLiveViewInSection(root, section, "inputLimit", _output_limit, "W", 0);
    addLiveViewInSection(root, section, "minSoC", _soc_min, "%", 1);
    addLiveViewInSection(root, section, "maxSoC", _soc_max, "%", 1);
    addLiveViewBooleanInSection(root, section, "autoRecover", _auto_recover);
    addLiveViewBooleanInSection(root, section, "autoShutdown", _auto_shutdown);
    addLiveViewTextInSection(root, section, "bypassMode", std::string(bypassModeToString(_bypass_mode)));
    addLiveViewBooleanInSection(root, section, "buzzer", _buzzer);

    // pack data goes to dedicated cards of the web application
    char buff[30];
    for (const auto& [index, value] : _packData) {
        snprintf(buff, sizeof(buff), "_%s [%s]", value->getName().c_str(), value->getSerial().c_str());
        section = std::string(buff);
        if (value->_state.has_value()) {
            addLiveViewTextInSection(root, section, "zendure.mainState", "zendure.mainStates." + std::string(stateToString(value->_state)));
        }
        addLiveViewInSection(root, section, "cellMinVoltage", value->_cell_voltage_min, "mV", 0, false);
        addLiveViewInSection(root, section, "cellAvgVoltage", value->_cell_voltage_avg, "mV", 0, true);
        addLiveViewInSection(root, section, "cellMaxVoltage", value->_cell_voltage_max, "mV", 0, false);
        addLiveViewInSection(root, section, "cellDiffVoltage", value->_cell_voltage_spread, "mV", 0, false);
        addLiveViewInSection(root, section, "cellMaxTemperature", value->_cell_temperature_max, "°C", 1, false);
        addLiveViewInSection(root, section, "voltage", value->_voltage_total, "V", 2, true);
        addLiveViewInSection(root, section, "power", value->_power, "W", 0, true);
        addLiveViewInSection(root, section, "current", value->_current, "A", 2, true);
        addLiveViewInSection(root, section, "SoC", value->_soc_level, "%", 1, false);
        addLiveViewInSection(root, section, "stateOfHealth", value->_state_of_health, "%", 1, true);
        addLiveViewInSection(root, section, "capacity", value->_capacity, "Wh", 0, false);
        addLiveViewInSection(root, section, "availableCapacity", value->_capacity_avail, "Wh", 0, true);
        if (!_fwversion.isEmpty()) {
            addLiveViewTextInSection(root, section, "FwVersion", std::string(value->_fwversion.c_str()), false);
        }
    }
}

void Stats::mqttPublish() const
{
    ::Batteries::Stats::mqttPublish();

    auto const& config = getConfig();

    auto boolToString = [](const std::optional<bool> value) -> std::optional<String> {
        if (value.has_value()) {
            return String(static_cast<uint8_t>(*value));
        }
        return std::nullopt;
    };

    publish("cellMinMilliVolt", _cellMinMilliVolt);
    publish("cellAvgMilliVolt", _cellAvgMilliVolt);
    publish("cellMaxMilliVolt", _cellMaxMilliVolt);
    publish("cellDiffMilliVolt", _cellDeltaMilliVolt);
    publish("cellMaxTemperature", _cellTemperature);
    publish("chargePower", _charge_power);
    publish("dischargePower", _discharge_power);
    publish("heating", boolToString(_heat_state));
    publish("state", String(stateToString(_state)));
    publish("controlState", String(controlStateToString(_controlState)));
    publish("numPacks", _num_batteries);
    publish("efficiency", _efficiency);
    publish("serial", _serial);

    for (const auto& [index, value] : _packData) {
        auto id = String(index);
        publish("packs/" + id + "/cellMinMilliVolt", value->_cell_voltage_min);
        publish("packs/" + id + "/cellMaxMilliVolt", value->_cell_voltage_max);
        publish("packs/" + id + "/cellDiffMilliVolt", value->_cell_voltage_spread);
        publish("packs/" + id + "/cellAvgMilliVolt", value->_cell_voltage_avg);
        publish("packs/" + id + "/cellMaxTemperature", value->_cell_temperature_max);
        publish("packs/" + id + "/voltage", value->_voltage_total);
        publish("packs/" + id + "/power", value->_power);
        publish("packs/" + id + "/current", value->_current);
        publish("packs/" + id + "/stateOfCharge", value->_soc_level, 1);
        publish("packs/" + id + "/stateOfHealth", value->_state_of_health, 1);
        publish("packs/" + id + "/state", String(stateToString(value->_state)));
        publish("packs/" + id + "/serial", value->getSerial());
        publish("packs/" + id + "/name", value->getName());
        publish("packs/" + id + "/capacity", value->_capacity);
    }

    publish("battery/solarPowerMppt1", getSolarPower(SolarChargers::Integrated::MPPT::Number_1));
    publish("battery/solarPowerMppt2", getSolarPower(SolarChargers::Integrated::MPPT::Number_2));
    publish("outputPower", _output_power);
    publish("inputPower", getInputPower());
    publish("bypass", boolToString(_bypass_state));
    publish("lastFullCharge", _last_full_hours);
    publish("lastEmpty", _last_empty_hours);

    publish("chargeThroughState", String(chargeThroughStateToString(_charge_through_state)));

    if (config.Zendure.ConnectionType != BatteryZendureConfig::ConnectionType::ZendureMqtt) {
        publish("settings/controlMode", String(controlModeToString(config.Zendure.ControlMode)));
        publish("settings/batteryProtection", boolToString(config.Zendure.BatteryProtectionEnable));
    }

    publish("settings/outputLimitPower", _output_limit);
    publish("settings/inputLimitPower", _input_limit);
    publish("settings/stateOfChargeMin", _soc_min, 1);
    publish("settings/stateOfChargeMax", _soc_max, 1);
    publish("settings/bypassMode", String(bypassModeToString(_bypass_mode)));
}

std::shared_ptr<PackStats> Stats::getPackData(size_t index) const {
    try
    {
        return _packData.at(index);
    }
    catch(const std::out_of_range& ex)
    {
        return nullptr;
    }
}

std::shared_ptr<PackStats> Stats::addPackData(size_t index, String serial) {
    std::shared_ptr<PackStats> pack;
    try
    {
        pack = _packData.at(index);
        pack->setSerial(serial);
    }
    catch(const std::out_of_range& ex)
    {
        pack = PackStats::fromSerial(serial);

        if (pack != nullptr) {
            _packData[index] = pack;
        }
    }
    return pack;
}

} // namespace Batteries::Zendure
