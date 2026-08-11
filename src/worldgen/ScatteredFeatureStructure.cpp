#include "worldgen/ScatteredFeatureStructure.h"

#include "worldgen/Vanilla112State.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <memory>
#include <utility>

namespace mc112
{
namespace
{
Facing randomHorizontal(JavaRandom& random)
{
    // EnumFacing.Plane.HORIZONTAL.facings(): N, E, S, W.
    constexpr std::array<Facing, 4> values{
        Facing::North, Facing::East, Facing::South, Facing::West};
    return values[static_cast<std::size_t>(random.nextInt(4))];
}

Rotation randomRotation(JavaRandom& random)
{
    // Rotation.values(): NONE, CLOCKWISE_90, CLOCKWISE_180,
    // COUNTERCLOCKWISE_90.
    switch(random.nextInt(4))
    {
        case 1: return Rotation::Clockwise90;
        case 2: return Rotation::Clockwise180;
        case 3: return Rotation::CounterClockwise90;
        default: return Rotation::None;
    }
}

Box featureBox(
    int x,
    int y,
    int z,
    int sizeX,
    int sizeY,
    int sizeZ,
    Facing facing)
{
    if(facing == Facing::North || facing == Facing::South)
        return {x, y, z, x + sizeX - 1, y + sizeY - 1, z + sizeZ - 1};
    return {x, y, z, x + sizeZ - 1, y + sizeY - 1, z + sizeX - 1};
}

class FeaturePiece : public Piece
{
public:
    FeaturePiece(
        JavaRandom& random,
        int x,
        int y,
        int z,
        int sizeX,
        int sizeY,
        int sizeZ)
        : sizeX_(sizeX), sizeY_(sizeY), sizeZ_(sizeZ)
    {
        facing = randomHorizontal(random);
        box = featureBox(x, y, z, sizeX, sizeY, sizeZ, *facing);
    }

protected:
    int sizeX_ = 0;
    int sizeY_ = 0;
    int sizeZ_ = 0;
    int horizontalPos_ = -1;

    bool offsetToAverageGroundLevel(
        WorldGenerationContext& context,
        const Box& clip,
        int yOffset)
    {
        if(horizontalPos_ >= 0)
            return true;

        int total = 0;
        int count = 0;
        for(int z = box.minZ; z <= box.maxZ; ++z)
        {
            for(int x = box.minX; x <= box.maxX; ++x)
            {
                if(!clip.contains(x, 64, z))
                    continue;
                // WorldProvider#getAverageGroundLevel is seaLevel + 1 in the
                // normal overworld, i.e. 64 for vanilla's sea level 63.
                total += std::max(context.getTopSolidOrLiquidBlockY(x, z), 64);
                ++count;
            }
        }
        if(count == 0)
            return false;

        horizontalPos_ = total / count;
        box.offset(0, horizontalPos_ - box.minY + yOffset, 0);
        return true;
    }
};

class DesertPyramid final : public FeaturePiece
{
public:
    DesertPyramid(JavaRandom& random, int x, int z)
        : FeaturePiece(random, x, 64, z, 21, 15, 21)
    {
    }

