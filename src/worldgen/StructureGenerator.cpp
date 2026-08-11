#include "worldgen/StructureGenerator.h"

#include "Block.h"
#include "Chunk.h"
#include "worldgen/BiomeMap.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/MineshaftStructure.h"
#include "worldgen/ScatteredFeatureStructure.h"
#include "worldgen/StrongholdStructure.h"
#include "worldgen/VillageStructure.h"
#include "worldgen/StructurePrimitives.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <limits>
#include <numbers>
#include <string>
#include <tuple>

namespace
{
int floorDivide(int value, int divisor)
{
    int q=value/divisor; const int r=value%divisor;
    if(r!=0 && ((r<0)!=(divisor<0))) --q;
    return q;
}

std::int64_t addWrap(std::int64_t a, std::int64_t b) noexcept
{
    return std::bit_cast<std::int64_t>(
        static_cast<std::uint64_t>(a)+static_cast<std::uint64_t>(b));
}
std::int64_t mulWrap(std::int64_t a, std::int64_t b) noexcept
{
    return std::bit_cast<std::int64_t>(
        static_cast<std::uint64_t>(a)*static_cast<std::uint64_t>(b));
}

JavaRandom worldStructureRandom(std::int64_t seed,int regionX,int regionZ,std::int64_t salt)
{
    // World#setRandomSeed(x,z,salt)
    std::int64_t s=mulWrap(regionX,341873128712LL);
    s=addWrap(s,mulWrap(regionZ,132897987541LL));
    s=addWrap(s,seed); s=addWrap(s,salt);
    return JavaRandom(s);
}

std::pair<int,int> scatteredCandidate(std::int64_t seed,int regionX,int regionZ,
                                      int spacing,int separation,std::int64_t salt)
{
    JavaRandom r=worldStructureRandom(seed,regionX,regionZ,salt);
    return {regionX*spacing+r.nextInt(spacing-separation),
            regionZ*spacing+r.nextInt(spacing-separation)};
}
std::pair<int,int> triangularCandidate(std::int64_t seed,int regionX,int regionZ,
                                       int spacing,int separation,std::int64_t salt)
{
    JavaRandom r=worldStructureRandom(seed,regionX,regionZ,salt);
    const int bound=spacing-separation;
    const int xFirst = r.nextInt(bound);
    const int xSecond = r.nextInt(bound);
    const int zFirst = r.nextInt(bound);
    const int zSecond = r.nextInt(bound);
    return {regionX * spacing + (xFirst + xSecond) / 2,
            regionZ * spacing + (zFirst + zSecond) / 2};
}

bool villageBiome(BiomeId b)
{
    return b==VanillaBiomes::Plains||b==VanillaBiomes::Desert||
           b==VanillaBiomes::Savanna||b==VanillaBiomes::Taiga;
}
bool templeBiome(BiomeId b)
{
    // MapGenScatteredFeature BIOMELIST exactly; mutations are not included.
    return b==VanillaBiomes::Desert||b==VanillaBiomes::DesertHills||
           b==VanillaBiomes::Jungle||b==VanillaBiomes::JungleHills||
           b==VanillaBiomes::Swampland||b==VanillaBiomes::IcePlains||
           b==VanillaBiomes::ColdTaiga;
}
bool monumentWaterBiome(BiomeId b)
{
    return b==VanillaBiomes::Ocean||b==VanillaBiomes::DeepOcean||
           b==VanillaBiomes::River||b==VanillaBiomes::FrozenOcean||
           b==VanillaBiomes::FrozenRiver;
}
bool mansionBiome(BiomeId b)
{
    return b==VanillaBiomes::RoofedForest||b==VanillaBiomes::RoofedForestMountains;
}

bool mesaBiome(BiomeId b)
{
    return b==VanillaBiomes::Mesa||b==VanillaBiomes::MesaPlateauF||
           b==VanillaBiomes::MesaPlateau||b==VanillaBiomes::MesaBryce||
           b==VanillaBiomes::MesaPlateauFMountains||
           b==VanillaBiomes::MesaPlateauMountains;
}

bool areGenerationBiomesViable(std::int64_t seed,int blockX,int blockZ,int radius,
                               bool (*allowed)(BiomeId))
{
    const int minX=floorDivide(blockX-radius,4);
    const int minZ=floorDivide(blockZ-radius,4);
    const int maxX=floorDivide(blockX+radius,4);
    const int maxZ=floorDivide(blockZ+radius,4);
    BiomeMap map(seed);
    const auto samples=map.sampleGenerationArea(
        minX,minZ,maxX-minX+1,maxZ-minZ+1);
    return std::all_of(samples.begin(),samples.end(),
        [allowed](const ClimateSample& s){return allowed(s.biome);});
}

std::optional<std::pair<int,int>> findStrongholdBiome(
    std::int64_t seed,int blockX,int blockZ,int range,JavaRandom& random)
{
    const int minX=floorDivide(blockX-range,4);
    const int minZ=floorDivide(blockZ-range,4);
    const int maxX=floorDivide(blockX+range,4);
    const int maxZ=floorDivide(blockZ+range,4);
    const int width=maxX-minX+1,depth=maxZ-minZ+1;
    BiomeMap map(seed);
    const auto samples=map.sampleGenerationArea(minX,minZ,width,depth);
    std::optional<std::pair<int,int>> selected;
    int count=0;
    for(int z=0;z<depth;++z) for(int x=0;x<width;++x)
    {
        const BiomeId b=samples[static_cast<std::size_t>(x*depth+z)].biome;
        const BiomeDefinition* def=BiomeRegistry::active().find(b);
        if(def==nullptr||def->baseHeight<=0.0f) continue;
        if(!selected||random.nextInt(count+1)==0)
            selected=std::pair{(minX+x)<<2,(minZ+z)<<2};
        ++count;
    }
    return selected;
}

std::uint64_t structureStartKey(int chunkX,int chunkZ) noexcept
{
    return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(chunkX))<<32U) |
           static_cast<std::uint32_t>(chunkZ);
}

