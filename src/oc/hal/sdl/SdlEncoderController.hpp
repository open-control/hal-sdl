#pragma once

#include <oc/interface/IEncoder.hpp>
#include <oc/core/input/EncoderLogic.hpp>
#include <oc/type/Result.hpp>
#include <unordered_map>
#include <memory>

namespace oc::hal::sdl {

/**
 * @brief SDL encoder controller with full feature parity to Teensy
 *
 * Uses the same EncoderLogic class as teensy::EncoderController to ensure
 * identical behavior for modes, bounds, discrete steps, and quantization.
 *
 * ## Difference with Teensy
 *
 * - Teensy: Hardware interrupts publish; update() consumes in foreground.
 * - SDL: InputMapper events publish and consume synchronously in foreground.
 *
 * The logic layer is identical, only the input source differs.
 */
class SdlEncoderController : public interface::IEncoder {
public:
    oc::type::Result<void> init() override {
        return oc::type::Result<void>::ok();
    }

    void update() override {
        // SDL is event-driven and consumes encoder input in onEvent().
    }

    float getPosition(oc::type::EncoderID id) const override {
        auto it = logics_.find(id);
        return it != logics_.end() ? it->second->getLastValue() : 0.0f;
    }

    void setPosition(oc::type::EncoderID id, float value) override {
        ensureLogic(id)->setPosition(value);
    }

    void setMode(oc::type::EncoderID id, interface::EncoderMode mode) override {
        ensureLogic(id)->setMode(mode);
    }

    void setBounds(oc::type::EncoderID id, float min, float max) override {
        ensureLogic(id)->setBounds(min, max);
    }

    void setDelta(oc::type::EncoderID id, float delta) override {
        ensureLogic(id)->setDelta(delta);
    }

    void setDiscreteSteps(oc::type::EncoderID id, uint8_t steps) override {
        ensureLogic(id)->setDiscreteSteps(steps);
    }

    void setDiscreteTicksPerStep(oc::type::EncoderID id, uint16_t ticksPerStep) override {
        ensureLogic(id)->setDiscreteTicksPerStep(ticksPerStep);
    }

    void setNormalizedTurns(oc::type::EncoderID id, float turns) override {
        ensureLogic(id)->setNormalizedTurns(turns);
    }

    void setContinuous(oc::type::EncoderID id) override {
        ensureLogic(id)->setContinuous();
    }

    void setCallback(oc::type::EncoderCallback cb) override {
        callback_ = cb;
    }

    /**
     * @brief Called by InputMapper when an encoder event occurs
     *
     * @param id Encoder ID
     * @param delta Raw delta from input
     *
     * The event is already in foreground. Publish its aggregate integer ticks,
     * consume policy immediately, and callback once so InputMapper's following
     * getPosition() observes the current value.
     */
    void onEvent(oc::type::EncoderID id, float delta) {
        // Convert float delta to ticks
        // Scale: 0.01 delta = 1 tick (100 pixels of mouse movement = full range)
        constexpr float SCALE = 100.0f;
        int32_t ticks = static_cast<int32_t>(delta * SCALE);

        if (ticks == 0) return;

        auto* logic = ensureLogic(id);
        logic->publishDeltaFromISR(ticks);
        auto value = logic->consumePublishedDeltas();
        if (value.has_value() && callback_) {
            callback_(id, value.value());
        }
    }

private:
    oc::type::EncoderCallback callback_;
    mutable std::unordered_map<oc::type::EncoderID, std::unique_ptr<core::input::EncoderLogic>> logics_;

    /**
     * @brief Get or create EncoderLogic for an ID
     *
     * Lazily creates EncoderLogic instances with default desktop config.
     * This allows the controller to handle any encoder ID without
     * pre-registration (unlike Teensy which has fixed hardware).
     */
    core::input::EncoderLogic* ensureLogic(oc::type::EncoderID id) const {
        auto it = logics_.find(id);
        if (it != logics_.end()) {
            return it->second.get();
        }

        // Default config for desktop (no physical encoder constraints)
        core::input::EncoderConfig cfg{
            .id = id,
            .ppr = 100,            // Virtual PPR for desktop
            .rangeAngle = 300,     // Standard pot range (uint16_t, not float)
            .ticksPerEvent = 1,    // Every tick counts
            .invertDirection = false
        };

        auto logic = std::make_unique<core::input::EncoderLogic>(cfg);
        auto* ptr = logic.get();
        logics_[id] = std::move(logic);
        return ptr;
    }
};

} // namespace oc::hal::sdl
