#pragma once

#include <compare>
#include <cstddef>
#include <string>
#include <string_view>

namespace mc::core
{
class ResourceLocation
{
public:
    ResourceLocation();
    explicit ResourceLocation(std::string_view combined);
    ResourceLocation(std::string_view nameSpace, std::string_view path);

    [[nodiscard]] const std::string& nameSpace() const noexcept;
    [[nodiscard]] const std::string& path() const noexcept;
    [[nodiscard]] std::string toString() const;

    [[nodiscard]] auto operator<=>(const ResourceLocation&) const = default;

private:
    std::string namespace_{"minecraft"};
    std::string path_{"empty"};

    static void validate(std::string_view nameSpace, std::string_view path);
};

struct ResourceLocationHash
{
    [[nodiscard]] std::size_t operator()(
        const ResourceLocation& location
    ) const noexcept;
};
}
