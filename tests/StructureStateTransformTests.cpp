#include "content/BlockState.h"
#include "worldgen/StructurePrimitives.h"

#include <array>
#include <cassert>

namespace
{
using mc::content::BlockState;

void expectOrientations(
    BlockState input,
    std::array<std::uint16_t, 4> expected)
{
    constexpr std::array<mc112::Facing,4> facings{
        mc112::Facing::North,
        mc112::Facing::South,
        mc112::Facing::East,
        mc112::Facing::West
    };
    for(std::size_t i=0;i<facings.size();++i)
    {
        assert(mc112::transformStateForFacing(input,facings[i]).properties()==expected[i]);
    }
}
}

int main()
{
    // BlockLever.EnumOrientation.NORTH = metadata 4.
    expectOrientations(BlockState(BlockType::Lever,4), {4,3,1,2});

    // Piston facing WEST = EnumFacing index 4.
    expectOrientations(BlockState(BlockType::Piston,4), {4,4,2,2});

    // Repeater facing NORTH, delay 1 = horizontal index 2.
    expectOrientations(BlockState(BlockType::Repeater,2), {2,0,3,1});

    // Vine SOUTH flag = bit 0. Mirror/rotate must follow BlockVine exactly.
    expectOrientations(BlockState(BlockType::Vine,1), {1,4,2,8});
}
