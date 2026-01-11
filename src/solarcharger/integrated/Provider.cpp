// SPDX-License-Identifier: GPL-2.0-or-later
#include <solarcharger/integrated/Provider.h>
#include <Configuration.h>
#include <LogHelper.h>

#undef TAG
static const char* TAG = "solarCharger";
static const char* SUBTAG = "integrated";

namespace SolarChargers::Integrated {

bool Provider::init()
{
    const auto& config = Configuration.get();

    for (uint8_t i = 0; i < BAT_MAX_COUNT; i++) {
        const auto& cfg = config.Batteries[i];
        if (cfg.Provider == 7 && cfg.Enabled) { return true;}
    }

    DTU_LOGE("Init failed - you must use a supported battery integration. Currently supported integrations are: Zendure");
    return false;
}

} // namespace SolarChargers::Integrated