    bool place(
        WorldGenerationContext& context,
        JavaRandom& random,
        const Box& clip) override
    {
        constexpr int W = 21;
        constexpr int D = 21;
        const auto sandstone = state("sandstone");
        const auto smooth = state("smooth_sandstone");
        const auto chiseled = state("chiseled_sandstone");
        const auto air = state("air");
        const auto orange = state("orange_stained_hardened_clay");
        const auto blue = state("blue_stained_hardened_clay");
        const auto north = state("sandstone_stairs", {{"facing","north"},{"half","bottom"},{"shape","straight"}});
        const auto south = state("sandstone_stairs", {{"facing","south"},{"half","bottom"},{"shape","straight"}});
        const auto east = state("sandstone_stairs", {{"facing","east"},{"half","bottom"},{"shape","straight"}});
        const auto west = state("sandstone_stairs", {{"facing","west"},{"half","bottom"},{"shape","straight"}});
        const auto sandSlab = state("sandstone_slab", {{"half","bottom"}});

        fill(context,clip,0,-4,0,W-1,0,D-1,sandstone,sandstone);
        for(int i=1;i<=9;++i)
        {
            fill(context,clip,i,i,i,W-1-i,i,D-1-i,sandstone,sandstone);
            fill(context,clip,i+1,i,i+1,W-2-i,i,D-2-i,air,air);
        }
        for(int x=0;x<W;++x)
            for(int z=0;z<D;++z)
                replaceAirAndLiquidDownwards(context,clip,sandstone,x,-5,z);

        fill(context,clip,0,0,0,4,9,4,sandstone,air);
        fill(context,clip,1,10,1,3,10,3,sandstone,sandstone);
        setBlock(context,clip,north,2,10,0); setBlock(context,clip,south,2,10,4);
        setBlock(context,clip,east,0,10,2); setBlock(context,clip,west,4,10,2);
        fill(context,clip,W-5,0,0,W-1,9,4,sandstone,air);
        fill(context,clip,W-4,10,1,W-2,10,3,sandstone,sandstone);
        setBlock(context,clip,north,W-3,10,0); setBlock(context,clip,south,W-3,10,4);
        setBlock(context,clip,east,W-5,10,2); setBlock(context,clip,west,W-1,10,2);
        fill(context,clip,8,0,0,12,4,4,sandstone,air);
        fill(context,clip,9,1,0,11,3,4,air,air);
        for(auto [x,y,z] : std::array<std::array<int,3>,7>{{
            {{9,1,1}},{{9,2,1}},{{9,3,1}},{{10,3,1}},{{11,3,1}},{{11,2,1}},{{11,1,1}}
        }}) setBlock(context,clip,smooth,x,y,z);
        fill(context,clip,4,1,1,8,3,3,sandstone,air);
        fill(context,clip,4,1,2,8,2,2,air,air);
        fill(context,clip,12,1,1,16,3,3,sandstone,air);
        fill(context,clip,12,1,2,16,2,2,air,air);
        fill(context,clip,5,4,5,W-6,4,D-6,sandstone,sandstone);
        fill(context,clip,9,4,9,11,4,11,air,air);
        for(auto [x,z] : std::array<std::array<int,2>,4>{{{{8,8}},{{12,8}},{{8,12}},{{12,12}}}})
            fill(context,clip,x,1,z,x,3,z,smooth,smooth);
        fill(context,clip,1,1,5,4,4,11,sandstone,sandstone);
        fill(context,clip,W-5,1,5,W-2,4,11,sandstone,sandstone);
        fill(context,clip,6,7,9,6,7,11,sandstone,sandstone);
        fill(context,clip,W-7,7,9,W-7,7,11,sandstone,sandstone);
        fill(context,clip,5,5,9,5,7,11,smooth,smooth);
        fill(context,clip,W-6,5,9,W-6,7,11,smooth,smooth);
        setBlock(context,clip,air,5,5,10);setBlock(context,clip,air,5,6,10);setBlock(context,clip,air,6,6,10);
        setBlock(context,clip,air,W-6,5,10);setBlock(context,clip,air,W-6,6,10);setBlock(context,clip,air,W-7,6,10);
        fill(context,clip,2,4,4,2,6,4,air,air);
        fill(context,clip,W-3,4,4,W-3,6,4,air,air);
        setBlock(context,clip,north,2,4,5);setBlock(context,clip,north,2,3,4);
        setBlock(context,clip,north,W-3,4,5);setBlock(context,clip,north,W-3,3,4);
        fill(context,clip,1,1,3,2,2,3,sandstone,sandstone);
        fill(context,clip,W-3,1,3,W-2,2,3,sandstone,sandstone);
        setBlock(context,clip,sandstone,1,1,2);setBlock(context,clip,sandstone,W-2,1,2);
        setBlock(context,clip,sandSlab,1,2,2);setBlock(context,clip,sandSlab,W-2,2,2);
        setBlock(context,clip,west,2,1,2);setBlock(context,clip,east,W-3,1,2);
        fill(context,clip,4,3,5,4,3,18,sandstone,sandstone);
        fill(context,clip,W-5,3,5,W-5,3,17,sandstone,sandstone);
        fill(context,clip,3,1,5,4,2,16,air,air);
        fill(context,clip,W-6,1,5,W-5,2,16,air,air);
        for(int z=5;z<=17;z+=2)
        {
            setBlock(context,clip,smooth,4,1,z);setBlock(context,clip,chiseled,4,2,z);
            setBlock(context,clip,smooth,W-5,1,z);setBlock(context,clip,chiseled,W-5,2,z);
        }

        // The central orange/blue terracotta floor glyph.
        for(auto [x,z] : std::array<std::array<int,2>,12>{{
            {{10,7}},{{10,8}},{{9,9}},{{11,9}},{{8,10}},{{12,10}},
            {{7,10}},{{13,10}},{{9,11}},{{11,11}},{{10,12}},{{10,13}}
        }}) setBlock(context,clip,orange,x,0,z);
        setBlock(context,clip,blue,10,0,10);

        for(int x : {0,W-1})
        {
            setBlock(context,clip,smooth,x,2,1);setBlock(context,clip,orange,x,2,2);setBlock(context,clip,smooth,x,2,3);
            setBlock(context,clip,smooth,x,3,1);setBlock(context,clip,orange,x,3,2);setBlock(context,clip,smooth,x,3,3);
            setBlock(context,clip,orange,x,4,1);setBlock(context,clip,chiseled,x,4,2);setBlock(context,clip,orange,x,4,3);
            setBlock(context,clip,smooth,x,5,1);setBlock(context,clip,orange,x,5,2);setBlock(context,clip,smooth,x,5,3);
            setBlock(context,clip,orange,x,6,1);setBlock(context,clip,chiseled,x,6,2);setBlock(context,clip,orange,x,6,3);
            setBlock(context,clip,orange,x,7,1);setBlock(context,clip,orange,x,7,2);setBlock(context,clip,orange,x,7,3);
            setBlock(context,clip,smooth,x,8,1);setBlock(context,clip,smooth,x,8,2);setBlock(context,clip,smooth,x,8,3);
        }
        for(int x : {2,W-3})
        {
            setBlock(context,clip,smooth,x-1,2,0);setBlock(context,clip,orange,x,2,0);setBlock(context,clip,smooth,x+1,2,0);
            setBlock(context,clip,smooth,x-1,3,0);setBlock(context,clip,orange,x,3,0);setBlock(context,clip,smooth,x+1,3,0);
            setBlock(context,clip,orange,x-1,4,0);setBlock(context,clip,chiseled,x,4,0);setBlock(context,clip,orange,x+1,4,0);
            setBlock(context,clip,smooth,x-1,5,0);setBlock(context,clip,orange,x,5,0);setBlock(context,clip,smooth,x+1,5,0);
            setBlock(context,clip,orange,x-1,6,0);setBlock(context,clip,chiseled,x,6,0);setBlock(context,clip,orange,x+1,6,0);
            setBlock(context,clip,orange,x-1,7,0);setBlock(context,clip,orange,x,7,0);setBlock(context,clip,orange,x+1,7,0);
            setBlock(context,clip,smooth,x-1,8,0);setBlock(context,clip,smooth,x,8,0);setBlock(context,clip,smooth,x+1,8,0);
        }
        fill(context,clip,8,4,0,12,6,0,smooth,smooth);
        setBlock(context,clip,air,8,6,0);setBlock(context,clip,air,12,6,0);
        setBlock(context,clip,orange,9,5,0);setBlock(context,clip,chiseled,10,5,0);setBlock(context,clip,orange,11,5,0);

        fill(context,clip,8,-14,8,12,-11,12,smooth,smooth);
        fill(context,clip,8,-10,8,12,-10,12,chiseled,chiseled);
        fill(context,clip,8,-9,8,12,-9,12,smooth,smooth);
        fill(context,clip,8,-8,8,12,-1,12,sandstone,sandstone);
        fill(context,clip,9,-11,9,11,-1,11,air,air);
        setBlock(context,clip,state("stone_pressure_plate"),10,-11,10);
        fill(context,clip,9,-13,9,11,-13,11,state("tnt"),air);
        setBlock(context,clip,air,8,-11,10);setBlock(context,clip,air,8,-10,10);
        setBlock(context,clip,chiseled,7,-10,10);setBlock(context,clip,smooth,7,-11,10);
        setBlock(context,clip,air,12,-11,10);setBlock(context,clip,air,12,-10,10);
        setBlock(context,clip,chiseled,13,-10,10);setBlock(context,clip,smooth,13,-11,10);
        setBlock(context,clip,air,10,-11,8);setBlock(context,clip,air,10,-10,8);
        setBlock(context,clip,chiseled,10,-10,7);setBlock(context,clip,smooth,10,-11,7);
        setBlock(context,clip,air,10,-11,12);setBlock(context,clip,air,10,-10,12);
        setBlock(context,clip,chiseled,10,-10,13);setBlock(context,clip,smooth,10,-11,13);

        // EnumFacing.Plane.HORIZONTAL iteration is N,E,S,W. Chest placement
        // uses BlockChest#correctFacing in world coordinates, then assigns the
        // desert-pyramid loot table with random.nextLong().
        constexpr std::array<std::array<int,2>,4> dirs{{
            {{0,-1}},{{1,0}},{{0,1}},{{-1,0}}
        }};
        for(const auto& dir:dirs)
        {
            const int lx=10+dir[0]*2;
            const int lz=10+dir[1]*2;
            const int wx=worldX(lx,lz), wy=worldY(-11), wz=worldZ(lx,lz);
            if(!clip.contains(wx,wy,wz) || named(context.getBlockState(wx,wy,wz),"chest"))
                continue;

            std::string facingName="north";
            struct D{int dx,dz;const char* name;};
            constexpr std::array<D,4> around{{
                {0,-1,"north"},{1,0,"east"},{0,1,"south"},{-1,0,"west"}
            }};
            const D* oneSolid=nullptr;
            bool multiple=false;
            for(const auto& d:around)
            {
                const auto neighbor=context.getBlockState(wx+d.dx,wy,wz+d.dz);
                if(named(neighbor,"chest")){oneSolid=nullptr;multiple=true;break;}
                if(isSolid(neighbor))
                {
                    if(oneSolid){multiple=true;break;}
                    oneSolid=&d;
                }
            }
            if(!multiple && oneSolid)
            {
                if(std::string_view(oneSolid->name)=="north")facingName="south";
                else if(std::string_view(oneSolid->name)=="south")facingName="north";
                else if(std::string_view(oneSolid->name)=="east")facingName="west";
                else facingName="east";
            }
            else if(!multiple)
            {
                auto blocked=[&](std::string_view name){
                    for(const auto& d:around) if(name==d.name)
                        return isSolid(context.getBlockState(wx+d.dx,wy,wz+d.dz));
                    return false;
                };
                if(blocked(facingName)) facingName="south";
                if(blocked(facingName)) facingName="west";
                if(blocked(facingName)) facingName="east";
            }
            context.setBlockState(wx,wy,wz,state("chest",{{"facing",facingName}}));
            const auto seed=random.nextLong();
            context.assignStructureLoot(wx,wy,wz,"minecraft:chests/desert_pyramid",seed);
        }
        return true;
    }
};

class JunglePyramid final : public FeaturePiece
{
public:
    JunglePyramid(JavaRandom& random, int x, int z)
        : FeaturePiece(random, x, 64, z, 12, 10, 15)
    {
    }

