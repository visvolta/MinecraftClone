#include "worldgen/SurfaceBuilder.h"

#include "Chunk.h"
#include "content/ContentCatalog.h"
#include "core/ResourceLocation.h"
#include "worldgen/BetaSimplexOctaves.h"
#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <numbers>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace
{
using State=mc::content::BlockState;
State fallback(BlockType b){return State(b);}
State named(std::string_view name,BlockType fb)
{
    const auto* c=mc::content::ContentCatalog::active();
    if(c){if(auto s=c->state(mc::core::ResourceLocation("minecraft",name)))return *s;}
    return fallback(fb);
}
State named(std::string_view name,std::initializer_list<std::pair<std::string,std::string>> props,BlockType fb)
{
    const auto* c=mc::content::ContentCatalog::active();
    if(c){std::vector<std::pair<std::string,std::string>> v(props);if(auto s=c->state(mc::core::ResourceLocation("minecraft",name),v))return *s;}
    return fallback(fb);
}
const BetaSimplexOctaves& grassNoise()
{
    static JavaRandom random(2345LL);
    static BetaSimplexOctaves noise(random,1);
    return noise;
}
bool isMegaTaiga(BiomeId b){return b==VanillaBiomes::MegaTaiga||b==VanillaBiomes::MegaTaigaHills||b==VanillaBiomes::MegaSpruceTaiga||b==VanillaBiomes::MegaSpruceTaigaHills;}
bool isMesa(BiomeId b){return b==VanillaBiomes::Mesa||b==VanillaBiomes::MesaPlateauF||b==VanillaBiomes::MesaPlateau||b==VanillaBiomes::MesaBryce||b==VanillaBiomes::MesaPlateauFMountains||b==VanillaBiomes::MesaPlateauMountains;}
bool mesaForest(BiomeId b){return b==VanillaBiomes::MesaPlateauF||b==VanillaBiomes::MesaPlateauFMountains;}
bool mesaBryce(BiomeId b){return b==VanillaBiomes::MesaBryce;}

void genericColumn(Chunk& chunk,int x,int z,const ClimateSample& climate,
                   double noise,JavaRandom& random,BlockType forcedTop=BlockType::Air,
                   BlockType forcedFiller=BlockType::Air,bool force=false)
{
    const int depth=static_cast<int>(noise/3.0+3.0+random.nextDouble()*0.25);
    int remaining=-1;
    BlockType top=force?forcedTop:SurfaceBuilder::biomeTopBlock(climate.biome);
    BlockType filler=force?forcedFiller:SurfaceBuilder::biomeFillerBlock(climate.biome);
    for(int y=255;y>=0;--y)
    {
        if(y<=random.nextInt(5)){chunk.setBlock(x,y,z,BlockType::Bedrock);continue;}
        const BlockType existing=chunk.getBlock(x,y,z);
        if(existing==BlockType::Air){remaining=-1;continue;}
        if(existing!=BlockType::Stone)continue;
        if(remaining==-1)
        {
            if(depth<=0){top=BlockType::Air;filler=BlockType::Stone;}
            else if(y>=SurfaceBuilder::SEA_LEVEL-4&&y<=SurfaceBuilder::SEA_LEVEL+1){top=force?forcedTop:SurfaceBuilder::biomeTopBlock(climate.biome);filler=force?forcedFiller:SurfaceBuilder::biomeFillerBlock(climate.biome);}
            if(y<SurfaceBuilder::SEA_LEVEL&&top==BlockType::Air)top=climate.temperature<0.15?BlockType::Ice:BlockType::Water;
            remaining=depth;
            if(y>=SurfaceBuilder::SEA_LEVEL-1)chunk.setBlock(x,y,z,top);
            else if(y<SurfaceBuilder::SEA_LEVEL-7-depth){top=BlockType::Air;filler=BlockType::Stone;chunk.setBlock(x,y,z,BlockType::Gravel);}
            else chunk.setBlock(x,y,z,filler);
        }
        else if(remaining>0)
        {
            --remaining;chunk.setBlock(x,y,z,filler);
            if(remaining==0&&filler==BlockType::Sand&&depth>1){remaining=random.nextInt(4)+std::max(0,y-SurfaceBuilder::SEA_LEVEL);filler=BlockType::Sandstone;}
        }
    }
}

// Build bands and the offset generator using the same single Random stream.
std::pair<std::array<State,64>,BetaSimplexOctaves> mesaBands(std::int64_t seed)
{
    const State hardened=named("hardened_clay",BlockType::Sandstone);
    const State orange=named("stained_hardened_clay",{{"color","orange"}},BlockType::Sandstone);
    const State yellow=named("stained_hardened_clay",{{"color","yellow"}},BlockType::Sandstone);
    const State brown=named("stained_hardened_clay",{{"color","brown"}},BlockType::Sandstone);
    const State red=named("stained_hardened_clay",{{"color","red"}},BlockType::Sandstone);
    const State white=named("stained_hardened_clay",{{"color","white"}},BlockType::Sandstone);
    const State silver=named("stained_hardened_clay",{{"color","silver"}},BlockType::Sandstone);
    std::array<State,64> bands;bands.fill(hardened);JavaRandom random(seed);
    for(int i=0;i<64;++i){i+=random.nextInt(5)+1;if(i<64)bands[static_cast<std::size_t>(i)]=orange;}
    int n=random.nextInt(4)+2;for(int i=0;i<n;++i){int len=random.nextInt(3)+1,start=random.nextInt(64);for(int j=0;start+j<64&&j<len;++j)bands[static_cast<std::size_t>(start+j)]=yellow;}
    n=random.nextInt(4)+2;for(int i=0;i<n;++i){int len=random.nextInt(3)+2,start=random.nextInt(64);for(int j=0;start+j<64&&j<len;++j)bands[static_cast<std::size_t>(start+j)]=brown;}
    n=random.nextInt(4)+2;for(int i=0;i<n;++i){int len=random.nextInt(3)+1,start=random.nextInt(64);for(int j=0;start+j<64&&j<len;++j)bands[static_cast<std::size_t>(start+j)]=red;}
    n=random.nextInt(3)+3;int cursor=0;for(int i=0;i<n;++i){cursor+=random.nextInt(16)+4;if(cursor<64){bands[static_cast<std::size_t>(cursor)]=white;if(cursor>1&&random.nextBoolean())bands[static_cast<std::size_t>(cursor-1)]=silver;if(cursor<63&&random.nextBoolean())bands[static_cast<std::size_t>(cursor+1)]=silver;}}
    BetaSimplexOctaves offset(random,1);return {bands,std::move(offset)};
}

void mesaColumn(Chunk& chunk,int lx,int lz,BiomeId biome,double noiseVal,JavaRandom& random,std::int64_t worldSeed)
{
    const int worldX=chunk.getWorldOriginX()+lx,worldZ=chunk.getWorldOriginZ()+lz;
    const State hardened=named("hardened_clay",BlockType::Sandstone);
    const State stained=named("stained_hardened_clay",BlockType::Sandstone);
    const State orange=named("stained_hardened_clay",{{"color","orange"}},BlockType::Sandstone);
    const State redSand=named("sand",{{"variant","red_sand"}},BlockType::Sand);
    const State coarse=named("dirt",{{"variant","coarse_dirt"}},BlockType::Dirt);
    const State grass=fallback(BlockType::Grass),water=fallback(BlockType::Water),stone=fallback(BlockType::Stone);

    auto [bands,offsetNoise]=mesaBands(worldSeed);
    JavaRandom pillarRandom(worldSeed);BetaSimplexOctaves pillar(pillarRandom,4),roof(pillarRandom,1);
    double bryceHeight=0.0;
    if(mesaBryce(biome))
    {
        const int i=(worldX&-16)+(worldZ&15),j=(worldZ&-16)+(worldX&15);
        const double d0=std::min(std::abs(noiseVal),pillar.value(i*.25,j*.25));
        if(d0>0.0){const double d2=std::abs(roof.value(i*.001953125,j*.001953125));bryceHeight=d0*d0*2.5;const double cap=std::ceil(d2*50.0)+14.0;bryceHeight=std::min(bryceHeight,cap)+64.0;}
    }
    const int depth=static_cast<int>(noiseVal/3.0+3.0+random.nextDouble()*.25);
    const bool flag=std::cos(noiseVal/3.0*std::numbers::pi)>0.0;
    int remaining=-1,countStone=0;bool topFlag=false;State top=stained,filler=stained;
    const int bandOffset=static_cast<int>(std::round(offsetNoise.value(worldX/512.0,worldX/512.0)*2.0));
    auto bandAt=[&](int y){return bands[static_cast<std::size_t>((y+bandOffset+64)%64)];};
    for(int y=255;y>=0;--y)
    {
        if(chunk.getBlock(lx,y,lz)==BlockType::Air&&y<static_cast<int>(bryceHeight))chunk.setBlockState(lx,y,lz,stone);
        if(y<=random.nextInt(5)){chunk.setBlock(lx,y,lz,BlockType::Bedrock);continue;}
        if(countStone>=15&&!mesaBryce(biome))continue;
        const BlockType current=chunk.getBlock(lx,y,lz);
        if(current==BlockType::Air){remaining=-1;continue;}
        if(current!=BlockType::Stone)continue;
        if(remaining==-1)
        {
            topFlag=false;top=stained;filler=stained;
            if(depth<=0){top=fallback(BlockType::Air);filler=stone;}
            else if(y>=SurfaceBuilder::SEA_LEVEL-4&&y<=SurfaceBuilder::SEA_LEVEL+1){top=stained;filler=stained;}
            if(y<SurfaceBuilder::SEA_LEVEL&&top.isAir())top=water;
            remaining=depth+std::max(0,y-SurfaceBuilder::SEA_LEVEL);
            if(y>=SurfaceBuilder::SEA_LEVEL-1)
            {
                if(mesaForest(biome)&&y>86+depth*2)chunk.setBlockState(lx,y,lz,flag?coarse:grass);
                else if(y>SurfaceBuilder::SEA_LEVEL+3+depth)chunk.setBlockState(lx,y,lz,(y>=64&&y<=127)?(flag?hardened:bandAt(y)):orange);
                else {chunk.setBlockState(lx,y,lz,redSand);topFlag=true;}
            }
            else {chunk.setBlockState(lx,y,lz,filler);if(filler==stained)chunk.setBlockState(lx,y,lz,orange);}
        }
        else if(remaining>0){--remaining;chunk.setBlockState(lx,y,lz,topFlag?orange:bandAt(y));}
        ++countStone;
    }
}
}

