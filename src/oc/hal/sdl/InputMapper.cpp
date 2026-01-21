#include "InputMapper.hpp"
#include "SdlButtonController.hpp"
#include "SdlEncoderController.hpp"

namespace oc::hal::sdl {

// ══════════════════════════════════════════════════════════════════════════════
// Configuration API (fluent)
// ══════════════════════════════════════════════════════════════════════════════

InputMapper& InputMapper::button(SDL_Keycode key, oc::type::ButtonID id) {
    keyButtons_.push_back({key, id});
    return *this;
}

InputMapper& InputMapper::encoder(SDL_Keycode keyIncr, SDL_Keycode keyDecr,
                                   oc::type::EncoderID id, float delta) {
    keyEncoders_.push_back({keyIncr, keyDecr, id, delta});
    return *this;
}

InputMapper& InputMapper::buttonCircle(int cx, int cy, int radius, oc::type::ButtonID id) {
    circleButtons_.push_back({cx, cy, radius, id});
    return *this;
}

InputMapper& InputMapper::encoderRing(int cx, int cy, int outerRadius, int innerRadius,
                                       oc::type::EncoderID id, float sensitivity) {
    ringEncoders_.push_back({cx, cy, outerRadius, innerRadius, id, sensitivity});
    return *this;
}

InputMapper& InputMapper::encoderCircle(int cx, int cy, int radius,
                                         oc::type::EncoderID id, float sensitivity) {
    // Circle is just a ring with innerR = 0
    ringEncoders_.push_back({cx, cy, radius, 0, id, sensitivity});
    return *this;
}

InputMapper& InputMapper::encoderWheel(int cx, int cy, int radius,
                                        oc::type::EncoderID id, float delta) {
    wheelEncoders_.push_back({cx, cy, radius, id, delta});
    return *this;
}

InputMapper& InputMapper::encoderWithButton(int cx, int cy,
                                             int outerRadius, int innerRadius,
                                             oc::type::EncoderID encId, oc::type::ButtonID btnId,
                                             float sensitivity) {
    combos_.push_back({cx, cy, outerRadius, innerRadius, encId, btnId, sensitivity});
    return *this;
}

// ══════════════════════════════════════════════════════════════════════════════
// Connection
// ══════════════════════════════════════════════════════════════════════════════

void InputMapper::connect(SdlButtonController& buttons, SdlEncoderController& encoders) {
    buttonController_ = &buttons;
    encoderController_ = &encoders;
}

// ══════════════════════════════════════════════════════════════════════════════
// External injection
// ══════════════════════════════════════════════════════════════════════════════

void InputMapper::post(oc::type::ButtonID id, bool pressed) {
    emitButton(id, pressed);
}

void InputMapper::post(oc::type::EncoderID id, float delta) {
    emitEncoder(id, delta);
}

// ══════════════════════════════════════════════════════════════════════════════
// Emission helpers
// ══════════════════════════════════════════════════════════════════════════════

void InputMapper::emitButton(oc::type::ButtonID id, bool pressed) {
    if (buttonController_) {
        buttonController_->onEvent(id, pressed);
    }
    if (buttonFeedback_) {
        buttonFeedback_(id, pressed);
    }
}

void InputMapper::emitEncoder(oc::type::EncoderID id, float delta) {
    if (encoderController_) {
        encoderController_->onEvent(id, delta);
    }
    if (encoderFeedback_) {
        encoderFeedback_(id, encoderController_ ? encoderController_->getPosition(id) : 0.0f);
    }
}

// ══════════════════════════════════════════════════════════════════════════════
// Event handling
// ══════════════════════════════════════════════════════════════════════════════

void InputMapper::handleEvent(const SDL_Event& event) {
    assert(isConnected() && "InputMapper::handleEvent called before connect()");

    switch (event.type) {
        // ─────────────────────────────────────────────────────────────────────
        // Keyboard events
        // ─────────────────────────────────────────────────────────────────────
        case SDL_KEYDOWN:
        case SDL_KEYUP: {
            bool pressed = (event.type == SDL_KEYDOWN);
            SDL_Keycode key = event.key.keysym.sym;

            // Check key -> button mappings
            for (const auto& map : keyButtons_) {
                if (map.key == key) {
                    emitButton(map.id, pressed);
                }
            }

            // Check key -> encoder mappings (only on press)
            if (pressed) {
                for (const auto& map : keyEncoders_) {
                    if (key == map.keyIncr) {
                        emitEncoder(map.id, map.delta);
                    } else if (key == map.keyDecr) {
                        emitEncoder(map.id, -map.delta);
                    }
                }
            }
            break;
        }

        // ─────────────────────────────────────────────────────────────────────
        // Mouse button events
        // ─────────────────────────────────────────────────────────────────────
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP: {
            bool pressed = (event.type == SDL_MOUSEBUTTONDOWN);
            int mx = event.button.x;
            int my = event.button.y;

            // Check circle button zones
            for (const auto& map : circleButtons_) {
                if (isInCircle(mx, my, map.cx, map.cy, map.radius)) {
                    emitButton(map.id, pressed);
                }
            }

            // Check combo zones (button in center, encoder in ring)
            for (const auto& map : combos_) {
                if (isInCircle(mx, my, map.cx, map.cy, map.innerR)) {
                    // Center = button
                    emitButton(map.btnId, pressed);
                } else if (isInRing(mx, my, map.cx, map.cy, map.outerR, map.innerR)) {
                    // Ring = start/stop encoder drag
                    if (pressed) {
                        drag_.active = true;
                        drag_.encoderId = map.encId;
                        drag_.startY = my;
                        drag_.sensitivity = map.sensitivity;
                    }
                }
            }

            // Check encoder ring zones (no button)
            for (const auto& map : ringEncoders_) {
                if (isInRing(mx, my, map.cx, map.cy, map.outerR, map.innerR)) {
                    if (pressed) {
                        drag_.active = true;
                        drag_.encoderId = map.id;
                        drag_.startY = my;
                        drag_.sensitivity = map.sensitivity;
                    }
                }
            }

            // Stop drag on release
            if (!pressed && drag_.active) {
                drag_.active = false;
            }
            break;
        }

        // ─────────────────────────────────────────────────────────────────────
        // Mouse motion (drag for encoders)
        // ─────────────────────────────────────────────────────────────────────
        case SDL_MOUSEMOTION: {
            if (drag_.active) {
                int my = event.motion.y;
                float delta = static_cast<float>(drag_.startY - my) / drag_.sensitivity;
                drag_.startY = my;
                if (delta != 0.0f) {
                    emitEncoder(drag_.encoderId, delta);
                }
            }
            break;
        }

        // ─────────────────────────────────────────────────────────────────────
        // Mouse wheel
        // ─────────────────────────────────────────────────────────────────────
        case SDL_MOUSEWHEEL: {
            int mx, my;
            SDL_GetMouseState(&mx, &my);

            for (const auto& map : wheelEncoders_) {
                if (isInCircle(mx, my, map.cx, map.cy, map.radius)) {
                    float delta = event.wheel.y * map.delta;
                    emitEncoder(map.id, delta);
                }
            }

            // Also check combos for wheel
            for (const auto& map : combos_) {
                if (isInCircle(mx, my, map.cx, map.cy, map.outerR)) {
                    float delta = event.wheel.y * 0.02f;
                    emitEncoder(map.encId, delta);
                }
            }
            break;
        }

        default:
            break;
    }
}

} // namespace oc::hal::sdl
