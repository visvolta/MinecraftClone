#include "worldgen/StructureGenerator.h"

#include "Block.h"
#include "Chunk.h"
#include "worldgen/BiomeMap.h"
#include "worldgen/JavaRandom.h"
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
{JavaRandom r=mapGenRandom(worldSeed_,x,z);return r.nextDouble()<0.004&&r.nextInt(80)<std::max(std::abs(x),std::abs(z));}
bool StructureGenerator::villageBiomeViable(int x,int z) const
{return areGenerationBiomesViable(worldSeed_,x*16+8,z*16+8,0,villageBiome);}
bool StructureGenerator::monumentBiomeViable(int x,int z) const
{return areGenerationBiomesViable(worldSeed_,x*16+8,z*16+8,16,[](BiomeId b){return b==VanillaBiomes::DeepOcean;})&&
       areGenerationBiomesViable(worldSeed_,x*16+8,z*16+8,29,monumentWaterBiome);}
bool StructureGenerator::mansionBiomeViable(int x,int z) const
{return areGenerationBiomesViable(worldSeed_,x*16+8,z*16+8,32,mansionBiome);}

void StructureGenerator::populate(Chunk& target,WorldGenerationContext& c) const
{
    // Replay starts that can intersect the immutable target chunk. Piece
    // layouts below remain the clone's procedural stand-ins until a 1.12 NBT
    // structure-template/piece serialization layer is available.
    for(int sx=target.getChunkX()-4;sx<=target.getChunkX()+4;++sx)
    for(int sz=target.getChunkZ()-4;sz<=target.getChunkZ()+4;++sz)
    {
        const ClimateSample climate=c.sampleClimate(sx*16+8,sz*16+8);
        JavaRandom r=mapGenRandom(worldSeed_,sx,sz);
        if(isMineshaftChunk(sx,sz)){c.beginIsolatedFeature();generateMineshaft(c,r,sx,sz);c.finishIsolatedFeature(true);}
        if(isVillageChunk(sx,sz)&&villageBiomeViable(sx,sz)){c.beginIsolatedFeature();generateVillage(c,r,sx,sz,climate.biome);c.finishIsolatedFeature(true);}
        if(isStrongholdChunk(sx,sz)){c.beginIsolatedFeature();generateStronghold(c,r,sx,sz);c.finishIsolatedFeature(true);}
        if(isTempleChunk(sx,sz)&&templeBiome(climate.biome)){c.beginIsolatedFeature();generateTemple(c,r,sx,sz,climate.biome);c.finishIsolatedFeature(true);}
        if(isOceanMonumentChunk(sx,sz)&&monumentBiomeViable(sx,sz)){c.beginIsolatedFeature();generateOceanMonument(c,r,sx,sz);c.finishIsolatedFeature(true);}
        if(isWoodlandMansionChunk(sx,sz)&&mansionBiomeViable(sx,sz)){c.beginIsolatedFeature();generateWoodlandMansion(c,r,sx,sz);c.finishIsolatedFeature(true);}
    }
}

void StructureGenerator::generateMineshaft(WorldGenerationContext& c,JavaRandom& r,int cx,int cz) const
{
    const int x=cx*16+8,z=cz*16+8,y=18+r.nextInt(28);hollow(c,x-3,y,z-3,x+3,y+3,z+3,BlockType::OakPlanks);
    for(int d=0;d<4;++d){const int dx=d==0?1:d==1?-1:0,dz=d==2?1:d==3?-1:0;const int len=18+r.nextInt(23);
    for(int s=4;s<=len;++s){const int px=x+dx*s,pz=z+dz*s;fill(c,px-(dz!=0),y,pz-(dx!=0),px+(dz!=0),y+2,pz+(dx!=0),BlockType::Air);
    if(s%5==0){if(dx){fill(c,px,y,pz-2,px,y+2,pz-2,BlockType::OakLog);fill(c,px,y,pz+2,px,y+2,pz+2,BlockType::OakLog);}else{fill(c,px-2,y,pz,px-2,y+2,pz,BlockType::OakLog);fill(c,px+2,y,pz,px+2,y+2,pz,BlockType::OakLog);}}}}
}

