#include "worldgen/VillageStructure.h"

#include "content/ContentCatalog.h"
#include "worldgen/Vanilla112State.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace mc112
{
namespace
{
enum class Kind : std::uint8_t
{
    Well,Path,House4Garden,Church,House1,WoodHut,Hall,Field1,Field2,House2,House3,Torch
};

Facing randomFacing(JavaRandom& r)
{
    switch(r.nextInt(4))
    {
        case 0:return Facing::South; // EnumFacing horizontal index 0
        case 1:return Facing::West;
        case 2:return Facing::North;
        default:return Facing::East;
    }
}

std::string_view facingName(Facing f)
{
    switch(f){case Facing::North:return "north";case Facing::South:return "south";
        case Facing::West:return "west";case Facing::East:return "east";}
    return "north";
}

mc::content::BlockState vstate(
    std::string_view registry,std::initializer_list<Property> props={})
{
    std::vector<Property> p(props);
    return vanilla112State(registry,p);
}

mc::content::BlockState simple(std::string_view name){return state(name);}
mc::content::BlockState air(){return simple("air");}
mc::content::BlockState glassPane(){return vstate("minecraft:glass_pane");}
mc::content::BlockState oakFence(){return vstate("minecraft:oak_fence");}
mc::content::BlockState spruceFence(){return vstate("minecraft:spruce_fence");}
mc::content::BlockState acaciaFence(){return vstate("minecraft:acacia_fence");}
mc::content::BlockState cobble(){return vstate("minecraft:cobblestone");}
mc::content::BlockState sandstone(std::string_view type="sandstone")
{return vstate("minecraft:sandstone",{{"type",std::string(type)}});}
mc::content::BlockState planks(std::string_view variant="oak")
{return vstate("minecraft:planks",{{"variant",std::string(variant)}});}
mc::content::BlockState log(std::string_view variant="oak",std::string_view axis="y")
{
    if(variant=="acacia"||variant=="dark_oak")
        return vstate("minecraft:log2",{{"variant",std::string(variant)},{"axis",std::string(axis)}});
    return vstate("minecraft:log",{{"variant",std::string(variant)},{"axis",std::string(axis)}});
}
mc::content::BlockState stairs(std::string_view registry,Facing f)
{return vstate(registry,{{"facing",std::string(facingName(f))},{"half","bottom"},{"shape","straight"}});}
mc::content::BlockState ladder(Facing f)
{return vstate("minecraft:ladder",{{"facing",std::string(facingName(f))}});}
mc::content::BlockState torch(Facing f)
{return vstate("minecraft:torch",{{"facing",std::string(facingName(f))}});}
mc::content::BlockState crop(std::string_view name,int age)
{return vstate(name,{{"age",std::to_string(age)}});}

class VillagePiece : public Piece
{
public:
    Kind kind;
    int structureType=0;
    bool zombie=false;
    int averageGround=-1;
    bool boolA=false;
    int dataA=0,dataB=0,dataC=0,dataD=0;
    bool chestPlaced=false;

    VillagePiece(Kind k,int comp,Box b,std::optional<Facing> f,int st,bool z)
        :kind(k),structureType(st),zombie(z){componentType=comp;box=b;facing=f;}

    mc::content::BlockState biomeCobble()const
    {
        if(structureType==1)return sandstone("sandstone");
        if(structureType==2)return log("acacia","y");
        return cobble();
    }
    mc::content::BlockState biomePlanks()const
    {
        if(structureType==1)return sandstone("smooth_sandstone");
        if(structureType==2)return planks("acacia");
        if(structureType==3)return planks("spruce");
        return planks("oak");
    }
    mc::content::BlockState biomeLog(std::string_view axis="y")const
    {
        if(structureType==1)return sandstone();
        if(structureType==2)return log("acacia",axis);
        if(structureType==3)return log("spruce",axis);
        return log("oak",axis);
    }
    mc::content::BlockState biomeFence()const
    {
        if(structureType==2)return acaciaFence();
        if(structureType==3)return spruceFence();
        return oakFence();
    }
    mc::content::BlockState biomeWoodStairs(Facing f)const
    {
        if(structureType==1)return stairs("minecraft:sandstone_stairs",f);
        if(structureType==2)return stairs("minecraft:acacia_stairs",f);
        if(structureType==3)return stairs("minecraft:spruce_stairs",f);
        return stairs("minecraft:oak_stairs",f);
    }
    mc::content::BlockState biomeStoneStairs(Facing f)const
    {
        if(structureType==1)return stairs("minecraft:sandstone_stairs",f);
        return stairs("minecraft:stone_stairs",f);
    }
    mc::content::BlockState biomeGravel()const
    {
        return structureType==1?sandstone():vstate("minecraft:gravel");
    }
    mc::content::BlockState biomePath()const
    {
        return structureType==1?sandstone():vstate("minecraft:grass_path");
    }

    bool ground(WorldGenerationContext& c,const Box& clip,int pieceHeight)
    {
        if(averageGround>=0)return true;
        long long sum=0;int count=0;
        for(int z=box.minZ;z<=box.maxZ;++z)for(int x=box.minX;x<=box.maxX;++x)
        {
            if(!clip.contains(x,64,z))continue;
            sum+=std::max(c.getTopSolidOrLiquidBlockY(x,z),64);++count;
        }
        if(count==0)return false;
        averageGround=static_cast<int>(sum/count);
        box.offset(0,averageGround-box.maxY+pieceHeight-1,0);
        return true;
    }

    void clearUp(WorldGenerationContext& c,const Box& clip,int x,int y,int z)const
    {
        int wx=worldX(x,z),wy=worldY(y),wz=worldZ(x,z);
        if(!clip.contains(wx,wy,wz))return;
        while(wy<255&&!isAir(c.getBlockState(wx,wy,wz)))
            c.setBlockState(wx,wy++,wz,air());
    }

    void placeTorchLocal(WorldGenerationContext& c,const Box&clip,Facing f,int x,int y,int z)const
    {if(!zombie)setBlock(c,clip,torch(f),x,y,z);}

    void placeDoorLocal(WorldGenerationContext& c,const Box&clip,int x,int y,int z,Facing localFacing)const
    {
        if(zombie)return;
        // StructureVillagePieces passes NORTH to ItemDoor.placeDoor regardless
        // of the nominal argument. Piece::setBlock applies the component mirror/rotation.
        const std::string door=structureType==2?"minecraft:acacia_door":
            structureType==3?"minecraft:spruce_door":"minecraft:oak_door";
        const int wx=worldX(x,z),wy=worldY(y),wz=worldZ(x,z);
        Facing worldFacing=localFacing;
        if(facing)
        {
            // Transform a temporary facing-only stair to reuse exact component transform.
            auto temp=transformStateForFacing(stairs("minecraft:oak_stairs",localFacing),*facing);
            const auto* cat=mc::content::ContentCatalog::active();
            if(cat)
                for(auto& [n,val]:cat->serializeStateProperties(temp))if(n=="facing")
                {if(val=="north")worldFacing=Facing::North;else if(val=="south")worldFacing=Facing::South;
                 else if(val=="west")worldFacing=Facing::West;else if(val=="east")worldFacing=Facing::East;}
        }
        const auto [rx,rz]=step(rotateY(worldFacing));
        const auto [lx,lz]=step(rotateYCCW(worldFacing));
        auto normal=[&](int xx,int yy,int zz){return isSolid(c.getBlockState(xx,yy,zz));};
        const int right=(normal(wx+rx,wy,wz+rz)?1:0)+(normal(wx+rx,wy+1,wz+rz)?1:0);
        const int left=(normal(wx+lx,wy,wz+lz)?1:0)+(normal(wx+lx,wy+1,wz+lz)?1:0);
        auto sameDoor=[&](int xx,int yy,int zz){const auto p=path(c.getBlockState(xx,yy,zz));return p==door.substr(10);};
        const bool leftDoor=sameDoor(wx+lx,wy,wz+lz)||sameDoor(wx+lx,wy+1,wz+lz);
        const bool rightDoor=sameDoor(wx+rx,wy,wz+rz)||sameDoor(wx+rx,wy+1,wz+rz);
        bool rightHinge=false;
        if((!leftDoor||rightDoor)&&right<=left){if(rightDoor&&!leftDoor||right<left)rightHinge=false;}
        else rightHinge=true;
        // No redstone power is present during normal village structure placement.
        const auto lower=vstate(door,{{"facing",std::string(facingName(localFacing))},{"half","lower"},
            {"hinge",rightHinge?"right":"left"},{"open","false"},{"powered","false"}});
        const auto upper=vstate(door,{{"facing",std::string(facingName(localFacing))},{"half","upper"},
            {"hinge",rightHinge?"right":"left"},{"open","false"},{"powered","false"}});
        setBlock(c,clip,lower,x,y,z);setBlock(c,clip,upper,x,y+1,z);
    }

    static int cropCode(int roll){return roll;}
    static std::pair<std::string_view,int> cropInfo(int code)
    {
        if(code<=1)return {"minecraft:carrots",7};
        if(code<=3)return {"minecraft:potatoes",7};
        if(code==4)return {"minecraft:beetroots",3};
        return {"minecraft:wheat",7};
    }
};

struct Weight{Kind kind;int weight,limit,placed=0;};

class StartPiece;

std::unique_ptr<VillagePiece> makeCandidate(
    Kind,int component,int x,int y,int z,Facing,int structureType,bool zombie,
    JavaRandom& random,const std::vector<std::unique_ptr<Piece>>& pieces);

class BasicPiece final : public VillagePiece
{
public:
    using VillagePiece::VillagePiece;
    bool place(WorldGenerationContext& c,JavaRandom& r,const Box& clip)override;
};

class PathPiece final : public VillagePiece
{
public:
    StartPiece* start=nullptr;int length=0;
    PathPiece(int comp,Box b,Facing f,int st,bool z,StartPiece* s)
        :VillagePiece(Kind::Path,comp,b,f,st,z),start(s){length=std::max(b.xSize(),b.zSize());}
    void build(std::vector<std::unique_ptr<Piece>>& pieces,JavaRandom& random)override;
    bool place(WorldGenerationContext& c,JavaRandom&,const Box& clip)override;
};

class StartPiece final : public VillagePiece
{
public:
    int terrainType=0;
    std::vector<Weight> weights;
    Weight* previous=nullptr;
    std::vector<Piece*> pendingRoads,pendingHouses;
    std::vector<std::unique_ptr<Piece>>* owner=nullptr;

    StartPiece(int x,int z,int st,bool zomb,int terrain,Facing f)
        :VillagePiece(Kind::Well,0,Box{x,64,z,x+5,78,z+5},f,st,zomb),terrainType(terrain){}

    void setup(JavaRandom&r)
    {
        const auto inclusive=[&](int a,int b){return a+r.nextInt(b-a+1);};
        weights={{Kind::House4Garden,4,inclusive(2+terrainType,4+terrainType*2)},
                 {Kind::Church,20,inclusive(terrainType,1+terrainType)},
                 {Kind::House1,20,inclusive(terrainType,2+terrainType)},
                 {Kind::WoodHut,3,inclusive(2+terrainType,5+terrainType*3)},
                 {Kind::Hall,15,inclusive(terrainType,2+terrainType)},
                 {Kind::Field1,3,inclusive(1+terrainType,4+terrainType)},
                 {Kind::Field2,3,inclusive(2+terrainType,4+terrainType*2)},
                 {Kind::House2,15,inclusive(0,1+terrainType)},
                 {Kind::House3,8,inclusive(terrainType,3+terrainType*2)}};
        weights.erase(std::remove_if(weights.begin(),weights.end(),[](auto&w){return w.limit==0;}),weights.end());
    }

    int totalWeight()const
    {bool any=false;int sum=0;for(auto&w:weights){if(w.limit>0&&w.placed<w.limit)any=true;sum+=w.weight;}return any?sum:-1;}

    VillagePiece* addHouse(std::vector<std::unique_ptr<Piece>>& pieces,JavaRandom&r,int x,int y,int z,Facing f,int component)
    {
        if(component>50||std::abs(x-box.minX)>112||std::abs(z-box.minZ)>112)return nullptr;
        const int total=totalWeight();if(total<=0)return nullptr;
        for(int attempt=0;attempt<5;++attempt)
        {
            int roll=r.nextInt(total);
            for(std::size_t i=0;i<weights.size();++i)
            {
                auto&w=weights[i];roll-=w.weight;if(roll>=0)continue;
                if(((w.limit>0&&w.placed>=w.limit)||&w==previous)&&weights.size()>1)break;
                auto p=makeCandidate(w.kind,component+1,x,y,z,f,structureType,zombie,r,pieces);
                if(p)
                {
                    ++w.placed;previous=&w;VillagePiece* raw=p.get();pieces.push_back(std::move(p));pendingHouses.push_back(raw);
                    if(w.limit>0&&w.placed>=w.limit)
                    {
                        if(previous==&w)previous=nullptr;
                        weights.erase(weights.begin()+static_cast<std::ptrdiff_t>(i));
                    }
                    return raw;
                }
                break;
            }
        }
        auto p=makeCandidate(Kind::Torch,component,x,y,z,f,structureType,zombie,r,pieces);
        if(!p)return nullptr;auto*raw=p.get();pieces.push_back(std::move(p));pendingHouses.push_back(raw);return raw;
    }

    PathPiece* addRoad(std::vector<std::unique_ptr<Piece>>&pieces,JavaRandom&r,int x,int y,int z,Facing f,int component)
    {
        if(component>3+terrainType||std::abs(x-box.minX)>112||std::abs(z-box.minZ)>112)return nullptr;
        for(int len=7*(3+r.nextInt(3));len>=7;len-=7)
        {
            Box b=Box::component(x,y,z,0,0,0,3,3,len,f);
            if(findIntersecting(pieces,b)||b.minY<=10)continue;
            auto p=std::make_unique<PathPiece>(component,b,f,structureType,zombie,this);auto*raw=p.get();pieces.push_back(std::move(p));pendingRoads.push_back(raw);return raw;
        }
        return nullptr;
    }

    void build(std::vector<std::unique_ptr<Piece>>& pieces,JavaRandom&r)override
    {
        owner=&pieces;
        addRoad(pieces,r,box.minX-1,box.maxY-4,box.minZ+1,Facing::West,componentType);
        addRoad(pieces,r,box.maxX+1,box.maxY-4,box.minZ+1,Facing::East,componentType);
        addRoad(pieces,r,box.minX+1,box.maxY-4,box.minZ-1,Facing::North,componentType);
        addRoad(pieces,r,box.minX+1,box.maxY-4,box.maxZ+1,Facing::South,componentType);
    }
    bool place(WorldGenerationContext&c,JavaRandom&,const Box&clip)override
    {
        if(!ground(c,clip,4))return true;
        const auto stone=biomeCobble(),f=biomeFence(),water=vstate("minecraft:flowing_water",{{"level","0"}});
        fill(c,clip,1,0,1,4,12,4,stone,water);
        setBlock(c,clip,air(),2,12,2);setBlock(c,clip,air(),3,12,2);setBlock(c,clip,air(),2,12,3);setBlock(c,clip,air(),3,12,3);
        for(int x:{1,4})for(int z:{1,4})for(int y=13;y<=14;++y)setBlock(c,clip,f,x,y,z);
        fill(c,clip,1,15,1,4,15,4,stone,stone);
        for(int x=0;x<=5;++x)for(int z=0;z<=5;++z)if(x==0||x==5||z==0||z==5){setBlock(c,clip,stone,x,11,z);clearUp(c,clip,x,12,z);}
        return true;
    }
};

// Piece-specific generation is intentionally expressed with the same local
// coordinates as StructureVillagePieces. Piece::setBlock supplies the vanilla
// mirror/rotation step at the final write.
bool BasicPiece::place(WorldGenerationContext&c,JavaRandom&r,const Box&clip)
{
    auto support=[&](int width,int depth,int clearY,mc::content::BlockState s){for(int z=0;z<depth;++z)for(int x=0;x<width;++x){clearUp(c,clip,x,clearY,z);replaceAirAndLiquidDownwards(c,clip,s,x,-1,z);}};
    const auto A=air(),stone=biomeCobble(),wood=biomePlanks(),woodLog=biomeLog(),fence=biomeFence(),pane=glassPane();
    switch(kind)
    {
    case Kind::Church:
    {
        if(!ground(c,clip,12))return true;
        const auto sn=biomeStoneStairs(Facing::North),sw=biomeStoneStairs(Facing::West),se=biomeStoneStairs(Facing::East);
        fillAir(c,clip,1,1,1,3,3,7);fillAir(c,clip,1,5,1,3,9,3);
        fill(c,clip,1,0,0,3,0,8,stone,stone);fill(c,clip,1,1,0,3,10,0,stone,stone);
        fill(c,clip,0,1,1,0,10,3,stone,stone);fill(c,clip,4,1,1,4,10,3,stone,stone);
        fill(c,clip,0,0,4,0,4,7,stone,stone);fill(c,clip,4,0,4,4,4,7,stone,stone);
        fill(c,clip,1,1,8,3,4,8,stone,stone);fill(c,clip,1,5,4,3,10,4,stone,stone);fill(c,clip,1,5,5,3,5,7,stone,stone);
        fill(c,clip,0,9,0,4,9,4,stone,stone);fill(c,clip,0,4,0,4,4,4,stone,stone);
        for(auto p:std::array<std::array<int,3>,9>{{{{0,11,2}},{{4,11,2}},{{2,11,0}},{{2,11,4}},{{1,1,6}},{{1,1,7}},{{2,1,7}},{{3,1,6}},{{3,1,7}}}})setBlock(c,clip,stone,p[0],p[1],p[2]);
        setBlock(c,clip,sn,1,1,5);setBlock(c,clip,sn,2,1,6);setBlock(c,clip,sn,3,1,5);setBlock(c,clip,sw,1,2,7);setBlock(c,clip,se,3,2,7);
        for(auto p:std::array<std::array<int,3>,15>{{{{0,2,2}},{{0,3,2}},{{4,2,2}},{{4,3,2}},{{0,6,2}},{{0,7,2}},{{4,6,2}},{{4,7,2}},{{2,6,0}},{{2,7,0}},{{2,6,4}},{{2,7,4}},{{0,3,6}},{{4,3,6}},{{2,3,8}}}})setBlock(c,clip,pane,p[0],p[1],p[2]);
        placeTorchLocal(c,clip,Facing::South,2,4,7);placeTorchLocal(c,clip,Facing::East,1,4,6);placeTorchLocal(c,clip,Facing::West,3,4,6);placeTorchLocal(c,clip,Facing::North,2,4,5);
        auto ld=ladder(Facing::West);for(int y=1;y<=9;++y)setBlock(c,clip,ld,3,y,3);
        setBlock(c,clip,A,2,1,0);setBlock(c,clip,A,2,2,0);placeDoorLocal(c,clip,2,1,0,Facing::North);
        if(isAir(getBlock(c,clip,2,0,-1))&&!isAir(getBlock(c,clip,2,-1,-1))){setBlock(c,clip,sn,2,0,-1);if(named(getBlock(c,clip,2,-1,-1),"grass_path"))setBlock(c,clip,vstate("minecraft:grass"),2,-1,-1);}
        support(5,9,12,stone);return true;
    }
    case Kind::Field1:
    case Kind::Field2:
    {
        const bool wide=kind==Kind::Field1;if(!ground(c,clip,4))return true;const int width=wide?13:7;
        fillAir(c,clip,0,1,0,width-1,4,8);
        const auto farmland=vstate("minecraft:farmland",{{"moisture","0"}}),water=vstate("minecraft:water",{{"level","0"}});
        if(wide){fill(c,clip,1,0,1,2,0,7,farmland,farmland);fill(c,clip,4,0,1,5,0,7,farmland,farmland);fill(c,clip,7,0,1,8,0,7,farmland,farmland);fill(c,clip,10,0,1,11,0,7,farmland,farmland);
            fill(c,clip,0,0,0,0,0,8,woodLog,woodLog);fill(c,clip,6,0,0,6,0,8,woodLog,woodLog);fill(c,clip,12,0,0,12,0,8,woodLog,woodLog);fill(c,clip,1,0,0,11,0,0,woodLog,woodLog);fill(c,clip,1,0,8,11,0,8,woodLog,woodLog);fill(c,clip,3,0,1,3,0,7,water,water);fill(c,clip,9,0,1,9,0,7,water,water);
        }else{fill(c,clip,1,0,1,2,0,7,farmland,farmland);fill(c,clip,4,0,1,5,0,7,farmland,farmland);fill(c,clip,0,0,0,0,0,8,woodLog,woodLog);fill(c,clip,6,0,0,6,0,8,woodLog,woodLog);fill(c,clip,1,0,0,5,0,0,woodLog,woodLog);fill(c,clip,1,0,8,5,0,8,woodLog,woodLog);fill(c,clip,3,0,1,3,0,7,water,water);}
        const int codes[4]={dataA,dataB,dataC,dataD};const int xs[4]={1,4,7,10};const int pairs=wide?4:2;
        for(int z=1;z<=7;++z)for(int p=0;p<pairs;++p){auto [name,maxAge]=cropInfo(codes[p]);const int minAge=maxAge/3;for(int dx=0;dx<2;++dx)setBlock(c,clip,crop(name,minAge+r.nextInt(maxAge-minAge+1)),xs[p]+dx,1,z);}
        support(width,9,4,vstate("minecraft:dirt",{{"variant","dirt"}}));return true;
    }
    case Kind::Hall:
    {
        if(!ground(c,clip,7))return true;const auto n=biomeWoodStairs(Facing::North),s=biomeWoodStairs(Facing::South),w=biomeWoodStairs(Facing::West);
        fillAir(c,clip,1,1,1,7,4,4);fillAir(c,clip,2,1,6,8,4,10);fill(c,clip,2,0,6,8,0,10,vstate("minecraft:dirt",{{"variant","dirt"}}),vstate("minecraft:dirt",{{"variant","dirt"}}));setBlock(c,clip,stone,6,0,6);
        fill(c,clip,2,1,6,2,1,10,fence,fence);fill(c,clip,8,1,6,8,1,10,fence,fence);fill(c,clip,3,1,10,7,1,10,fence,fence);
        fill(c,clip,1,0,1,7,0,4,wood,wood);fill(c,clip,0,0,0,0,3,5,stone,stone);fill(c,clip,8,0,0,8,3,5,stone,stone);fill(c,clip,1,0,0,7,1,0,stone,stone);fill(c,clip,1,0,5,7,1,5,stone,stone);fill(c,clip,1,2,0,7,3,0,wood,wood);fill(c,clip,1,2,5,7,3,5,wood,wood);fill(c,clip,0,4,1,8,4,1,wood,wood);fill(c,clip,0,4,4,8,4,4,wood,wood);fill(c,clip,0,5,2,8,5,3,wood,wood);
        setBlock(c,clip,wood,0,4,2);setBlock(c,clip,wood,0,4,3);setBlock(c,clip,wood,8,4,2);setBlock(c,clip,wood,8,4,3);
        for(int i=-1;i<=2;++i)for(int x=0;x<=8;++x){setBlock(c,clip,n,x,4+i,i);setBlock(c,clip,s,x,4+i,5-i);}
        for(auto p:std::array<std::array<int,3>,4>{{{{0,2,1}},{{0,2,4}},{{8,2,1}},{{8,2,4}}}})setBlock(c,clip,woodLog,p[0],p[1],p[2]);
        for(auto p:std::array<std::array<int,3>,8>{{{{0,2,2}},{{0,2,3}},{{8,2,2}},{{8,2,3}},{{2,2,5}},{{3,2,5}},{{5,2,0}},{{6,2,5}}}})setBlock(c,clip,pane,p[0],p[1],p[2]);
        setBlock(c,clip,fence,2,1,3);setBlock(c,clip,vstate("minecraft:wooden_pressure_plate",{{"powered","false"}}),2,2,3);setBlock(c,clip,wood,1,1,4);setBlock(c,clip,n,2,1,4);setBlock(c,clip,w,1,1,3);
        auto dbl=vstate("minecraft:double_stone_slab",{{"variant","stone"},{"seamless","false"}});fill(c,clip,5,0,1,7,0,3,dbl,dbl);setBlock(c,clip,dbl,6,1,1);setBlock(c,clip,dbl,6,1,2);
        setBlock(c,clip,A,2,1,0);setBlock(c,clip,A,2,2,0);placeTorchLocal(c,clip,Facing::North,2,3,1);placeDoorLocal(c,clip,2,1,0,Facing::North);
        if(isAir(getBlock(c,clip,2,0,-1))&&!isAir(getBlock(c,clip,2,-1,-1))){setBlock(c,clip,n,2,0,-1);if(named(getBlock(c,clip,2,-1,-1),"grass_path"))setBlock(c,clip,vstate("minecraft:grass"),2,-1,-1);}
        setBlock(c,clip,A,6,1,5);setBlock(c,clip,A,6,2,5);placeTorchLocal(c,clip,Facing::South,6,3,4);placeDoorLocal(c,clip,6,1,5,Facing::South);support(9,5,7,stone);return true;
    }
    case Kind::House1:
    {
        if(!ground(c,clip,9))return true;const auto n=biomeWoodStairs(Facing::North),s=biomeWoodStairs(Facing::South),e=biomeWoodStairs(Facing::East),sn=biomeStoneStairs(Facing::North);
        fillAir(c,clip,1,1,1,7,5,4);fill(c,clip,0,0,0,8,0,5,stone,stone);fill(c,clip,0,5,0,8,5,5,stone,stone);fill(c,clip,0,6,1,8,6,4,stone,stone);fill(c,clip,0,7,2,8,7,3,stone,stone);for(int i=-1;i<=2;++i)for(int x=0;x<=8;++x){setBlock(c,clip,n,x,6+i,i);setBlock(c,clip,s,x,6+i,5-i);}fill(c,clip,0,1,0,0,1,5,stone,stone);fill(c,clip,1,1,5,8,1,5,stone,stone);fill(c,clip,8,1,0,8,1,4,stone,stone);fill(c,clip,2,1,0,7,1,0,stone,stone);fill(c,clip,0,2,0,0,4,0,stone,stone);fill(c,clip,0,2,5,0,4,5,stone,stone);fill(c,clip,8,2,5,8,4,5,stone,stone);fill(c,clip,8,2,0,8,4,0,stone,stone);fill(c,clip,0,2,1,0,4,4,wood,wood);fill(c,clip,1,2,5,7,4,5,wood,wood);fill(c,clip,8,2,1,8,4,4,wood,wood);fill(c,clip,1,2,0,7,4,0,wood,wood);
        for(auto p:std::array<std::array<int,3>,16>{{{{4,2,0}},{{5,2,0}},{{6,2,0}},{{4,3,0}},{{5,3,0}},{{6,3,0}},{{0,2,2}},{{0,2,3}},{{0,3,2}},{{0,3,3}},{{8,2,2}},{{8,2,3}},{{8,3,2}},{{8,3,3}},{{2,2,5}},{{3,2,5}}}})setBlock(c,clip,pane,p[0],p[1],p[2]);setBlock(c,clip,pane,5,2,5);setBlock(c,clip,pane,6,2,5);
        fill(c,clip,1,4,1,7,4,1,wood,wood);fill(c,clip,1,4,4,7,4,4,wood,wood);fill(c,clip,1,3,4,7,3,4,vstate("minecraft:bookshelf"),vstate("minecraft:bookshelf"));setBlock(c,clip,wood,7,1,4);setBlock(c,clip,e,7,1,3);for(int x=3;x<=6;++x)setBlock(c,clip,n,x,1,4);setBlock(c,clip,fence,6,1,3);setBlock(c,clip,vstate("minecraft:wooden_pressure_plate",{{"powered","false"}}),6,2,3);setBlock(c,clip,fence,4,1,3);setBlock(c,clip,vstate("minecraft:wooden_pressure_plate",{{"powered","false"}}),4,2,3);setBlock(c,clip,vstate("minecraft:crafting_table"),7,1,1);setBlock(c,clip,A,1,1,0);setBlock(c,clip,A,1,2,0);placeDoorLocal(c,clip,1,1,0,Facing::North);
        if(isAir(getBlock(c,clip,1,0,-1))&&!isAir(getBlock(c,clip,1,-1,-1))){setBlock(c,clip,sn,1,0,-1);if(named(getBlock(c,clip,1,-1,-1),"grass_path"))setBlock(c,clip,vstate("minecraft:grass"),1,-1,-1);}support(9,6,9,stone);return true;
    }
    case Kind::House2:
    {
        if(!ground(c,clip,6))return true;const auto n=biomeWoodStairs(Facing::North),w=biomeWoodStairs(Facing::West),sn=biomeStoneStairs(Facing::North);
        fillAir(c,clip,0,1,0,9,4,6);fill(c,clip,0,0,0,9,0,6,stone,stone);fill(c,clip,0,4,0,9,4,6,stone,stone);auto slab=vstate("minecraft:stone_slab",{{"variant","stone"},{"half","bottom"}});fill(c,clip,0,5,0,9,5,6,slab,slab);fillAir(c,clip,1,5,1,8,5,5);fill(c,clip,1,1,0,2,3,0,wood,wood);fill(c,clip,0,1,0,0,4,0,woodLog,woodLog);fill(c,clip,3,1,0,3,4,0,woodLog,woodLog);fill(c,clip,0,1,6,0,4,6,woodLog,woodLog);setBlock(c,clip,wood,3,3,1);fill(c,clip,3,1,2,3,3,2,wood,wood);fill(c,clip,4,1,3,5,3,3,wood,wood);fill(c,clip,0,1,1,0,3,5,wood,wood);fill(c,clip,1,1,6,5,3,6,wood,wood);fill(c,clip,5,1,0,5,3,0,fence,fence);fill(c,clip,9,1,0,9,3,0,fence,fence);fill(c,clip,6,1,4,9,4,6,stone,stone);setBlock(c,clip,vstate("minecraft:flowing_lava",{{"level","0"}}),7,1,5);setBlock(c,clip,vstate("minecraft:flowing_lava",{{"level","0"}}),8,1,5);setBlock(c,clip,vstate("minecraft:iron_bars"),9,2,5);setBlock(c,clip,vstate("minecraft:iron_bars"),9,2,4);fillAir(c,clip,7,2,4,8,2,5);setBlock(c,clip,stone,6,1,3);setBlock(c,clip,vstate("minecraft:furnace",{{"facing","north"}}),6,2,3);setBlock(c,clip,vstate("minecraft:furnace",{{"facing","north"}}),6,3,3);auto dbl=vstate("minecraft:double_stone_slab",{{"variant","stone"},{"seamless","false"}});setBlock(c,clip,dbl,8,1,1);for(auto p:std::array<std::array<int,3>,4>{{{{0,2,2}},{{0,2,4}},{{2,2,6}},{{4,2,6}}}})setBlock(c,clip,pane,p[0],p[1],p[2]);setBlock(c,clip,fence,2,1,4);setBlock(c,clip,vstate("minecraft:wooden_pressure_plate",{{"powered","false"}}),2,2,4);setBlock(c,clip,wood,1,1,5);setBlock(c,clip,n,2,1,5);setBlock(c,clip,w,1,1,4);
        if(!chestPlaced){const int wx=worldX(5,5),wy=worldY(1),wz=worldZ(5,5);if(clip.contains(wx,wy,wz)){chestPlaced=true;setBlock(c,clip,vstate("minecraft:chest",{{"facing","north"}}),5,1,5);const auto seed=r.nextLong();c.assignStructureLoot(wx,wy,wz,"minecraft:chests/village_blacksmith",seed);}}
        for(int x=6;x<=8;++x)if(isAir(getBlock(c,clip,x,0,-1))&&!isAir(getBlock(c,clip,x,-1,-1))){setBlock(c,clip,sn,x,0,-1);if(named(getBlock(c,clip,x,-1,-1),"grass_path"))setBlock(c,clip,vstate("minecraft:grass"),x,-1,-1);}support(10,7,6,stone);return true;
    }
    case Kind::House4Garden:
    {
        if(!ground(c,clip,6))return true;const auto sn=biomeStoneStairs(Facing::North);fill(c,clip,0,0,0,4,0,4,stone,stone);fill(c,clip,0,4,0,4,4,4,woodLog,woodLog);fill(c,clip,1,4,1,3,4,3,wood,wood);for(auto p:std::array<std::array<int,3>,12>{{{{0,1,0}},{{0,2,0}},{{0,3,0}},{{4,1,0}},{{4,2,0}},{{4,3,0}},{{0,1,4}},{{0,2,4}},{{0,3,4}},{{4,1,4}},{{4,2,4}},{{4,3,4}}}})setBlock(c,clip,stone,p[0],p[1],p[2]);fill(c,clip,0,1,1,0,3,3,wood,wood);fill(c,clip,4,1,1,4,3,3,wood,wood);fill(c,clip,1,1,4,3,3,4,wood,wood);setBlock(c,clip,pane,0,2,2);setBlock(c,clip,pane,2,2,4);setBlock(c,clip,pane,4,2,2);setBlock(c,clip,wood,1,1,0);setBlock(c,clip,wood,1,2,0);setBlock(c,clip,wood,1,3,0);setBlock(c,clip,wood,2,3,0);setBlock(c,clip,wood,3,3,0);setBlock(c,clip,wood,3,2,0);setBlock(c,clip,wood,3,1,0);if(isAir(getBlock(c,clip,2,0,-1))&&!isAir(getBlock(c,clip,2,-1,-1))){setBlock(c,clip,sn,2,0,-1);if(named(getBlock(c,clip,2,-1,-1),"grass_path"))setBlock(c,clip,vstate("minecraft:grass"),2,-1,-1);}fillAir(c,clip,1,1,1,3,3,3);
        if(boolA){for(int x=0;x<=4;++x){setBlock(c,clip,fence,x,5,0);setBlock(c,clip,fence,x,5,4);}for(int z=1;z<=3;++z){setBlock(c,clip,fence,0,5,z);setBlock(c,clip,fence,4,5,z);}auto ld=ladder(Facing::South);for(int y=1;y<=4;++y)setBlock(c,clip,ld,3,y,3);}placeTorchLocal(c,clip,Facing::North,2,3,1);support(5,5,6,stone);return true;
    }
    case Kind::Torch:
    {
        if(!ground(c,clip,4))return true;fillAir(c,clip,0,0,0,2,3,1);setBlock(c,clip,fence,1,0,0);setBlock(c,clip,fence,1,1,0);setBlock(c,clip,fence,1,2,0);setBlock(c,clip,vstate("minecraft:wool",{{"color","white"}}),1,3,0);placeTorchLocal(c,clip,Facing::East,2,3,0);placeTorchLocal(c,clip,Facing::North,1,3,1);placeTorchLocal(c,clip,Facing::West,0,3,0);placeTorchLocal(c,clip,Facing::South,1,3,-1);return true;
    }
    case Kind::WoodHut:
    {
        if(!ground(c,clip,6))return true;const auto sn=biomeStoneStairs(Facing::North);
        fillAir(c,clip,1,1,1,3,5,4);fill(c,clip,0,0,0,3,0,4,stone,stone);
        auto dirt=vstate("minecraft:dirt",{{"variant","dirt"}});fill(c,clip,1,0,1,2,0,3,dirt,dirt);
        if(boolA)fill(c,clip,1,4,1,2,4,3,woodLog,woodLog);else fill(c,clip,1,5,1,2,5,3,woodLog,woodLog);
        for(auto p:std::array<std::array<int,3>,10>{{{{1,4,0}},{{2,4,0}},{{1,4,4}},{{2,4,4}},{{0,4,1}},{{0,4,2}},{{0,4,3}},{{3,4,1}},{{3,4,2}},{{3,4,3}}}})setBlock(c,clip,woodLog,p[0],p[1],p[2]);
        fill(c,clip,0,1,0,0,3,0,woodLog,woodLog);fill(c,clip,3,1,0,3,3,0,woodLog,woodLog);fill(c,clip,0,1,4,0,3,4,woodLog,woodLog);fill(c,clip,3,1,4,3,3,4,woodLog,woodLog);
        fill(c,clip,0,1,1,0,3,3,wood,wood);fill(c,clip,3,1,1,3,3,3,wood,wood);fill(c,clip,1,1,0,2,3,0,wood,wood);fill(c,clip,1,1,4,2,3,4,wood,wood);
        setBlock(c,clip,pane,0,2,2);setBlock(c,clip,pane,3,2,2);
        if(dataA>0){setBlock(c,clip,fence,dataA,1,3);setBlock(c,clip,vstate("minecraft:wooden_pressure_plate",{{"powered","false"}}),dataA,2,3);}
        setBlock(c,clip,A,1,1,0);setBlock(c,clip,A,1,2,0);placeDoorLocal(c,clip,1,1,0,Facing::North);
        if(isAir(getBlock(c,clip,1,0,-1))&&!isAir(getBlock(c,clip,1,-1,-1))){setBlock(c,clip,sn,1,0,-1);if(named(getBlock(c,clip,1,-1,-1),"grass_path"))setBlock(c,clip,vstate("minecraft:grass"),1,-1,-1);}support(4,5,6,stone);return true;
    }
    case Kind::House3:
    {
        if(!ground(c,clip,7))return true;
        const auto n=biomeWoodStairs(Facing::North),s=biomeWoodStairs(Facing::South),e=biomeWoodStairs(Facing::East),w=biomeWoodStairs(Facing::West);
        fillAir(c,clip,1,1,1,7,4,4);fillAir(c,clip,2,1,6,8,4,10);fill(c,clip,2,0,5,8,0,10,wood,wood);fill(c,clip,1,0,1,7,0,4,wood,wood);
        fill(c,clip,0,0,0,0,3,5,stone,stone);fill(c,clip,8,0,0,8,3,10,stone,stone);fill(c,clip,1,0,0,7,2,0,stone,stone);fill(c,clip,1,0,5,2,1,5,stone,stone);fill(c,clip,2,0,6,2,3,10,stone,stone);fill(c,clip,3,0,10,7,3,10,stone,stone);
        fill(c,clip,1,2,0,7,3,0,wood,wood);fill(c,clip,1,2,5,2,3,5,wood,wood);fill(c,clip,0,4,1,8,4,1,wood,wood);fill(c,clip,0,4,4,3,4,4,wood,wood);fill(c,clip,0,5,2,8,5,3,wood,wood);
        setBlock(c,clip,wood,0,4,2);setBlock(c,clip,wood,0,4,3);setBlock(c,clip,wood,8,4,2);setBlock(c,clip,wood,8,4,3);setBlock(c,clip,wood,8,4,4);
        for(int i=-1;i<=2;++i)for(int x=0;x<=8;++x){setBlock(c,clip,n,x,4+i,i);if((i>-1||x<=1)&&(i>0||x<=3)&&(i>1||x<=4||x>=6))setBlock(c,clip,s,x,4+i,5-i);}
        fill(c,clip,3,4,5,3,4,10,wood,wood);fill(c,clip,7,4,2,7,4,10,wood,wood);fill(c,clip,4,5,4,4,5,10,wood,wood);fill(c,clip,6,5,4,6,5,10,wood,wood);fill(c,clip,5,6,3,5,6,10,wood,wood);
        for(int k=4;k>=1;--k){setBlock(c,clip,wood,k,2+k,7-k);for(int z=8-k;z<=10;++z)setBlock(c,clip,e,k,2+k,z);}setBlock(c,clip,wood,6,6,3);setBlock(c,clip,wood,7,5,4);setBlock(c,clip,w,6,6,4);for(int x=6;x<=8;++x)for(int z=5;z<=10;++z)setBlock(c,clip,w,x,12-x,z);
        for(auto p:std::array<std::array<int,3>,9>{{{{0,2,1}},{{0,2,4}},{{4,2,0}},{{6,2,0}},{{8,2,1}},{{8,2,4}},{{8,2,6}},{{8,2,9}},{{2,2,6}}}})setBlock(c,clip,woodLog,p[0],p[1],p[2]);
        for(auto p:std::array<std::array<int,3>,9>{{{{0,2,2}},{{0,2,3}},{{5,2,0}},{{8,2,2}},{{8,2,3}},{{8,2,7}},{{8,2,8}},{{2,2,7}},{{2,2,8}}}})setBlock(c,clip,pane,p[0],p[1],p[2]);
        setBlock(c,clip,woodLog,2,2,9);setBlock(c,clip,woodLog,4,4,10);setBlock(c,clip,pane,5,4,10);setBlock(c,clip,woodLog,6,4,10);setBlock(c,clip,wood,5,5,10);
        setBlock(c,clip,A,2,1,0);setBlock(c,clip,A,2,2,0);placeTorchLocal(c,clip,Facing::North,2,3,1);placeDoorLocal(c,clip,2,1,0,Facing::North);fillAir(c,clip,1,0,-1,3,2,-1);
        if(isAir(getBlock(c,clip,2,0,-1))&&!isAir(getBlock(c,clip,2,-1,-1))){setBlock(c,clip,n,2,0,-1);if(named(getBlock(c,clip,2,-1,-1),"grass_path"))setBlock(c,clip,vstate("minecraft:grass"),2,-1,-1);}
        for(int z=0;z<5;++z)for(int x=0;x<9;++x){clearUp(c,clip,x,7,z);replaceAirAndLiquidDownwards(c,clip,stone,x,-1,z);}for(int z=5;z<11;++z)for(int x=2;x<9;++x){clearUp(c,clip,x,7,z);replaceAirAndLiquidDownwards(c,clip,stone,x,-1,z);}return true;
    }
    default:
        break;
    }
    return false;
}

std::unique_ptr<VillagePiece> makeCandidate(
    Kind kind,int component,int x,int y,int z,Facing f,int st,bool zombie,
    JavaRandom&r,const std::vector<std::unique_ptr<Piece>>&pieces)
{
    int xs=0,ys=0,zs=0;
    switch(kind){case Kind::House4Garden:xs=5;ys=6;zs=5;break;case Kind::Church:xs=5;ys=12;zs=9;break;case Kind::House1:xs=9;ys=9;zs=6;break;case Kind::WoodHut:xs=4;ys=6;zs=5;break;case Kind::Hall:xs=9;ys=7;zs=11;break;case Kind::Field1:xs=13;ys=4;zs=9;break;case Kind::Field2:xs=7;ys=4;zs=9;break;case Kind::House2:xs=10;ys=6;zs=7;break;case Kind::House3:xs=9;ys=7;zs=12;break;case Kind::Torch:xs=3;ys=4;zs=2;break;default:return nullptr;}
    Box b=Box::component(x,y,z,0,0,0,xs,ys,zs,f);if((kind!=Kind::House4Garden&&b.minY<=10)||findIntersecting(pieces,b))return nullptr;
    auto p=std::make_unique<BasicPiece>(kind,component,b,f,st,zombie);
    if(kind==Kind::WoodHut){p->boolA=r.nextBoolean();p->dataA=r.nextInt(3);}else if(kind==Kind::House4Garden)p->boolA=r.nextBoolean();else if(kind==Kind::Field2){p->dataA=r.nextInt(10);p->dataB=r.nextInt(10);}else if(kind==Kind::Field1){p->dataA=r.nextInt(10);p->dataB=r.nextInt(10);p->dataC=r.nextInt(10);p->dataD=r.nextInt(10);}return p;
}

void PathPiece::build(std::vector<std::unique_ptr<Piece>>&pieces,JavaRandom&r)
{
    if(!start)return;bool flag=false;
    for(int i=r.nextInt(5);i<length-8;i+=2+r.nextInt(5))
    {
        VillagePiece*p=nullptr;switch(*facing){case Facing::North:case Facing::South:p=start->addHouse(pieces,r,box.minX-1,box.minY,box.minZ+i,Facing::West,componentType);break;case Facing::West:case Facing::East:p=start->addHouse(pieces,r,box.minX+i,box.minY,box.minZ-1,Facing::North,componentType);break;}
        if(p){i+=std::max(p->box.xSize(),p->box.zSize());flag=true;}
    }
    for(int j=r.nextInt(5);j<length-8;j+=2+r.nextInt(5))
    {
        VillagePiece*p=nullptr;switch(*facing){case Facing::North:case Facing::South:p=start->addHouse(pieces,r,box.maxX+1,box.minY,box.minZ+j,Facing::East,componentType);break;case Facing::West:case Facing::East:p=start->addHouse(pieces,r,box.minX+j,box.minY,box.maxZ+1,Facing::South,componentType);break;}
        if(p){j+=std::max(p->box.xSize(),p->box.zSize());flag=true;}
    }
    if(flag&&r.nextInt(3)>0)switch(*facing){case Facing::North:start->addRoad(pieces,r,box.minX-1,box.minY,box.minZ,Facing::West,componentType);break;case Facing::South:start->addRoad(pieces,r,box.minX-1,box.minY,box.maxZ-2,Facing::West,componentType);break;case Facing::West:start->addRoad(pieces,r,box.minX,box.minY,box.minZ-1,Facing::North,componentType);break;case Facing::East:start->addRoad(pieces,r,box.maxX-2,box.minY,box.minZ-1,Facing::North,componentType);break;}
    if(flag&&r.nextInt(3)>0)switch(*facing){case Facing::North:start->addRoad(pieces,r,box.maxX+1,box.minY,box.minZ,Facing::East,componentType);break;case Facing::South:start->addRoad(pieces,r,box.maxX+1,box.minY,box.maxZ-2,Facing::East,componentType);break;case Facing::West:start->addRoad(pieces,r,box.minX,box.minY,box.maxZ+1,Facing::South,componentType);break;case Facing::East:start->addRoad(pieces,r,box.maxX-2,box.minY,box.maxZ+1,Facing::South,componentType);break;}
}

bool PathPiece::place(WorldGenerationContext&c,JavaRandom&,const Box&clip)
{
    const auto pathState=biomePath(),wood=biomePlanks(),gravel=biomeGravel(),stone=biomeCobble();
    for(int x=box.minX;x<=box.maxX;++x)for(int z=box.minZ;z<=box.maxZ;++z)
    {
        if(!clip.contains(x,64,z))continue;int y=c.getTopSolidOrLiquidBlockY(x,z)-1;if(y<63)y=62;
        while(y>=62)
        {
            auto cur=c.getBlockState(x,y,z),above=c.getBlockState(x,y+1,z);
            if(named(cur,"grass")&&isAir(above)){c.setBlockState(x,y,z,pathState);break;}
            if(isLiquid(cur)){c.setBlockState(x,y,z,wood);break;}
            if(named(cur,"sand")||named(cur,"sandstone")||named(cur,"red_sandstone")){c.setBlockState(x,y,z,gravel);c.setBlockState(x,y-1,z,stone);break;}
            --y;
        }
    }
    return true;
}

int structureTypeFor(BiomeId biome)
{
    if(biome==VanillaBiomes::Desert)return 1;
    if(static_cast<unsigned>(biome)==35U)return 2;
    if(static_cast<unsigned>(biome)==5U)return 3;
    return 0;
}
}

VillageStructure::Start VillageStructure::create(
    int chunkX,int chunkZ,BiomeId biome,JavaRandom&r,int terrainType)
{
    Start out;out.structureType=structureTypeFor(biome);
    // Weight list is constructed before Start/Well orientation and zombie roll.
    // Construct a temporary start after consuming the nine limits.
    const int x=(chunkX<<4)+2,z=(chunkZ<<4)+2;
    // Start constructor consumes Well horizontal-facing then zombie roll, but its
    // PieceWeight list was created immediately before it in MapGenVillage.Start.
    // We therefore create the object, populate weights using a saved sequence by
    // explicitly consuming weight limits first.
    std::vector<Weight> temp;
    const auto inc=[&](int a,int b){return a+r.nextInt(b-a+1);};
    temp={{Kind::House4Garden,4,inc(2+terrainType,4+terrainType*2)},
          {Kind::Church,20,inc(terrainType,1+terrainType)},
          {Kind::House1,20,inc(terrainType,2+terrainType)},
          {Kind::WoodHut,3,inc(2+terrainType,5+terrainType*3)},
          {Kind::Hall,15,inc(terrainType,2+terrainType)},
          {Kind::Field1,3,inc(1+terrainType,4+terrainType)},
          {Kind::Field2,3,inc(2+terrainType,4+terrainType*2)},
          {Kind::House2,15,inc(0,1+terrainType)},
          {Kind::House3,8,inc(terrainType,3+terrainType*2)}};
    temp.erase(std::remove_if(temp.begin(),temp.end(),[](auto&w){return w.limit==0;}),temp.end());
    Facing f=randomFacing(r);const bool zombie=r.nextInt(50)==0;out.zombie=zombie;
    auto root=std::make_unique<StartPiece>(x,z,out.structureType,zombie,terrainType,f);
    root->weights=std::move(temp);
    out.pieces.push_back(std::move(root));
    auto* start=static_cast<StartPiece*>(out.pieces.front().get());start->owner=&out.pieces;start->build(out.pieces,r);
    while(!start->pendingRoads.empty()||!start->pendingHouses.empty())
    {
        Piece* p=nullptr;
        if(start->pendingRoads.empty()){const int i=r.nextInt(static_cast<int>(start->pendingHouses.size()));p=start->pendingHouses[i];start->pendingHouses.erase(start->pendingHouses.begin()+i);}
        else{const int i=r.nextInt(static_cast<int>(start->pendingRoads.size()));p=start->pendingRoads[i];start->pendingRoads.erase(start->pendingRoads.begin()+i);}
        if(p)p->build(out.pieces,r);
    }
    out.bounds=boundsOf(out.pieces);int nonRoad=0;for(auto&p:out.pieces)if(dynamic_cast<PathPiece*>(p.get())==nullptr)++nonRoad;out.sizeable=nonRoad>2;return out;
}

void VillageStructure::place(Start&start,WorldGenerationContext&c,JavaRandom&r,const Box&clip)
{for(auto&p:start.pieces)if(p&&p->box.intersects(clip))p->place(c,r,clip);start.bounds=boundsOf(start.pieces);}
}
