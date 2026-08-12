#pragma once

class World;

namespace mc::entity
{
class WorldEntitySpawner
{
public:
    static int findChunksForSpawning(World& world);
};
}
