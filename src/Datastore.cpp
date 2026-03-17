// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023-2024 Thomas Basler and others
 */
#include "Datastore.h"
#include "Configuration.h"
#include "battery/Controller.h"
#include <Hoymiles.h>

DatastoreClass Datastore;

DatastoreClass::DatastoreClass()
    : _loopTask(1 * TASK_SECOND, TASK_FOREVER, std::bind(&DatastoreClass::loop, this))
{
}

void DatastoreClass::init(Scheduler& scheduler)
{
    scheduler.addTask(_loopTask);
    _loopTask.enable();
}

void DatastoreClass::loop()
{
    if (!Hoymiles.isAllRadioIdle()) {
        _loopTask.forceNextIteration();
        return;
    }

    uint8_t isProducing = 0;
    uint8_t isReachable = 0;
    uint8_t pollEnabledCount = 0;

    std::lock_guard<std::mutex> lock(_mutex);

    _totalAcYieldTotalEnabled = 0;
    _totalAcYieldTotalDigits = 0;

    _totalAcYieldDayEnabled = 0;
    _totalAcYieldDayDigits = 0;

    _totalAcPowerEnabled = 0;
    _totalAcPowerDigits = 0;

    _totalDcPowerEnabled = 0;
    _totalDcPowerDigits = 0;

    _totalDcPowerIrradiation = 0;
    _totalDcIrradiationInstalled = 0;

    _isAllEnabledProducing = true;
    _isAllEnabledReachable = true;

    for (uint8_t i = 0; i < Hoymiles.getNumInverters(); i++) {
        auto inv = Hoymiles.getInverterByPos(i);
        if (inv == nullptr) {
            continue;
        }

        auto cfg = Configuration.getInverterConfig(inv->serial());
        if (cfg == nullptr) {
            continue;
        }

        if (inv->getEnablePolling()) {
            pollEnabledCount++;
        }

        if (inv->isProducing()) {
            isProducing++;
        } else {
            if (inv->getEnablePolling()) {
                _isAllEnabledProducing = false;
            }
        }

        if (inv->isReachable()) {
            isReachable++;
        } else {
            if (inv->getEnablePolling()) {
                _isAllEnabledReachable = false;
            }
        }

        for (auto& c : inv->Statistics()->getChannelsByType(TYPE_INV)) {
            if (cfg->Poll_Enable) {
                _totalAcYieldTotalEnabled += inv->Statistics()->getChannelFieldValue(TYPE_INV, c, FLD_YT);
                _totalAcYieldDayEnabled += inv->Statistics()->getChannelFieldValue(TYPE_INV, c, FLD_YD);

                _totalAcYieldTotalDigits = max<unsigned int>(_totalAcYieldTotalDigits, inv->Statistics()->getChannelFieldDigits(TYPE_INV, c, FLD_YT));
                _totalAcYieldDayDigits = max<unsigned int>(_totalAcYieldDayDigits, inv->Statistics()->getChannelFieldDigits(TYPE_INV, c, FLD_YD));
            }
        }

        for (auto& c : inv->Statistics()->getChannelsByType(TYPE_AC)) {
            if (inv->getEnablePolling()) {
                _totalAcPowerEnabled += inv->Statistics()->getChannelFieldValue(TYPE_AC, c, FLD_PAC);
                _totalAcPowerDigits = max<unsigned int>(_totalAcPowerDigits, inv->Statistics()->getChannelFieldDigits(TYPE_AC, c, FLD_PAC));
            }
        }

        for (auto& c : inv->Statistics()->getChannelsByType(TYPE_DC)) {
            if (inv->getEnablePolling()) {
                _totalDcPowerEnabled += inv->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC);
                _totalDcPowerDigits = max<unsigned int>(_totalDcPowerDigits, inv->Statistics()->getChannelFieldDigits(TYPE_DC, c, FLD_PDC));

                if (inv->Statistics()->getStringMaxPower(c) > 0) {
                    _totalDcPowerIrradiation += inv->Statistics()->getChannelFieldValue(TYPE_DC, c, FLD_PDC);
                    _totalDcIrradiationInstalled += inv->Statistics()->getStringMaxPower(c);
                }
            }
        }
    }

    _isAtLeastOneProducing = isProducing > 0;
    _isAtLeastOneReachable = isReachable > 0;
    _isAtLeastOnePollEnabled = pollEnabledCount > 0;

    _totalDcIrradiation = _totalDcIrradiationInstalled > 0 ? _totalDcPowerIrradiation / _totalDcIrradiationInstalled * 100.0f : 0;


    {
        std::optional<uint32_t> totalCapacityWh = std::nullopt;
        std::optional<uint32_t> availableCapacityWh = std::nullopt;
        std::optional<uint32_t> useableCapacityWh = std::nullopt;
        std::optional<uint32_t> chargedCapacityWh = std::nullopt;
        std::optional<uint32_t> storedEnergyWh = std::nullopt;

        std::optional<float> totalPower = std::nullopt;
        std::optional<float> totalCurrent = std::nullopt;
        std::optional<float> totalVoltage = std::nullopt;

        uint8_t maxPrecisionSoc = 0;
        uint8_t maxPrecisionPower = 0;
        uint8_t maxPrecisionCurrent = 0;
        uint8_t maxPrecisionVoltage = 0;

        for (uint8_t i = 0; i < BAT_MAX_COUNT; i++) {
            const auto& config = Configuration.get().Batteries[i];
            if (!config.Enabled || config.Uid == 0U) { continue; }

            const auto& spStats = Battery.getStatsByUid(config.Uid);
            if (spStats == nullptr) { continue; }

            if (spStats->getTotalCapacityWh().has_value()) {
                totalCapacityWh = totalCapacityWh.value_or(0) + *spStats->getTotalCapacityWh();
            }

            if (spStats->getAvailableCapacityWh().has_value()) {
                const auto cap = *spStats->getAvailableCapacityWh();

                if (spStats->isSoCValid()) {
                    const auto soc = spStats->getSoC();

                    availableCapacityWh = availableCapacityWh.value_or(0) + cap;
                    chargedCapacityWh = chargedCapacityWh.value_or(0) + static_cast<uint32_t>((cap * soc) / 100.0f);

                    const auto min = spStats->getMinimumSoC();
                    if (min.has_value()) {
                        storedEnergyWh = storedEnergyWh.value_or(0) + static_cast<uint32_t>((cap * std::max(soc - *min, 0.0F)) / 100.0f);
                    }

                    const auto useable = spStats->getUseableCapacityWh();
                    if (useable.has_value()) {
                        useableCapacityWh = useableCapacityWh.value_or(0) + *useable;
                    }

                    maxPrecisionSoc = std::max<uint8_t>(maxPrecisionSoc, spStats->getSoCPrecision());
                }
            }

            if (spStats->isCurrentValid()) {
                totalCurrent = totalCurrent.value_or(0.0f) + spStats->getChargeCurrent();
                maxPrecisionCurrent = std::max<uint8_t>(maxPrecisionCurrent, spStats->getChargeCurrentPrecision());
            }

            if (spStats->isVoltageValid()) {
                totalVoltage = totalVoltage.value_or(0.0f) + spStats->getVoltage();
                maxPrecisionVoltage = std::max<uint8_t>(maxPrecisionVoltage, spStats->getVoltagePrecision());
            }

            if (spStats->isVoltageValid() && spStats->isCurrentValid()) {
                totalPower = totalPower.value_or(0) + (spStats->getVoltage() * spStats->getChargeCurrent());
            }
        }

        maxPrecisionPower = std::max<uint8_t>(maxPrecisionPower, maxPrecisionCurrent);

        _totalBatteryInstalledCapacity = totalCapacityWh.value_or(0);
        _totalBatteryAvailableCapacity = availableCapacityWh.value_or(0);
        _totalBatteryUseableCapacity = useableCapacityWh.value_or(0);
        _totalBatteryStoredEnergy = storedEnergyWh.value_or(0);

        _totalBatteryPower = totalPower.value_or(0);
        _totalBatteryPowerDigits = maxPrecisionPower;

        if (availableCapacityWh.value_or(0) > 0) {
            _totalBatteryStateOfCharge = 100.0 * (static_cast<float>(chargedCapacityWh.value_or(0)) / static_cast<float>(*availableCapacityWh));
            _totalBatteryStateOfChargeDigits = maxPrecisionSoc;
        }

        if (useableCapacityWh.value_or(0) > 0) {
            _totalBatteryUseableStateOfCharge = 100.0 * (static_cast<float>(storedEnergyWh.value_or(0)) / static_cast<float>(*useableCapacityWh));
            _totalBatteryUseableStateOfChargeDigits = maxPrecisionSoc;
        }
    }
}

