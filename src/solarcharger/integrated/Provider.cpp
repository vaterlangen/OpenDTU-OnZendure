// SPDX-License-Identifier: GPL-2.0-or-later
#include <solarcharger/integrated/Provider.h>
#include <Configuration.h>
#include <MessageOutput.h>

namespace SolarChargers::Integrated {

bool Provider::init(bool verboseLogging)
{
    _verboseLogging = verboseLogging;

    if (Configuration.get().Battery.Provider != 7) {
        MessageOutput.printf("[SolarChargers::Integrated]: Init failed - you must use a supported battery integration. Currently supported integrations are: Zendure");
        return false;
    }

    return true;
}

} // namespace SolarChargers::Integrated
