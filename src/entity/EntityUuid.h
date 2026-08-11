#pragma once

#include <cstdint>

namespace mc::entity
{
struct EntityUuid
{
    std::uint64_t most = 0;
    std::uint64_t least = 0;

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return most == 0 && least == 0;
    }

    [[nodiscard]] static EntityUuid random();

    friend constexpr bool operator==(
        const EntityUuid&,
        const EntityUuid&
    ) noexcept = default;
};
}
