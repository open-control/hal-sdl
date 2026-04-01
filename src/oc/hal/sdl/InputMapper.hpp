#pragma once

#include <SDL.h>
#include <oc/type/Ids.hpp>
#include <oc/type/Callbacks.hpp>
#include <cassert>
#include <vector>
#include <functional>
#include <cmath>

namespace oc::hal::sdl {

// Forward declarations
class SdlButtonController;
class SdlEncoderController;

// Feedback callbacks for visual sync (e.g., HwSimulator)
using ButtonFeedback = std::function<void(oc::type::ButtonID, bool pressed)>;
using EncoderFeedback = std::function<void(oc::type::EncoderID, float value)>;

/**
 * @brief Maps SDL input events to HAL button/encoder events
 *
 * This class provides the MECHANISM for input mapping. The consumer
 * defines the CONFIGURATION (which zones, which IDs) via fluent API.
 *
 * ## Lifetime Contract
 *
 * After calling connect(), this InputMapper holds raw pointers to the
 * controllers. These pointers must remain valid for all subsequent
 * calls to handleEvent().
 *
 * **Typical safe usage pattern:**
 * @code
 * int main() {
 *     InputMapper input;
 *     // configure...
 *
 *     auto app = AppBuilder(input).controllers().build();
 *
 *     while (running) {
 *         input.handleEvent(event);  // Safe: same scope as app
 *         app.update();
 *     }
 * }  // input and app destroyed together
 * @endcode
 *
 * @warning Do not use InputMapper after the connected controllers
 * (owned by OpenControlApp) are destroyed.
 */
class InputMapper {
public:
    // ══════════════════════════════════════════════════════════════
    // Keyboard -> Buttons
    // ══════════════════════════════════════════════════════════════

    InputMapper& button(SDL_Keycode key, oc::type::ButtonID id);
    InputMapper& buttonScancode(SDL_Scancode scancode, oc::type::ButtonID id);

    // ══════════════════════════════════════════════════════════════
    // Keyboard -> Encoders (two keys incr/decr)
    // ══════════════════════════════════════════════════════════════

    InputMapper& encoder(SDL_Keycode keyIncr, SDL_Keycode keyDecr,
                         oc::type::EncoderID id, float delta = 0.05f);

    // ══════════════════════════════════════════════════════════════
    // Mouse -> Buttons (clickable zones)
    // ══════════════════════════════════════════════════════════════

    InputMapper& buttonCircle(int cx, int cy, int radius, oc::type::ButtonID id);

    // ══════════════════════════════════════════════════════════════
    // Mouse -> Encoders (vertical drag zones)
    // ══════════════════════════════════════════════════════════════

    /// Ring zone (hole in center for button)
    InputMapper& encoderRing(int cx, int cy, int outerRadius, int innerRadius,
                             oc::type::EncoderID id, float sensitivity = 100.0f);

    /// Full circle zone
    InputMapper& encoderCircle(int cx, int cy, int radius,
                               oc::type::EncoderID id, float sensitivity = 100.0f);

    /// Mouse wheel on zone
    InputMapper& encoderWheel(int cx, int cy, int radius,
                              oc::type::EncoderID id, float delta = 0.02f);

    // ══════════════════════════════════════════════════════════════
    // Combo: Encoder + Integrated Button (for macros)
    // ══════════════════════════════════════════════════════════════

    InputMapper& encoderWithButton(int cx, int cy,
                                   int outerRadius, int innerRadius,
                                   oc::type::EncoderID encId, oc::type::ButtonID btnId,
                                   float sensitivity = 100.0f);

    // ══════════════════════════════════════════════════════════════
    // External injection (for custom sources)
    // ══════════════════════════════════════════════════════════════

    void post(oc::type::ButtonID id, bool pressed);
    void post(oc::type::EncoderID id, float delta);

    // ══════════════════════════════════════════════════════════════
    // Connection to controllers
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief Connect to button and encoder controllers
     *
     * @param buttons Button controller (must outlive this InputMapper)
     * @param encoders Encoder controller (must outlive this InputMapper)
     *
     * @warning Lifetime contract: The controllers must remain valid for
     * all subsequent calls to handleEvent(). See class documentation.
     */
    void connect(SdlButtonController& buttons, SdlEncoderController& encoders);

    /**
     * @brief Check if connected to controllers
     */
    bool isConnected() const {
        return buttonController_ != nullptr && encoderController_ != nullptr;
    }

    // ══════════════════════════════════════════════════════════════
    // Feedback hooks (for visual sync, e.g., HwSimulator)
    // ══════════════════════════════════════════════════════════════

    void setButtonFeedback(ButtonFeedback callback) { buttonFeedback_ = callback; }
    void setEncoderFeedback(EncoderFeedback callback) { encoderFeedback_ = callback; }

    // ══════════════════════════════════════════════════════════════
    // SDL event processing
    // ══════════════════════════════════════════════════════════════

    /**
     * @brief Process an SDL event and dispatch to connected controllers
     *
     * @pre isConnected() must be true (asserted in debug builds)
     */
    void handleEvent(const SDL_Event& event);

private:
    // Mapping structures
    struct KeyButtonMap {
        SDL_Keycode key = SDLK_UNKNOWN;
        SDL_Scancode scancode = SDL_SCANCODE_UNKNOWN;
        oc::type::ButtonID id = 0;
    };
    struct KeyEncoderMap { SDL_Keycode keyIncr; SDL_Keycode keyDecr; oc::type::EncoderID id; float delta; };
    struct CircleButtonMap { int cx, cy, radius; oc::type::ButtonID id; };
    struct RingEncoderMap { int cx, cy, outerR, innerR; oc::type::EncoderID id; float sensitivity; };
    struct WheelEncoderMap { int cx, cy, radius; oc::type::EncoderID id; float delta; };
    struct ComboMap { int cx, cy, outerR, innerR; oc::type::EncoderID encId; oc::type::ButtonID btnId; float sensitivity; };

    std::vector<KeyButtonMap> keyButtons_;
    std::vector<KeyEncoderMap> keyEncoders_;
    std::vector<CircleButtonMap> circleButtons_;
    std::vector<RingEncoderMap> ringEncoders_;
    std::vector<WheelEncoderMap> wheelEncoders_;
    std::vector<ComboMap> combos_;

    SdlButtonController* buttonController_ = nullptr;
    SdlEncoderController* encoderController_ = nullptr;

    ButtonFeedback buttonFeedback_;
    EncoderFeedback encoderFeedback_;

    // Drag state
    struct DragState {
        bool active = false;
        oc::type::EncoderID encoderId = 0;
        int startY = 0;
        float sensitivity = 100.0f;
    } drag_;

    // Helpers
    bool isInCircle(int x, int y, int cx, int cy, int r) const {
        int dx = x - cx;
        int dy = y - cy;
        return (dx * dx + dy * dy) <= (r * r);
    }

    bool isInRing(int x, int y, int cx, int cy, int outerR, int innerR) const {
        int dx = x - cx;
        int dy = y - cy;
        int distSq = dx * dx + dy * dy;
        return distSq <= (outerR * outerR) && distSq >= (innerR * innerR);
    }

    void emitButton(oc::type::ButtonID id, bool pressed);
    void emitEncoder(oc::type::EncoderID id, float delta);
};

} // namespace oc::hal::sdl
