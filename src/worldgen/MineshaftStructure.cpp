#include "worldgen/MineshaftStructure.h"

#include "worldgen/Vanilla112State.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

namespace mc112
{
namespace
{
class ShaftPiece : public Piece
{
public:
    MineshaftStructure::Type type = MineshaftStructure::Type::Normal;
    ShaftPiece(int component, MineshaftStructure::Type t) { componentType=component; type=t; }
    [[nodiscard]] mc::content::BlockState planks() const
    {
        return state(type==MineshaftStructure::Type::Mesa ? "dark_oak_planks" : "oak_planks");
    }
    [[nodiscard]] mc::content::BlockState fence() const
    {
        return state(type==MineshaftStructure::Type::Mesa ? "dark_oak_fence" : "oak_fence");
    }
    [[nodiscard]] bool supportingBox(
        const WorldGenerationContext& c,const Box& clip,int x0,int x1,int y,int z) const
    {
        for(int x=x0;x<=x1;++x) if(isAir(getBlock(c,clip,x,y+1,z))) return false;
        return true;
    }
};

std::unique_ptr<ShaftPiece> createRandom(
    std::vector<std::unique_ptr<Piece>>& pieces,JavaRandom& random,
    int x,int y,int z,Facing facing,int component,MineshaftStructure::Type type);

ShaftPiece* generateAndAdd(
    Piece& root,std::vector<std::unique_ptr<Piece>>& pieces,JavaRandom& random,
    int x,int y,int z,Facing facing,int depth,MineshaftStructure::Type type)
{
    if(depth>8) return nullptr;
    if(std::abs(x-root.box.minX)>80||std::abs(z-root.box.minZ)>80) return nullptr;
    auto next=createRandom(pieces,random,x,y,z,facing,depth+1,type);
    if(!next)return nullptr;
    ShaftPiece* raw=next.get();
    pieces.push_back(std::move(next));
    raw->build(pieces,random);
    return raw;
}

class Corridor final : public ShaftPiece
{
public:
    bool hasRails=false,hasSpiders=false,spawnerPlaced=false;
    int sections=0;
    Piece* root=nullptr;

    Corridor(int component,JavaRandom& r,Box b,Facing f,MineshaftStructure::Type t,Piece* rootPiece)
        :ShaftPiece(component,t),root(rootPiece)
    {
        facing=f;box=b;hasRails=r.nextInt(3)==0;hasSpiders=!hasRails&&r.nextInt(23)==0;
        sections=(f==Facing::North||f==Facing::South)?b.zSize()/5:b.xSize()/5;
    }

    static std::optional<Box> find(const std::vector<std::unique_ptr<Piece>>&pieces,JavaRandom&r,int x,int y,int z,Facing f)
    {
        Box b{x,y,z,x,y+2,z};int count;
        for(count=r.nextInt(3)+2;count>0;--count)
        {
            const int length=count*5;
            switch(f){
                case Facing::North:b.maxX=x+2;b.minZ=z-(length-1);break;
                case Facing::South:b.maxX=x+2;b.maxZ=z+(length-1);break;
                case Facing::West:b.minX=x-(length-1);b.maxZ=z+2;break;
                case Facing::East:b.maxX=x+(length-1);b.maxZ=z+2;break;}
            if(findIntersecting(pieces,b)==nullptr)break;
        }
        if(count<=0)return std::nullopt;return b;
    }

