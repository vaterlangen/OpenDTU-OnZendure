// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include "PowerLimiterOverscalingInverter.h"

class PowerLimiterSmartBufferInverter : public PowerLimiterOverscalingInverter {
public:
    explicit PowerLimiterSmartBufferInverter(PowerLimiterInverterConfig const& config);

    uint16_t getMaxReductionWatts(bool allowStandby) const final;
    uint16_t getMaxIncreaseWatts() const final;
    uint16_t applyReduction(uint16_t reduction, bool allowStandby) final;
    uint16_t standby() final;

    const std::shared_ptr<const Batteries::SmartBufferStats> getBatteryStatsByMppt(const uint8_t mppt) const {
        uint32_t uid = 0;
        try {
            uid = _mpptInputs.at(mppt);
        } catch (const std::out_of_range& e) {
            return nullptr;
        }

        return std::static_pointer_cast<const Batteries::SmartBufferStats>(Battery.getStatsByUid(uid));
    }

    const std::optional<float> getSolarInputByMppt(const uint8_t mppt) const {
        auto& stats = getBatteryStatsByMppt(mppt);
        if (stats == nullptr) { return std::nullopt; }

        return stats->getSolarPowerOverall();
    }

    const std::optional<float> getBatteryInputByMppt(const uint8_t mppt) const {
        auto& stats = getBatteryStatsByMppt(mppt);
        if (stats == nullptr) { return std::nullopt; }

        auto power = stats->getPower();
        return power > 0 ? 0 : -power;
    }

private:
    std::map<uint8_t, uint32_t> _mpptInputs = std::map<uint8_t, uint32_t>();
};
