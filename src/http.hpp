#pragma once

#include <filesystem>
#include <optional>
#include <string>

std::optional<std::string> HttpGet(const std::string& url);
std::optional<std::string> HttpGetAndCache(const std::string& url, const std::filesystem::path& directory);