    void build(std::vector<std::unique_ptr<Piece>>&pieces,JavaRandom&r) override
    {
        if(root==nullptr)root=this;const int depth=componentType;const int choice=r.nextInt(4);const Facing f=*facing;
        switch(f)
        {
            case Facing::North:
                if(choice<=1)generateAndAdd(*root,pieces,r,box.minX,box.minY-1+r.nextInt(3),box.minZ-1,f,depth,type);
                else if(choice==2)generateAndAdd(*root,pieces,r,box.minX-1,box.minY-1+r.nextInt(3),box.minZ,Facing::West,depth,type);
                else generateAndAdd(*root,pieces,r,box.maxX+1,box.minY-1+r.nextInt(3),box.minZ,Facing::East,depth,type);break;
            case Facing::South:
                if(choice<=1)generateAndAdd(*root,pieces,r,box.minX,box.minY-1+r.nextInt(3),box.maxZ+1,f,depth,type);
                else if(choice==2)generateAndAdd(*root,pieces,r,box.minX-1,box.minY-1+r.nextInt(3),box.maxZ-3,Facing::West,depth,type);
                else generateAndAdd(*root,pieces,r,box.maxX+1,box.minY-1+r.nextInt(3),box.maxZ-3,Facing::East,depth,type);break;
            case Facing::West:
                if(choice<=1)generateAndAdd(*root,pieces,r,box.minX-1,box.minY-1+r.nextInt(3),box.minZ,f,depth,type);
                else if(choice==2)generateAndAdd(*root,pieces,r,box.minX,box.minY-1+r.nextInt(3),box.minZ-1,Facing::North,depth,type);
                else generateAndAdd(*root,pieces,r,box.minX,box.minY-1+r.nextInt(3),box.maxZ+1,Facing::South,depth,type);break;
            case Facing::East:
                if(choice<=1)generateAndAdd(*root,pieces,r,box.maxX+1,box.minY-1+r.nextInt(3),box.minZ,f,depth,type);
                else if(choice==2)generateAndAdd(*root,pieces,r,box.maxX-3,box.minY-1+r.nextInt(3),box.minZ-1,Facing::North,depth,type);
                else generateAndAdd(*root,pieces,r,box.maxX-3,box.minY-1+r.nextInt(3),box.maxZ+1,Facing::South,depth,type);break;
        }
        if(depth<8)
        {
            if(f==Facing::North||f==Facing::South)
            {
                for(int z=box.minZ+3;z+3<=box.maxZ;z+=5)
                {const int c=r.nextInt(5);if(c==0)generateAndAdd(*root,pieces,r,box.minX-1,box.minY,z,Facing::West,depth+1,type);else if(c==1)generateAndAdd(*root,pieces,r,box.maxX+1,box.minY,z,Facing::East,depth+1,type);}
            }
            else
            {
                for(int x=box.minX+3;x+3<=box.maxX;x+=5)
                {const int c=r.nextInt(5);if(c==0)generateAndAdd(*root,pieces,r,x,box.minY,box.minZ-1,Facing::North,depth+1,type);else if(c==1)generateAndAdd(*root,pieces,r,x,box.minY,box.maxZ+1,Facing::South,depth+1,type);}
            }
        }
    }

    void support(WorldGenerationContext&c,const Box&clip,int x0,int y0,int z,int x1,int y1,JavaRandom&r) const
    {
        if(!supportingBox(c,clip,x0,x1,y1,z))return;
        const auto air=state("air"),p=planks(),f=fence();
        fill(c,clip,x0,y0,z,x0,y1-1,z,f,air);fill(c,clip,x1,y0,z,x1,y1-1,z,f,air);
        if(r.nextInt(4)==0){fill(c,clip,x0,y1,z,x0,y1,z,p,air);fill(c,clip,x1,y1,z,x1,y1,z,p,air);}
        else{fill(c,clip,x0,y1,z,x1,y1,z,p,air);maybeBlock(c,clip,r,0.05f,x0+1,y1,z-1,state("torch",{{"facing","north"}}));maybeBlock(c,clip,r,0.05f,x0+1,y1,z+1,state("torch",{{"facing","south"}}));}
    }

    void web(WorldGenerationContext&c,const Box&clip,JavaRandom&r,float chance,int x,int y,int z) const
    {if(skyBrightness(c,clip,x,y,z)<8)maybeBlock(c,clip,r,chance,x,y,z,tryState("web").value_or(state("cobweb")));}

