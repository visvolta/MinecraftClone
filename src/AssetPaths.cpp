#include "AssetPaths.h"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace
{
    std::filesystem::path gAssetRoot;

    void addUniqueCandidate(
        std::vector<std::filesystem::path>& candidates,
        const std::filesystem::path& candidate)
    {
        if (candidate.empty())
            return;

        const auto normalized = candidate.lexically_normal();
        const auto duplicate = std::find(candidates.begin(), candidates.end(), normalized);
        if (duplicate == candidates.end())
            candidates.push_back(normalized);
    }

    bool looksLikeAssetRoot(const std::filesystem::path& path)
    {
        std::error_code error;
        return std::filesystem::is_directory(path, error)
            && std::filesystem::is_directory(path / "shaders", error)
            && std::filesystem::is_directory(path / "textures", error);
    }
}

void AssetPaths::initialize(const char* executablePath)
{
    gAssetRoot = findAssetRoot(executablePath);
}

std::filesystem::path AssetPaths::get(std::string_view relativePath)
{
    if (gAssetRoot.empty())
        throw std::runtime_error("AssetPaths::initialize() was not called before loading assets.");

    const std::filesystem::path fullPath = gAssetRoot / std::filesystem::path(relativePath);
    std::error_code error;
    if (!std::filesystem::is_regular_file(fullPath, error))
    {
        throw std::runtime_error(
            "Required asset does not exist: " + fullPath.string()
        );
    }

    return fullPath;
}

const std::filesystem::path& AssetPaths::root()
{
    if (gAssetRoot.empty())
        throw std::runtime_error("AssetPaths::initialize() was not called.");
    return gAssetRoot;
}

std::filesystem::path AssetPaths::findAssetRoot(const char* executablePath)
{
    std::vector<std::filesystem::path> candidates;

#ifdef MINECRAFT_CLONE_SOURCE_ASSET_DIR
    // Development builds should always prefer the current source assets.
    // This prevents an older atlas copied beside the executable from being
    // selected after the atlas layout changes.
    addUniqueCandidate(
        candidates,
        std::filesystem::path(MINECRAFT_CLONE_SOURCE_ASSET_DIR)
    );
#endif

    std::error_code error;
    const std::filesystem::path currentDirectory = std::filesystem::current_path(error);
    if (!error)
    {
        addUniqueCandidate(candidates, currentDirectory / "assets");
        addUniqueCandidate(candidates, currentDirectory.parent_path() / "assets");
        addUniqueCandidate(candidates, currentDirectory.parent_path().parent_path() / "assets");
    }

    if (executablePath != nullptr && *executablePath != '\0')
    {
        std::filesystem::path executable = std::filesystem::absolute(executablePath, error);
        if (!error)
        {
            executable = executable.lexically_normal();
            const auto executableDirectory = executable.parent_path();
            addUniqueCandidate(candidates, executableDirectory / "assets");
            addUniqueCandidate(candidates, executableDirectory.parent_path() / "assets");
            addUniqueCandidate(candidates, executableDirectory.parent_path().parent_path() / "assets");
        }
    }

    for (const auto& candidate : candidates)
    {
        if (looksLikeAssetRoot(candidate))
            return std::filesystem::weakly_canonical(candidate, error);
    }

    std::ostringstream message;
    message << "Could not locate the assets directory. Searched:";
    for (const auto& candidate : candidates)
        message << "\n  - " << candidate.string();
    message << "\nOpen the project root in VS Code and rebuild, or keep the copied assets beside the executable.";
    throw std::runtime_error(message.str());
}
