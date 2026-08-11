#pragma once

#include "content/BlockState.h"

#include <array>

namespace mc::content
{
// Horizontal neighbours are ordered north, east, south, west. In 1.12 fence,
// wall, pane, and stair connection properties are "actual state" values:
// they are derived from neighbours rather than persisted as metadata.
[[nodiscard]] BlockState resolveActualBlockState(
    BlockState state,
    const std::array<BlockState, 4>& horizontalNeighbours,
    BlockState above
);
}