    bool place(
        WorldGenerationContext& context,
        JavaRandom& random,
        const Box& clip) override
    {
        if(!offsetToAverageGroundLevel(context, clip, 0))
            return false;

        const auto air = state("air");
        const auto cobble = state("cobblestone");
        const auto mossy = state("mossy_cobblestone");
        const auto redstone = state("redstone_wire");
        const auto chiseled = state("chiseled_stonebrick");
        const auto stairEast = state("stone_stairs", {
            {"facing","east"},{"half","bottom"},{"shape","straight"}});
        const auto stairWest = state("stone_stairs", {
            {"facing","west"},{"half","bottom"},{"shape","straight"}});
        const auto stairSouth = state("stone_stairs", {
            {"facing","south"},{"half","bottom"},{"shape","straight"}});
        const auto stairNorth = state("stone_stairs", {
            {"facing","north"},{"half","bottom"},{"shape","straight"}});

        const auto stones = [&](int x0,int y0,int z0,int x1,int y1,int z1)
        {
            // JunglePyramid.Stones#selectBlocks is called once for every
            // coordinate, in StructureComponent's y/x/z loop order.
            for(int y=y0; y<=y1; ++y)
                for(int x=x0; x<=x1; ++x)
                    for(int z=z0; z<=z1; ++z)
                        setBlock(
                            context, clip,
                            random.nextFloat() < 0.4F ? cobble : mossy,
                            x, y, z);
        };

        stones(0,-4,0,sizeX_-1,0,sizeZ_-1);
        stones(2,1,2,9,2,2);
        stones(2,1,12,9,2,12);
        stones(2,1,3,2,2,11);
        stones(9,1,3,9,2,11);
        stones(1,3,1,10,6,1);
        stones(1,3,13,10,6,13);
        stones(1,3,2,1,6,12);
        stones(10,3,2,10,6,12);
        stones(2,3,2,9,3,12);
        stones(2,6,2,9,6,12);
        stones(3,7,3,8,7,11);
        stones(4,8,4,7,8,10);
        fillAir(context,clip,3,1,3,8,2,11);
        fillAir(context,clip,4,3,6,7,3,9);
        fillAir(context,clip,2,4,2,9,5,12);
        fillAir(context,clip,4,6,5,7,6,9);
        fillAir(context,clip,5,7,6,6,7,8);
        fillAir(context,clip,5,1,2,6,2,2);
        fillAir(context,clip,5,2,12,6,2,12);
        fillAir(context,clip,5,5,1,6,5,1);
        fillAir(context,clip,5,5,13,6,5,13);
        setBlock(context,clip,air,1,5,5);
        setBlock(context,clip,air,10,5,5);
        setBlock(context,clip,air,1,5,9);
        setBlock(context,clip,air,10,5,9);

        for(int z : {0,14})
        {
            stones(2,4,z,2,5,z);
            stones(4,4,z,4,5,z);
            stones(7,4,z,7,5,z);
            stones(9,4,z,9,5,z);
        }
        stones(5,6,0,6,6,0);
        for(int x : {0,11})
        {
            for(int z=2; z<=12; z+=2)
                stones(x,4,z,x,5,z);
            stones(x,6,5,x,6,5);
            stones(x,6,9,x,6,9);
        }
        stones(2,7,2,2,9,2);
        stones(9,7,2,9,9,2);
        stones(2,7,12,2,9,12);
        stones(9,7,12,9,9,12);
        stones(4,9,4,4,9,4);
        stones(7,9,4,7,9,4);
        stones(4,9,10,4,9,10);
        stones(7,9,10,7,9,10);
        stones(5,9,7,6,9,7);

        setBlock(context,clip,stairNorth,5,9,6);
        setBlock(context,clip,stairNorth,6,9,6);
        setBlock(context,clip,stairSouth,5,9,8);
        setBlock(context,clip,stairSouth,6,9,8);
        setBlock(context,clip,stairNorth,4,0,0);
        setBlock(context,clip,stairNorth,5,0,0);
        setBlock(context,clip,stairNorth,6,0,0);
        setBlock(context,clip,stairNorth,7,0,0);
        setBlock(context,clip,stairNorth,4,1,8);
        setBlock(context,clip,stairNorth,4,2,9);
        setBlock(context,clip,stairNorth,4,3,10);
        setBlock(context,clip,stairNorth,7,1,8);
        setBlock(context,clip,stairNorth,7,2,9);
        setBlock(context,clip,stairNorth,7,3,10);
        stones(4,1,9,4,1,9);
        stones(7,1,9,7,1,9);
        stones(4,1,10,7,2,10);
        stones(5,4,5,6,4,5);
        setBlock(context,clip,stairEast,4,4,5);
        setBlock(context,clip,stairWest,7,4,5);

        for(int i=0; i<4; ++i)
        {
            setBlock(context,clip,stairSouth,5,-i,6+i);
            setBlock(context,clip,stairSouth,6,-i,6+i);
            fillAir(context,clip,5,-i,7+i,6,-i,9+i);
        }

        fillAir(context,clip,1,-3,12,10,-1,13);
        fillAir(context,clip,1,-3,1,3,-1,13);
        fillAir(context,clip,1,-3,1,9,-1,5);
        for(int z=1; z<=13; z+=2)
            stones(1,-3,z,1,-2,z);
        for(int z=2; z<=12; z+=2)
            stones(1,-1,z,3,-1,z);
        stones(2,-2,1,5,-2,1);
        stones(7,-2,1,9,-2,1);
        stones(6,-3,1,6,-3,1);
        stones(6,-1,1,6,-1,1);

        setBlock(context,clip,state("tripwire_hook", {
            {"attached","true"},{"facing","east"},{"powered","false"}}),
            1,-3,8);
        setBlock(context,clip,state("tripwire_hook", {
            {"attached","true"},{"facing","west"},{"powered","false"}}),
            4,-3,8);
        const auto attachedTripwire = state("tripwire", {
            {"attached","true"},{"disarmed","false"},{"powered","false"}});
        setBlock(context,clip,attachedTripwire,2,-3,8);
        setBlock(context,clip,attachedTripwire,3,-3,8);
        for(int z=7; z>=1; --z)
            setBlock(context,clip,redstone,5,-3,z);
        setBlock(context,clip,redstone,4,-3,1);
        setBlock(context,clip,mossy,3,-3,1);

        if(!placedTrap1_)
            placedTrap1_ = createDispenser(
                context,clip,random,3,-2,1,"north");

        setBlock(context,clip,state("vine", {
            {"east","false"},{"north","false"},{"south","true"},
            {"up","false"},{"west","false"}}),3,-2,2);
        setBlock(context,clip,state("tripwire_hook", {
            {"attached","true"},{"facing","north"},{"powered","false"}}),
            7,-3,1);
        setBlock(context,clip,state("tripwire_hook", {
            {"attached","true"},{"facing","south"},{"powered","false"}}),
            7,-3,5);
        setBlock(context,clip,attachedTripwire,7,-3,2);
        setBlock(context,clip,attachedTripwire,7,-3,3);
        setBlock(context,clip,attachedTripwire,7,-3,4);
        setBlock(context,clip,redstone,8,-3,6);
        setBlock(context,clip,redstone,9,-3,6);
        setBlock(context,clip,redstone,9,-3,5);
        setBlock(context,clip,mossy,9,-3,4);
        setBlock(context,clip,redstone,9,-2,4);

        if(!placedTrap2_)
            placedTrap2_ = createDispenser(
                context,clip,random,9,-2,3,"west");

        const auto vineEast = state("vine", {
            {"east","true"},{"north","false"},{"south","false"},
            {"up","false"},{"west","false"}});
        setBlock(context,clip,vineEast,8,-1,3);
        setBlock(context,clip,vineEast,8,-2,3);

        if(!placedMainChest_)
            placedMainChest_ = createChest(context,clip,random,8,-3,3);

        setBlock(context,clip,mossy,9,-3,2);
        setBlock(context,clip,mossy,8,-3,1);
        setBlock(context,clip,mossy,4,-3,5);
        setBlock(context,clip,mossy,5,-2,5);
        setBlock(context,clip,mossy,5,-1,5);
        setBlock(context,clip,mossy,6,-3,5);
        setBlock(context,clip,mossy,7,-2,5);
        setBlock(context,clip,mossy,7,-1,5);
        setBlock(context,clip,mossy,8,-3,5);
        stones(9,-1,1,9,-1,5);
        fillAir(context,clip,8,-3,8,10,-1,10);
        setBlock(context,clip,chiseled,8,-2,11);
        setBlock(context,clip,chiseled,9,-2,11);
        setBlock(context,clip,chiseled,10,-2,11);
        const auto leverNorth = state("lever", {
            {"facing","north"},{"powered","false"}});
        setBlock(context,clip,leverNorth,8,-2,12);
        setBlock(context,clip,leverNorth,9,-2,12);
        setBlock(context,clip,leverNorth,10,-2,12);
        stones(8,-3,8,8,-3,10);
        stones(10,-3,8,10,-3,10);
        setBlock(context,clip,mossy,10,-2,9);
        setBlock(context,clip,redstone,8,-2,9);
        setBlock(context,clip,redstone,8,-2,10);
        setBlock(context,clip,redstone,10,-1,9);
        setBlock(context,clip,state("sticky_piston", {
            {"extended","false"},{"facing","up"}}),9,-2,8);
        setBlock(context,clip,state("sticky_piston", {
            {"extended","false"},{"facing","west"}}),10,-2,8);
        setBlock(context,clip,state("sticky_piston", {
            {"extended","false"},{"facing","west"}}),10,-1,8);
        setBlock(context,clip,state("unpowered_repeater", {
            {"delay","1"},{"facing","north"},{"locked","false"}}),
            10,-2,10);

        if(!placedHiddenChest_)
            placedHiddenChest_ = createChest(context,clip,random,9,-3,10);

        return true;
    }

private:
    bool placedMainChest_ = false;
    bool placedHiddenChest_ = false;
    bool placedTrap1_ = false;
    bool placedTrap2_ = false;