void SurfaceBuilder::replaceColumn(Chunk& chunk,int x,int z,const ClimateSample& climate,
                                   double,double,double stoneNoise,JavaRandom& random) const
{
    const BiomeId b=climate.biome;
    if(isMesa(b)){mesaColumn(chunk,x,z,b,stoneNoise,random,worldSeed_);return;}

    BlockType top=biomeTopBlock(b),filler=biomeFillerBlock(b);bool force=false;
    if(isMegaTaiga(b)){top=BlockType::Grass;filler=BlockType::Dirt;if(stoneNoise>1.75)top=BlockType::Dirt;else if(stoneNoise>-.95)top=BlockType::Podzol;force=true;}
    if(b==VanillaBiomes::ExtremeHillsMountains){top=BlockType::Grass;filler=BlockType::Dirt;if(stoneNoise<-1.0||stoneNoise>2.0){top=filler=BlockType::Gravel;}else if(stoneNoise>1.0){top=filler=BlockType::Stone;}force=true;}
    else if(b==VanillaBiomes::ExtremeHills||b==VanillaBiomes::ExtremeHillsPlus||b==VanillaBiomes::ExtremeHillsPlusMountains){top=BlockType::Grass;filler=BlockType::Dirt;if(stoneNoise>1.0&&b!=VanillaBiomes::ExtremeHillsPlus){top=filler=BlockType::Stone;}force=true;}
    if(b==VanillaBiomes::SavannaMountains||b==VanillaBiomes::SavannaPlateauMountains){top=BlockType::Grass;filler=BlockType::Dirt;if(stoneNoise>1.75)top=filler=BlockType::Stone;else if(stoneNoise>-.5)top=BlockType::Dirt;force=true;}

    // BiomeSwamp adjusts the y=62 shoreline before normal terrain replacement.
    if(b==VanillaBiomes::Swampland||b==VanillaBiomes::SwamplandMountains)
    {
        const int wx=chunk.getWorldOriginX()+x,wz=chunk.getWorldOriginZ()+z;
        const double n=grassNoise().value(wx*.25,wz*.25);
        if(n>0.0)
        {
            for(int y=255;y>=0;--y)
            {
                if(chunk.getBlock(x,y,z)==BlockType::Air) continue;
                if(y==62&&chunk.getBlock(x,y,z)!=BlockType::Water)
                {
                    chunk.setBlock(x,y,z,BlockType::Water);
                    if(n<0.12)
                        chunk.setBlockState(x,y+1,z,named("waterlily",BlockType::Air));
                }
                break;
            }
        }
    }
    genericColumn(chunk,x,z,climate,stoneNoise,random,top,filler,force);

    // BiomeSnow(superIcy=true) uses the full snow block as its surface.
    // The legacy enum only exposes a snow layer, so use the registry-backed
    // minecraft:snow state when available.
    if(b==VanillaBiomes::IcePlainsSpikes)
    {
        for(int y=255;y>=0;--y)
        {
            if(chunk.getBlock(x,y,z)==BlockType::Grass)
            {
                chunk.setBlockState(x,y,z,named("snow",BlockType::Snow));
                break;
            }
            if(chunk.getBlock(x,y,z)!=BlockType::Air&&
               chunk.getBlock(x,y,z)!=BlockType::Water&&
               chunk.getBlock(x,y,z)!=BlockType::Ice)
                break;
        }
    }
}

BlockType SurfaceBuilder::biomeTopBlock(BiomeId b) noexcept
{const BiomeDefinition* d=BiomeRegistry::active().find(b);return d?d->topBlock:BlockType::Grass;}
BlockType SurfaceBuilder::biomeFillerBlock(BiomeId b) noexcept
{const BiomeDefinition* d=BiomeRegistry::active().find(b);return d?d->fillerBlock:BlockType::Dirt;}