void StructureGenerator::generateVillage(WorldGenerationContext& c,JavaRandom& r,int cx,int cz,BiomeId biome) const
{
    const int x=cx*16+8,z=cz*16+8,y=c.getHeightValue(x,z);const BlockType wall=biome==VanillaBiomes::Desert?BlockType::Sandstone:(biome==VanillaBiomes::Taiga?BlockType::SprucePlanks:(biome==VanillaBiomes::Savanna?BlockType::AcaciaPlanks:BlockType::OakPlanks));
    fill(c,x-2,y-1,z-2,x+2,y,z+2,BlockType::Cobblestone);fill(c,x-1,y,z-1,x+1,y+2,z+1,BlockType::Water);
    for(int yy=y+1;yy<=y+4;++yy)for(auto [dx,dz]:{std::pair{-2,-2},std::pair{2,-2},std::pair{-2,2},std::pair{2,2}})c.setBlock(x+dx,yy,z+dz,wall);
    fill(c,x-2,y+4,z-2,x+2,y+4,z+2,wall);
    for(int d=-24;d<=24;++d){int gy=c.getHeightValue(x+d,z);c.setBlock(x+d,gy-1,z,biome==VanillaBiomes::Desert?BlockType::Sandstone:BlockType::Gravel);gy=c.getHeightValue(x,z+d);c.setBlock(x,gy-1,z+d,biome==VanillaBiomes::Desert?BlockType::Sandstone:BlockType::Gravel);}
    const int houses=4+r.nextInt(5);for(int i=0;i<houses;++i){const int px=x+(i%2?12:-16)+(i/2)*4,pz=z+(i%2?-14:10);const int py=c.getHeightValue(px,pz);hollow(c,px,py,pz,px+7,py+4,pz+5,wall);c.setBlock(px+3,py+1,pz,BlockType::Air);c.setBlock(px+3,py+2,pz,BlockType::Air);}
}

void StructureGenerator::generateTemple(WorldGenerationContext& c,JavaRandom&,int cx,int cz,BiomeId biome) const
{
    const int x=cx*16,z=cz*16,y=c.getHeightValue(x+8,z+8);
    if(biome==VanillaBiomes::Desert||biome==VanillaBiomes::DesertHills){for(int l=0;l<4;++l)fill(c,x-2+l,y+l,z-2+l,x+18-l,y+l,z+18-l,BlockType::Sandstone);hollow(c,x+3,y+1,z+3,x+13,y+8,z+13,BlockType::Sandstone);fill(c,x+8,y-10,z+8,x+8,y-1,z+8,BlockType::Air);c.setBlock(x+8,y-10,z+8,BlockType::TNT);}
    else if(biome==VanillaBiomes::Jungle||biome==VanillaBiomes::JungleHills){hollow(c,x+2,y,z+1,x+13,y+7,z+15,BlockType::MossyCobblestone);fill(c,x+4,y-4,z+4,x+11,y-1,z+12,BlockType::MossyCobblestone);}
    else if(biome==VanillaBiomes::Swampland){const int fy=y+3;fill(c,x+2,fy,z+2,x+8,fy,z+8,BlockType::OakPlanks);hollow(c,x+2,fy+1,z+2,x+8,fy+4,z+8,BlockType::OakPlanks);for(auto [dx,dz]:{std::pair{2,2},std::pair{8,2},std::pair{2,8},std::pair{8,8}})fill(c,x+dx,y-3,z+dz,x+dx,fy+2,z+dz,BlockType::OakLog);}
    else {for(int dx=-4;dx<=4;++dx)for(int dz=-4;dz<=4;++dz)if(dx*dx+dz*dz<=16)c.setBlock(x+8+dx,y,z+8+dz,BlockType::Snow);}
}

void StructureGenerator::generateStronghold(WorldGenerationContext& c,JavaRandom& r,int cx,int cz) const
{
    const int x=cx*16+2,z=cz*16+2,y=20+r.nextInt(20);hollow(c,x-4,y-1,z-4,x+4,y+4,z+4,BlockType::StoneBricks);
    for(int d=0;d<4;++d){const int dx=d==0?1:d==1?-1:0,dz=d==2?1:d==3?-1:0;for(int s=4;s<28;++s){const int px=x+dx*s,pz=z+dz*s;fill(c,px-(dz!=0),y,pz-(dx!=0),px+(dz!=0),y+2,pz+(dx!=0),BlockType::Air);}}
    const int px=x+30;hollow(c,px-6,y-1,z-5,px+7,y+6,z+5,BlockType::StoneBricks);for(int dx=-5;dx<=5;dx+=2){fill(c,px+dx,y,z-4,px+dx,y+4,z-4,BlockType::Bookshelf);fill(c,px+dx,y,z+4,px+dx,y+4,z+4,BlockType::Bookshelf);}
}