    bool createDispenser(
        WorldGenerationContext& context,
        const Box& clip,
        JavaRandom& random,
        int x,int y,int z,
        std::string_view facingName)
    {
        const int wx=worldX(x,z), wy=worldY(y), wz=worldZ(x,z);
        if(!clip.contains(wx,wy,wz) ||
           named(context.getBlockState(wx,wy,wz),"dispenser"))
            return false;
        setBlock(context,clip,state("dispenser", {
            {"facing",std::string(facingName)},{"triggered","false"}}),x,y,z);
        const auto seed=random.nextLong();
        context.assignStructureLoot(wx,wy,wz,"minecraft:chests/jungle_temple_dispenser",seed);
        return true;
    }

    bool createChest(
        WorldGenerationContext& context,
        const Box& clip,
        JavaRandom& random,
        int x,int y,int z)
    {
        const int wx=worldX(x,z), wy=worldY(y), wz=worldZ(x,z);
        if(!clip.contains(wx,wy,wz) ||
           named(context.getBlockState(wx,wy,wz),"chest"))
            return false;

        std::string facingName="north";
        struct D{int dx,dz;const char* name;};
        constexpr std::array<D,4> around{{
            {0,-1,"north"},{1,0,"east"},{0,1,"south"},{-1,0,"west"}
        }};
        const D* oneSolid=nullptr;
        bool multiple=false;
        for(const auto& d:around)
        {
            const auto neighbor=context.getBlockState(wx+d.dx,wy,wz+d.dz);
            if(named(neighbor,"chest"))
            {
                oneSolid=nullptr;
                multiple=true;
                break;
            }
            if(isSolid(neighbor))
            {
                if(oneSolid)
                {
                    multiple=true;
                    break;
                }
                oneSolid=&d;
            }
        }
        if(!multiple && oneSolid)
        {
            if(std::string_view(oneSolid->name)=="north") facingName="south";
            else if(std::string_view(oneSolid->name)=="south") facingName="north";
            else if(std::string_view(oneSolid->name)=="east") facingName="west";
            else facingName="east";
        }
        else if(!multiple)
        {
            auto blocked=[&](std::string_view name){
                for(const auto& d:around)
                    if(name==d.name)
                        return isSolid(context.getBlockState(wx+d.dx,wy,wz+d.dz));
                return false;
            };
            if(blocked(facingName)) facingName="south";
            if(blocked(facingName)) facingName="west";
            if(blocked(facingName)) facingName="east";
        }
        context.setBlockState(wx,wy,wz,state("chest",{{"facing",facingName}}));
        const auto seed=random.nextLong();
        context.assignStructureLoot(wx,wy,wz,"minecraft:chests/jungle_temple",seed);
        return true;
    }
};

class SwampHut final : public FeaturePiece
{
public:
    SwampHut(JavaRandom& random, int x, int z)
        : FeaturePiece(random, x, 64, z, 7, 7, 9)
    {
    }

