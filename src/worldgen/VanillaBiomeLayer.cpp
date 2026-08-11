#include "worldgen/VanillaBiomeLayer.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <utility>
#include <vector>

namespace
{
constexpr std::uint64_t Multiplier = 6364136223846793005ULL;
constexpr std::uint64_t Addend = 1442695040888963407ULL;
using Values = std::vector<int>;

std::size_t at(int x, int z, int width) noexcept
{
    return static_cast<std::size_t>(x + z * width);
}

std::uint64_t mix(std::uint64_t value, std::uint64_t addend) noexcept
{
    return value * (value * Multiplier + Addend) + addend;
}

bool oceanic(int id) noexcept
{
    return id == VanillaBiomes::Ocean || id == VanillaBiomes::DeepOcean ||
           id == VanillaBiomes::FrozenOcean;
}

bool contains(int id, std::initializer_list<BiomeId> ids) noexcept
{
    return std::find(ids.begin(), ids.end(), static_cast<BiomeId>(id)) != ids.end();
}

enum class BiomeClass : std::uint8_t
{
    Other,
    Ocean,
    Plains,
    Desert,
    Hills,
    Forest,
    Taiga,
    Swamp,
    River,
    Snow,
    Mushroom,
    Beach,
    Jungle,
    StoneBeach,
    Savanna,
    Mesa
};

BiomeClass biomeClass(int id) noexcept
{
    if (contains(id, {VanillaBiomes::Ocean, VanillaBiomes::FrozenOcean,
                      VanillaBiomes::DeepOcean})) return BiomeClass::Ocean;
    if (contains(id, {VanillaBiomes::Plains, VanillaBiomes::SunflowerPlains}))
        return BiomeClass::Plains;
    if (contains(id, {VanillaBiomes::Desert, VanillaBiomes::DesertHills,
                      VanillaBiomes::DesertMountains})) return BiomeClass::Desert;
    if (contains(id, {VanillaBiomes::ExtremeHills,
                      VanillaBiomes::ExtremeHillsEdge,
                      VanillaBiomes::ExtremeHillsPlus,
                      VanillaBiomes::ExtremeHillsMountains,
                      VanillaBiomes::ExtremeHillsPlusMountains}))
        return BiomeClass::Hills;
    if (contains(id, {VanillaBiomes::Forest, VanillaBiomes::ForestHills,
                      VanillaBiomes::BirchForest,
                      VanillaBiomes::BirchForestHills,
                      VanillaBiomes::RoofedForest,
                      VanillaBiomes::FlowerForest,
                      VanillaBiomes::BirchForestMountains,
                      VanillaBiomes::BirchForestHillsMountains,
                      VanillaBiomes::RoofedForestMountains}))
        return BiomeClass::Forest;
    if (contains(id, {VanillaBiomes::Taiga, VanillaBiomes::TaigaHills,
                      VanillaBiomes::ColdTaiga,
                      VanillaBiomes::ColdTaigaHills,
                      VanillaBiomes::MegaTaiga,
                      VanillaBiomes::MegaTaigaHills,
                      VanillaBiomes::TaigaMountains,
                      VanillaBiomes::ColdTaigaMountains,
                      VanillaBiomes::MegaSpruceTaiga,
                      VanillaBiomes::MegaSpruceTaigaHills}))
        return BiomeClass::Taiga;
    if (contains(id, {VanillaBiomes::Swampland,
                      VanillaBiomes::SwamplandMountains})) return BiomeClass::Swamp;
    if (contains(id, {VanillaBiomes::River, VanillaBiomes::FrozenRiver}))
        return BiomeClass::River;
    if (contains(id, {VanillaBiomes::IcePlains, VanillaBiomes::IceMountains,
                      VanillaBiomes::IcePlainsSpikes})) return BiomeClass::Snow;
    if (contains(id, {VanillaBiomes::MushroomIsland,
                      VanillaBiomes::MushroomShore})) return BiomeClass::Mushroom;
    if (contains(id, {VanillaBiomes::Beach, VanillaBiomes::ColdBeach}))
        return BiomeClass::Beach;
    if (contains(id, {VanillaBiomes::Jungle, VanillaBiomes::JungleHills,
                      VanillaBiomes::JungleEdge,
                      VanillaBiomes::JungleMountains,
                      VanillaBiomes::JungleEdgeMountains})) return BiomeClass::Jungle;
    if (id == VanillaBiomes::StoneBeach) return BiomeClass::StoneBeach;
    if (contains(id, {VanillaBiomes::Savanna, VanillaBiomes::SavannaPlateau,
                      VanillaBiomes::SavannaMountains,
                      VanillaBiomes::SavannaPlateauMountains})) return BiomeClass::Savanna;
    if (contains(id, {VanillaBiomes::Mesa, VanillaBiomes::MesaPlateauF,
                      VanillaBiomes::MesaPlateau, VanillaBiomes::MesaBryce,
                      VanillaBiomes::MesaPlateauFMountains,
                      VanillaBiomes::MesaPlateauMountains})) return BiomeClass::Mesa;
    return BiomeClass::Other;
}

bool biomesEqualOrMesaPlateau(int a, int b) noexcept
{
    if (a == b) return true;
    const BiomeClass ca = biomeClass(a);
    const BiomeClass cb = biomeClass(b);
    if (ca == BiomeClass::Mesa &&
        (a == VanillaBiomes::MesaPlateauF || a == VanillaBiomes::MesaPlateau))
        return b == VanillaBiomes::MesaPlateauF || b == VanillaBiomes::MesaPlateau;
    return ca != BiomeClass::Other && ca == cb;
}

enum class TempCategory : std::uint8_t { Ocean, Cold, Medium, Warm };
TempCategory tempCategory(int id) noexcept
{
    if (oceanic(id)) return TempCategory::Ocean;
    const BiomeDefinition* biome = BiomeRegistry::active().find(
        static_cast<BiomeId>(id));
    const float temperature = biome == nullptr ? 0.5f : biome->temperature;
    if (temperature < 0.2f) return TempCategory::Cold;
    if (temperature < 1.0f) return TempCategory::Medium;
    return TempCategory::Warm;
}

bool canBiomesBeNeighbors(int a, int b) noexcept
{
    if (biomesEqualOrMesaPlateau(a, b)) return true;
    const TempCategory ta = tempCategory(a);
    const TempCategory tb = tempCategory(b);
    return ta == tb || ta == TempCategory::Medium || tb == TempCategory::Medium;
}

bool snowy(int id) noexcept
{
    const BiomeDefinition* biome = BiomeRegistry::active().find(
        static_cast<BiomeId>(id));
    return biome != nullptr && biome->snowy;
}

int mutationFor(int id) noexcept
{
    for (const BiomeDefinition& biome : BiomeRegistry::active().entries())
        if (biome.mutationOf && *biome.mutationOf == static_cast<BiomeId>(id))
            return static_cast<int>(biome.id);
    return -1;
}

bool isMutation(int id) noexcept
{
    const BiomeDefinition* biome = BiomeRegistry::active().find(
        static_cast<BiomeId>(id));
    return biome != nullptr && biome->mutationOf.has_value();
}

class Layer
{
public:
    explicit Layer(std::int64_t seed)
    {
        const std::uint64_t original = static_cast<std::uint64_t>(seed);
        baseSeed_ = original;
        for (int i = 0; i < 3; ++i)
            baseSeed_ = mix(baseSeed_, original);
    }
    virtual ~Layer() = default;

