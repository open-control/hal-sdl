#pragma once

#include <oc/interface/IButton.hpp>
#include <oc/types/Result.hpp>
#include <unordered_map>

namespace oc::hal::sdl {

/**
 * @brief SDL button controller implementing IButton
 *
 * Event-driven controller for desktop simulation. Button events are
 * injected via onEvent() from InputMapper, not polled from hardware.
 */
class SdlButtonController : public interface::IButton {
public:
    oc::Result<void> init() override {
        return oc::Result<void>::ok();
    }

    void update(uint32_t) override {
        // No-op: SDL is event-driven, not polled
    }

    bool isPressed(oc::ButtonID id) const override {
        auto it = states_.find(id);
        return it != states_.end() && it->second;
    }

    void setCallback(oc::ButtonCallback cb) override {
        callback_ = cb;
    }

    /**
     * @brief Called by InputMapper when a button event occurs
     *
     * @param id Button identifier
     * @param pressed true if pressed, false if released
     */
    void onEvent(oc::ButtonID id, bool pressed) {
        states_[id] = pressed;
        if (callback_) {
            callback_(id, pressed ? oc::ButtonEvent::PRESSED : oc::ButtonEvent::RELEASED);
        }
    }

private:
    oc::ButtonCallback callback_;
    std::unordered_map<oc::ButtonID, bool> states_;
};

} // namespace oc::hal::sdl
