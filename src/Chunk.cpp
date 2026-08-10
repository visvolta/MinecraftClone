#include "Chunk.h"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>

Chunk::Chunk(int chunkX, int chunkZ) : chunkX(chunkX), chunkZ(chunkZ)
{
    clear();
}

mc::content::BlockState Chunk::getBlockState(int x, int y, int z) const
{
    if (!inBounds(x, y, z)) return {};
    return sections_[static_cast<std::size_t>(y / SECTION_HEIGHT)].get(
        sectionIndex(x, y, z)
    );
}

bool Chunk::setBlockState(
    int x,
    int y,
    int z,
    mc::content::BlockState state)
{
    if (!inBounds(x, y, z)) return false;
    const std::size_t section = static_cast<std::size_t>(y / SECTION_HEIGHT);
    const std::size_t localIndex = sectionIndex(x, y, z);
    const mc::content::BlockState old = sections_[section].get(localIndex);
    if (old == state) return false;
    if (old.isAir() && !state.isAir()) ++nonAirPerSection[section];
    if (!old.isAir() && state.isAir()) --nonAirPerSection[section];
    sections_[section].set(localIndex, state);
    updateHeightmaps(x, y, z, old, state);
    return true;
}

BlockType Chunk::getBlock(int x, int y, int z) const
{
    return getBlockState(x, y, z).block();
}

bool Chunk::setBlock(int x, int y, int z, BlockType block)
{
    return setBlockAndMetadata(x, y, z, block, 0);
}

bool Chunk::setBlockAndMetadata(
    int x,
    int y,
    int z,
    BlockType block,
    std::uint8_t blockMetadata)
{
    return setBlockState(
        x, y, z, mc::content::BlockState(block, blockMetadata)
    );
}

std::uint8_t Chunk::getMetadata(int x, int y, int z) const
{
    return getBlockState(x, y, z).properties();
}

bool Chunk::setMetadata(int x, int y, int z, std::uint8_t blockMetadata)
{
    if (!inBounds(x, y, z)) return false;
    return setBlockState(
        x, y, z, getBlockState(x, y, z).withProperties(blockMetadata)
    );
}

void Chunk::fill(BlockType block)
{
    const mc::content::BlockState state(block);
    for (auto& section : sections_)
        section.fill(state);
    const std::uint16_t count = block == BlockType::Air
        ? 0
        : static_cast<std::uint16_t>(SECTION_BLOCK_COUNT);
    nonAirPerSection.fill(count);
    const std::uint16_t height = block == BlockType::Air ? 0 : HEIGHT;
    worldSurfaceHeight.fill(height);
    motionBlockingHeight.fill(isSolid(block) ? height : 0);
}

void Chunk::clear()
{
    fill(BlockType::Air);
    temperatures.fill(0.8f);
    humidities.fill(0.4f);
    biomeIds.fill(VanillaBiomes::Plains);
    clearLighting();
}

void Chunk::setClimate(int x, int z, float temperature, float humidity)
{
    if (x < 0 || x >= WIDTH || z < 0 || z >= DEPTH) return;
    const auto i = columnIndex(x, z);
    temperatures[i] = std::clamp(temperature, -0.5f, 2.0f);
    humidities[i] = std::clamp(humidity, 0.0f, 1.0f);
}

float Chunk::getTemperature(int x, int z) const
{
    if (x < 0 || x >= WIDTH || z < 0 || z >= DEPTH) return 0.8f;
    return temperatures[columnIndex(x, z)];
}

float Chunk::getHumidity(int x, int z) const
{
    if (x < 0 || x >= WIDTH || z < 0 || z >= DEPTH) return 0.4f;
    return humidities[columnIndex(x, z)];
}

void Chunk::setBiome(int x, int z, BiomeId biome)
{
    if (x < 0 || x >= WIDTH || z < 0 || z >= DEPTH) return;
    biomeIds[columnIndex(x, z)] = biome;
    if (const BiomeDefinition* definition = BiomeRegistry::active().find(biome))
        setClimate(x, z, definition->temperature, definition->rainfall);
}

BiomeId Chunk::getBiome(int x, int z) const
{
    if (x < 0 || x >= WIDTH || z < 0 || z >= DEPTH)
        return VanillaBiomes::Plains;
    return biomeIds[columnIndex(x, z)];
}

int Chunk::getWorldSurfaceHeight(int x, int z) const
{
    if (x < 0 || x >= WIDTH || z < 0 || z >= DEPTH) return 0;
    return worldSurfaceHeight[columnIndex(x, z)];
}

int Chunk::getMotionBlockingHeight(int x, int z) const
{
    if (x < 0 || x >= WIDTH || z < 0 || z >= DEPTH) return 0;
    return motionBlockingHeight[columnIndex(x, z)];
}

bool Chunk::isSectionEmpty(int sectionY) const
{
    return sectionY < 0 || sectionY >= SECTION_COUNT ||
           nonAirPerSection[static_cast<std::size_t>(sectionY)] == 0;
}

