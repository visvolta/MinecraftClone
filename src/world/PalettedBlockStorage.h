#pragma once

#include "content/BlockState.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace mc::world
{
template<std::size_t Size>
class PalettedBlockStorage
{
public:
    using PaletteIndex = std::uint16_t;

    PalettedBlockStorage()
    {
        fill({});
    }

    [[nodiscard]] content::BlockState get(std::size_t index) const noexcept
    {
        return palette_[indices_[index]];
    }

    bool set(std::size_t index, content::BlockState state)
    {
        if (get(index) == state)
            return false;

        const auto found = reversePalette_.find(state);
        PaletteIndex paletteIndex = 0;
        if (found != reversePalette_.end())
        {
            paletteIndex = found->second;
        }
        else
        {
            if (palette_.size() >
                static_cast<std::size_t>(
                    std::numeric_limits<PaletteIndex>::max()))
            {
                throw std::overflow_error("Chunk block-state palette is full");
            }
            paletteIndex = static_cast<PaletteIndex>(palette_.size());
            palette_.push_back(state);
            reversePalette_.emplace(state, paletteIndex);
        }
        indices_[index] = paletteIndex;
        return true;
    }

    void fill(content::BlockState state)
    {
        palette_.assign(1, state);
        reversePalette_.clear();
        reversePalette_.emplace(state, 0);
        indices_.fill(0);
    }

    void restore(
        std::vector<content::BlockState> palette,
        const std::array<PaletteIndex, Size>& indices)
    {
        if (palette.empty() ||
            palette.size() >
                static_cast<std::size_t>(
                    std::numeric_limits<PaletteIndex>::max()) + 1U)
        {
            throw std::invalid_argument("Invalid chunk block-state palette");
        }
        for (const PaletteIndex index : indices)
        {
            if (index >= palette.size())
                throw std::invalid_argument("Chunk palette index is out of range");
        }

        palette_ = std::move(palette);
        indices_ = indices;
        reversePalette_.clear();
        for (std::size_t index = 0; index < palette_.size(); ++index)
        {
            reversePalette_.try_emplace(
                palette_[index],
                static_cast<PaletteIndex>(index)
            );
        }
    }

    [[nodiscard]] const std::vector<content::BlockState>& palette() const noexcept
    {
        return palette_;
    }

    [[nodiscard]] const std::array<PaletteIndex, Size>& indices() const noexcept
    {
        return indices_;
    }

private:
    std::vector<content::BlockState> palette_;
    std::array<PaletteIndex, Size> indices_{};
    std::unordered_map<
        content::BlockState,
        PaletteIndex,
        content::BlockStateHash
    > reversePalette_;
};
}
