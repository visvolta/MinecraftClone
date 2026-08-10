#pragma once

#include <filesystem>
#include <string_view>

class AssetPaths
{
public:
    // Call once at startup. executablePath should normally be argv[0].
    static void initialize(const char* executablePath);

    // Returns an existing asset path or throws with all searched locations.
    [[nodiscard]] static std::filesystem::path get(std::string_view relativePath);

    [[nodiscard]] static const std::filesystem::path& root();

private:
    static std::filesystem::path findAssetRoot(const char* executablePath);
};
