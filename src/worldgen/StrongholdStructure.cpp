#include "worldgen/StrongholdStructure.h"

#include "content/ContentCatalog.h"
#include "worldgen/Vanilla112State.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace mc112
{
namespace
{
enum class Kind : std::uint8_t
{
    Straight,Prison,LeftTurn,RightTurn,RoomCrossing,StairsStraight,Stairs,
    Crossing,ChestCorridor,Library,PortalRoom,Corridor
};
enum class Door : std::uint8_t{Opening,Wood,Grates,Iron};

Facing randomFacing(JavaRandom&r){switch(r.nextInt(4)){case 0:return Facing::South;case 1:return Facing::West;case 2:return Facing::North;default:return Facing::East;}}
std::string_view fn(Facing f){switch(f){case Facing::North:return"north";case Facing::South:return"south";case Facing::West:return"west";case Facing::East:return"east";}return"north";}
mc::content::BlockState V(std::string_view n,std::initializer_list<Property> p={}){std::vector<Property>v(p);return vanilla112State(n,v);}
mc::content::BlockState stoneBrick(std::string_view variant="stonebrick"){return V("minecraft:stonebrick",{{"variant",std::string(variant)}});}
mc::content::BlockState slab(){return V("minecraft:stone_slab",{{"variant","stone"},{"half","bottom"}});}
mc::content::BlockState brickSlab(){return V("minecraft:stone_slab",{{"variant","stone_brick"},{"half","bottom"}});}
mc::content::BlockState doubleSlab(){return V("minecraft:double_stone_slab",{{"variant","stone"},{"seamless","false"}});}
mc::content::BlockState cobbleStair(Facing f){return V("minecraft:stone_stairs",{{"facing",std::string(fn(f))},{"half","bottom"},{"shape","straight"}});}
mc::content::BlockState stoneBrickStair(Facing f){return V("minecraft:stone_brick_stairs",{{"facing",std::string(fn(f))},{"half","bottom"},{"shape","straight"}});}
mc::content::BlockState torchS(Facing f){return V("minecraft:torch",{{"facing",std::string(fn(f))}});}
mc::content::BlockState airS(){return state("air");}

struct Weight{Kind kind;int weight,limit,minDepth,placed=0;};

class Root;
class StrongPiece : public Piece
{
public:
    Kind kind;Door entry=Door::Opening;Root* root=nullptr;
    bool a=false,b=false,c=false,d=false,large=false,chest=false,spawner=false,source=false;
    int roomType=0;
    StrongPiece(Kind k,int depth,Box bb,Facing f,Root* rt):kind(k),root(rt){componentType=depth;box=bb;facing=f;}
    void randomDoor(JavaRandom&r){switch(r.nextInt(5)){case 2:entry=Door::Wood;break;case 3:entry=Door::Grates;break;case 4:entry=Door::Iron;break;default:entry=Door::Opening;break;}}

    void randomized(WorldGenerationContext&ctx,const Box&clip,JavaRandom&r,int x0,int y0,int z0,int x1,int y1,int z1,bool existingOnly)const
    {
        for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)for(int z=z0;z<=z1;++z)
        {
            if(existingOnly&&isAir(getBlock(ctx,clip,x,y,z)))continue;
            const bool boundary=y==y0||y==y1||x==x0||x==x1||z==z0||z==z1;
            mc::content::BlockState s=airS();
            if(boundary){const float f=r.nextFloat();if(f<.2f)s=stoneBrick("cracked_stonebrick");else if(f<.5f)s=stoneBrick("mossy_stonebrick");else if(f<.55f)s=V("minecraft:monster_egg",{{"variant","stonebrick"}});else s=stoneBrick();}
            setBlock(ctx,clip,s,x,y,z);
        }
    }
    void door(WorldGenerationContext&ctx,const Box&clip,Door type,int x,int y,int z)const
    {
        auto sb=stoneBrick(),A=airS(),bars=V("minecraft:iron_bars");
        if(type==Door::Opening){fillAir(ctx,clip,x,y,z,x+2,y+2,z);return;}
        if(type==Door::Grates){setBlock(ctx,clip,A,x+1,y,z);setBlock(ctx,clip,A,x+1,y+1,z);for(auto p:std::array<std::array<int,3>,7>{{{{0,0,0}},{{0,1,0}},{{0,2,0}},{{1,2,0}},{{2,2,0}},{{2,1,0}},{{2,0,0}}}})setBlock(ctx,clip,bars,x+p[0],y+p[1],z);return;}
        for(auto p:std::array<std::array<int,3>,7>{{{{0,0,0}},{{0,1,0}},{{0,2,0}},{{1,2,0}},{{2,2,0}},{{2,1,0}},{{2,0,0}}}})setBlock(ctx,clip,sb,x+p[0],y+p[1],z);
        const char* name=type==Door::Iron?"minecraft:iron_door":"minecraft:oak_door";
        setBlock(ctx,clip,V(name,{{"facing","north"},{"half","lower"},{"hinge","left"},{"open","false"},{"powered","false"}}),x+1,y,z);
        setBlock(ctx,clip,V(name,{{"facing","north"},{"half","upper"},{"hinge","left"},{"open","false"},{"powered","false"}}),x+1,y+1,z);
        if(type==Door::Iron){setBlock(ctx,clip,V("minecraft:stone_button",{{"facing","north"},{"powered","false"}}),x+2,y+1,z+1);setBlock(ctx,clip,V("minecraft:stone_button",{{"facing","south"},{"powered","false"}}),x+2,y+1,z-1);}
    }
    void build(std::vector<std::unique_ptr<Piece>>&,JavaRandom&)override;
    bool place(WorldGenerationContext&,JavaRandom&,const Box&)override;
};

class Root final : public StrongPiece
{
public:
    std::vector<Weight> weights;
    std::optional<Kind> forced;
    std::optional<Kind> previous;
    std::vector<StrongPiece*> pending;
    StrongPiece* portal=nullptr;
    int startX=0,startZ=0;

    Root(int x,int z,Facing f):StrongPiece(Kind::Stairs,0,Box{x,64,z,x+4,74,z+4},f,this),startX(x),startZ(z){source=true;entry=Door::Opening;resetWeights();}
    void resetWeights(){weights={{Kind::Straight,40,0,0},{Kind::Prison,5,5,0},{Kind::LeftTurn,20,0,0},{Kind::RightTurn,20,0,0},{Kind::RoomCrossing,10,6,0},{Kind::StairsStraight,5,5,0},{Kind::Stairs,5,5,0},{Kind::Crossing,5,4,0},{Kind::ChestCorridor,5,4,0},{Kind::Library,10,2,5},{Kind::PortalRoom,20,1,6}};}
    bool canAny()const{for(auto&w:weights)if(w.limit>0&&w.placed<w.limit)return true;return false;}
    int total()const{int t=0;for(auto&w:weights)t+=w.weight;return t;}

    std::optional<Box> pieceBox(Kind k,int x,int y,int z,Facing f,const std::vector<std::unique_ptr<Piece>>&ps)const
    {
        int ox=-1,oy=-1,oz=0,xs=5,ys=5,zs=7;
        switch(k){case Kind::Straight:break;case Kind::Prison:xs=9;ys=5;zs=11;break;case Kind::LeftTurn:case Kind::RightTurn:zs=5;break;case Kind::RoomCrossing:ox=-4;xs=11;ys=7;zs=11;break;case Kind::StairsStraight:oy=-7;ys=11;zs=8;break;case Kind::Stairs:oy=-7;ys=11;zs=5;break;case Kind::Crossing:ox=-4;oy=-3;xs=10;ys=9;zs=11;break;case Kind::ChestCorridor:break;case Kind::Library:ox=-4;xs=14;ys=11;zs=15;break;case Kind::PortalRoom:ox=-4;xs=11;ys=8;zs=16;break;case Kind::Corridor:zs=4;break;}
        Box b=Box::component(x,y,z,ox,oy,oz,xs,ys,zs,f);
        if(b.minY<=10||findIntersecting(ps,b))
        {
            if(k==Kind::Library){b=Box::component(x,y,z,-4,-1,0,14,6,15,f);if(b.minY>10&&!findIntersecting(ps,b))return b;}
            return std::nullopt;
        }
        return b;
    }

    StrongPiece* create(Kind k,int x,int y,int z,Facing f,int depth,JavaRandom&r,std::vector<std::unique_ptr<Piece>>&ps)
    {
        auto bb=pieceBox(k,x,y,z,f,ps);if(!bb)return nullptr;auto p=std::make_unique<StrongPiece>(k,depth,*bb,f,this);auto*raw=p.get();
        if(k!=Kind::PortalRoom)p->randomDoor(r);
        switch(k){case Kind::Straight:p->a=r.nextInt(2)==0;p->b=r.nextInt(2)==0;break;case Kind::RoomCrossing:p->roomType=r.nextInt(5);break;case Kind::Crossing:p->a=r.nextBoolean();p->b=r.nextBoolean();p->c=r.nextBoolean();p->d=r.nextInt(3)>0;break;case Kind::Library:p->large=bb->ySize()>6;break;default:break;}
        ps.push_back(std::move(p));pending.push_back(raw);if(k==Kind::PortalRoom)portal=raw;return raw;
    }

    StrongPiece* generate(int x,int y,int z,Facing f,int depth,JavaRandom&r,std::vector<std::unique_ptr<Piece>>&ps)
    {
        if(depth>50||std::abs(x-startX)>112||std::abs(z-startZ)>112)return nullptr;
        const int nextDepth=depth+1;
        if(forced){auto k=*forced;forced.reset();if(auto*p=create(k,x,y,z,f,nextDepth,r,ps))return p;}
        if(canAny())
        {
            for(int attempt=0;attempt<5;++attempt)
            {
                int roll=r.nextInt(total());
                for(std::size_t i=0;i<weights.size();++i)
                {
                    auto&w=weights[i];roll-=w.weight;if(roll>=0)continue;
                    const bool depthOK=nextDepth>=w.minDepth;const bool countOK=w.limit==0||w.placed<w.limit;
                    if(!depthOK||!countOK||(previous&&*previous==w.kind))break;
                    if(auto*p=create(w.kind,x,y,z,f,nextDepth,r,ps))
                    {
                        ++w.placed;previous=w.kind;if(w.limit>0&&w.placed>=w.limit)weights.erase(weights.begin()+static_cast<std::ptrdiff_t>(i));return p;
                    }
                    break;
                }
            }
        }
        // Corridor.findPieceBox: start with four deep, intersect one existing
        // piece, then shrink until the shortened box no longer intersects it.
        Box full=Box::component(x,y,z,-1,-1,0,5,5,4,f);Piece* hit=findIntersecting(ps,full);if(!hit)return nullptr;
        if(hit->box.minY==full.minY)for(int len=3;len>=1;--len){Box shortB=Box::component(x,y,z,-1,-1,0,5,5,len-1,f);if(hit->box.intersects(shortB))continue;Box b=Box::component(x,y,z,-1,-1,0,5,5,len,f);if(b.minY<=1)return nullptr;auto p=std::make_unique<StrongPiece>(Kind::Corridor,nextDepth,b,f,this);auto*raw=p.get();ps.push_back(std::move(p));pending.push_back(raw);return raw;}
        return nullptr;
    }

    StrongPiece* forward(StrongPiece&p,std::vector<std::unique_ptr<Piece>>&ps,JavaRandom&r,int n,int n2)
    {switch(*p.facing){case Facing::North:return generate(p.box.minX+n,p.box.minY+n2,p.box.minZ-1,Facing::North,p.componentType,r,ps);case Facing::South:return generate(p.box.minX+n,p.box.minY+n2,p.box.maxZ+1,Facing::South,p.componentType,r,ps);case Facing::West:return generate(p.box.minX-1,p.box.minY+n2,p.box.minZ+n,Facing::West,p.componentType,r,ps);case Facing::East:return generate(p.box.maxX+1,p.box.minY+n2,p.box.minZ+n,Facing::East,p.componentType,r,ps);}return nullptr;}
    StrongPiece* left(StrongPiece&p,std::vector<std::unique_ptr<Piece>>&ps,JavaRandom&r,int n,int n2)
    {if(*p.facing==Facing::North||*p.facing==Facing::South)return generate(p.box.minX-1,p.box.minY+n,p.box.minZ+n2,Facing::West,p.componentType,r,ps);return generate(p.box.minX+n2,p.box.minY+n,p.box.minZ-1,Facing::North,p.componentType,r,ps);}
    StrongPiece* right(StrongPiece&p,std::vector<std::unique_ptr<Piece>>&ps,JavaRandom&r,int n,int n2)
    {if(*p.facing==Facing::North||*p.facing==Facing::South)return generate(p.box.maxX+1,p.box.minY+n,p.box.minZ+n2,Facing::East,p.componentType,r,ps);return generate(p.box.minX+n2,p.box.minY+n,p.box.maxZ+1,Facing::South,p.componentType,r,ps);}

    void expand(StrongPiece&p,std::vector<std::unique_ptr<Piece>>&ps,JavaRandom&r)
    {
        switch(p.kind){case Kind::Straight:forward(p,ps,r,1,1);if(p.a)left(p,ps,r,1,2);if(p.b)right(p,ps,r,1,2);break;case Kind::Prison:forward(p,ps,r,1,1);break;case Kind::LeftTurn:if(*p.facing==Facing::North||*p.facing==Facing::East)left(p,ps,r,1,1);else right(p,ps,r,1,1);break;case Kind::RightTurn:if(*p.facing==Facing::North||*p.facing==Facing::East)right(p,ps,r,1,1);else left(p,ps,r,1,1);break;case Kind::RoomCrossing:forward(p,ps,r,4,1);left(p,ps,r,1,4);right(p,ps,r,1,4);break;case Kind::StairsStraight:forward(p,ps,r,1,1);break;case Kind::Stairs:if(p.source)forced=Kind::Crossing;forward(p,ps,r,1,1);break;case Kind::Crossing:{const int n=(*p.facing==Facing::East||*p.facing==Facing::South)?3:5,n2=8-n;forward(p,ps,r,5,1);if(p.a)left(p,ps,r,n,1);if(p.b)left(p,ps,r,n2,7);if(p.c)right(p,ps,r,n,1);if(p.d)right(p,ps,r,n2,7);break;}case Kind::ChestCorridor:forward(p,ps,r,1,1);break;default:break;}
    }
};

void StrongPiece::build(std::vector<std::unique_ptr<Piece>>&ps,JavaRandom&r){if(root)root->expand(*this,ps,r);}

void randomCobwebs(const StrongPiece&p,WorldGenerationContext&ctx,const Box&clip,JavaRandom&r,float chance,int x0,int y0,int z0,int x1,int y1,int z1)
{for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)for(int z=z0;z<=z1;++z)if(r.nextFloat()<=chance)p.setBlock(ctx,clip,V("minecraft:web"),x,y,z);}

