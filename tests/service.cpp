#include <SDL3/SDL.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <execution>
#include <filesystem>
#include <iostream>
#include <memory>

#include "service.hpp"

int main(int argc, char** argv)
{
    std::array<std::unique_ptr<Service>, 10> services =
    {
        ServiceCreateNRCan(),
        ServiceCreateESAWorldCover(),
        ServiceCreateOpenTopography(),
        ServiceCreateLandfireFuelModel(),
        ServiceCreateLandfireElevation(),
        ServiceCreateLandfireSlope(),
        ServiceCreateLandfireAspect(),
        ServiceCreateLandfireCanopyCover(),
        ServiceCreateLandfireCanopyHeight(),
        ServiceCreateOpenMeteo(),
    };
    const std::filesystem::path root = std::filesystem::path(SDL_GetBasePath()) / "fire_simulator_test_service";
    std::filesystem::remove_all(root);
    std::for_each(std::execution::par, services.begin(), services.end(), [&](const std::unique_ptr<Service>& service)
    {
        const std::filesystem::path directory = root / service->GetName();
        service->Download(
            service->GetSupportedTypes(),
            {48.999, -122.751},
            {49.001, -122.749},
            0.001f,
            1.0f,
            Date(2025, 1, 1),
            Date(2025, 1, 1),
            directory);
        SDL_assert(std::filesystem::exists(directory) && !std::filesystem::is_empty(directory));
        for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator(directory))
        {
            SDL_assert(entry.is_regular_file() && entry.file_size() > 0);
        }
    });
    return 0;
}
