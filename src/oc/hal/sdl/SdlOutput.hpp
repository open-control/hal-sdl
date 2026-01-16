#pragma once

/**
 * @file DesktopOutput.hpp
 * @brief Desktop-specific log output using std::cout
 *
 * Provides the oc::log::Output implementation for desktop platforms.
 * Uses standard C++ iostream for output.
 *
 * Usage: Automatically configured by AppBuilder constructor.
 * No manual configuration needed.
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <oc/log/Log.hpp>

namespace oc::hal::sdl {

namespace detail {

/**
 * @brief Get the start time for timestamp calculations
 */
inline auto& getStartTime() {
    static auto start = std::chrono::steady_clock::now();
    return start;
}

}  // namespace detail

/**
 * @brief Get the console-based log output for Desktop
 *
 * Returns a reference to a static Output instance configured
 * for std::cout output with timestamps relative to app start.
 *
 * @return Reference to the Desktop console output implementation
 */
inline const oc::log::Output& consoleOutput() {
    static const oc::log::Output output = {
        // printChar
        [](char c) { std::cout << c; },
        // printStr
        [](const char* str) { std::cout << str; },
        // printInt32
        [](int32_t value) { std::cout << value; },
        // printUint32
        [](uint32_t value) { std::cout << value; },
        // printFloat
        [](float value) { std::cout << std::fixed << std::setprecision(4) << value; },
        // printBool
        [](bool value) { std::cout << (value ? "true" : "false"); },
        // getTimeMs
        []() -> uint32_t {
            auto now = std::chrono::steady_clock::now();
            return static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - detail::getStartTime()
                ).count()
            );
        }
    };
    return output;
}

}  // namespace oc::hal::sdl