    bool place(
        WorldGenerationContext& context,
        JavaRandom&,
        const Box& clip) override
    {
        if(!offsetToAverageGroundLevel(context, clip, 0))
            return false;

        const auto spruce = state("spruce_planks");
        const auto oakLog = state("oak_log");
        const auto oakFence = state("oak_fence");
        const auto air = state("air");
        const auto flowerPot = state(
            "flower_pot", {{"contents", "mushroom_red"}});
        const auto crafting = state("crafting_table");
        const auto cauldron = state("cauldron");

        fill(context, clip, 1,1,1,5,1,7, spruce, spruce);
        fill(context, clip, 1,4,2,5,4,7, spruce, spruce);
        fill(context, clip, 2,1,0,4,1,0, spruce, spruce);
        fill(context, clip, 2,2,2,3,3,2, spruce, spruce);
        fill(context, clip, 1,2,3,1,3,6, spruce, spruce);
        fill(context, clip, 5,2,3,5,3,6, spruce, spruce);
        fill(context, clip, 2,2,7,4,3,7, spruce, spruce);
        fill(context, clip, 1,0,2,1,3,2, oakLog, oakLog);
        fill(context, clip, 5,0,2,5,3,2, oakLog, oakLog);
        fill(context, clip, 1,0,7,1,3,7, oakLog, oakLog);
        fill(context, clip, 5,0,7,5,3,7, oakLog, oakLog);
        setBlock(context, clip, oakFence, 2,3,2);
        setBlock(context, clip, oakFence, 3,3,7);
        setBlock(context, clip, air, 1,3,4);
        setBlock(context, clip, air, 5,3,4);
        setBlock(context, clip, air, 5,3,5);
        setBlock(context, clip, flowerPot, 1,3,5);
        setBlock(context, clip, crafting, 3,2,6);
        setBlock(context, clip, cauldron, 4,2,6);
        setBlock(context, clip, oakFence, 1,2,1);
        setBlock(context, clip, oakFence, 5,2,1);

        const auto north = state("spruce_stairs", {{"facing","north"},{"half","bottom"},{"shape","straight"}});
        const auto east = state("spruce_stairs", {{"facing","east"},{"half","bottom"},{"shape","straight"}});
        const auto west = state("spruce_stairs", {{"facing","west"},{"half","bottom"},{"shape","straight"}});
        const auto south = state("spruce_stairs", {{"facing","south"},{"half","bottom"},{"shape","straight"}});
        fill(context, clip, 0,4,1,6,4,1, north, north);
        fill(context, clip, 0,4,2,0,4,7, east, east);
        fill(context, clip, 6,4,2,6,4,7, west, west);
        fill(context, clip, 0,4,8,6,4,8, south, south);

        for(int z : {2, 7})
            for(int x : {1, 5})
                replaceAirAndLiquidDownwards(
                    context, clip, oakLog, x, -1, z);

        // Vanilla spawns one persistent witch at local (2,2,5) the first
        // time that point is inside the post-process box. Entity generation is
        // intentionally kept out of the block-only context; no RNG is consumed
        // by this spawn path, so block generation remains exact.
        return true;
    }
};

class Igloo final : public FeaturePiece
{
public:
    Igloo(JavaRandom& random, int x, int z)
        : FeaturePiece(random, x, 64, z, 7, 5, 8)
    {
    }

