#include "worldgen/WoodlandMansionStructure.h"

#include "content/ContentCatalog.h"
#include "worldgen/Vanilla112State.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace mc112
{
namespace
{
enum class Dir : std::uint8_t { North,South,West,East,Up };

std::pair<int,int> step(Dir d) noexcept
{
    switch(d){case Dir::North:return{0,-1};case Dir::South:return{0,1};case Dir::West:return{-1,0};case Dir::East:return{1,0};case Dir::Up:return{0,0};}
    return{0,0};
}
Dir opposite(Dir d) noexcept
{
    switch(d){case Dir::North:return Dir::South;case Dir::South:return Dir::North;case Dir::West:return Dir::East;case Dir::East:return Dir::West;case Dir::Up:return Dir::Up;}
    return d;
}
Dir rotateY(Dir d) noexcept
{
    switch(d){case Dir::North:return Dir::East;case Dir::East:return Dir::South;case Dir::South:return Dir::West;case Dir::West:return Dir::North;case Dir::Up:return Dir::Up;}
    return d;
}
Dir rotateYCCW(Dir d) noexcept
{
    switch(d){case Dir::North:return Dir::West;case Dir::West:return Dir::South;case Dir::South:return Dir::East;case Dir::East:return Dir::North;case Dir::Up:return Dir::Up;}
    return d;
}
Rotation addRotation(Rotation a,Rotation b) noexcept
{
    const int av=a==Rotation::None?0:a==Rotation::Clockwise90?1:a==Rotation::Clockwise180?2:3;
    const int bv=b==Rotation::None?0:b==Rotation::Clockwise90?1:b==Rotation::Clockwise180?2:3;
    switch((av+bv)&3){case 0:return Rotation::None;case 1:return Rotation::Clockwise90;case 2:return Rotation::Clockwise180;default:return Rotation::CounterClockwise90;}
}
Dir rotateDir(Dir d,Rotation r) noexcept
{
    int turns=r==Rotation::None?0:r==Rotation::Clockwise90?1:r==Rotation::Clockwise180?2:3;
    while(turns-->0)d=rotateY(d);return d;
}
struct Pos{int x=0,y=0,z=0;};
Pos offset(Pos p,Dir d,int amount=1) noexcept{auto [dx,dz]=step(d);p.x+=dx*amount;p.z+=dz*amount;return p;}
Pos up(Pos p,int n=1) noexcept{p.y+=n;return p;}
Pos add(Pos p,int x,int y,int z) noexcept{p.x+=x;p.y+=y;p.z+=z;return p;}
Pos rotatePos(Pos p,Rotation r) noexcept
{
    switch(r){case Rotation::None:return p;case Rotation::Clockwise90:return{-p.z,p.y,p.x};case Rotation::Clockwise180:return{-p.x,p.y,-p.z};case Rotation::CounterClockwise90:return{p.z,p.y,-p.x};}
    return p;
}

class SimpleGrid
{
public:
    int width,height,outside;
    explicit SimpleGrid(int w=11,int h=11,int o=5):width(w),height(h),outside(o),grid(static_cast<std::size_t>(w*h),0){}
    int get(int x,int z)const noexcept{return x>=0&&x<width&&z>=0&&z<height?grid[static_cast<std::size_t>(x+z*width)]:outside;}
    void set(int x,int z,int v) noexcept{if(x>=0&&x<width&&z>=0&&z<height)grid[static_cast<std::size_t>(x+z*width)]=v;}
    void set(int x0,int z0,int x1,int z1,int v) noexcept{for(int z=z0;z<=z1;++z)for(int x=x0;x<=x1;++x)set(x,z,v);}
    void setIf(int x,int z,int expected,int v) noexcept{if(get(x,z)==expected)set(x,z,v);}
    bool edgesTo(int x,int z,int v)const noexcept{return get(x-1,z)==v||get(x+1,z)==v||get(x,z-1)==v||get(x,z+1)==v;}
private:std::vector<int>grid;
};

bool isHouse(const SimpleGrid&g,int x,int z) noexcept{const int v=g.get(x,z);return v==1||v==2||v==3||v==4;}

template<class T> void vanillaShuffle(std::vector<T>&v,JavaRandom&r)
{
    for(int i=static_cast<int>(v.size());i>1;--i)std::swap(v[static_cast<std::size_t>(i-1)],v[static_cast<std::size_t>(r.nextInt(i))]);
}

class Grid
{
public:
    JavaRandom& random;
    SimpleGrid baseGrid{11,11,5},thirdFloorGrid{11,11,5};
    std::array<SimpleGrid,3> floorRooms{SimpleGrid(11,11,5),SimpleGrid(11,11,5),SimpleGrid(11,11,5)};
    int entranceX=7,entranceY=4;

    explicit Grid(JavaRandom&r):random(r)
    {
        baseGrid.set(entranceX,entranceY,entranceX+1,entranceY+1,3);
        baseGrid.set(entranceX-1,entranceY,entranceX-1,entranceY+1,2);
        baseGrid.set(entranceX+2,entranceY-2,entranceX+3,entranceY+3,5);
        baseGrid.set(entranceX+1,entranceY-2,entranceX+1,entranceY-1,1);
        baseGrid.set(entranceX+1,entranceY+2,entranceX+1,entranceY+3,1);
        baseGrid.set(entranceX-1,entranceY-1,1);baseGrid.set(entranceX-1,entranceY+2,1);
        baseGrid.set(0,0,11,1,5);baseGrid.set(0,9,11,11,5);
        recursiveCorridor(baseGrid,entranceX,entranceY-2,Dir::West,6);
        recursiveCorridor(baseGrid,entranceX,entranceY+3,Dir::West,6);
        recursiveCorridor(baseGrid,entranceX-2,entranceY-1,Dir::West,3);
        recursiveCorridor(baseGrid,entranceX-2,entranceY+2,Dir::West,3);
        while(cleanEdges(baseGrid)){}
        identifyRooms(baseGrid,floorRooms[0]);identifyRooms(baseGrid,floorRooms[1]);
        floorRooms[0].set(entranceX+1,entranceY,entranceX+1,entranceY+1,8388608);
        floorRooms[1].set(entranceX+1,entranceY,entranceX+1,entranceY+1,8388608);
        setupThirdFloor();identifyRooms(thirdFloorGrid,floorRooms[2]);
    }

    bool isRoomId(const SimpleGrid&,int x,int z,int floor,int id)const noexcept{return (floorRooms[static_cast<std::size_t>(floor)].get(x,z)&65535)==id;}
    Dir get1x2RoomDirection(const SimpleGrid&grid,int x,int z,int floor,int id)const noexcept
    {
        for(Dir d:{Dir::North,Dir::East,Dir::South,Dir::West}){auto [dx,dz]=step(d);if(isRoomId(grid,x+dx,z+dz,floor,id))return d;}
        return Dir::Up;
    }
private:
    void recursiveCorridor(SimpleGrid&g,int x,int z,Dir dir,int depth)
    {
        if(depth<=0)return;g.set(x,z,1);auto [dx,dz]=step(dir);g.setIf(x+dx,z+dz,0,1);
        for(int i=0;i<8;++i)
        {
            const Dir d=std::array<Dir,4>{Dir::South,Dir::West,Dir::North,Dir::East}[static_cast<std::size_t>(random.nextInt(4))];
            if(d==opposite(dir)||(d==Dir::East&&random.nextBoolean()))continue;
            auto [sx,sz]=step(d);const int nx=x+dx,nz=z+dz;
            if(g.get(nx+sx,nz+sz)==0&&g.get(nx+sx*2,nz+sz*2)==0){recursiveCorridor(g,nx+sx,nz+sz,d,depth-1);break;}
        }
        for(Dir side:{rotateY(dir),rotateYCCW(dir)}){auto [sx,sz]=step(side);g.setIf(x+sx,z+sz,0,2);g.setIf(x+dx+sx,z+dz+sz,0,2);g.setIf(x+sx*2,z+sz*2,0,2);}g.setIf(x+dx*2,z+dz*2,0,2);
    }
    bool cleanEdges(SimpleGrid&g)
    {
        bool changed=false;for(int z=0;z<g.height;++z)for(int x=0;x<g.width;++x)if(g.get(x,z)==0){int n=(isHouse(g,x+1,z)?1:0)+(isHouse(g,x-1,z)?1:0)+(isHouse(g,x,z+1)?1:0)+(isHouse(g,x,z-1)?1:0);if(n>=3){g.set(x,z,2);changed=true;}else if(n==2){int d=(isHouse(g,x+1,z+1)?1:0)+(isHouse(g,x-1,z+1)?1:0)+(isHouse(g,x+1,z-1)?1:0)+(isHouse(g,x-1,z-1)?1:0);if(d<=1){g.set(x,z,2);changed=true;}}}return changed;
    }
    void setupThirdFloor()
    {
        std::vector<std::pair<int,int>> choices;for(int z=0;z<thirdFloorGrid.height;++z)for(int x=0;x<thirdFloorGrid.width;++x){const int v=floorRooms[1].get(x,z);if((v&983040)==131072&&(v&2097152)==2097152)choices.emplace_back(x,z);}
        if(choices.empty()){thirdFloorGrid.set(0,0,thirdFloorGrid.width,thirdFloorGrid.height,5);return;}
        auto [x,z]=choices[static_cast<std::size_t>(random.nextInt(static_cast<int>(choices.size())))];const int original=floorRooms[1].get(x,z);floorRooms[1].set(x,z,original|4194304);const Dir roomDir=get1x2RoomDirection(baseGrid,x,z,1,original&65535);auto [dx,dz]=step(roomDir);const int sx=x+dx,sz=z+dz;
        for(int zz=0;zz<thirdFloorGrid.height;++zz)for(int xx=0;xx<thirdFloorGrid.width;++xx){if(!isHouse(baseGrid,xx,zz))thirdFloorGrid.set(xx,zz,5);else if(xx==x&&zz==z)thirdFloorGrid.set(xx,zz,3);else if(xx==sx&&zz==sz){thirdFloorGrid.set(xx,zz,3);floorRooms[2].set(xx,zz,8388608);}}
        std::vector<Dir> open;for(Dir d:{Dir::North,Dir::East,Dir::South,Dir::West}){auto [ox,oz]=step(d);if(thirdFloorGrid.get(sx+ox,sz+oz)==0)open.push_back(d);}if(open.empty()){thirdFloorGrid.set(0,0,thirdFloorGrid.width,thirdFloorGrid.height,5);floorRooms[1].set(x,z,original);return;}const Dir d=open[static_cast<std::size_t>(random.nextInt(static_cast<int>(open.size())))];auto [ox,oz]=step(d);recursiveCorridor(thirdFloorGrid,sx+ox,sz+oz,d,4);while(cleanEdges(thirdFloorGrid)){}
    }
    void identifyRooms(const SimpleGrid&base,SimpleGrid&rooms)
    {
        std::vector<std::pair<int,int>> list;for(int z=0;z<base.height;++z)for(int x=0;x<base.width;++x)if(base.get(x,z)==2)list.emplace_back(x,z);vanillaShuffle(list,random);int id=10;
        for(auto [x,z]:list)if(rooms.get(x,z)==0)
        {
            int x0=x,x1=x,z0=z,z1=z,type=65536;
            if(rooms.get(x+1,z)==0&&rooms.get(x,z+1)==0&&rooms.get(x+1,z+1)==0&&base.get(x+1,z)==2&&base.get(x,z+1)==2&&base.get(x+1,z+1)==2){x1=x+1;z1=z+1;type=262144;}
            else if(rooms.get(x-1,z)==0&&rooms.get(x,z+1)==0&&rooms.get(x-1,z+1)==0&&base.get(x-1,z)==2&&base.get(x,z+1)==2&&base.get(x-1,z+1)==2){x0=x-1;z1=z+1;type=262144;}
            else if(rooms.get(x-1,z)==0&&rooms.get(x,z-1)==0&&rooms.get(x-1,z-1)==0&&base.get(x-1,z)==2&&base.get(x,z-1)==2&&base.get(x-1,z-1)==2){x0=x-1;z0=z-1;type=262144;}
            else if(rooms.get(x+1,z)==0&&base.get(x+1,z)==2){x1=x+1;type=131072;}
            else if(rooms.get(x,z+1)==0&&base.get(x,z+1)==2){z1=z+1;type=131072;}
            else if(rooms.get(x-1,z)==0&&base.get(x-1,z)==2){x0=x-1;type=131072;}
            else if(rooms.get(x,z-1)==0&&base.get(x,z-1)==2){z0=z-1;type=131072;}
            int doorX=random.nextBoolean()?x0:x1,doorZ=random.nextBoolean()?z0:z1;int doorFlag=2097152;
            if(!base.edgesTo(doorX,doorZ,1)){doorX=doorX==x0?x1:x0;doorZ=doorZ==z0?z1:z0;if(!base.edgesTo(doorX,doorZ,1)){doorZ=doorZ==z0?z1:z0;if(!base.edgesTo(doorX,doorZ,1)){doorX=doorX==x0?x1:x0;doorZ=doorZ==z0?z1:z0;if(!base.edgesTo(doorX,doorZ,1)){doorFlag=0;doorX=x0;doorZ=z0;}}}}
            for(int zz=z0;zz<=z1;++zz)for(int xx=x0;xx<=x1;++xx)rooms.set(xx,zz,(xx==doorX&&zz==doorZ?1048576|doorFlag:0)|type|id);++id;
        }
    }
};

struct RoomCollection
{
    int floor=0;
    std::string one(JavaRandom&r)const{return floor==0?"1x1_a"+std::to_string(r.nextInt(5)+1):"1x1_b"+std::to_string(r.nextInt(4)+1);}
    std::string oneSecret(JavaRandom&r)const{return "1x1_as"+std::to_string(r.nextInt(4)+1);}
    std::string side(JavaRandom&r,bool stairs)const{if(floor==0)return "1x2_a"+std::to_string(r.nextInt(9)+1);return stairs?"1x2_c_stairs":"1x2_c"+std::to_string(r.nextInt(4)+1);}
    std::string front(JavaRandom&r,bool stairs)const{if(floor==0)return "1x2_b"+std::to_string(r.nextInt(5)+1);return stairs?"1x2_d_stairs":"1x2_d"+std::to_string(r.nextInt(5)+1);}
    std::string secret(JavaRandom&r)const{return floor==0?"1x2_s"+std::to_string(r.nextInt(2)+1):"1x2_se1";}
    std::string two(JavaRandom&r)const{return floor==0?"2x2_a"+std::to_string(r.nextInt(4)+1):"2x2_b"+std::to_string(r.nextInt(5)+1);}
    std::string twoSecret(JavaRandom&)const{return "2x2_s1";}
};

struct PlacementData{Pos position;Rotation rotation=Rotation::None;std::string wallType;};

class Placer
{
public:
    JavaRandom& random;int startX=0,startY=0;std::vector<WoodlandMansionStructure::TemplatePiece>&out;
    Placer(JavaRandom&r,std::vector<WoodlandMansionStructure::TemplatePiece>&o):random(r),out(o){}
    void addPiece(std::string name,Pos p,Rotation r,Mirror m=Mirror::None){out.push_back({std::move(name),p.x,p.y,p.z,r,m});}
    void create(Pos origin,Rotation rotation,Grid&g)
    {
        PlacementData p{origin,rotation,"wall_flat"},p1;entrance(p);p1={up(p.position,8),p.rotation,"wall_window"};startX=g.entranceX+1;startY=g.entranceY+1;const int ex=g.entranceX+1,ez=g.entranceY;traverseOuterWalls(p,g.baseGrid,Dir::South,startX,startY,ex,ez);traverseOuterWalls(p1,g.baseGrid,Dir::South,startX,startY,ex,ez);
        PlacementData p2{up(p.position,19),p.rotation,"wall_window"};bool found=false;for(int z=0;z<g.thirdFloorGrid.height&&!found;++z)for(int x=g.thirdFloorGrid.width-1;x>=0&&!found;--x)if(isHouse(g.thirdFloorGrid,x,z)){p2.position=offset(p2.position,rotateDir(Dir::South,rotation),8+(z-startY)*8);p2.position=offset(p2.position,rotateDir(Dir::East,rotation),(x-startX)*8);traverseWallPiece(p2);traverseOuterWalls(p2,g.thirdFloorGrid,Dir::South,x,z,x,z);found=true;}
        createRoof(up(origin,16),rotation,g.baseGrid,&g.thirdFloorGrid);createRoof(up(origin,27),rotation,g.thirdFloorGrid,nullptr);
        for(int floor=0;floor<3;++floor)placeFloor(origin,rotation,g,floor);
    }
private:
    void entrance(PlacementData&p){addPiece("entrance",offset(p.position,rotateDir(Dir::West,p.rotation),9),p.rotation);p.position=offset(p.position,rotateDir(Dir::South,p.rotation),16);}
    void traverseWallPiece(PlacementData&p){addPiece(p.wallType,offset(p.position,rotateDir(Dir::East,p.rotation),7),p.rotation);p.position=offset(p.position,rotateDir(Dir::South,p.rotation),8);}
    void traverseTurn(PlacementData&p){p.position=offset(p.position,rotateDir(Dir::South,p.rotation),-1);addPiece("wall_corner",p.position,p.rotation);p.position=offset(p.position,rotateDir(Dir::South,p.rotation),-7);p.position=offset(p.position,rotateDir(Dir::West,p.rotation),-6);p.rotation=addRotation(p.rotation,Rotation::Clockwise90);}
    void traverseInnerTurn(PlacementData&p){p.position=offset(p.position,rotateDir(Dir::South,p.rotation),6);p.position=offset(p.position,rotateDir(Dir::East,p.rotation),8);p.rotation=addRotation(p.rotation,Rotation::CounterClockwise90);}
    void traverseOuterWalls(PlacementData&p,const SimpleGrid&g,Dir dir,int x,int z,int endX,int endZ)
    {
        const Dir startDir=dir;for(;;){auto [dx,dz]=step(dir);if(!isHouse(g,x+dx,z+dz)){traverseTurn(p);dir=rotateY(dir);if(x!=endX||z!=endZ||dir!=startDir)traverseWallPiece(p);}else{auto [lx,lz]=step(rotateYCCW(dir));if(isHouse(g,x+dx+lx,z+dz+lz)){traverseInnerTurn(p);x+=dx;z+=dz;dir=rotateYCCW(dir);}else{x+=dx;z+=dz;if(x!=endX||z!=endZ||dir!=startDir)traverseWallPiece(p);}}if(x==endX&&z==endZ&&dir==startDir)break;}
    }
    void createRoof(Pos base,Rotation rot,const SimpleGrid&g,const SimpleGrid*above)
    {
        for(int z=0;z<g.height;++z)for(int x=0;x<g.width;++x){Pos p=offset(base,rotateDir(Dir::South,rot),8+(z-startY)*8);p=offset(p,rotateDir(Dir::East,rot),(x-startX)*8);const bool covered=above&&isHouse(*above,x,z);if(isHouse(g,x,z)&&!covered){addPiece("roof",up(p,3),rot);if(!isHouse(g,x+1,z))addPiece("roof_front",offset(p,rotateDir(Dir::East,rot),6),rot);if(!isHouse(g,x-1,z)){Pos q=offset(p,rotateDir(Dir::South,rot),7);addPiece("roof_front",q,addRotation(rot,Rotation::Clockwise180));}if(!isHouse(g,x,z-1))addPiece("roof_front",offset(p,rotateDir(Dir::West,rot),1),addRotation(rot,Rotation::CounterClockwise90));if(!isHouse(g,x,z+1)){Pos q=offset(p,rotateDir(Dir::East,rot),6);q=offset(q,rotateDir(Dir::South,rot),6);addPiece("roof_front",q,addRotation(rot,Rotation::Clockwise90));}}}
        if(above)for(int z=0;z<g.height;++z)for(int x=0;x<g.width;++x){Pos p=offset(base,rotateDir(Dir::South,rot),8+(z-startY)*8);p=offset(p,rotateDir(Dir::East,rot),(x-startX)*8);if(isHouse(g,x,z)&&isHouse(*above,x,z)){if(!isHouse(g,x+1,z))addPiece("small_wall",offset(p,rotateDir(Dir::East,rot),7),rot);if(!isHouse(g,x-1,z)){Pos q=offset(p,rotateDir(Dir::West,rot),1);q=offset(q,rotateDir(Dir::South,rot),6);addPiece("small_wall",q,addRotation(rot,Rotation::Clockwise180));}if(!isHouse(g,x,z-1)){Pos q=offset(p,rotateDir(Dir::North,rot),1);addPiece("small_wall",q,addRotation(rot,Rotation::CounterClockwise90));}if(!isHouse(g,x,z+1)){Pos q=offset(p,rotateDir(Dir::East,rot),6);q=offset(q,rotateDir(Dir::South,rot),7);addPiece("small_wall",q,addRotation(rot,Rotation::Clockwise90));}}}
        for(int z=0;z<g.height;++z)for(int x=0;x<g.width;++x){Pos p=offset(base,rotateDir(Dir::South,rot),8+(z-startY)*8);p=offset(p,rotateDir(Dir::East,rot),(x-startX)*8);const bool covered=above&&isHouse(*above,x,z);if(!isHouse(g,x,z)||covered)continue;if(!isHouse(g,x+1,z)){Pos e=offset(p,rotateDir(Dir::East,rot),6);if(!isHouse(g,x,z+1))addPiece("roof_corner",offset(e,rotateDir(Dir::South,rot),6),rot);else if(isHouse(g,x+1,z+1))addPiece("roof_inner_corner",offset(e,rotateDir(Dir::South,rot),5),rot);if(!isHouse(g,x,z-1))addPiece("roof_corner",e,addRotation(rot,Rotation::CounterClockwise90));else if(isHouse(g,x+1,z-1)){Pos q=offset(p,rotateDir(Dir::East,rot),9);q=offset(q,rotateDir(Dir::North,rot),2);addPiece("roof_inner_corner",q,addRotation(rot,Rotation::Clockwise90));}}if(!isHouse(g,x-1,z)){Pos w=p;if(!isHouse(g,x,z+1))addPiece("roof_corner",offset(w,rotateDir(Dir::South,rot),6),addRotation(rot,Rotation::Clockwise90));else if(isHouse(g,x-1,z+1)){Pos q=offset(w,rotateDir(Dir::South,rot),8);q=offset(q,rotateDir(Dir::West,rot),3);addPiece("roof_inner_corner",q,addRotation(rot,Rotation::CounterClockwise90));}if(!isHouse(g,x,z-1))addPiece("roof_corner",w,addRotation(rot,Rotation::Clockwise180));else if(isHouse(g,x-1,z-1))addPiece("roof_inner_corner",offset(w,rotateDir(Dir::South,rot),1),addRotation(rot,Rotation::Clockwise180));}}
    }
    void placeFloor(Pos origin,Rotation rot,Grid&g,int floor)
    {
        Pos base=up(origin,8*floor+(floor==2?3:0));const SimpleGrid&rooms=g.floorRooms[static_cast<std::size_t>(floor)];const SimpleGrid&layout=floor==2?g.thirdFloorGrid:g.baseGrid;const std::string carpetS=floor==0?"carpet_south":"carpet_south_2",carpetW=floor==0?"carpet_west":"carpet_west_2";
        for(int z=0;z<layout.height;++z)for(int x=0;x<layout.width;++x)if(layout.get(x,z)==1){Pos p=offset(base,rotateDir(Dir::South,rot),8+(z-startY)*8);p=offset(p,rotateDir(Dir::East,rot),(x-startX)*8);addPiece("corridor_floor",p,rot);if(layout.get(x,z-1)==1||(rooms.get(x,z-1)&8388608))addPiece("carpet_north",up(offset(p,rotateDir(Dir::East,rot),1)),rot);if(layout.get(x+1,z)==1||(rooms.get(x+1,z)&8388608)){Pos q=offset(p,rotateDir(Dir::South,rot),1);q=offset(q,rotateDir(Dir::East,rot),5);addPiece("carpet_east",up(q),rot);}if(layout.get(x,z+1)==1||(rooms.get(x,z+1)&8388608)){Pos q=offset(p,rotateDir(Dir::South,rot),5);q=offset(q,rotateDir(Dir::West,rot),1);addPiece(carpetS,q,rot);}if(layout.get(x-1,z)==1||(rooms.get(x-1,z)&8388608)){Pos q=offset(p,rotateDir(Dir::West,rot),1);q=offset(q,rotateDir(Dir::North,rot),1);addPiece(carpetW,q,rot);}}
        const std::string wall=floor==0?"indoors_wall":"indoors_wall_2",door=floor==0?"indoors_door":"indoors_door_2";RoomCollection collection{floor};std::vector<Dir> doors;
        for(int z=0;z<layout.height;++z)for(int x=0;x<layout.width;++x){bool third=floor==2&&layout.get(x,z)==3;if(layout.get(x,z)!=2&&!third)continue;const int rv=rooms.get(x,z),type=rv&983040,id=rv&65535;third=third&&(rv&8388608);doors.clear();if(rv&2097152)for(Dir d:{Dir::North,Dir::East,Dir::South,Dir::West}){auto [dx,dz]=step(d);if(layout.get(x+dx,z+dz)==1)doors.push_back(d);}Dir entrance=Dir::Up;if(!doors.empty())entrance=doors[static_cast<std::size_t>(random.nextInt(static_cast<int>(doors.size())))];else if(rv&1048576)entrance=Dir::Up;Pos p=offset(base,rotateDir(Dir::South,rot),8+(z-startY)*8);p=offset(p,rotateDir(Dir::East,rot),-1+(x-startX)*8);
            if(isHouse(layout,x-1,z)&&!g.isRoomId(layout,x-1,z,floor,id))addPiece(entrance==Dir::West?door:wall,p,rot);if(layout.get(x+1,z)==1&&!third)addPiece(entrance==Dir::East?door:wall,offset(p,rotateDir(Dir::East,rot),8),rot);if(isHouse(layout,x,z+1)&&!g.isRoomId(layout,x,z+1,floor,id)){Pos q=offset(p,rotateDir(Dir::South,rot),7);q=offset(q,rotateDir(Dir::East,rot),7);addPiece(entrance==Dir::South?door:wall,q,addRotation(rot,Rotation::Clockwise90));}if(layout.get(x,z-1)==1&&!third){Pos q=offset(p,rotateDir(Dir::North,rot),1);q=offset(q,rotateDir(Dir::East,rot),7);addPiece(entrance==Dir::North?door:wall,q,addRotation(rot,Rotation::Clockwise90));}
            if(type==65536)addRoom1x1(p,rot,entrance,collection);else if(type==131072&&entrance!=Dir::Up){Dir rd=g.get1x2RoomDirection(layout,x,z,floor,id);addRoom1x2(p,rot,rd,entrance,collection,(rv&4194304)!=0);}else if(type==262144&&entrance!=Dir::Up){Dir side=rotateY(entrance);auto [dx,dz]=step(side);if(!g.isRoomId(layout,x+dx,z+dz,floor,id))side=opposite(side);addRoom2x2(p,rot,side,entrance,collection);}else if(type==262144&&entrance==Dir::Up)addRoom2x2Secret(p,rot,collection);
        }
    }
    void addRoom1x1(Pos p,Rotation world,Dir entrance,const RoomCollection&c)
    {
        Rotation local=Rotation::None;std::string name=c.one(random);if(entrance!=Dir::East){if(entrance==Dir::North)local=Rotation::CounterClockwise90;else if(entrance==Dir::West)local=Rotation::Clockwise180;else if(entrance==Dir::South)local=Rotation::Clockwise90;else name=c.oneSecret(random);} // Template#getZeroPositionWithTransform(1,0,0,7,7)
        Pos delta{1,0,0};if(local==Rotation::CounterClockwise90)delta={1,0,6};else if(local==Rotation::Clockwise90)delta={7,0,0};else if(local==Rotation::Clockwise180)delta={7,0,6};delta=rotatePos(delta,world);addPiece(name,add(p,delta.x,0,delta.z),addRotation(local,world));
    }
    void addRoom1x2(Pos p,Rotation world,Dir roomDir,Dir entrance,const RoomCollection&c,bool stairs)
    {
        auto E=[&](Dir d,int n){return offset(p,rotateDir(d,world),n);};
        if(entrance==Dir::East&&roomDir==Dir::South)addPiece(c.side(random,stairs),E(Dir::East,1),world);
        else if(entrance==Dir::East&&roomDir==Dir::North){Pos q=E(Dir::East,1);q=offset(q,rotateDir(Dir::South,world),6);addPiece(c.side(random,stairs),q,world,Mirror::LeftRight);}
        else if(entrance==Dir::West&&roomDir==Dir::North){Pos q=E(Dir::East,7);q=offset(q,rotateDir(Dir::South,world),6);addPiece(c.side(random,stairs),q,addRotation(world,Rotation::Clockwise180));}
        else if(entrance==Dir::West&&roomDir==Dir::South)addPiece(c.side(random,stairs),E(Dir::East,7),world,Mirror::FrontBack);
        else if(entrance==Dir::South&&roomDir==Dir::East)addPiece(c.side(random,stairs),E(Dir::East,1),addRotation(world,Rotation::Clockwise90),Mirror::LeftRight);
        else if(entrance==Dir::South&&roomDir==Dir::West)addPiece(c.side(random,stairs),E(Dir::East,7),addRotation(world,Rotation::Clockwise90));
        else if(entrance==Dir::North&&roomDir==Dir::West){Pos q=E(Dir::East,7);q=offset(q,rotateDir(Dir::South,world),6);addPiece(c.side(random,stairs),q,addRotation(world,Rotation::Clockwise90),Mirror::FrontBack);}
        else if(entrance==Dir::North&&roomDir==Dir::East){Pos q=E(Dir::East,1);q=offset(q,rotateDir(Dir::South,world),6);addPiece(c.side(random,stairs),q,addRotation(world,Rotation::CounterClockwise90));}
        else if(entrance==Dir::South&&roomDir==Dir::North){Pos q=E(Dir::East,1);q=offset(q,rotateDir(Dir::North,world),8);addPiece(c.front(random,stairs),q,world);}
        else if(entrance==Dir::North&&roomDir==Dir::South){Pos q=E(Dir::East,7);q=offset(q,rotateDir(Dir::South,world),14);addPiece(c.front(random,stairs),q,addRotation(world,Rotation::Clockwise180));}
        else if(entrance==Dir::West&&roomDir==Dir::East)addPiece(c.front(random,stairs),E(Dir::East,15),addRotation(world,Rotation::Clockwise90));
        else if(entrance==Dir::East&&roomDir==Dir::West){Pos q=E(Dir::West,7);q=offset(q,rotateDir(Dir::South,world),6);addPiece(c.front(random,stairs),q,addRotation(world,Rotation::CounterClockwise90));}
        else if(entrance==Dir::Up&&roomDir==Dir::East)addPiece(c.secret(random),E(Dir::East,15),addRotation(world,Rotation::Clockwise90));
        else if(entrance==Dir::Up&&roomDir==Dir::South)addPiece(c.secret(random),E(Dir::East,1),world);
    }
    void addRoom2x2(Pos p,Rotation world,Dir side,Dir entrance,const RoomCollection&c)
    {
        int ex=0,sz=0;Rotation r=world;Mirror m=Mirror::None;if(entrance==Dir::East&&side==Dir::South)ex=-7;else if(entrance==Dir::East&&side==Dir::North){ex=-7;sz=6;m=Mirror::LeftRight;}else if(entrance==Dir::North&&side==Dir::East){ex=1;sz=14;r=addRotation(world,Rotation::CounterClockwise90);}else if(entrance==Dir::North&&side==Dir::West){ex=7;sz=14;r=addRotation(world,Rotation::CounterClockwise90);m=Mirror::LeftRight;}else if(entrance==Dir::South&&side==Dir::West){ex=7;sz=-8;r=addRotation(world,Rotation::Clockwise90);}else if(entrance==Dir::South&&side==Dir::East){ex=1;sz=-8;r=addRotation(world,Rotation::Clockwise90);m=Mirror::LeftRight;}else if(entrance==Dir::West&&side==Dir::North){ex=15;sz=6;r=addRotation(world,Rotation::Clockwise180);}else if(entrance==Dir::West&&side==Dir::South){ex=15;m=Mirror::FrontBack;}Pos q=offset(p,rotateDir(Dir::East,world),ex);q=offset(q,rotateDir(Dir::South,world),sz);addPiece(c.two(random),q,r,m);
    }
    void addRoom2x2Secret(Pos p,Rotation world,const RoomCollection&c){addPiece(c.twoSecret(random),offset(p,rotateDir(Dir::East,world),1),world);}
};

mc::content::BlockState mansionState(std::string_view name,std::initializer_list<Property>props={})
{return vanilla112State(name,std::span<const Property>(props.begin(),props.size()));}

mc::content::BlockState chestState(std::string_view facing)
{return mansionState("minecraft:chest",{{"facing",std::string(facing)}});}

Box conservativeBounds(const std::vector<WoodlandMansionStructure::TemplatePiece>&pieces,int minY)
{
    if(pieces.empty())return{};Box b{pieces[0].x-16,minY,pieces[0].z-16,pieces[0].x+31,minY+40,pieces[0].z+31};for(const auto&p:pieces)b.expand(Box{p.x-16,minY,p.z-16,p.x+31,minY+40,p.z+31});return b;
}
}

WoodlandMansionStructure::Start WoodlandMansionStructure::create(
    int chunkX,int chunkZ,JavaRandom&random,const WorldGenerationContext&context)
{
    Start out;const Rotation rot=std::array<Rotation,4>{Rotation::None,Rotation::Clockwise90,Rotation::Clockwise180,Rotation::CounterClockwise90}[static_cast<std::size_t>(random.nextInt(4))];int dx=5,dz=5;if(rot==Rotation::Clockwise90)dx=-5;else if(rot==Rotation::Clockwise180){dx=-5;dz=-5;}else if(rot==Rotation::CounterClockwise90)dz=-5;
    const int baseX=chunkX*16,baseZ=chunkZ*16;const int h0=context.getHeightValue(baseX+7,baseZ+7)-1,h1=context.getHeightValue(baseX+7,baseZ+7+dz)-1,h2=context.getHeightValue(baseX+7+dx,baseZ+7)-1,h3=context.getHeightValue(baseX+7+dx,baseZ+7+dz)-1;const int minHeight=std::min(std::min(h0,h1),std::min(h2,h3));if(minHeight<60)return out;out.minY=minHeight+1;Grid grid(random);Placer placer(random,out.pieces);placer.create(Pos{baseX+8,out.minY,baseZ+8},rot,grid);out.bounds=conservativeBounds(out.pieces,out.minY);out.sizeable=!out.pieces.empty();return out;
}

void WoodlandMansionStructure::place(
    Start&start,WorldGenerationContext&context,JavaRandom&random,const Box&clip)
{
    if(!start.sizeable)return;StructureTemplateLibrary library;
    struct ActiveBox{Box box;};std::vector<ActiveBox> boxes;boxes.reserve(start.pieces.size());
    for(const auto&p:start.pieces)
    {
        const StructureTemplate&tpl=library.get("mansion/"+p.name);const Box tb=tpl.transformedBox(p.x,p.y,p.z,p.rotation,p.mirror);boxes.push_back({tb});if(!tb.intersects(clip))continue;
        tpl.place(context,p.x,p.y,p.z,p.rotation,clip,1.0f,nullptr,true,
            [&context,&random](int x,int y,int z,const TemplateNbt&nbt,Rotation rotation)
            {
                const std::string marker=nbt.string("metadata");
                if(marker.rfind("Chest",0)==0)
                {
                    Dir facing=Dir::North;if(marker=="ChestWest")facing=Dir::West;else if(marker=="ChestEast")facing=Dir::East;else if(marker=="ChestSouth")facing=Dir::South;facing=rotateDir(facing,rotation);std::string f=facing==Dir::North?"north":facing==Dir::South?"south":facing==Dir::West?"west":"east";const auto lootSeed=random.nextLong();context.setBlockState(x,y,z,chestState(f));context.assignStructureLoot(x,y,z,"minecraft:chests/woodland_mansion",lootSeed);
                }
                else if(marker=="Mage"||marker=="Warrior"){context.spawnStructureMob(marker=="Mage"?"evocation_illager":"vindication_illager",x,y,z);context.setBlockState(x,y,z,mansionState("minecraft:air"));}
            },p.mirror);
    }
    // WoodlandMansion.Start#generateStructure foundation columns. Use the
    // exact generated template boxes rather than the conservative Start box.
    const int y=start.minY;for(int x=clip.minX;x<=clip.maxX;++x)for(int z=clip.minZ;z<=clip.maxZ;++z){if(mc112::isAir(context.getBlockState(x,y,z)))continue;bool inComponent=false;for(const auto&b:boxes)if(b.box.contains(x,y,z)){inComponent=true;break;}if(!inComponent)continue;for(int yy=y-1;yy>1;--yy){auto cur=context.getBlockState(x,yy,z);if(!mc112::isAir(cur)&&!mc112::isLiquid(cur))break;context.setBlockState(x,yy,z,mansionState("minecraft:cobblestone"));}}
}
}