    virtual void initWorldSeed(std::int64_t seed)
    {
        if (parent_) parent_->initWorldSeed(seed);
        worldSeed_ = static_cast<std::uint64_t>(seed);
        for (int i = 0; i < 3; ++i)
            worldSeed_ = mix(worldSeed_, baseSeed_);
    }

    virtual Values getInts(int x, int z, int width, int depth) = 0;

protected:
    std::shared_ptr<Layer> parent_;

    void initChunkSeed(std::int64_t x, std::int64_t z) noexcept
    {
        chunkSeed_ = worldSeed_;
        chunkSeed_ = mix(chunkSeed_, static_cast<std::uint64_t>(x));
        chunkSeed_ = mix(chunkSeed_, static_cast<std::uint64_t>(z));
        chunkSeed_ = mix(chunkSeed_, static_cast<std::uint64_t>(x));
        chunkSeed_ = mix(chunkSeed_, static_cast<std::uint64_t>(z));
    }

    int nextInt(int bound) noexcept
    {
        assert(bound > 0);
        const std::int64_t signedSeed = std::bit_cast<std::int64_t>(chunkSeed_);
        int value = static_cast<int>((signedSeed >> 24) % bound);
        if (value < 0) value += bound;
        chunkSeed_ = mix(chunkSeed_, worldSeed_);
        return value;
    }

    int selectRandom(std::initializer_list<int> values) noexcept
    {
        const int index = nextInt(static_cast<int>(values.size()));
        return *(values.begin() + index);
    }

    int selectModeOrRandom(int a, int b, int c, int d) noexcept
    {
        if (b == c && c == d) return b;
        if (a == b && a == c) return a;
        if (a == b && a == d) return a;
        if (a == c && a == d) return a;
        if (a == b && c != d) return a;
        if (a == c && b != d) return a;
        if (a == d && b != c) return a;
        if (b == c && a != d) return b;
        if (b == d && a != c) return b;
        if (c == d && a != b) return c;
        return selectRandom({a, b, c, d});
    }

private:
    std::uint64_t baseSeed_ = 0;
    std::uint64_t worldSeed_ = 0;
    std::uint64_t chunkSeed_ = 0;
};

class IslandLayer final : public Layer
{
public:
    explicit IslandLayer(std::int64_t seed) : Layer(seed) {}
    Values getInts(int x, int z, int width, int depth) override
    {
        Values out(static_cast<std::size_t>(width * depth));
        for (int dz = 0; dz < depth; ++dz)
        for (int dx = 0; dx < width; ++dx)
        {
            initChunkSeed(x + dx, z + dz);
            out[at(dx, dz, width)] = nextInt(10) == 0 ? 1 : 0;
        }
        if (x > -width && x <= 0 && z > -depth && z <= 0)
            out[at(-x, -z, width)] = 1;
        return out;
    }
};

class ZoomLayer : public Layer
{
public:
    ZoomLayer(std::int64_t seed, std::shared_ptr<Layer> parent, bool fuzzy = false)
        : Layer(seed), fuzzy_(fuzzy) { parent_ = std::move(parent); }