int Chunk::getChunkX() const { return chunkX; }
int Chunk::getChunkZ() const { return chunkZ; }
int Chunk::getWorldOriginX() const { return chunkX * WIDTH; }
int Chunk::getWorldOriginZ() const { return chunkZ * DEPTH; }

ChunkSnapshot Chunk::snapshot() const
{
    ChunkSnapshot result;
    result.chunkX = chunkX;
    result.chunkZ = chunkZ;
    result.temperatures = temperatures;
    result.humidities = humidities;
    result.biomeIds = biomeIds;
    result.worldSurfaceHeight = worldSurfaceHeight;
    result.motionBlockingHeight = motionBlockingHeight;
    result.palette.clear();
    result.paletteIndices.resize(BLOCK_COUNT);

    std::unordered_map<
        mc::content::BlockState,
        std::uint16_t,
        mc::content::BlockStateHash
    > reverse;
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int z = 0; z < DEPTH; ++z)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                const mc::content::BlockState state = getBlockState(x, y, z);
                auto found = reverse.find(state);
                if (found == reverse.end())
                {
                    const auto paletteIndex = static_cast<std::uint16_t>(
                        result.palette.size()
                    );
                    result.palette.push_back(state);
                    found = reverse.emplace(state, paletteIndex).first;
                }
                result.paletteIndices[index(x, y, z)] = found->second;
            }
        }
    }
    return result;
}

void Chunk::restore(const ChunkSnapshot& state)
{
    if (state.palette.empty() || state.paletteIndices.size() != BLOCK_COUNT)
        throw std::invalid_argument("Invalid 1.12 chunk snapshot");
    chunkX = state.chunkX;
    chunkZ = state.chunkZ;
    temperatures = state.temperatures;
    humidities = state.humidities;
    biomeIds = state.biomeIds;
    nonAirPerSection.fill(0);
    worldSurfaceHeight.fill(0);
    motionBlockingHeight.fill(0);
    for (auto& section : sections_)
        section.fill({});

    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int z = 0; z < DEPTH; ++z)
        {
            for (int x = 0; x < WIDTH; ++x)
            {
                const std::size_t global = index(x, y, z);
                const std::uint16_t paletteIndex = state.paletteIndices[global];
                if (paletteIndex >= state.palette.size())
                    throw std::invalid_argument("Chunk palette index is invalid");
                setBlockState(x, y, z, state.palette[paletteIndex]);
            }
        }
    }
    clearLighting();
}

std::uint8_t Chunk::getSkyLight(int x, int y, int z) const
{
    if (!inBounds(x, y, z)) return 15;
    return skyLight[index(x, y, z)];
}

std::uint8_t Chunk::getBlockLight(int x, int y, int z) const
{
    if (!inBounds(x, y, z)) return 0;
    return blockLight[index(x, y, z)];
}

void Chunk::setSkyLight(int x, int y, int z, std::uint8_t level)
{
    if (!inBounds(x, y, z)) return;
    skyLight[index(x, y, z)] = std::min<std::uint8_t>(level, 15);
}

void Chunk::setBlockLight(int x, int y, int z, std::uint8_t level)
{
    if (!inBounds(x, y, z)) return;
    blockLight[index(x, y, z)] = std::min<std::uint8_t>(level, 15);
}

void Chunk::clearLighting()
{
    skyLight.fill(0);
    blockLight.fill(0);
}

const std::array<std::uint8_t, Chunk::BLOCK_COUNT>&
Chunk::skyLightData() const noexcept { return skyLight; }

const std::array<std::uint8_t, Chunk::BLOCK_COUNT>&
Chunk::blockLightData() const noexcept { return blockLight; }

void Chunk::updateHeightmaps(
    int x,
    int y,
    int z,
    mc::content::BlockState oldState,
    mc::content::BlockState newState)
{
    const std::size_t column = columnIndex(x, z);
    const auto update = [this, x, y, z, column](
        std::array<std::uint16_t, COLUMN_COUNT>& heights,
        bool oldCounts,
        bool newCounts,
        bool solidOnly)
    {
        if (newCounts && y + 1 > heights[column])
        {
            heights[column] = static_cast<std::uint16_t>(y + 1);
            return;
        }
        if (!oldCounts || newCounts || y + 1 != heights[column])
            return;
        for (int scan = y - 1; scan >= 0; --scan)
        {
            const BlockType block = getBlock(x, scan, z);
            if ((!solidOnly && block != BlockType::Air) ||
                (solidOnly && isSolid(block)))
            {
                heights[column] = static_cast<std::uint16_t>(scan + 1);
                return;
            }
        }
        heights[column] = 0;
    };

    update(
        worldSurfaceHeight,
        !oldState.isAir(),
        !newState.isAir(),
        false
    );
    update(
        motionBlockingHeight,
        isSolid(oldState.block()),
        isSolid(newState.block()),
        true
    );
}