bool StrongPiece::place(WorldGenerationContext&ctx,JavaRandom&r,const Box&clip)
{
    if(kind!=Kind::PortalRoom&&liquidAround(ctx,clip))return false;auto A=airS(),sb=stoneBrick(),bars=V("minecraft:iron_bars"),pl=V("minecraft:planks",{{"variant","oak"}}),books=V("minecraft:bookshelf"),fence=V("minecraft:oak_fence");
    switch(kind)
    {
    case Kind::Straight:
        randomized(ctx,clip,r,0,0,0,4,4,6,true);door(ctx,clip,entry,1,1,0);door(ctx,clip,Door::Opening,1,1,6);maybeBlock(ctx,clip,r,.1f,1,2,1,torchS(Facing::East));maybeBlock(ctx,clip,r,.1f,3,2,1,torchS(Facing::West));maybeBlock(ctx,clip,r,.1f,1,2,5,torchS(Facing::East));maybeBlock(ctx,clip,r,.1f,3,2,5,torchS(Facing::West));if(a)fillAir(ctx,clip,0,1,2,0,3,4);if(b)fillAir(ctx,clip,4,1,2,4,3,4);return true;
    case Kind::Corridor:
    {const int steps=(*facing==Facing::North||*facing==Facing::South)?box.zSize():box.xSize();for(int z=0;z<steps;++z){fill(ctx,clip,0,0,z,4,0,z,sb,sb);for(int y=1;y<=3;++y){setBlock(ctx,clip,sb,0,y,z);setBlock(ctx,clip,A,1,y,z);setBlock(ctx,clip,A,2,y,z);setBlock(ctx,clip,A,3,y,z);setBlock(ctx,clip,sb,4,y,z);}fill(ctx,clip,0,4,z,4,4,z,sb,sb);}return true;}
    case Kind::ChestCorridor:
        randomized(ctx,clip,r,0,0,0,4,4,6,true);door(ctx,clip,entry,1,1,0);door(ctx,clip,Door::Opening,1,1,6);fill(ctx,clip,3,1,2,3,1,4,sb,sb);setBlock(ctx,clip,brickSlab(),3,1,1);setBlock(ctx,clip,brickSlab(),3,1,5);setBlock(ctx,clip,brickSlab(),3,2,2);setBlock(ctx,clip,brickSlab(),3,2,4);for(int z=2;z<=4;++z)setBlock(ctx,clip,brickSlab(),2,1,z);if(!chest){const int wx=worldX(3,3),wy=worldY(2),wz=worldZ(3,3);if(clip.contains(wx,wy,wz)){chest=true;setBlock(ctx,clip,V("minecraft:chest",{{"facing","north"}}),3,2,3);const auto seed=r.nextLong();ctx.assignStructureLoot(wx,wy,wz,"minecraft:chests/stronghold_corridor",seed);}}return true;
    case Kind::LeftTurn:
    case Kind::RightTurn:
        randomized(ctx,clip,r,0,0,0,4,4,4,true);door(ctx,clip,entry,1,1,0);{const bool leftPiece=kind==Kind::LeftTurn;const bool northEast=*facing==Facing::North||*facing==Facing::East;const bool openLeft=leftPiece?northEast:!northEast;if(openLeft)fillAir(ctx,clip,0,1,1,0,3,3);else fillAir(ctx,clip,4,1,1,4,3,3);}return true;
    case Kind::Stairs:
        randomized(ctx,clip,r,0,0,0,4,10,4,true);door(ctx,clip,entry,1,7,0);door(ctx,clip,Door::Opening,1,1,4);setBlock(ctx,clip,sb,2,6,1);setBlock(ctx,clip,sb,1,5,1);setBlock(ctx,clip,slab(),1,6,1);setBlock(ctx,clip,sb,1,5,2);setBlock(ctx,clip,sb,1,4,3);setBlock(ctx,clip,slab(),1,5,3);setBlock(ctx,clip,sb,2,4,3);setBlock(ctx,clip,sb,3,3,3);setBlock(ctx,clip,slab(),3,4,3);setBlock(ctx,clip,sb,3,3,2);setBlock(ctx,clip,sb,3,2,1);setBlock(ctx,clip,slab(),3,3,1);setBlock(ctx,clip,sb,2,2,1);setBlock(ctx,clip,sb,1,1,1);setBlock(ctx,clip,slab(),1,2,1);setBlock(ctx,clip,sb,1,1,2);setBlock(ctx,clip,slab(),1,1,3);return true;
    case Kind::StairsStraight:
        randomized(ctx,clip,r,0,0,0,4,10,7,true);door(ctx,clip,entry,1,7,0);door(ctx,clip,Door::Opening,1,1,7);for(int i=0;i<6;++i){for(int x=1;x<=3;++x)setBlock(ctx,clip,cobbleStair(Facing::South),x,6-i,1+i);if(i<5)for(int x=1;x<=3;++x)setBlock(ctx,clip,sb,x,5-i,1+i);}return true;
    case Kind::Prison:
    {
        randomized(ctx,clip,r,0,0,0,8,4,10,true);door(ctx,clip,entry,1,1,0);fillAir(ctx,clip,1,1,10,3,3,10);for(int z:{1,3,7,9})randomized(ctx,clip,r,4,1,z,4,3,z,false);fill(ctx,clip,4,1,4,4,3,6,bars,bars);fill(ctx,clip,5,1,5,7,3,5,bars,bars);setBlock(ctx,clip,bars,4,3,2);setBlock(ctx,clip,bars,4,3,8);auto iron=V("minecraft:iron_door",{{"facing","west"},{"half","lower"},{"hinge","left"},{"open","false"},{"powered","false"}}),ironTop=V("minecraft:iron_door",{{"facing","west"},{"half","upper"},{"hinge","left"},{"open","false"},{"powered","false"}});setBlock(ctx,clip,iron,4,1,2);setBlock(ctx,clip,ironTop,4,2,2);setBlock(ctx,clip,iron,4,1,8);setBlock(ctx,clip,ironTop,4,2,8);return true;
    }
    case Kind::Crossing:
        randomized(ctx,clip,r,0,0,0,9,8,10,true);door(ctx,clip,entry,4,3,0);if(a)fillAir(ctx,clip,0,3,1,0,5,3);if(c)fillAir(ctx,clip,9,3,1,9,5,3);if(b)fillAir(ctx,clip,0,5,7,0,7,9);if(d)fillAir(ctx,clip,9,5,7,9,7,9);fillAir(ctx,clip,5,1,10,7,3,10);randomized(ctx,clip,r,1,2,1,8,2,6,false);randomized(ctx,clip,r,4,1,5,4,4,9,false);randomized(ctx,clip,r,8,1,5,8,4,9,false);randomized(ctx,clip,r,1,4,7,3,4,9,false);randomized(ctx,clip,r,1,3,5,3,3,6,false);fill(ctx,clip,1,3,4,3,3,4,slab(),slab());fill(ctx,clip,1,4,6,3,4,6,slab(),slab());randomized(ctx,clip,r,5,1,7,7,1,8,false);fill(ctx,clip,5,1,9,7,1,9,slab(),slab());fill(ctx,clip,5,2,7,7,2,7,slab(),slab());fill(ctx,clip,4,5,7,4,5,9,slab(),slab());fill(ctx,clip,8,5,7,8,5,9,slab(),slab());fill(ctx,clip,5,5,7,7,5,9,doubleSlab(),doubleSlab());setBlock(ctx,clip,torchS(Facing::South),6,5,6);return true;
    case Kind::Library:
    {
        const int h=large?10:5;randomized(ctx,clip,r,0,0,0,13,h,14,true);door(ctx,clip,entry,4,1,0);randomCobwebs(*this,ctx,clip,r,.07f,2,1,1,11,4,13);
        for(int z=1;z<=13;++z){const bool post=(z-1)%4==0;auto wall=post?pl:books;fill(ctx,clip,1,1,z,1,4,z,wall,wall);fill(ctx,clip,12,1,z,12,4,z,wall,wall);if(post){setBlock(ctx,clip,torchS(Facing::East),2,3,z);setBlock(ctx,clip,torchS(Facing::West),11,3,z);}if(large){fill(ctx,clip,1,6,z,1,9,z,wall,wall);fill(ctx,clip,12,6,z,12,9,z,wall,wall);}}
        for(int z=3;z<12;z+=2){fill(ctx,clip,3,1,z,4,3,z,books,books);fill(ctx,clip,6,1,z,7,3,z,books,books);fill(ctx,clip,9,1,z,10,3,z,books,books);}if(large){fill(ctx,clip,1,5,1,3,5,13,pl,pl);fill(ctx,clip,10,5,1,12,5,13,pl,pl);fill(ctx,clip,4,5,1,9,5,2,pl,pl);fill(ctx,clip,4,5,12,9,5,13,pl,pl);setBlock(ctx,clip,pl,9,5,11);setBlock(ctx,clip,pl,8,5,11);setBlock(ctx,clip,pl,9,5,10);fill(ctx,clip,3,6,2,3,6,12,fence,fence);fill(ctx,clip,10,6,2,10,6,10,fence,fence);fill(ctx,clip,4,6,2,9,6,2,fence,fence);fill(ctx,clip,4,6,12,8,6,12,fence,fence);setBlock(ctx,clip,fence,9,6,11);setBlock(ctx,clip,fence,8,6,11);setBlock(ctx,clip,fence,9,6,10);auto ld=V("minecraft:ladder",{{"facing","south"}});for(int y=1;y<=7;++y)setBlock(ctx,clip,ld,10,y,13);for(auto p:std::array<std::array<int,3>,12>{{{{6,9,7}},{{7,9,7}},{{6,8,7}},{{7,8,7}},{{6,7,7}},{{7,7,7}},{{5,7,7}},{{8,7,7}},{{6,7,6}},{{6,7,8}},{{7,7,6}},{{7,7,8}}}})setBlock(ctx,clip,fence,p[0],p[1],p[2]);for(auto p:std::array<std::array<int,3>,6>{{{{5,8,7}},{{8,8,7}},{{6,8,6}},{{6,8,8}},{{7,8,6}},{{7,8,8}}}})setBlock(ctx,clip,V("minecraft:torch",{{"facing","up"}}),p[0],p[1],p[2]);}
        setBlock(ctx,clip,V("minecraft:chest",{{"facing","north"}}),3,3,5);{const int wx=worldX(3,5),wy=worldY(3),wz=worldZ(3,5);const auto seed=r.nextLong();ctx.assignStructureLoot(wx,wy,wz,"minecraft:chests/stronghold_library",seed);}if(large){setBlock(ctx,clip,A,12,9,1);setBlock(ctx,clip,V("minecraft:chest",{{"facing","north"}}),12,8,1);{const int wx=worldX(12,1),wy=worldY(8),wz=worldZ(12,1);const auto seed=r.nextLong();ctx.assignStructureLoot(wx,wy,wz,"minecraft:chests/stronghold_library",seed);}}return true;
    }
    case Kind::PortalRoom:
    {
        randomized(ctx,clip,r,0,0,0,10,7,15,false);door(ctx,clip,Door::Grates,4,1,0);randomized(ctx,clip,r,1,6,1,1,6,14,false);randomized(ctx,clip,r,9,6,1,9,6,14,false);randomized(ctx,clip,r,2,6,1,8,6,2,false);randomized(ctx,clip,r,2,6,14,8,6,14,false);randomized(ctx,clip,r,1,1,1,2,1,4,false);randomized(ctx,clip,r,8,1,1,9,1,4,false);auto lava=V("minecraft:lava",{{"level","0"}});fill(ctx,clip,1,1,1,1,1,3,lava,lava);fill(ctx,clip,9,1,1,9,1,3,lava,lava);randomized(ctx,clip,r,3,1,8,7,1,12,false);fill(ctx,clip,4,1,9,6,1,11,lava,lava);for(int z=3;z<14;z+=2){fill(ctx,clip,0,3,z,0,4,z,bars,bars);fill(ctx,clip,10,3,z,10,4,z,bars,bars);}for(int x=2;x<9;x+=2)fill(ctx,clip,x,3,15,x,4,15,bars,bars);randomized(ctx,clip,r,4,1,5,6,1,7,false);randomized(ctx,clip,r,4,2,6,6,2,7,false);randomized(ctx,clip,r,4,3,7,6,3,7,false);for(int x=4;x<=6;++x){setBlock(ctx,clip,stoneBrickStair(Facing::North),x,1,4);setBlock(ctx,clip,stoneBrickStair(Facing::North),x,2,5);setBlock(ctx,clip,stoneBrickStair(Facing::North),x,3,6);}bool all=true;std::array<bool,12>eyes{};for(bool&e:eyes){e=r.nextFloat()>.9f;all&=e;}for(int x=4;x<=6;++x){setBlock(ctx,clip,V("minecraft:end_portal_frame",{{"facing","north"},{"eye",eyes[x-4]?"true":"false"}}),x,3,8);setBlock(ctx,clip,V("minecraft:end_portal_frame",{{"facing","south"},{"eye",eyes[x-1]?"true":"false"}}),x,3,12);}for(int z=9;z<=11;++z){setBlock(ctx,clip,V("minecraft:end_portal_frame",{{"facing","east"},{"eye",eyes[z-3]?"true":"false"}}),3,3,z);setBlock(ctx,clip,V("minecraft:end_portal_frame",{{"facing","west"},{"eye",eyes[z]?"true":"false"}}),7,3,z);}if(all)fill(ctx,clip,4,3,9,6,3,11,V("minecraft:end_portal"),V("minecraft:end_portal"));if(!spawner){const int wx=worldX(5,6),wy=worldY(3),wz=worldZ(5,6);if(clip.contains(wx,wy,wz)){spawner=true;setBlock(ctx,clip,V("minecraft:mob_spawner"),5,3,6);ctx.assignStructureSpawner(wx,wy,wz,"minecraft:silverfish");}}return true;
    }
    case Kind::RoomCrossing:
    {
        randomized(ctx,clip,r,0,0,0,10,6,10,true);
        door(ctx,clip,entry,4,1,0);
        fillAir(ctx,clip,4,1,10,6,3,10);
        fillAir(ctx,clip,0,1,4,0,3,6);
        fillAir(ctx,clip,10,1,4,10,3,6);
        if(roomType==0)
        {
            setBlock(ctx,clip,sb,5,1,5);setBlock(ctx,clip,sb,5,2,5);setBlock(ctx,clip,sb,5,3,5);
            setBlock(ctx,clip,torchS(Facing::West),4,3,5);setBlock(ctx,clip,torchS(Facing::East),6,3,5);
            setBlock(ctx,clip,torchS(Facing::South),5,3,4);setBlock(ctx,clip,torchS(Facing::North),5,3,6);
            for(auto p:std::array<std::array<int,3>,8>{{{{4,1,4}},{{4,1,5}},{{4,1,6}},{{6,1,4}},{{6,1,5}},{{6,1,6}},{{5,1,4}},{{5,1,6}}}})
                setBlock(ctx,clip,slab(),p[0],p[1],p[2]);
        }
        else if(roomType==1)
        {
            for(int i=0;i<5;++i){setBlock(ctx,clip,sb,3,1,3+i);setBlock(ctx,clip,sb,7,1,3+i);setBlock(ctx,clip,sb,3+i,1,3);setBlock(ctx,clip,sb,3+i,1,7);}
            setBlock(ctx,clip,sb,5,1,5);setBlock(ctx,clip,sb,5,2,5);setBlock(ctx,clip,sb,5,3,5);
            setBlock(ctx,clip,V("minecraft:flowing_water",{{"level","0"}}),5,4,5);
        }
        else if(roomType==2)
        {
            const auto cobble=V("minecraft:cobblestone");
            for(int i=1;i<=9;++i){setBlock(ctx,clip,cobble,1,3,i);setBlock(ctx,clip,cobble,9,3,i);setBlock(ctx,clip,cobble,i,3,1);setBlock(ctx,clip,cobble,i,3,9);}
            for(auto p:std::array<std::array<int,3>,8>{{{{5,1,4}},{{5,1,6}},{{5,3,4}},{{5,3,6}},{{4,1,5}},{{6,1,5}},{{4,3,5}},{{6,3,5}}}})
                setBlock(ctx,clip,cobble,p[0],p[1],p[2]);
            for(int y=1;y<=3;++y)for(auto p:std::array<std::array<int,2>,4>{{{{4,4}},{{6,4}},{{4,6}},{{6,6}}}})
                setBlock(ctx,clip,cobble,p[0],y,p[1]);
            setBlock(ctx,clip,V("minecraft:torch",{{"facing","up"}}),5,3,5);
            for(int z=2;z<=8;++z)
            {
                setBlock(ctx,clip,pl,2,3,z);setBlock(ctx,clip,pl,3,3,z);
                if(z<=3||z>=7){setBlock(ctx,clip,pl,4,3,z);setBlock(ctx,clip,pl,5,3,z);setBlock(ctx,clip,pl,6,3,z);}
                setBlock(ctx,clip,pl,7,3,z);setBlock(ctx,clip,pl,8,3,z);
            }
            auto ladder=V("minecraft:ladder",{{"facing","west"}});
            setBlock(ctx,clip,ladder,9,1,3);setBlock(ctx,clip,ladder,9,2,3);setBlock(ctx,clip,ladder,9,3,3);
            setBlock(ctx,clip,V("minecraft:chest",{{"facing","north"}}),3,4,8);{const int wx=worldX(3,8),wy=worldY(4),wz=worldZ(3,8);const auto seed=r.nextLong();ctx.assignStructureLoot(wx,wy,wz,"minecraft:chests/stronghold_crossing",seed);}
        }
        return true;
    }
    }
    return true;
}
}

StrongholdStructure::Start StrongholdStructure::create(int chunkX,int chunkZ,JavaRandom&r,int seaLevel)
{
    Start out;
    do
    {
        out.pieces.clear();out.hasPortalRoom=false;
        const Facing f=randomFacing(r);auto root=std::make_unique<Root>((chunkX<<4)+2,(chunkZ<<4)+2,f);Root*rt=root.get();out.pieces.push_back(std::move(root));rt->build(out.pieces,r);
        while(!rt->pending.empty()){const int i=r.nextInt(static_cast<int>(rt->pending.size()));StrongPiece*p=rt->pending[i];rt->pending.erase(rt->pending.begin()+i);if(p)p->build(out.pieces,r);}
        out.bounds=boundsOf(out.pieces);markAvailableHeight(out.pieces,r,10,seaLevel);out.bounds=boundsOf(out.pieces);out.hasPortalRoom=rt->portal!=nullptr;
    }while(!out.hasPortalRoom);
    out.sizeable=!out.pieces.empty();return out;
}

void StrongholdStructure::place(Start&s,WorldGenerationContext&ctx,JavaRandom&r,const Box&clip)
{for(auto&p:s.pieces)if(p&&p->box.intersects(clip))p->place(ctx,r,clip);}
}
