#include <SDL3/SDL.h>
#include <spdlog/spdlog.h>

#include <string>
#include <utility>

#include "timer.hpp"

Timer::Timer(std::string label)
    : Label{std::move(label)}
    , Start{SDL_GetTicksNS()}
{
    spdlog::info("{} started at {:.3f} ms", Label, Start / 1.0e6);
}

Timer::~Timer()
{
    Uint64 stop = SDL_GetTicksNS();
    double milliseconds = (stop - Start) / 1.0e6;
    spdlog::info("{} stopped at {:.3f} ms (took {:.3f} ms)", Label, stop / 1.0e6, milliseconds);
}