    Values getInts(int areaX, int areaZ, int areaWidth, int areaDepth) override
    {
        const int parentX = areaX >> 1;
        const int parentZ = areaZ >> 1;
        const int parentWidth = (areaWidth >> 1) + 2;
        const int parentDepth = (areaDepth >> 1) + 2;
        const Values parent = parent_->getInts(parentX, parentZ, parentWidth, parentDepth);
        const int expandedWidth = (parentWidth - 1) << 1;
        const int expandedDepth = (parentDepth - 1) << 1;
        Values expanded(static_cast<std::size_t>(expandedWidth * expandedDepth));

        for (int pz = 0; pz < parentDepth - 1; ++pz)
        {
            int nw = parent[at(0, pz, parentWidth)];
            int sw = parent[at(0, pz + 1, parentWidth)];
            for (int px = 0; px < parentWidth - 1; ++px)
            {
                initChunkSeed((px + parentX) << 1, (pz + parentZ) << 1);
                const int ne = parent[at(px + 1, pz, parentWidth)];
                const int se = parent[at(px + 1, pz + 1, parentWidth)];
                expanded[at(px * 2, pz * 2, expandedWidth)] = nw;
                expanded[at(px * 2, pz * 2 + 1, expandedWidth)] = selectRandom({nw, sw});
                expanded[at(px * 2 + 1, pz * 2, expandedWidth)] = selectRandom({nw, ne});
                expanded[at(px * 2 + 1, pz * 2 + 1, expandedWidth)] =
                    fuzzy_ ? selectRandom({nw, ne, sw, se})
                           : selectModeOrRandom(nw, ne, sw, se);
                nw = ne;
                sw = se;
            }
        }

        Values out(static_cast<std::size_t>(areaWidth * areaDepth));
        const int offX = areaX & 1;
        const int offZ = areaZ & 1;
        for (int z = 0; z < areaDepth; ++z)
            for (int x = 0; x < areaWidth; ++x)
                out[at(x, z, areaWidth)] = expanded[at(x + offX, z + offZ, expandedWidth)];
        return out;
    }

private:
    bool fuzzy_ = false;
};

std::shared_ptr<Layer> magnify(
    std::int64_t seed, std::shared_ptr<Layer> layer, int times)
{
    for (int i = 0; i < times; ++i)
        layer = std::make_shared<ZoomLayer>(seed + i, layer);
    return layer;
}

class AddIslandLayer final : public Layer
{
public:
    AddIslandLayer(std::int64_t seed, std::shared_ptr<Layer> parent) : Layer(seed)
    { parent_ = std::move(parent); }

