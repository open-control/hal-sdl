#pragma once

#include <SDL.h>
#include <oc/type/Ids.hpp>
#include <oc/type/Callbacks.hpp>
#include <oc/time/Time.hpp>

namespace oc::hal::sdl {

/**
 * @brief Default time provider using SDL_GetTicks
 */
inline uint32_t defaultTimeProvider() {
    return SDL_GetTicks();
}

inline uint32_t defaultMicrosProvider() {
    static const uint64_t start = SDL_GetPerformanceCounter();
    const uint64_t elapsed = SDL_GetPerformanceCounter() - start;
    const uint64_t frequency = SDL_GetPerformanceFrequency();
    if (frequency == 0) {
        return 0;
    }
    return static_cast<uint32_t>((elapsed * 1000000ULL) / frequency);
}

/**
 * @brief Initialize the global time provider for oc::time functions
 */
inline void initTime() {
    oc::time::setProvider(defaultTimeProvider);
    oc::time::setMicrosProvider(defaultMicrosProvider);
}

} // namespace oc::hal::sdl