JavaRandom mapGenRandom(std::int64_t seed,int chunkX,int chunkZ)
{
    JavaRandom base(seed);
    const std::int64_t a=base.nextLong(),b=base.nextLong();
    const std::uint64_t mixed=
        static_cast<std::uint64_t>(mulWrap(chunkX,a)) ^
        static_cast<std::uint64_t>(mulWrap(chunkZ,b)) ^
        static_cast<std::uint64_t>(seed);
    return JavaRandom(std::bit_cast<std::int64_t>(mixed));
}

void fill(WorldGenerationContext& c,int x0,int y0,int z0,int x1,int y1,int z1,BlockType b)
{
    for(int x=x0;x<=x1;++x)for(int y=y0;y<=y1;++y)for(int z=z0;z<=z1;++z)
        c.setBlock(x,y,z,b);
}
void hollow(WorldGenerationContext& c,int x0,int y0,int z0,int x1,int y1,int z1,BlockType b)
{
    for(int x=x0;x<=x1;++x)for(int y=y0;y<=y1;++y)for(int z=z0;z<=z1;++z)
        c.setBlock(x,y,z,(x==x0||x==x1||y==y0||y==y1||z==z0||z==z1)?b:BlockType::Air);
}
}

const char* structureName(WorldStructure s) noexcept
{
    switch(s){case WorldStructure::Mineshaft:return "Mineshaft";case WorldStructure::Village:return "Village";
    case WorldStructure::Temple:return "Temple";case WorldStructure::Stronghold:return "Stronghold";
    case WorldStructure::OceanMonument:return "Monument";case WorldStructure::WoodlandMansion:return "Mansion";}
    return "Unknown";
}

