#include "core/ResourceLocation.h"

#include <functional>
#include <stdexcept>

namespace mc::core
{
namespace
{
bool validNamespaceCharacter(char character) noexcept
{
    return (character >= 'a' && character <= 'z') ||
           (character >= '0' && character <= '9') ||
           character == '_' || character == '-' || character == '.';
}

bool validPathCharacter(char character) noexcept
{
    return validNamespaceCharacter(character) || character == '/';
}
}

ResourceLocation::ResourceLocation() = default;

ResourceLocation::ResourceLocation(std::string_view combined)
{
    const std::size_t separator = combined.find(':');
    if (separator == std::string_view::npos)
    {
        namespace_ = "minecraft";
        path_ = combined;
    }
    else
    {
        namespace_ = combined.substr(0, separator);
        path_ = combined.substr(separator + 1);
    }
    validate(namespace_, path_);
}

ResourceLocation::ResourceLocation(
    std::string_view nameSpace,
    std::string_view path)
    : namespace_(nameSpace), path_(path)
{
    validate(namespace_, path_);
}

const std::string& ResourceLocation::nameSpace() const noexcept
{
    return namespace_;
}

const std::string& ResourceLocation::path() const noexcept
{
    return path_;
}

std::string ResourceLocation::toString() const
{
    return namespace_ + ':' + path_;
}

void ResourceLocation::validate(
    std::string_view nameSpace,
    std::string_view path)
{
    if (nameSpace.empty() || path.empty())
        throw std::invalid_argument("Resource location components cannot be empty");
    for (char character : nameSpace)
    {
        if (!validNamespaceCharacter(character))
            throw std::invalid_argument("Invalid resource namespace: " + std::string(nameSpace));
    }
    for (char character : path)
    {
        if (!validPathCharacter(character))
            throw std::invalid_argument("Invalid resource path: " + std::string(path));
    }
}

std::size_t ResourceLocationHash::operator()(
    const ResourceLocation& location) const noexcept
{
    std::size_t seed = std::hash<std::string>{}(location.nameSpace());
    seed ^= std::hash<std::string>{}(location.path()) + 0x9e3779b9U +
            (seed << 6U) + (seed >> 2U);
    return seed;
}
}
