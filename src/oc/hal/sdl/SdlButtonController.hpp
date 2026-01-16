#pragma once

#include <oc/hal/IButtonController.hpp>
#include <oc/core/Result.hpp>
#include <unordered_map>

namespace oc::hal::sdl {

/**
 * @brief SDL button controller implementing IButtonController
 *
 * Event-driven controller for desktop simulation. Button events are
 * injected via onEvent() from InputMapper, not polled from hardware.
 */
class SdlButtonController : public hal::IButtonController {
public:
    core::Result<void> init() override {
        return core::Result<void>::ok();
    }

    void update(uint32_t) override {
        // No-op: SDL is event-driven, not polled
    }

    bool isPressed(hal::ButtonID id) const override {
        auto it = states_.find(id);
        return it != states_.end() && it->second;
    }

    void setCallback(hal::ButtonCallback cb) override {
        callback_ = cb;
    }

    /**
     * @brief Called by InputMapper when a button event occurs
     *
     * @param id Button identifier
     * @param pressed true if pressed, false if released
     */
    void onEvent(hal::ButtonID id, bool pressed) {
        states_[id] = pressed;
        if (callback_) {
            callback_(id, pressed ? hal::ButtonEvent::PRESSED : hal::ButtonEvent::RELEASED);
        }
    }

private:
    hal::ButtonCallback callback_;
    std::unordered_map<hal::ButtonID, bool> states_;
};

} // namespace oc::hal::sdl