std::optional<WorldStructure> parseStructureName(std::string_view name) noexcept
{
    std::string s(name); std::transform(s.begin(),s.end(),s.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    if(s=="mineshaft")return WorldStructure::Mineshaft;if(s=="village")return WorldStructure::Village;
    if(s=="temple"||s=="pyramid")return WorldStructure::Temple;if(s=="stronghold")return WorldStructure::Stronghold;
    if(s=="monument"||s=="ocean_monument")return WorldStructure::OceanMonument;
    if(s=="mansion"||s=="woodland_mansion")return WorldStructure::WoodlandMansion;return std::nullopt;
}

StructureGenerator::StructureGenerator(std::int64_t worldSeed):worldSeed_(worldSeed)
{
    // MapGenStronghold::generatePositions, including the 112-block biome
    // relocation that the old clone omitted.
    JavaRandom random(worldSeed_);
    double angle=random.nextDouble()*std::numbers::pi*2.0;
    int ring=0,inRing=0,spread=3;
    strongholdChunks_.reserve(128);
    for(int index=0;index<128;++index)
    {
        const double distance=4.0*32.0+32.0*ring*6.0+
            (random.nextDouble()-0.5)*32.0*2.5;
        int cx=static_cast<int>(std::floor(std::cos(angle)*distance+0.5));
        int cz=static_cast<int>(std::floor(std::sin(angle)*distance+0.5));
        if(auto pos=findStrongholdBiome(worldSeed_,(cx<<4)+8,(cz<<4)+8,112,random))
        { cx=floorDivide(pos->first,16); cz=floorDivide(pos->second,16); }
        strongholdChunks_.emplace_back(cx,cz);
        angle+=std::numbers::pi*2.0/static_cast<double>(spread);
        if(++inRing==spread){++ring;inRing=0;spread+=2*spread/(ring+1);spread=std::min(spread,128-index);angle+=random.nextDouble()*std::numbers::pi*2.0;}
    }
}

bool StructureGenerator::isVillageChunk(int x,int z) const
{const int rx=floorDivide(x,32),rz=floorDivide(z,32);return scatteredCandidate(worldSeed_,rx,rz,32,8,10387312)==std::pair{x,z};}
bool StructureGenerator::isTempleChunk(int x,int z) const
{const int rx=floorDivide(x,32),rz=floorDivide(z,32);return scatteredCandidate(worldSeed_,rx,rz,32,8,14357617)==std::pair{x,z};}
bool StructureGenerator::isOceanMonumentChunk(int x,int z) const
{const int rx=floorDivide(x,32),rz=floorDivide(z,32);return triangularCandidate(worldSeed_,rx,rz,32,5,10387313)==std::pair{x,z};}
bool StructureGenerator::isWoodlandMansionChunk(int x,int z) const
{const int rx=floorDivide(x,80),rz=floorDivide(z,80);return triangularCandidate(worldSeed_,rx,rz,80,20,10387319)==std::pair{x,z};}
bool StructureGenerator::isStrongholdChunk(int x,int z) const
{return std::find(strongholdChunks_.begin(),strongholdChunks_.end(),std::pair{x,z})!=strongholdChunks_.end();}
bool StructureGenerator::isMineshaftChunk(int x,int z) const
{
    JavaRandom r=mapGenRandom(worldSeed_,x,z);
    // MapGenStructure::recursiveGenerate advances once before canSpawn.
    (void)r.nextInt();
    return r.nextDouble()<0.004&&
           r.nextInt(80)<std::max(std::abs(x),std::abs(z));
}
bool StructureGenerator::villageBiomeViable(int x,int z) const
{return areGenerationBiomesViable(worldSeed_,x*16+8,z*16+8,0,villageBiome);}
bool StructureGenerator::monumentBiomeViable(int x,int z) const
{return areGenerationBiomesViable(worldSeed_,x*16+8,z*16+8,16,[](BiomeId b){return b==VanillaBiomes::DeepOcean;})&&
       areGenerationBiomesViable(worldSeed_,x*16+8,z*16+8,29,monumentWaterBiome);}
bool StructureGenerator::mansionBiomeViable(int x,int z) const
{return areGenerationBiomesViable(worldSeed_,x*16+8,z*16+8,32,mansionBiome);}

PopulationStructureResult StructureGenerator::populateSource(
    WorldGenerationContext& context,
    JavaRandom& populationRandom,
    int sourceChunkX,
    int sourceChunkZ) const
{
    PopulationStructureResult result;
    const mc112::Box clip{
        sourceChunkX * 16 + 8,
        1,
        sourceChunkZ * 16 + 8,
        sourceChunkX * 16 + 23,
        512,
        sourceChunkZ * 16 + 23};

    // MapGenBase has range=8. Recreate exactly the prepared starts that can
    // exist in this population pass, then post-process only pieces intersecting
    // the vanilla +8 clipping box. Start construction uses MapGenStructure's
    // independent Random; piece placement consumes `populationRandom`.
    for(int startX=sourceChunkX-8;startX<=sourceChunkX+8;++startX)
    {
        for(int startZ=sourceChunkZ-8;startZ<=sourceChunkZ+8;++startZ)
        {
            JavaRandom startRandom=mapGenRandom(worldSeed_,startX,startZ);
            (void)startRandom.nextInt();
            if(!(startRandom.nextDouble()<0.004 &&
                 startRandom.nextInt(80)<
                    std::max(std::abs(startX),std::abs(startZ))))
                continue;

            const auto key=structureStartKey(startX,startZ);
            auto it=mineshaftStarts_.find(key);
            if(it==mineshaftStarts_.end())
            {
                const BiomeId biome=context.sampleClimate(
                    startX*16+8,startZ*16+8).biome;
                const auto type=mesaBiome(biome)
                    ? mc112::MineshaftStructure::Type::Mesa
                    : mc112::MineshaftStructure::Type::Normal;
                auto created=std::make_shared<mc112::MineshaftStructure::Start>(
                    mc112::MineshaftStructure::create(
                        startX,startZ,type,startRandom,63));
                it=mineshaftStarts_.emplace(key,std::move(created)).first;
            }
            auto& start=*it->second;
            if(start.sizeable && start.bounds.intersects(clip))
            {
                mc112::MineshaftStructure::place(
                    start,context,populationRandom,clip);
            }
        }
    }

    // MapGenVillage post-processes immediately after mineshafts in
    // ChunkGeneratorOverworld::populate. Layout/start construction uses the
    // MapGenStructure RNG, while addComponentParts consumes populationRandom.
    for(int startX=sourceChunkX-8;startX<=sourceChunkX+8;++startX)
    {
        for(int startZ=sourceChunkZ-8;startZ<=sourceChunkZ+8;++startZ)
        {
            if(!isVillageChunk(startX,startZ) || !villageBiomeViable(startX,startZ))
                continue;

            const auto key=structureStartKey(startX,startZ);
            auto it=villageStarts_.find(key);
            if(it==villageStarts_.end())
            {
                JavaRandom startRandom=mapGenRandom(worldSeed_,startX,startZ);
                (void)startRandom.nextInt();
                const BiomeId startBiome=context.sampleClimate(
                    startX*16+2,startZ*16+2).biome;
                auto created=std::make_shared<mc112::VillageStructure::Start>(
                    mc112::VillageStructure::create(
                        startX,startZ,startBiome,startRandom,0));
                it=villageStarts_.emplace(key,std::move(created)).first;
            }
            auto& start=*it->second;
            if(start.sizeable && start.bounds.intersects(clip))
            {
                mc112::VillageStructure::place(
                    start,context,populationRandom,clip);
                result.villageGenerated=true;
            }
        }
    }

    // Strongholds are the next structure family in vanilla population order.
    // Only the 128 precomputed MapGenStronghold start chunks can construct a
    // start; its internal retry loop guarantees a portal room.
    for(int startX=sourceChunkX-8;startX<=sourceChunkX+8;++startX)
    {
        for(int startZ=sourceChunkZ-8;startZ<=sourceChunkZ+8;++startZ)
        {
            if(!isStrongholdChunk(startX,startZ))
                continue;
            const auto key=structureStartKey(startX,startZ);
            auto it=strongholdStarts_.find(key);
            if(it==strongholdStarts_.end())
            {
                JavaRandom startRandom=mapGenRandom(worldSeed_,startX,startZ);
                (void)startRandom.nextInt();
                auto created=std::make_shared<mc112::StrongholdStructure::Start>(
                    mc112::StrongholdStructure::create(
                        startX,startZ,startRandom,63));
                it=strongholdStarts_.emplace(key,std::move(created)).first;
            }
            auto& start=*it->second;
            if(start.sizeable && start.bounds.intersects(clip))
            {
                mc112::StrongholdStructure::place(
                    start,context,populationRandom,clip);
            }
        }
    }

    // Scattered features follow strongholds exactly in vanilla population
    // order.
    for(int startX=sourceChunkX-8;startX<=sourceChunkX+8;++startX)
    {
        for(int startZ=sourceChunkZ-8;startZ<=sourceChunkZ+8;++startZ)
        {
            if(!isTempleChunk(startX,startZ))
                continue;
            const BiomeId biome=context.sampleClimate(
                startX*16+8,startZ*16+8).biome;
            if(!templeBiome(biome))
                continue;

            const auto key=structureStartKey(startX,startZ);
            auto it=scatteredStarts_.find(key);
            if(it==scatteredStarts_.end())
            {
                JavaRandom startRandom=mapGenRandom(worldSeed_,startX,startZ);
                // MapGenStructure::recursiveGenerate advances the mapgen RNG before
                // canSpawnStructureAtCoords. The scattered spawn test itself uses
                // World#setRandomSeed, so it consumes no draws from startRandom.
                (void)startRandom.nextInt();
                auto created=std::make_shared<mc112::ScatteredFeatureStructure::Start>(
                    mc112::ScatteredFeatureStructure::create(
                        startX,startZ,biome,startRandom));
                it=scatteredStarts_.emplace(key,std::move(created)).first;
            }
            auto& start=*it->second;
            if(start.sizeable && start.bounds.intersects(clip))
            {
                mc112::ScatteredFeatureStructure::place(
                    start,context,populationRandom,clip);
            }
        }
    }

    // Ocean monuments follow scattered features. MapGenOceanMonument uses
    // the 32/5 triangular grid with salt 10387313, then requires Deep Ocean
    // in radius 16 and only ocean/river-family biomes in radius 29.
    for(int startX=sourceChunkX-8;startX<=sourceChunkX+8;++startX)
    {
        for(int startZ=sourceChunkZ-8;startZ<=sourceChunkZ+8;++startZ)
        {
            if(!isOceanMonumentChunk(startX,startZ) ||
               !monumentBiomeViable(startX,startZ))
                continue;

            const auto key=structureStartKey(startX,startZ);
            auto it=oceanMonumentStarts_.find(key);
            if(it==oceanMonumentStarts_.end())
            {
                JavaRandom startRandom=mapGenRandom(worldSeed_,startX,startZ);
                (void)startRandom.nextInt();
                auto created=std::make_shared<mc112::OceanMonumentStructure::Start>(
                    mc112::OceanMonumentStructure::create(
                        worldSeed_,startX,startZ,startRandom));
                it=oceanMonumentStarts_.emplace(key,std::move(created)).first;
            }
            auto& start=*it->second;
            if(start.sizeable && start.bounds.intersects(clip))
            {
                mc112::OceanMonumentStructure::place(
                    start,context,populationRandom,
                    sourceChunkX,sourceChunkZ,clip);
            }
        }
    }

    // Woodland mansions are the final Overworld MapGenStructure family in
    // ChunkGeneratorOverworld::populate. Their 80/20 triangular grid uses
    // salt 10387319 and requires Roofed Forest throughout radius 32.
    for(int startX=sourceChunkX-8;startX<=sourceChunkX+8;++startX)
    {
        for(int startZ=sourceChunkZ-8;startZ<=sourceChunkZ+8;++startZ)
        {
            if(!isWoodlandMansionChunk(startX,startZ) ||
               !mansionBiomeViable(startX,startZ))
                continue;
            const auto key=structureStartKey(startX,startZ);
            auto it=woodlandMansionStarts_.find(key);
            if(it==woodlandMansionStarts_.end())
            {
                JavaRandom startRandom=mapGenRandom(worldSeed_,startX,startZ);
                (void)startRandom.nextInt();
                auto created=std::make_shared<mc112::WoodlandMansionStructure::Start>(
                    mc112::WoodlandMansionStructure::create(
                        startX,startZ,startRandom,context));
                it=woodlandMansionStarts_.emplace(key,std::move(created)).first;
            }
            auto& start=*it->second;
            if(start.sizeable && start.bounds.intersects(clip))
                mc112::WoodlandMansionStructure::place(
                    start,context,populationRandom,clip);
        }
    }
    return result;
}

std::optional<StructureLocation> StructureGenerator::findNearest(
    WorldStructure s,int blockX,int blockZ,int maxRadius,const ClimateSampler& climate) const
{
    if(s==WorldStructure::Stronghold){double best=std::numeric_limits<double>::max();std::optional<StructureLocation> out;for(auto [cx,cz]:strongholdChunks_){const int x=cx*16+8,z=cz*16+8;const double d=std::hypot(x-blockX,z-blockZ);if(d<best){best=d;out=StructureLocation{s,x,z,climate(x,z).biome};}}return out;}
    const int spacing=s==WorldStructure::Mineshaft?1:(s==WorldStructure::WoodlandMansion?80:32);const int ocx=floorDivide(blockX,16),ocz=floorDivide(blockZ,16),orx=floorDivide(ocx,spacing),orz=floorDivide(ocz,spacing);double best=std::numeric_limits<double>::max();std::optional<StructureLocation> out;
    for(int radius=0;radius<=maxRadius;++radius){for(int dx=-radius;dx<=radius;++dx)for(int dz=-radius;dz<=radius;++dz){if(radius&&std::abs(dx)!=radius&&std::abs(dz)!=radius)continue;int cx=orx+dx,cz=orz+dz;bool ok=false;
        if(s==WorldStructure::Village){std::tie(cx,cz)=scatteredCandidate(worldSeed_,cx,cz,32,8,10387312);ok=villageBiomeViable(cx,cz);}else if(s==WorldStructure::Temple){std::tie(cx,cz)=scatteredCandidate(worldSeed_,cx,cz,32,8,14357617);ok=templeBiome(climate(cx*16+8,cz*16+8).biome);}else if(s==WorldStructure::OceanMonument){std::tie(cx,cz)=triangularCandidate(worldSeed_,cx,cz,32,5,10387313);ok=monumentBiomeViable(cx,cz);}else if(s==WorldStructure::WoodlandMansion){std::tie(cx,cz)=triangularCandidate(worldSeed_,cx,cz,80,20,10387319);ok=mansionBiomeViable(cx,cz);}else ok=isMineshaftChunk(cx,cz);if(!ok)continue;const int x=cx*16+8,z=cz*16+8;const double d=std::hypot(x-blockX,z-blockZ);if(d<best){best=d;out=StructureLocation{s,x,z,climate(x,z).biome};}}
        if(out&&radius>1)break;}return out;
}
