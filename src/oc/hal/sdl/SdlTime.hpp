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

/**
 * @brief Initialize the global time provider for oc::time functions
 */
inline void initTime() {
    oc::time::setProvider(defaultTimeProvider);
}

} // namespace oc::hal::sdl