    Values getInts(int areaX, int areaZ, int width, int depth) override
    {
        const int px0 = areaX - 1, pz0 = areaZ - 1;
        const int pw = width + 2, pd = depth + 2;
        const Values p = parent_->getInts(px0, pz0, pw, pd);
        Values out(static_cast<std::size_t>(width * depth));
        for (int z = 0; z < depth; ++z)
        for (int x = 0; x < width; ++x)
        {
            const int nw = p[at(x, z, pw)];
            const int ne = p[at(x + 2, z, pw)];
            const int sw = p[at(x, z + 2, pw)];
            const int se = p[at(x + 2, z + 2, pw)];
            const int center = p[at(x + 1, z + 1, pw)];
            initChunkSeed(areaX + x, areaZ + z);
            if (center != 0 || (nw == 0 && ne == 0 && sw == 0 && se == 0))
            {
                if (center > 0 && (nw == 0 || ne == 0 || sw == 0 || se == 0))
                {
                    if (nextInt(5) == 0)
                        out[at(x, z, width)] = center == 4 ? 4 : 0;
                    else
                        out[at(x, z, width)] = center;
                }
                else out[at(x, z, width)] = center;
            }
            else
            {
                int count = 1, chosen = 1;
                if (nw != 0 && nextInt(count++) == 0) chosen = nw;
                if (ne != 0 && nextInt(count++) == 0) chosen = ne;
                if (sw != 0 && nextInt(count++) == 0) chosen = sw;
                if (se != 0 && nextInt(count++) == 0) chosen = se;
                if (nextInt(3) == 0) out[at(x, z, width)] = chosen;
                else out[at(x, z, width)] = chosen == 4 ? 4 : 0;
            }
        }
        return out;
    }
};

class RemoveTooMuchOceanLayer final : public Layer
{
public:
    RemoveTooMuchOceanLayer(std::int64_t seed, std::shared_ptr<Layer> parent) : Layer(seed)
    { parent_ = std::move(parent); }
    Values getInts(int areaX, int areaZ, int width, int depth) override
    {
        const int pw = width + 2;
        const Values p = parent_->getInts(areaX - 1, areaZ - 1, pw, depth + 2);
        Values out(static_cast<std::size_t>(width * depth));
        for (int z = 0; z < depth; ++z)
        for (int x = 0; x < width; ++x)
        {
            const int north = p[at(x + 1, z, pw)];
            const int east = p[at(x + 2, z + 1, pw)];
            const int west = p[at(x, z + 1, pw)];
            const int south = p[at(x + 1, z + 2, pw)];
            const int center = p[at(x + 1, z + 1, pw)];
            out[at(x, z, width)] = center;
            initChunkSeed(areaX + x, areaZ + z);
            if (center == 0 && north == 0 && east == 0 && west == 0 && south == 0 &&
                nextInt(2) == 0)
                out[at(x, z, width)] = 1;
        }
        return out;
    }
};

class AddSnowLayer final : public Layer
{
public:
    AddSnowLayer(std::int64_t seed, std::shared_ptr<Layer> parent) : Layer(seed)
    { parent_ = std::move(parent); }
    Values getInts(int areaX, int areaZ, int width, int depth) override
    {
        const int pw = width + 2;
        const Values p = parent_->getInts(areaX - 1, areaZ - 1, pw, depth + 2);
        Values out(static_cast<std::size_t>(width * depth));
        for (int z = 0; z < depth; ++z)
        for (int x = 0; x < width; ++x)
        {
            const int center = p[at(x + 1, z + 1, pw)];
            initChunkSeed(areaX + x, areaZ + z);
            if (center == 0) out[at(x, z, width)] = 0;
            else
            {
                int value = nextInt(6);
                if (value == 0) value = 4;
                else if (value <= 1) value = 3;
                else value = 1;
                out[at(x, z, width)] = value;
            }
        }
        return out;
    }
};

enum class EdgeMode { CoolWarm, HeatIce, Special };
class EdgeLayer final : public Layer
{
public:
    EdgeLayer(std::int64_t seed, std::shared_ptr<Layer> parent, EdgeMode mode)
        : Layer(seed), mode_(mode) { parent_ = std::move(parent); }
    Values getInts(int areaX, int areaZ, int width, int depth) override
    {
        if (mode_ == EdgeMode::Special)
        {
            const Values p = parent_->getInts(areaX, areaZ, width, depth);
            Values out(p.size());
            for (int z = 0; z < depth; ++z)
            for (int x = 0; x < width; ++x)
            {
                initChunkSeed(areaX + x, areaZ + z);
                int value = p[at(x, z, width)];
                if (value != 0 && nextInt(13) == 0)
                    value |= (1 + nextInt(15)) << 8 & 3840;
                out[at(x, z, width)] = value;
            }
            return out;
        }
        const int pw = width + 2;
        const Values p = parent_->getInts(areaX - 1, areaZ - 1, pw, depth + 2);
        Values out(static_cast<std::size_t>(width * depth));
        for (int z = 0; z < depth; ++z)
        for (int x = 0; x < width; ++x)
        {
            int center = p[at(x + 1, z + 1, pw)];
            if (mode_ == EdgeMode::CoolWarm && center == 1)
            {
                const int n = p[at(x + 1, z, pw)], e = p[at(x + 2, z + 1, pw)];
                const int w = p[at(x, z + 1, pw)], s = p[at(x + 1, z + 2, pw)];
                if (n == 3 || e == 3 || w == 3 || s == 3 ||
                    n == 4 || e == 4 || w == 4 || s == 4) center = 2;
            }
            else if (mode_ == EdgeMode::HeatIce && center == 4)
            {
                const int n = p[at(x + 1, z, pw)], e = p[at(x + 2, z + 1, pw)];
                const int w = p[at(x, z + 1, pw)], s = p[at(x + 1, z + 2, pw)];
                if (n == 2 || e == 2 || w == 2 || s == 2 ||
                    n == 1 || e == 1 || w == 1 || s == 1) center = 3;
            }
            out[at(x, z, width)] = center;
        }
        return out;
    }
private:
    EdgeMode mode_;
};

class AddMushroomLayer final : public Layer
{
public:
    AddMushroomLayer(std::int64_t seed, std::shared_ptr<Layer> parent) : Layer(seed)
    { parent_ = std::move(parent); }
    Values getInts(int areaX, int areaZ, int width, int depth) override
    {
        const int pw = width + 2;
        const Values p = parent_->getInts(areaX - 1, areaZ - 1, pw, depth + 2);
        Values out(static_cast<std::size_t>(width * depth));
        for (int z = 0; z < depth; ++z)
        for (int x = 0; x < width; ++x)
        {
            const int nw = p[at(x, z, pw)], ne = p[at(x + 2, z, pw)];
            const int sw = p[at(x, z + 2, pw)], se = p[at(x + 2, z + 2, pw)];
            const int c = p[at(x + 1, z + 1, pw)];
            initChunkSeed(areaX + x, areaZ + z);
            out[at(x, z, width)] = c == 0 && nw == 0 && ne == 0 && sw == 0 && se == 0 &&
                nextInt(100) == 0 ? VanillaBiomes::MushroomIsland : c;
        }
        return out;
    }
};

class DeepOceanLayer final : public Layer
{
public:
    DeepOceanLayer(std::int64_t seed, std::shared_ptr<Layer> parent) : Layer(seed)
    { parent_ = std::move(parent); }
    Values getInts(int areaX, int areaZ, int width, int depth) override
    {
        const int pw = width + 2;
        const Values p = parent_->getInts(areaX - 1, areaZ - 1, pw, depth + 2);
        Values out(static_cast<std::size_t>(width * depth));
        for (int z = 0; z < depth; ++z)
        for (int x = 0; x < width; ++x)
        {
            int oceans = 0;
            oceans += p[at(x + 1, z, pw)] == 0;
            oceans += p[at(x + 2, z + 1, pw)] == 0;
            oceans += p[at(x, z + 1, pw)] == 0;
            oceans += p[at(x + 1, z + 2, pw)] == 0;
            const int c = p[at(x + 1, z + 1, pw)];
            out[at(x, z, width)] = c == 0 && oceans > 3 ? VanillaBiomes::DeepOcean : c;
        }
        return out;
    }
};

class RiverInitLayer final : public Layer
{
public:
    RiverInitLayer(std::int64_t seed, std::shared_ptr<Layer> parent) : Layer(seed)
    { parent_ = std::move(parent); }
    Values getInts(int areaX, int areaZ, int width, int depth) override
    {
        const Values p = parent_->getInts(areaX, areaZ, width, depth);
        Values out(p.size());
        for (int z = 0; z < depth; ++z)
        for (int x = 0; x < width; ++x)
        {
            initChunkSeed(areaX + x, areaZ + z);
            out[at(x, z, width)] = p[at(x, z, width)] > 0 ? nextInt(299999) + 2 : 0;
        }
        return out;
    }
};

class BiomeLayer final : public Layer
{
public:
    BiomeLayer(std::int64_t seed, std::shared_ptr<Layer> parent) : Layer(seed)
    { parent_ = std::move(parent); }
    Values getInts(int areaX, int areaZ, int width, int depth) override
    {
        static constexpr std::array<int, 6> warm = {2,2,2,35,35,1};
        static constexpr std::array<int, 6> medium = {4,29,3,1,27,6};
        static constexpr std::array<int, 4> cold = {4,3,5,1};
        static constexpr std::array<int, 4> ice = {12,12,12,30};
        const Values p = parent_->getInts(areaX, areaZ, width, depth);
        Values out(p.size());
        for (int z = 0; z < depth; ++z)
        for (int x = 0; x < width; ++x)
        {
            initChunkSeed(areaX + x, areaZ + z);
            int value = p[at(x, z, width)];
            const int special = (value & 3840) >> 8;
            value &= -3841;
            if (oceanic(value) || value == VanillaBiomes::MushroomIsland)
                out[at(x, z, width)] = value;
            else if (value == 1)
            {
                if (special > 0)
                    out[at(x, z, width)] = nextInt(3) == 0 ? VanillaBiomes::MesaPlateau : VanillaBiomes::MesaPlateauF;
                else out[at(x, z, width)] = warm[static_cast<std::size_t>(nextInt(static_cast<int>(warm.size())))];
            }
            else if (value == 2)
                out[at(x, z, width)] = special > 0 ? VanillaBiomes::Jungle : medium[static_cast<std::size_t>(nextInt(static_cast<int>(medium.size())))];
            else if (value == 3)
                out[at(x, z, width)] = special > 0 ? VanillaBiomes::MegaTaiga : cold[static_cast<std::size_t>(nextInt(static_cast<int>(cold.size())))];
            else if (value == 4)
                out[at(x, z, width)] = ice[static_cast<std::size_t>(nextInt(static_cast<int>(ice.size())))];
            else out[at(x, z, width)] = VanillaBiomes::MushroomIsland;
        }
        return out;
    }
};

class BiomeEdgeLayer final : public Layer
{
public:
    BiomeEdgeLayer(std::int64_t seed, std::shared_ptr<Layer> parent) : Layer(seed)
    { parent_ = std::move(parent); }
    Values getInts(int areaX, int areaZ, int width, int depth) override
    {
        const int pw = width + 2;
        const Values p = parent_->getInts(areaX - 1, areaZ - 1, pw, depth + 2);
        Values out(static_cast<std::size_t>(width * depth));
        const auto neighbors = [&](int x, int z)
        {
            return std::array<int,4>{p[at(x+1,z,pw)], p[at(x+2,z+1,pw)],
                                     p[at(x,z+1,pw)], p[at(x+1,z+2,pw)]};
        };
        for (int z = 0; z < depth; ++z)
        for (int x = 0; x < width; ++x)
        {
            initChunkSeed(areaX + x, areaZ + z);
            const int c = p[at(x + 1, z + 1, pw)];
            const auto n = neighbors(x,z);
            auto replaceNecessary = [&](int target, int edge, int& result)
            {
                if (!biomesEqualOrMesaPlateau(c, target)) return false;
                result = std::all_of(n.begin(), n.end(), [&](int v){ return canBiomesBeNeighbors(v,target); }) ? c : edge;
                return true;
            };
            auto replace = [&](int target, int edge, int& result)
            {
                if (c != target) return false;
                result = std::all_of(n.begin(), n.end(), [&](int v){ return biomesEqualOrMesaPlateau(v,target); }) ? c : edge;
                return true;
            };
            int result = c;
            if (replaceNecessary(VanillaBiomes::ExtremeHills, VanillaBiomes::ExtremeHillsEdge, result) ||
                replace(VanillaBiomes::MesaPlateauF, VanillaBiomes::Mesa, result) ||
                replace(VanillaBiomes::MesaPlateau, VanillaBiomes::Mesa, result) ||
                replace(VanillaBiomes::MegaTaiga, VanillaBiomes::Taiga, result))
            {
                out[at(x,z,width)] = result;
                continue;
            }
            if (c == VanillaBiomes::Desert)
            {
                const bool ice = std::find(n.begin(), n.end(), VanillaBiomes::IcePlains) != n.end();
                result = ice ? VanillaBiomes::ExtremeHillsPlus : c;
            }
            else if (c == VanillaBiomes::Swampland)
            {
                const bool bad = std::find(n.begin(), n.end(), VanillaBiomes::Desert) != n.end() ||
                    std::find(n.begin(), n.end(), VanillaBiomes::ColdTaiga) != n.end() ||
                    std::find(n.begin(), n.end(), VanillaBiomes::IcePlains) != n.end();
                if (bad) result = VanillaBiomes::Plains;
                else if (std::find(n.begin(), n.end(), VanillaBiomes::Jungle) != n.end())
                    result = VanillaBiomes::JungleEdge;
            }
            out[at(x,z,width)] = result;
        }
        return out;
    }
};

class RiverLayer final : public Layer
{
public:
    RiverLayer(std::int64_t seed, std::shared_ptr<Layer> parent) : Layer(seed)
    { parent_ = std::move(parent); }
    Values getInts(int areaX, int areaZ, int width, int depth) override
    {
        const int pw = width + 2;
        const Values p = parent_->getInts(areaX - 1, areaZ - 1, pw, depth + 2);
        Values out(static_cast<std::size_t>(width * depth));
        const auto filter=[](int v){ return v >= 2 ? 2 + (v & 1) : v; };
        for (int z=0; z<depth; ++z)
        for (int x=0; x<width; ++x)
        {
            const int w=filter(p[at(x,z+1,pw)]), e=filter(p[at(x+2,z+1,pw)]);
            const int n=filter(p[at(x+1,z,pw)]), s=filter(p[at(x+1,z+2,pw)]);
            const int c=filter(p[at(x+1,z+1,pw)]);
            out[at(x,z,width)] = c==w && c==e && c==n && c==s ? -1 : VanillaBiomes::River;
        }
        return out;
    }
};

class SmoothLayer final : public Layer
{
public:
    SmoothLayer(std::int64_t seed, std::shared_ptr<Layer> parent) : Layer(seed)
    { parent_ = std::move(parent); }
    Values getInts(int areaX, int areaZ, int width, int depth) override
    {
        const int pw=width+2;
        const Values p=parent_->getInts(areaX-1, areaZ-1, pw, depth+2);
        Values out(static_cast<std::size_t>(width*depth));
        for(int z=0;z<depth;++z) for(int x=0;x<width;++x)
        {
            const int w=p[at(x,z+1,pw)], e=p[at(x+2,z+1,pw)];
            const int n=p[at(x+1,z,pw)], s=p[at(x+1,z+2,pw)];
            int c=p[at(x+1,z+1,pw)];
            if(w==e && n==s){ initChunkSeed(areaX+x,areaZ+z); c=nextInt(2)==0?w:n; }
            else { if(w==e)c=w; if(n==s)c=n; }
            out[at(x,z,width)]=c;
        }
        return out;
    }
};

class RareBiomeLayer final : public Layer
{
public:
    RareBiomeLayer(std::int64_t seed, std::shared_ptr<Layer> parent) : Layer(seed)
    { parent_=std::move(parent); }
    Values getInts(int areaX,int areaZ,int width,int depth) override
    {
        const int pw=width+2;
        const Values p=parent_->getInts(areaX-1,areaZ-1,pw,depth+2);
        Values out(static_cast<std::size_t>(width*depth));
        for(int z=0;z<depth;++z) for(int x=0;x<width;++x)
        {
            initChunkSeed(areaX+x,areaZ+z);
            const int c=p[at(x+1,z+1,pw)];
            out[at(x,z,width)]=(nextInt(57)==0 && c==VanillaBiomes::Plains)
                ? VanillaBiomes::SunflowerPlains : c;
        }
        return out;
    }
};

class HillsLayer final : public Layer
{
public:
    HillsLayer(std::int64_t seed,std::shared_ptr<Layer> biome,std::shared_ptr<Layer> river)
        : Layer(seed), river_(std::move(river)){ parent_=std::move(biome); }
    void initWorldSeed(std::int64_t seed) override
    {
        parent_->initWorldSeed(seed); river_->initWorldSeed(seed); Layer::initWorldSeed(seed);
    }
    Values getInts(int areaX,int areaZ,int width,int depth) override
    {
        const int pw=width+2;
        const Values p=parent_->getInts(areaX-1,areaZ-1,pw,depth+2);
        const Values r=river_->getInts(areaX-1,areaZ-1,pw,depth+2);
        Values out(static_cast<std::size_t>(width*depth));
        for(int z=0;z<depth;++z) for(int x=0;x<width;++x)
        {
            initChunkSeed(areaX+x,areaZ+z);
            const int c=p[at(x+1,z+1,pw)], rv=r[at(x+1,z+1,pw)];
            const bool flag=(rv-2)%29==0;
            if(c!=0 && rv>=2 && (rv-2)%29==1 && !isMutation(c))
            {
                const int m=mutationFor(c); out[at(x,z,width)]=m<0?c:m; continue;
            }
            if(nextInt(3)!=0 && !flag){ out[at(x,z,width)]=c; continue; }
            int h=c;
            switch(c)
            {
                case VanillaBiomes::Desert: h=VanillaBiomes::DesertHills; break;
                case VanillaBiomes::Forest: h=VanillaBiomes::ForestHills; break;
                case VanillaBiomes::BirchForest: h=VanillaBiomes::BirchForestHills; break;
                case VanillaBiomes::RoofedForest: h=VanillaBiomes::Plains; break;
                case VanillaBiomes::Taiga: h=VanillaBiomes::TaigaHills; break;
                case VanillaBiomes::MegaTaiga: h=VanillaBiomes::MegaTaigaHills; break;
                case VanillaBiomes::ColdTaiga: h=VanillaBiomes::ColdTaigaHills; break;
                case VanillaBiomes::Plains: h=nextInt(3)==0?VanillaBiomes::ForestHills:VanillaBiomes::Forest; break;
                case VanillaBiomes::IcePlains: h=VanillaBiomes::IceMountains; break;
                case VanillaBiomes::Jungle: h=VanillaBiomes::JungleHills; break;
                case VanillaBiomes::Ocean: h=VanillaBiomes::DeepOcean; break;
                case VanillaBiomes::ExtremeHills: h=VanillaBiomes::ExtremeHillsPlus; break;
                case VanillaBiomes::Savanna: h=VanillaBiomes::SavannaPlateau; break;
                default:
                    if(biomesEqualOrMesaPlateau(c,VanillaBiomes::MesaPlateauF)) h=VanillaBiomes::Mesa;
                    else if(c==VanillaBiomes::DeepOcean && nextInt(3)==0)
                        h=nextInt(2)==0?VanillaBiomes::Plains:VanillaBiomes::Forest;
                    break;
            }
            if(flag && h!=c){ const int m=mutationFor(h); h=m<0?c:m; }
            if(h==c){ out[at(x,z,width)]=c; continue; }
            int same=0;
            same+=biomesEqualOrMesaPlateau(p[at(x+1,z,pw)],c);
            same+=biomesEqualOrMesaPlateau(p[at(x+2,z+1,pw)],c);
            same+=biomesEqualOrMesaPlateau(p[at(x,z+1,pw)],c);
            same+=biomesEqualOrMesaPlateau(p[at(x+1,z+2,pw)],c);
            out[at(x,z,width)]=same>=3?h:c;
        }
        return out;
    }
private:
    std::shared_ptr<Layer> river_;
};

class ShoreLayer final : public Layer
{
public:
    ShoreLayer(std::int64_t seed,std::shared_ptr<Layer> parent):Layer(seed){parent_=std::move(parent);}
    Values getInts(int areaX,int areaZ,int width,int depth) override
    {
        const int pw=width+2;
        const Values p=parent_->getInts(areaX-1,areaZ-1,pw,depth+2);
        Values out(static_cast<std::size_t>(width*depth));
        const auto jungleCompatible=[](int id)
        {
            return biomeClass(id)==BiomeClass::Jungle || id==VanillaBiomes::Forest ||
                   id==VanillaBiomes::Taiga || oceanic(id);
        };
        const auto isMesa=[](int id){return biomeClass(id)==BiomeClass::Mesa;};
        for(int z=0;z<depth;++z) for(int x=0;x<width;++x)
        {
            initChunkSeed(areaX+x,areaZ+z);
            const int c=p[at(x+1,z+1,pw)];
            const std::array<int,4> n={p[at(x+1,z,pw)],p[at(x+2,z+1,pw)],p[at(x,z+1,pw)],p[at(x+1,z+2,pw)]};
            int result=c;
            if(c==VanillaBiomes::MushroomIsland)
            {
                if(std::any_of(n.begin(),n.end(),[](int v){return v==VanillaBiomes::Ocean;})) result=VanillaBiomes::MushroomShore;
            }
            else if(biomeClass(c)==BiomeClass::Jungle)
            {
                if(!std::all_of(n.begin(),n.end(),jungleCompatible)) result=VanillaBiomes::JungleEdge;
                else if(std::any_of(n.begin(),n.end(),oceanic)) result=VanillaBiomes::Beach;
            }
            else if(c==VanillaBiomes::ExtremeHills || c==VanillaBiomes::ExtremeHillsPlus || c==VanillaBiomes::ExtremeHillsEdge)
            {
                if(std::any_of(n.begin(),n.end(),oceanic)) result=VanillaBiomes::StoneBeach;
            }
            else if(snowy(c))
            {
                if(std::any_of(n.begin(),n.end(),oceanic)) result=VanillaBiomes::ColdBeach;
            }
            else if(c==VanillaBiomes::Mesa || c==VanillaBiomes::MesaPlateauF)
            {
                if(!std::any_of(n.begin(),n.end(),oceanic) && !std::all_of(n.begin(),n.end(),isMesa)) result=VanillaBiomes::Desert;
            }
            else if(c!=VanillaBiomes::Ocean && c!=VanillaBiomes::DeepOcean && c!=VanillaBiomes::River && c!=VanillaBiomes::Swampland)
            {
                if(std::any_of(n.begin(),n.end(),oceanic)) result=VanillaBiomes::Beach;
            }
            out[at(x,z,width)]=result;
        }
        return out;
    }
};

class RiverMixLayer final : public Layer
{
public:
    RiverMixLayer(std::int64_t seed,std::shared_ptr<Layer> biome,std::shared_ptr<Layer> river)
        : Layer(seed), biome_(std::move(biome)), river_(std::move(river)) {}
    void initWorldSeed(std::int64_t seed) override
    {
        biome_->initWorldSeed(seed); river_->initWorldSeed(seed); Layer::initWorldSeed(seed);
    }
    Values getInts(int x,int z,int width,int depth) override
    {
        const Values b=biome_->getInts(x,z,width,depth), r=river_->getInts(x,z,width,depth);
        Values out(b.size());
        for(std::size_t i=0;i<b.size();++i)
        {
            if(b[i]!=VanillaBiomes::Ocean && b[i]!=VanillaBiomes::DeepOcean && r[i]==VanillaBiomes::River)
            {
                if(b[i]==VanillaBiomes::IcePlains) out[i]=VanillaBiomes::FrozenRiver;
                else if(b[i]==VanillaBiomes::MushroomIsland || b[i]==VanillaBiomes::MushroomShore) out[i]=VanillaBiomes::MushroomShore;
                else out[i]=r[i]&255;
            }
            else out[i]=b[i];
        }
        return out;
    }
private:
    std::shared_ptr<Layer> biome_,river_;
};

class VoronoiLayer final : public Layer
{
public:
    VoronoiLayer(std::int64_t seed,std::shared_ptr<Layer> parent):Layer(seed){parent_=std::move(parent);}
    Values getInts(int areaX,int areaZ,int areaWidth,int areaDepth) override
    {
        areaX-=2; areaZ-=2;
        const int px=areaX>>2,pz=areaZ>>2;
        const int pw=(areaWidth>>2)+2,pd=(areaDepth>>2)+2;
        const Values p=parent_->getInts(px,pz,pw,pd);
        const int ew=(pw-1)<<2, ed=(pd-1)<<2;
        Values expanded(static_cast<std::size_t>(ew*ed));
        for(int cz=0;cz<pd-1;++cz)
        {
            int nw=p[at(0,cz,pw)]&255, sw=p[at(0,cz+1,pw)]&255;
            for(int cx=0;cx<pw-1;++cx)
            {
                initChunkSeed((cx+px)<<2,(cz+pz)<<2);
                const double nwx=(nextInt(1024)/1024.0-0.5)*3.6;
                const double nwz=(nextInt(1024)/1024.0-0.5)*3.6;
                initChunkSeed((cx+px+1)<<2,(cz+pz)<<2);
                const double nex=(nextInt(1024)/1024.0-0.5)*3.6+4.0;
                const double nez=(nextInt(1024)/1024.0-0.5)*3.6;
                initChunkSeed((cx+px)<<2,(cz+pz+1)<<2);
                const double swx=(nextInt(1024)/1024.0-0.5)*3.6;
                const double swz=(nextInt(1024)/1024.0-0.5)*3.6+4.0;
                initChunkSeed((cx+px+1)<<2,(cz+pz+1)<<2);
                const double sex=(nextInt(1024)/1024.0-0.5)*3.6+4.0;
                const double sez=(nextInt(1024)/1024.0-0.5)*3.6+4.0;
                const int ne=p[at(cx+1,cz,pw)]&255,se=p[at(cx+1,cz+1,pw)]&255;
                for(int lz=0;lz<4;++lz) for(int lx=0;lx<4;++lx)
                {
                    const auto dist=[&](double sx,double sz){const double dx=lx-sx,dz=lz-sz;return dx*dx+dz*dz;};
                    const double dnw=dist(nwx,nwz),dne=dist(nex,nez),dsw=dist(swx,swz),dse=dist(sex,sez);
                    int selected;
                    if(dnw<dne&&dnw<dsw&&dnw<dse)selected=nw;
                    else if(dne<dnw&&dne<dsw&&dne<dse)selected=ne;
                    else if(dsw<dnw&&dsw<dne&&dsw<dse)selected=sw;
                    else selected=se;
                    expanded[at(cx*4+lx,cz*4+lz,ew)]=selected;
                }
                nw=ne; sw=se;
            }
        }
        Values out(static_cast<std::size_t>(areaWidth*areaDepth));
        const int ox=areaX&3,oz=areaZ&3;
        for(int z=0;z<areaDepth;++z)for(int x=0;x<areaWidth;++x)
            out[at(x,z,areaWidth)]=expanded[at(x+ox,z+oz,ew)];
        return out;
    }
};

struct Pipeline
{
    std::shared_ptr<Layer> generation;
    std::shared_ptr<Layer> voronoi;
};

Pipeline buildPipeline(std::int64_t seed)
{
    std::shared_ptr<Layer> layer=std::make_shared<IslandLayer>(1);
    layer=std::make_shared<ZoomLayer>(2000,layer,true);
    layer=std::make_shared<AddIslandLayer>(1,layer);
    layer=std::make_shared<ZoomLayer>(2001,layer);
    layer=std::make_shared<AddIslandLayer>(2,layer);
    layer=std::make_shared<AddIslandLayer>(50,layer);
    layer=std::make_shared<AddIslandLayer>(70,layer);
    layer=std::make_shared<RemoveTooMuchOceanLayer>(2,layer);
    layer=std::make_shared<AddSnowLayer>(2,layer);
    layer=std::make_shared<AddIslandLayer>(3,layer);
    layer=std::make_shared<EdgeLayer>(2,layer,EdgeMode::CoolWarm);
    layer=std::make_shared<EdgeLayer>(2,layer,EdgeMode::HeatIce);
    layer=std::make_shared<EdgeLayer>(3,layer,EdgeMode::Special);
    layer=std::make_shared<ZoomLayer>(2002,layer);
    layer=std::make_shared<ZoomLayer>(2003,layer);
    layer=std::make_shared<AddIslandLayer>(4,layer);
    layer=std::make_shared<AddMushroomLayer>(5,layer);
    auto deep=std::make_shared<DeepOceanLayer>(4,layer);

    auto riverInit=std::make_shared<RiverInitLayer>(100,deep);
    std::shared_ptr<Layer> biomes=std::make_shared<BiomeLayer>(200,deep);
    biomes=magnify(1000,biomes,2);
    biomes=std::make_shared<BiomeEdgeLayer>(1000,biomes);
    auto hillsRiver=magnify(1000,riverInit,2);
    std::shared_ptr<Layer> hills=std::make_shared<HillsLayer>(1000,biomes,hillsRiver);

    std::shared_ptr<Layer> river=magnify(1000,riverInit,2);
    river=magnify(1000,river,4);
    river=std::make_shared<RiverLayer>(1,river);
    river=std::make_shared<SmoothLayer>(1000,river);

    hills=std::make_shared<RareBiomeLayer>(1001,hills);
    for(int k=0;k<4;++k)
    {
        hills=std::make_shared<ZoomLayer>(1000+k,hills);
        if(k==0)hills=std::make_shared<AddIslandLayer>(3,hills);
        if(k==1)hills=std::make_shared<ShoreLayer>(1000,hills);
    }
    hills=std::make_shared<SmoothLayer>(1000,hills);
    auto mixed=std::make_shared<RiverMixLayer>(100,hills,river);
    auto voronoi=std::make_shared<VoronoiLayer>(10,mixed);
    mixed->initWorldSeed(seed);
    voronoi->initWorldSeed(seed);
    return {mixed,voronoi};
}

std::vector<BiomeId> toXMajor(const Values& rowMajor,int width,int depth)
{
    std::vector<BiomeId> out(static_cast<std::size_t>(width*depth));
    for(int x=0;x<width;++x)for(int z=0;z<depth;++z)
        out[static_cast<std::size_t>(x*depth+z)]=static_cast<BiomeId>(rowMajor[at(x,z,width)]);
    return out;
}
}

std::vector<BiomeId> vanillaGenerationBiomes(
    std::int64_t worldSeed,int originX,int originZ,int width,int depth)
{
    if(width<=0||depth<=0)return {};
    Pipeline pipeline=buildPipeline(worldSeed);
    return toXMajor(pipeline.generation->getInts(originX,originZ,width,depth),width,depth);
}

std::vector<BiomeId> vanillaVoronoiBiomes(
    std::int64_t worldSeed,int originX,int originZ,int width,int depth)
{
    if(width<=0||depth<=0)return {};
    Pipeline pipeline=buildPipeline(worldSeed);
    return toXMajor(pipeline.voronoi->getInts(originX,originZ,width,depth),width,depth);
}