void StructureGenerator::generateOceanMonument(WorldGenerationContext& c,JavaRandom&,int cx,int cz) const
{
    const int x=cx*16-21,z=cz*16-21,y=39;fill(c,x,y,z,x+57,y+2,z+57,BlockType::StoneBricks);for(int tier=0;tier<4;++tier){const int in=tier*6,top=y+8+tier*5;hollow(c,x+in,y+3,z+in,x+57-in,top,z+57-in,BlockType::StoneBricks);fill(c,x+in+2,y+4,z+in+2,x+55-in,top-1,z+55-in,BlockType::Water);}}

void StructureGenerator::generateWoodlandMansion(WorldGenerationContext& c,JavaRandom& r,int cx,int cz) const
{
    const int x=cx*16-16,z=cz*16-12,y=c.getHeightValue(cx*16+8,cz*16+8);fill(c,x,y-2,z,x+39,y-1,z+29,BlockType::Cobblestone);
    for(int f=0;f<3;++f){const int fy=y+f*7;fill(c,x,fy,z,x+39,fy,z+29,BlockType::DarkOakPlanks);hollow(c,x,fy+1,z,x+39,fy+6,z+29,BlockType::DarkOakPlanks);for(int rx=10;rx<39;rx+=10)fill(c,x+rx,fy+1,z+1,x+rx,fy+5,z+28,BlockType::DarkOakPlanks);if(r.nextBoolean())fill(c,x+4,fy+1,z+4,x+8,fy+4,z+4,BlockType::Bookshelf);}}

std::optional<StructureLocation> StructureGenerator::findNearest(
    WorldStructure s,int blockX,int blockZ,int maxRadius,const ClimateSampler& climate) const
{
    if(s==WorldStructure::Stronghold){double best=std::numeric_limits<double>::max();std::optional<StructureLocation> out;for(auto [cx,cz]:strongholdChunks_){const int x=cx*16+8,z=cz*16+8;const double d=std::hypot(x-blockX,z-blockZ);if(d<best){best=d;out=StructureLocation{s,x,z,climate(x,z).biome};}}return out;}
    const int spacing=s==WorldStructure::Mineshaft?1:(s==WorldStructure::WoodlandMansion?80:32);const int ocx=floorDivide(blockX,16),ocz=floorDivide(blockZ,16),orx=floorDivide(ocx,spacing),orz=floorDivide(ocz,spacing);double best=std::numeric_limits<double>::max();std::optional<StructureLocation> out;
    for(int radius=0;radius<=maxRadius;++radius){for(int dx=-radius;dx<=radius;++dx)for(int dz=-radius;dz<=radius;++dz){if(radius&&std::abs(dx)!=radius&&std::abs(dz)!=radius)continue;int cx=orx+dx,cz=orz+dz;bool ok=false;
        if(s==WorldStructure::Village){std::tie(cx,cz)=scatteredCandidate(worldSeed_,cx,cz,32,8,10387312);ok=villageBiomeViable(cx,cz);}else if(s==WorldStructure::Temple){std::tie(cx,cz)=scatteredCandidate(worldSeed_,cx,cz,32,8,14357617);ok=templeBiome(climate(cx*16+8,cz*16+8).biome);}else if(s==WorldStructure::OceanMonument){std::tie(cx,cz)=triangularCandidate(worldSeed_,cx,cz,32,5,10387313);ok=monumentBiomeViable(cx,cz);}else if(s==WorldStructure::WoodlandMansion){std::tie(cx,cz)=triangularCandidate(worldSeed_,cx,cz,80,20,10387319);ok=mansionBiomeViable(cx,cz);}else ok=isMineshaftChunk(cx,cz);if(!ok)continue;const int x=cx*16+8,z=cz*16+8;const double d=std::hypot(x-blockX,z-blockZ);if(d<best){best=d;out=StructureLocation{s,x,z,climate(x,z).biome};}}
        if(out&&radius>1)break;}return out;
}
