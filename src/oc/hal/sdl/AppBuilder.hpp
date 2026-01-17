#pragma once

/**
 * @file AppBuilder.hpp
 * @brief SDL-specific AppBuilder with simplified API
 *
 * @code
 * #include <oc/hal/sdl/Sdl.hpp>
 * #include <oc/hal/midi/LibreMidiTransport.hpp>  // User provides transport
 *
 * InputMapper input;
 * // Configure input mappings...
 *
 * auto app = oc::hal::sdl::AppBuilder()
 *     .midi(std::make_unique<LibreMidiTransport>(config))
 *     .remote(std::make_unique<UdpTransport>(udpConfig))  // Optional
 *     .controllers(input)
 *     .inputConfig(Config::Input::CONFIG);
 *
 * app.begin();
 *
 * while (running) {
 *     input.handleEvent(event);
 *     app.update();
 * }
 * @endcode
 *
 * For custom drivers or mocking, use oc::app::AppBuilder directly.
 */

#include <oc/app/AppBuilder.hpp>
#include <oc/app/OpenControlApp.hpp>
#include <oc/hal/IMidiTransport.hpp>
#include <oc/hal/IFrameTransport.hpp>
#include <oc/hal/NullMidiTransport.hpp>
#include "SdlOutput.hpp"
#include "SdlButtonController.hpp"
#include "SdlEncoderController.hpp"
#include "InputMapper.hpp"
#include "SdlTime.hpp"
#include <cassert>
#include <memory>

namespace oc::hal::sdl {

/**
 * @brief SDL-optimized application builder
 *
 * Wraps oc::app::AppBuilder with convenience methods for SDL.
 * Decoupled from specific transport implementations.
 *
 * Features:
 * - Auto-configure timeProvider
 * - Auto-configure log output to console (std::cout)
 * - InputMapper integration via .controllers()
 * - Generic transport injection via .midi() and .remote()
 * - Implicit conversion to OpenControlApp (no .build() needed)
 */
class AppBuilder {
public:
    // Non-copyable (contains unique_ptrs)
    AppBuilder(const AppBuilder&) = delete;
    AppBuilder& operator=(const AppBuilder&) = delete;

    // Moveable
    AppBuilder(AppBuilder&&) = default;
    AppBuilder& operator=(AppBuilder&&) = default;

    /**
     * @brief Construct builder with default SDL time provider and logging
     *
     * Configures log output to console and time provider.
     * Call .controllers(input) to connect an InputMapper.
     */
    AppBuilder() {
        // Configure logging output to console
        oc::log::setOutput(consoleOutput());

        initTime();
        builder_.timeProvider(defaultTimeProvider);
    }

    // ═══════════════════════════════════════════════════════════════════
    // MIDI
    // ═══════════════════════════════════════════════════════════════════

    /**
     * @brief Add no-op MIDI transport
     *
     * Enables contexts that require MidiAPI to work without real MIDI hardware.
     * Uses oc::hal::NullMidiTransport from framework.
     *
     * @return Reference to this builder for chaining
     */
    AppBuilder& midi() {
        builder_.midi(std::make_unique<hal::NullMidiTransport>());
        return *this;
    }

    /**
     * @brief Add MIDI transport
     *
     * Inject any IMidiTransport implementation (LibreMidiTransport, mock, etc.)
     *
     * @param transport MIDI transport implementation
     * @return Reference to this builder for chaining
     */
    AppBuilder& midi(std::unique_ptr<IMidiTransport> transport) {
        builder_.midi(std::move(transport));
        return *this;
    }

    // ═══════════════════════════════════════════════════════════════════
    // REMOTE TRANSPORT
    // ═══════════════════════════════════════════════════════════════════

    /**
     * @brief Add remote message transport
     *
     * Inject any IFrameTransport implementation for communication with
     * remote systems (e.g., oc-bridge via UDP, WebSocket, etc.)
     *
     * @param transport Message transport implementation
     * @return Reference to this builder for chaining
     */
    AppBuilder& remote(std::unique_ptr<IFrameTransport> transport) {
        remoteTransport_ = std::move(transport);
        return *this;
    }

    // ═══════════════════════════════════════════════════════════════════
    // CONTROLLERS
    // ═══════════════════════════════════════════════════════════════════

    /**
     * @brief Connect InputMapper and create SDL controllers
     *
     * This method:
     * 1. Creates SdlButtonController and SdlEncoderController
     * 2. Connects InputMapper to the controllers (raw pointers)
     * 3. Transfers controller ownership to the framework AppBuilder
     *
     * After this call, the InputMapper holds raw pointers to controllers
     * that will be owned by the OpenControlApp. This is safe as long as
     * InputMapper is not used after the app is destroyed.
     *
     * @param input InputMapper that will be connected to controllers.
     *              Must remain valid for the lifetime of the built app.
     * @return Reference to this builder for chaining
     *
     * @warning InputMapper must have same or shorter lifetime than the app.
     */
    AppBuilder& controllers(InputMapper& input) {
        auto buttonController = std::make_unique<SdlButtonController>();
        auto encoderController = std::make_unique<SdlEncoderController>();

        // Connect InputMapper BEFORE moving ownership
        input.connect(*buttonController, *encoderController);

        // Transfer ownership to framework builder
        builder_.buttons(std::move(buttonController));
        builder_.encoders(std::move(encoderController));
        return *this;
    }

    // ═══════════════════════════════════════════════════════════════════
    // CONFIGURATION
    // ═══════════════════════════════════════════════════════════════════

    /**
     * @brief Set input gesture timing configuration
     *
     * @param config Input timing configuration (long press, double tap thresholds)
     * @return Reference to this builder for chaining
     */
    AppBuilder& inputConfig(const core::InputConfig& config) {
        builder_.inputConfig(config);
        return *this;
    }

    // ═══════════════════════════════════════════════════════════════════
    // BUILD
    // ═══════════════════════════════════════════════════════════════════

    /**
     * @brief Implicit conversion to OpenControlApp
     *
     * Enables direct assignment without calling .build():
     * @code
     * auto app = oc::hal::sdl::AppBuilder()
     *     .midi(std::make_unique<MyMidiTransport>())
     *     .controllers(input)
     *     .inputConfig(config);
     * @endcode
     */
    operator app::OpenControlApp() {
        // Transfer remote transport if configured
        if (remoteTransport_) {
            builder_.frames(std::move(remoteTransport_));
        }
        return builder_.build();
    }

private:
    app::AppBuilder builder_;
    std::unique_ptr<IFrameTransport> remoteTransport_;
};

}  // namespace oc::hal::sdl