    bool place(
        WorldGenerationContext& context,
        JavaRandom& random,
        const Box& clip) override
    {
        if(!offsetToAverageGroundLevel(context, clip, -1))
            return false;

        const int baseX = box.minX;
        const int baseY = box.minY;
        const int baseZ = box.minZ;
        const Rotation rotation = randomRotation(random);
        const StructureTemplate& top = templates_.get("igloo/igloo_top");
        const StructureTemplate& middle = templates_.get("igloo/igloo_middle");
        const StructureTemplate& bottom = templates_.get("igloo/igloo_bottom");

        // Igloo calls Template#addBlocksToWorldChunk. Its template itself is
        // not restricted to the feature bounding box; WorldGenerationContext
        // still clips writes to the independently replayed target chunk.
        top.place(context, baseX, baseY, baseZ, rotation, clip);

        if(random.nextDouble() < 0.5)
        {
            const int sections = random.nextInt(8) + 4;
            const auto connected = [&](const StructureTemplate& first,
                                       std::array<int,3> firstPos,
                                       const StructureTemplate& second,
                                       std::array<int,3> secondPos)
            {
                const auto a = first.transformedBlockPosition(
                    firstPos[0], firstPos[1], firstPos[2], rotation);
                const auto b = second.transformedBlockPosition(
                    secondPos[0], secondPos[1], secondPos[2], rotation);
                return std::array<int,3>{
                    a[0] - b[0], a[1] - b[1], a[2] - b[2]};
            };

            for(int section = 0; section < sections; ++section)
            {
                const auto delta = connected(
                    top,
                    {3, -1 - section * 3, 5},
                    middle,
                    {1, 2, 1});
                middle.place(
                    context,
                    baseX + delta[0],
                    baseY + delta[1],
                    baseZ + delta[2],
                    rotation,
                    clip);
            }

            const auto bottomDelta = connected(
                top,
                {3, -1 - sections * 3, 5},
                bottom,
                {3, 5, 7});
            const int bottomX = baseX + bottomDelta[0];
            const int bottomY = baseY + bottomDelta[1];
            const int bottomZ = baseZ + bottomDelta[2];
            bottom.place(
                context,
                bottomX,
                bottomY,
                bottomZ,
                rotation,
                clip,
                1.0F,
                nullptr,
                true,
                [&context, &random](
                    int x, int y, int z, const TemplateNbt& nbt, Rotation)
                {
                    if(nbt.string("metadata") != "chest")
                        return;
                    context.setBlockState(x, y, z, state("air"));
                    const auto seed=random.nextLong();
                    context.assignStructureLoot(x,y,z,"minecraft:chests/igloo_chest",seed);
                });
        }
        else
        {
            const auto local = top.transformedBlockPosition(3, 0, 5, rotation);
            context.setBlockState(
                baseX + local[0],
                baseY + local[1],
                baseZ + local[2],
                state("snow"));
        }
        return true;
    }

private:
    StructureTemplateLibrary templates_;
};
}

ScatteredFeatureStructure::Start ScatteredFeatureStructure::create(
    int chunkX,
    int chunkZ,
    BiomeId biome,
    JavaRandom& random)
{
    Start result;
    const int x = chunkX * 16;
    const int z = chunkZ * 16;

    if(biome == VanillaBiomes::Swampland)
    {
        result.kind = Kind::SwampHut;
        result.piece = std::make_unique<SwampHut>(random, x, z);
    }
    else if(biome == VanillaBiomes::IcePlains ||
            biome == VanillaBiomes::ColdTaiga)
    {
        result.kind = Kind::Igloo;
        result.piece = std::make_unique<Igloo>(random, x, z);
    }
    else if(biome == VanillaBiomes::Desert ||
            biome == VanillaBiomes::DesertHills)
    {
        result.kind = Kind::DesertPyramid;
        result.piece = std::make_unique<DesertPyramid>(random, x, z);
    }
    else if(biome == VanillaBiomes::Jungle ||
            biome == VanillaBiomes::JungleHills)
    {
        result.kind = Kind::JunglePyramid;
        result.piece = std::make_unique<JunglePyramid>(random, x, z);
    }

    result.sizeable = result.piece != nullptr;
    if(result.piece)
        result.bounds = result.piece->box;
    return result;
}

void ScatteredFeatureStructure::place(
    Start& start,
    WorldGenerationContext& context,
    JavaRandom& populationRandom,
    const Box& clip)
{
    if(start.piece && start.piece->box.intersects(clip))
    {
        if(!start.piece->place(context, populationRandom, clip))
            start.sizeable = false;
        start.bounds = start.piece->box;
    }
}
}