    bool place(WorldGenerationContext&c,JavaRandom&r,const Box&clip) override
    {
        if(liquidAround(c,clip))return false;const int last=sections*5-1;const auto air=state("air");
        fill(c,clip,0,0,0,2,1,last,air,air);maybeBox(c,clip,r,0.8f,0,2,0,2,2,last,air,air,false);
        if(hasSpiders)maybeBox(c,clip,r,0.6f,0,0,0,2,1,last,tryState("web").value_or(state("cobweb")),air,false);
        for(int section=0;section<sections;++section)
        {
            const int z=2+section*5;support(c,clip,0,0,z,2,2,r);
            web(c,clip,r,.10f,0,2,z-1);web(c,clip,r,.10f,2,2,z-1);web(c,clip,r,.10f,0,2,z+1);web(c,clip,r,.10f,2,2,z+1);
            web(c,clip,r,.05f,0,2,z-2);web(c,clip,r,.05f,2,2,z-2);web(c,clip,r,.05f,0,2,z+2);web(c,clip,r,.05f,2,2,z+2);
            // Exact RNG is consumed even when entity-backed chest minecarts are
            // not represented by the chunk block palette.
            if(r.nextInt(100)==0){const int wx=worldX(2,z-1),wy=worldY(0),wz=worldZ(2,z-1);if(clip.contains(wx,wy,wz)&&isAir(c.getBlockState(wx,wy,wz))&&!isAir(c.getBlockState(wx,wy-1,wz))){setBlock(c,clip,state("rail",{{"shape",r.nextBoolean()?"north_south":"east_west"}}),2,0,z-1);const auto seed=r.nextLong();c.assignStructureLoot(wx,wy,wz,"minecraft:chests/abandoned_mineshaft",seed);}}
            if(r.nextInt(100)==0){const int wx=worldX(0,z+1),wy=worldY(0),wz=worldZ(0,z+1);if(clip.contains(wx,wy,wz)&&isAir(c.getBlockState(wx,wy,wz))&&!isAir(c.getBlockState(wx,wy-1,wz))){setBlock(c,clip,state("rail",{{"shape",r.nextBoolean()?"north_south":"east_west"}}),0,0,z+1);const auto seed=r.nextLong();c.assignStructureLoot(wx,wy,wz,"minecraft:chests/abandoned_mineshaft",seed);}}
            if(hasSpiders&&!spawnerPlaced){const int rz=z-1+r.nextInt(3);const int wx=worldX(1,rz),wy=worldY(0),wz=worldZ(1,rz);if(clip.contains(wx,wy,wz)&&skyBrightness(c,clip,1,0,rz)<8){spawnerPlaced=true;c.setBlockState(wx,wy,wz,tryState("mob_spawner").value_or(state("spawner")));c.assignStructureSpawner(wx,wy,wz,"minecraft:cave_spider");}}
        }
        for(int x=0;x<=2;++x)for(int z=0;z<=last;++z)if(isAir(getBlock(c,clip,x,-1,z))&&skyBrightness(c,clip,x,-1,z)<8)setBlock(c,clip,planks(),x,-1,z);
        if(hasRails){const auto rail=state("rail",{{"shape","north_south"}});for(int z=0;z<=last;++z){const auto below=getBlock(c,clip,1,-1,z);if(!isAir(below)&&isSolid(below)){const float chance=skyBrightness(c,clip,1,0,z)>8?.9f:.7f;maybeBlock(c,clip,r,chance,1,0,z,rail);}}}
        return true;
    }
};

class Cross final : public ShaftPiece
{
public:
    Facing direction;bool multiple=false;Piece* root=nullptr;
    Cross(int comp,Box b,Facing f,MineshaftStructure::Type t,Piece* rp):ShaftPiece(comp,t),direction(f),root(rp){box=b;multiple=b.ySize()>3;}
    static std::optional<Box> find(const std::vector<std::unique_ptr<Piece>>&ps,JavaRandom&r,int x,int y,int z,Facing f)
    {Box b{x,y,z,x,y+2,z};if(r.nextInt(4)==0)b.maxY+=4;switch(f){case Facing::North:b.minX=x-1;b.maxX=x+3;b.minZ=z-4;break;case Facing::South:b.minX=x-1;b.maxX=x+3;b.maxZ=z+4;break;case Facing::West:b.minX=x-4;b.minZ=z-1;b.maxZ=z+3;break;case Facing::East:b.maxX=x+4;b.minZ=z-1;b.maxZ=z+3;break;}return findIntersecting(ps,b)?std::nullopt:std::optional<Box>(b);}
    void build(std::vector<std::unique_ptr<Piece>>&ps,JavaRandom&r)override
    {if(!root)root=this;const int d=componentType;switch(direction){case Facing::North:generateAndAdd(*root,ps,r,box.minX+1,box.minY,box.minZ-1,Facing::North,d,type);generateAndAdd(*root,ps,r,box.minX-1,box.minY,box.minZ+1,Facing::West,d,type);generateAndAdd(*root,ps,r,box.maxX+1,box.minY,box.minZ+1,Facing::East,d,type);break;case Facing::South:generateAndAdd(*root,ps,r,box.minX+1,box.minY,box.maxZ+1,Facing::South,d,type);generateAndAdd(*root,ps,r,box.minX-1,box.minY,box.minZ+1,Facing::West,d,type);generateAndAdd(*root,ps,r,box.maxX+1,box.minY,box.minZ+1,Facing::East,d,type);break;case Facing::West:generateAndAdd(*root,ps,r,box.minX+1,box.minY,box.minZ-1,Facing::North,d,type);generateAndAdd(*root,ps,r,box.minX+1,box.minY,box.maxZ+1,Facing::South,d,type);generateAndAdd(*root,ps,r,box.minX-1,box.minY,box.minZ+1,Facing::West,d,type);break;case Facing::East:generateAndAdd(*root,ps,r,box.minX+1,box.minY,box.minZ-1,Facing::North,d,type);generateAndAdd(*root,ps,r,box.minX+1,box.minY,box.maxZ+1,Facing::South,d,type);generateAndAdd(*root,ps,r,box.maxX+1,box.minY,box.minZ+1,Facing::East,d,type);break;}if(multiple){if(r.nextBoolean())generateAndAdd(*root,ps,r,box.minX+1,box.minY+4,box.minZ-1,Facing::North,d,type);if(r.nextBoolean())generateAndAdd(*root,ps,r,box.minX-1,box.minY+4,box.minZ+1,Facing::West,d,type);if(r.nextBoolean())generateAndAdd(*root,ps,r,box.maxX+1,box.minY+4,box.minZ+1,Facing::East,d,type);if(r.nextBoolean())generateAndAdd(*root,ps,r,box.minX+1,box.minY+4,box.maxZ+1,Facing::South,d,type);}}
    bool place(WorldGenerationContext&c,JavaRandom&,const Box&clip)override
    {if(liquidAround(c,clip))return false;const auto air=state("air"),p=planks();if(multiple){fill(c,clip,box.minX+1,box.minY,box.minZ,box.maxX-1,box.minY+2,box.maxZ,air,air);fill(c,clip,box.minX,box.minY,box.minZ+1,box.maxX,box.minY+2,box.maxZ-1,air,air);fill(c,clip,box.minX+1,box.maxY-2,box.minZ,box.maxX-1,box.maxY,box.maxZ,air,air);fill(c,clip,box.minX,box.maxY-2,box.minZ+1,box.maxX,box.maxY,box.maxZ-1,air,air);fill(c,clip,box.minX+1,box.minY+3,box.minZ+1,box.maxX-1,box.minY+3,box.maxZ-1,air,air);}else{fill(c,clip,box.minX+1,box.minY,box.minZ,box.maxX-1,box.maxY,box.maxZ,air,air);fill(c,clip,box.minX,box.minY,box.minZ+1,box.maxX,box.maxY,box.maxZ-1,air,air);}const int xs[2]={box.minX+1,box.maxX-1},zs[2]={box.minZ+1,box.maxZ-1};for(int x:xs)for(int z:zs)if(!isAir(getBlock(c,clip,x,box.maxY+1,z)))fill(c,clip,x,box.minY,z,x,box.maxY,z,p,air);for(int x=box.minX;x<=box.maxX;++x)for(int z=box.minZ;z<=box.maxZ;++z)if(isAir(getBlock(c,clip,x,box.minY-1,z))&&skyBrightness(c,clip,x,box.minY-1,z)<8)setBlock(c,clip,p,x,box.minY-1,z);return true;}
};

class Stairs final : public ShaftPiece
{
public:Piece* root=nullptr;
    Stairs(int c,Box b,Facing f,MineshaftStructure::Type t,Piece*rp):ShaftPiece(c,t),root(rp){box=b;facing=f;}
    static std::optional<Box> find(const std::vector<std::unique_ptr<Piece>>&ps,int x,int y,int z,Facing f){Box b{x,y-5,z,x,y+2,z};switch(f){case Facing::North:b.maxX=x+2;b.minZ=z-8;break;case Facing::South:b.maxX=x+2;b.maxZ=z+8;break;case Facing::West:b.minX=x-8;b.maxZ=z+2;break;case Facing::East:b.maxX=x+8;b.maxZ=z+2;break;}return findIntersecting(ps,b)?std::nullopt:std::optional<Box>(b);}
    void build(std::vector<std::unique_ptr<Piece>>&ps,JavaRandom&r)override{if(!root)root=this;const int d=componentType;switch(*facing){case Facing::North:generateAndAdd(*root,ps,r,box.minX,box.minY,box.minZ-1,Facing::North,d,type);break;case Facing::South:generateAndAdd(*root,ps,r,box.minX,box.minY,box.maxZ+1,Facing::South,d,type);break;case Facing::West:generateAndAdd(*root,ps,r,box.minX-1,box.minY,box.minZ,Facing::West,d,type);break;case Facing::East:generateAndAdd(*root,ps,r,box.maxX+1,box.minY,box.minZ,Facing::East,d,type);break;}}
    bool place(WorldGenerationContext&c,JavaRandom&,const Box&clip)override{if(liquidAround(c,clip))return false;const auto air=state("air");fill(c,clip,0,5,0,2,7,1,air,air);fill(c,clip,0,0,7,2,2,8,air,air);for(int i=0;i<5;++i)fill(c,clip,0,5-i-(i<4?1:0),2+i,2,7-i,2+i,air,air);return true;}
};

class Room final : public ShaftPiece
{
public:std::vector<Box> entrances;
    Room(int c,JavaRandom&r,int x,int z,MineshaftStructure::Type t):ShaftPiece(c,t){box={x,50,z,x+7+r.nextInt(6),54+r.nextInt(6),z+7+r.nextInt(6)};}
    void build(std::vector<std::unique_ptr<Piece>>&ps,JavaRandom&r)override
    {int span=box.ySize()-4;if(span<=0)span=1;const int sx=box.xSize(),sz=box.zSize();for(int k=0;k<sx;k+=4){k+=r.nextInt(sx);if(k+3>sx)break;if(auto*p=generateAndAdd(*this,ps,r,box.minX+k,box.minY+r.nextInt(span)+1,box.minZ-1,Facing::North,componentType,type))entrances.emplace_back(p->box.minX,p->box.minY,box.minZ,p->box.maxX,p->box.maxY,box.minZ+1);}for(int k=0;k<sx;k+=4){k+=r.nextInt(sx);if(k+3>sx)break;if(auto*p=generateAndAdd(*this,ps,r,box.minX+k,box.minY+r.nextInt(span)+1,box.maxZ+1,Facing::South,componentType,type))entrances.emplace_back(p->box.minX,p->box.minY,box.maxZ-1,p->box.maxX,p->box.maxY,box.maxZ);}for(int k=0;k<sz;k+=4){k+=r.nextInt(sz);if(k+3>sz)break;if(auto*p=generateAndAdd(*this,ps,r,box.minX-1,box.minY+r.nextInt(span)+1,box.minZ+k,Facing::West,componentType,type))entrances.emplace_back(box.minX,p->box.minY,p->box.minZ,box.minX+1,p->box.maxY,p->box.maxZ);}for(int k=0;k<sz;k+=4){k+=r.nextInt(sz);if(k+3>sz)break;if(auto*p=generateAndAdd(*this,ps,r,box.maxX+1,box.minY+r.nextInt(span)+1,box.minZ+k,Facing::East,componentType,type))entrances.emplace_back(box.maxX-1,p->box.minY,p->box.minZ,box.maxX,p->box.maxY,p->box.maxZ);}}
    bool place(WorldGenerationContext&c,JavaRandom&,const Box&clip)override{if(liquidAround(c,clip))return false;const auto air=state("air");fill(c,clip,box.minX,box.minY,box.minZ,box.maxX,box.minY,box.maxZ,state("dirt"),air,true);fill(c,clip,box.minX,box.minY+1,box.minZ,box.maxX,std::min(box.minY+3,box.maxY),box.maxZ,air,air);for(const Box&e:entrances)fill(c,clip,e.minX,e.maxY-2,e.minZ,e.maxX,e.maxY,e.maxZ,air,air);rareFill(c,clip,box.minX,box.minY+4,box.minZ,box.maxX,box.maxY,box.maxZ,air,false);return true;}
    void offset(int x,int y,int z)override{ShaftPiece::offset(x,y,z);for(auto&e:entrances)e.offset(x,y,z);}
};

std::unique_ptr<ShaftPiece> createRandom(std::vector<std::unique_ptr<Piece>>&pieces,JavaRandom&r,int x,int y,int z,Facing f,int component,MineshaftStructure::Type type)
{
    const int roll=r.nextInt(100);
    if(roll>=80){auto b=Cross::find(pieces,r,x,y,z,f);if(b)return std::make_unique<Cross>(component,*b,f,type,pieces.empty()?nullptr:pieces.front().get());}
    else if(roll>=70){auto b=Stairs::find(pieces,x,y,z,f);if(b)return std::make_unique<Stairs>(component,*b,f,type,pieces.empty()?nullptr:pieces.front().get());}
    else{auto b=Corridor::find(pieces,r,x,y,z,f);if(b)return std::make_unique<Corridor>(component,r,*b,f,type,pieces.empty()?nullptr:pieces.front().get());}
    return nullptr;
}
}

MineshaftStructure::Start MineshaftStructure::create(int chunkX,int chunkZ,Type type,JavaRandom&random,int seaLevel)
{
    Start out;auto room=std::make_unique<Room>(0,random,(chunkX<<4)+2,(chunkZ<<4)+2,type);Room* root=room.get();out.pieces.push_back(std::move(room));root->build(out.pieces,random);out.bounds=boundsOf(out.pieces);
    if(type==Type::Mesa){const int dy=seaLevel-out.bounds.maxY+out.bounds.ySize()/2+5;offsetAll(out.pieces,0,dy,0);}else markAvailableHeight(out.pieces,random,10,seaLevel);
    out.bounds=boundsOf(out.pieces);out.sizeable=!out.pieces.empty();return out;
}

void MineshaftStructure::place(Start&start,WorldGenerationContext&context,JavaRandom&random,const Box&clip)
{for(auto&piece:start.pieces)if(piece&&piece->box.intersects(clip))piece->place(context,random,clip);}
}
