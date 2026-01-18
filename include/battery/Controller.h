// SPDX-License-Identifier: GPL-2.0-or-later
#pragma once

#include <memory>
#include <mutex>
#include <TaskSchedulerDeclarations.h>
#include <battery/Provider.h>
#include <battery/Stats.h>

namespace Batteries {

class Controller {
public:
    void init(Scheduler&);
    void updateSettings();
    bool updateSettings(const uint32_t uid);
    void removeByUid(const uint32_t uid);

    float getDischargeCurrentLimit();

    std::shared_ptr<Stats const> getStats() const;
    std::shared_ptr<Stats const> getStatsByUid(const uint32_t uid) const;

    void getLiveViewData(JsonVariant& root) const;
    size_t getBatteryCount() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _batteries.size();
    }
    bool updateAvailable(uint32_t since) const;

private:
    void loop();

    Task _loopTask;
    mutable std::mutex _mutex;
    std::vector<std::shared_ptr<Provider>> _batteries;
};

} // namespace Batteries

extern Batteries::Controller Battery;