float DatastoreClass::getTotalAcYieldTotalEnabled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcYieldTotalEnabled;
}

float DatastoreClass::getTotalAcYieldDayEnabled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcYieldDayEnabled;
}

float DatastoreClass::getTotalAcPowerEnabled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcPowerEnabled;
}

float DatastoreClass::getTotalDcPowerEnabled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalDcPowerEnabled;
}

float DatastoreClass::getTotalDcPowerIrradiation()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalDcPowerIrradiation;
}

float DatastoreClass::getTotalDcIrradiationInstalled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalDcIrradiationInstalled;
}

float DatastoreClass::getTotalDcIrradiation()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalDcIrradiation;
}

uint32_t DatastoreClass::getTotalAcYieldTotalDigits()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcYieldTotalDigits;
}

uint32_t DatastoreClass::getTotalAcYieldDayDigits()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcYieldDayDigits;
}

uint32_t DatastoreClass::getTotalAcPowerDigits()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalAcPowerDigits;
}

uint32_t DatastoreClass::getTotalDcPowerDigits()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalDcPowerDigits;
}

bool DatastoreClass::getIsAtLeastOneReachable()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isAtLeastOneReachable;
}

bool DatastoreClass::getIsAtLeastOneProducing()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isAtLeastOneProducing;
}

bool DatastoreClass::getIsAllEnabledProducing()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isAllEnabledProducing;
}

bool DatastoreClass::getIsAllEnabledReachable()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isAllEnabledReachable;
}

bool DatastoreClass::getIsAtLeastOnePollEnabled()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _isAtLeastOnePollEnabled;
}


float DatastoreClass::getTotalBatteryStateOfCharge()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalBatteryStateOfCharge;
}
uint32_t DatastoreClass::getTotalBatteryStateOfChargeDigits()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalBatteryStateOfChargeDigits;
}

float DatastoreClass::getTotalBatteryUseableStateOfCharge()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalBatteryUseableStateOfCharge;
}
uint32_t DatastoreClass::getTotalBatteryUseableStateOfChargeDigits()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalBatteryUseableStateOfChargeDigits;
}

uint32_t DatastoreClass::getTotalBatteryInstalledCapacity()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalBatteryInstalledCapacity;
}

uint32_t DatastoreClass::getTotalBatteryAvailableCapacity()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalBatteryAvailableCapacity;
}

uint32_t DatastoreClass::getTotalBatteryUsableCapacity()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalBatteryUseableCapacity;
}

uint32_t DatastoreClass::getTotalBatteryStoredEnergy()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalBatteryStoredEnergy;
}

float DatastoreClass::getTotalBatteryPower()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalBatteryPower;
}

uint32_t DatastoreClass::getTotalBatteryPowerDigits()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _totalBatteryPowerDigits;
}
