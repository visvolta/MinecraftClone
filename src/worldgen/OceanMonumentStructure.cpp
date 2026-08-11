#include "worldgen/OceanMonumentStructure.h"

#include "worldgen/Vanilla112State.h"
#include "worldgen/WorldGenerationContext.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace mc112
{
namespace
{
enum Direction : int { Down=0, Up=1, North=2, South=3, West=4, East=5 };

constexpr int roomIndex(int x,int y,int z) noexcept{return y*25+z*5+x;}
constexpr int SourceIndex=roomIndex(2,0,0);
constexpr int TopConnectIndex=roomIndex(2,2,0);
constexpr int LeftWingConnectIndex=roomIndex(0,1,0);
constexpr int RightWingConnectIndex=roomIndex(4,1,0);

Direction oppositeDirection(Direction d) noexcept
{
    static constexpr std::array<Direction,6> opposite{Up,Down,South,North,East,West};
    return opposite[static_cast<std::size_t>(d)];
}

struct RoomDefinition
{
    int index=-1;
    std::array<RoomDefinition*,6> connections{};
    std::array<bool,6> hasOpening{};
    bool claimed=false;
    bool source=false;
    int scanIndex=0;

    explicit RoomDefinition(int value=-1):index(value){}
    void connect(Direction direction,RoomDefinition* other)
    {
        connections[static_cast<std::size_t>(direction)]=other;
        other->connections[static_cast<std::size_t>(oppositeDirection(direction))]=this;
    }
    void updateOpenings()
    {
        for(std::size_t i=0;i<6;++i)hasOpening[i]=connections[i]!=nullptr;
    }
    bool findSource(int scan)
    {
        if(source)return true;
        scanIndex=scan;
        for(std::size_t i=0;i<6;++i)
            if(connections[i]&&hasOpening[i]&&connections[i]->scanIndex!=scan&&connections[i]->findSource(scan))
                return true;
        return false;
    }
    [[nodiscard]] bool special()const noexcept{return index>=75;}
    [[nodiscard]] int openings()const noexcept
    {return static_cast<int>(std::count(hasOpening.begin(),hasOpening.end(),true));}
};

mc::content::BlockState rough(){return state("minecraft:prismarine",{{"variant","prismarine"}});}
mc::content::BlockState bricks(){return state("minecraft:prismarine",{{"variant","prismarine_bricks"}});}
mc::content::BlockState dark(){return state("minecraft:prismarine",{{"variant","dark_prismarine"}});}
mc::content::BlockState water(){return state("minecraft:water",{{"level","0"}});}
mc::content::BlockState air(){return state("minecraft:air");}
mc::content::BlockState seaLantern(){return state("minecraft:sea_lantern");}
mc::content::BlockState gold(){return state("minecraft:gold_block");}
mc::content::BlockState wetSponge(){return state("minecraft:sponge",{{"wet","true"}});}

class MonumentPiece:public Piece
{
public:
    RoomDefinition* room=nullptr;

    MonumentPiece()=default;
    MonumentPiece(Facing direction,const Box& bounds){facing=direction;box=bounds;}
    MonumentPiece(Facing direction,RoomDefinition* definition,int xRooms,int yRooms,int zRooms)
    {
        facing=direction;room=definition;
        const int i=definition->index,j=i%5,k=i/5%5,l=i/25;
        if(direction!=Facing::North&&direction!=Facing::South)
            box={0,0,0,zRooms*8-1,yRooms*4-1,xRooms*8-1};
        else
            box={0,0,0,xRooms*8-1,yRooms*4-1,zRooms*8-1};
        switch(direction)
        {
            case Facing::North:box.offset(j*8,l*4,-(k+zRooms)*8+1);break;
            case Facing::South:box.offset(j*8,l*4,k*8);break;
            case Facing::West:box.offset(-(k+zRooms)*8+1,l*4,j*8);break;
            case Facing::East:box.offset(k*8,l*4,j*8);break;
        }
    }

    void waterBox(WorldGenerationContext& context,const Box& clip,int x0,int y0,int z0,int x1,int y1,int z1,bool existingOnly)const
    {
        for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)for(int z=z0;z<=z1;++z)
        {
            if(existingOnly&&isAir(getBlock(context,clip,x,y,z)))continue;
            setBlock(context,clip,worldY(y)>=63?air():water(),x,y,z);
        }
    }
    void defaultFloor(WorldGenerationContext& context,const Box& clip,int x,int z,bool downOpening)const
    {
        if(!downOpening){fill(context,clip,x,0,z,x+7,0,z+7,rough(),rough());return;}
        fill(context,clip,x,0,z,x+2,0,z+7,rough(),rough());
        fill(context,clip,x+5,0,z,x+7,0,z+7,rough(),rough());
        fill(context,clip,x+3,0,z,x+4,0,z+2,rough(),rough());
        fill(context,clip,x+3,0,z+5,x+4,0,z+7,rough(),rough());
        fill(context,clip,x+3,0,z+2,x+4,0,z+2,bricks(),bricks());
        fill(context,clip,x+3,0,z+5,x+4,0,z+5,bricks(),bricks());
        fill(context,clip,x+2,0,z+3,x+2,0,z+4,bricks(),bricks());
        fill(context,clip,x+5,0,z+3,x+5,0,z+4,bricks(),bricks());
    }
    void boxOnWater(WorldGenerationContext& context,const Box& clip,int x0,int y0,int z0,int x1,int y1,int z1,mc::content::BlockState value)const
    {
        for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)for(int z=z0;z<=z1;++z)
            if(isWater(getBlock(context,clip,x,y,z)))setBlock(context,clip,value,x,y,z);
    }
    [[nodiscard]] bool intersectsLocal(const Box& clip,int x0,int z0,int x1,int z1)const noexcept
    {
        const int ax=worldX(x0,z0),az=worldZ(x0,z0),bx=worldX(x1,z1),bz=worldZ(x1,z1);
        return clip.intersectsXZ(std::min(ax,bx),std::min(az,bz),std::max(ax,bx),std::max(az,bz));
    }
};

class EntryRoom final:public MonumentPiece
{
public:
    EntryRoom(Facing f,RoomDefinition* r):MonumentPiece(f,r,1,1,1){}
    bool place(WorldGenerationContext& c,JavaRandom&,const Box& clip)override
    {
        fill(c,clip,0,3,0,2,3,7,bricks(),bricks());fill(c,clip,5,3,0,7,3,7,bricks(),bricks());
        fill(c,clip,0,2,0,1,2,7,bricks(),bricks());fill(c,clip,6,2,0,7,2,7,bricks(),bricks());
        fill(c,clip,0,1,0,0,1,7,bricks(),bricks());fill(c,clip,7,1,0,7,1,7,bricks(),bricks());
        fill(c,clip,0,1,7,7,3,7,bricks(),bricks());fill(c,clip,1,1,0,2,3,0,bricks(),bricks());
        fill(c,clip,5,1,0,6,3,0,bricks(),bricks());
        if(room->hasOpening[North])waterBox(c,clip,3,1,7,4,2,7,false);
        if(room->hasOpening[West])waterBox(c,clip,0,1,3,1,2,4,false);
        if(room->hasOpening[East])waterBox(c,clip,6,1,3,7,2,4,false);
        return true;
    }
};

class CoreRoom final:public MonumentPiece
{
public:
    CoreRoom(Facing f,RoomDefinition* r):MonumentPiece(f,r,2,2,2){}
    bool place(WorldGenerationContext& c,JavaRandom&,const Box& clip)override
    {
        boxOnWater(c,clip,1,8,0,14,8,14,rough());
        fill(c,clip,0,7,0,0,7,15,bricks(),bricks());fill(c,clip,15,7,0,15,7,15,bricks(),bricks());
        fill(c,clip,1,7,0,15,7,0,bricks(),bricks());fill(c,clip,1,7,15,14,7,15,bricks(),bricks());
        for(int y=1;y<=6;++y)
        {
            const auto s=(y==2||y==6)?rough():bricks();
            for(int x:{0,15}){fill(c,clip,x,y,0,x,y,1,s,s);fill(c,clip,x,y,6,x,y,9,s,s);fill(c,clip,x,y,14,x,y,15,s,s);}
            fill(c,clip,1,y,0,1,y,0,s,s);fill(c,clip,6,y,0,9,y,0,s,s);fill(c,clip,14,y,0,14,y,0,s,s);fill(c,clip,1,y,15,14,y,15,s,s);
        }
        fill(c,clip,6,3,6,9,6,9,dark(),dark());fill(c,clip,7,4,7,8,5,8,gold(),gold());
        for(int y=3;y<=6;y+=3)for(int x=6;x<=9;x+=3){setBlock(c,clip,seaLantern(),x,y,6);setBlock(c,clip,seaLantern(),x,y,9);}
        for(const auto [x,z]:std::array<std::pair<int,int>,8>{{{5,6},{5,9},{10,6},{10,9},{6,5},{9,5},{6,10},{9,10}}})fill(c,clip,x,1,z,x,2,z,bricks(),bricks());
        return true;
    }
};

enum class RoomKind:std::uint8_t{DoubleXY,DoubleYZ,DoubleZ,DoubleX,DoubleY,SimpleTop,Simple};

class DeferredRoom final:public MonumentPiece
{
public:
    RoomKind kind;
    int design=0;
    DeferredRoom(RoomKind k,Facing f,RoomDefinition* r,JavaRandom& random):MonumentPiece(),kind(k)
    {
        int xs=1,ys=1,zs=1;
        switch(kind){case RoomKind::DoubleXY:xs=2;ys=2;break;case RoomKind::DoubleYZ:ys=2;zs=2;break;case RoomKind::DoubleZ:zs=2;break;case RoomKind::DoubleX:xs=2;break;case RoomKind::DoubleY:ys=2;break;case RoomKind::Simple:design=random.nextInt(3);break;case RoomKind::SimpleTop:break;}
        facing=f;room=r;
        const int i=r->index,j=i%5,zz=i/5%5,yy=i/25;
        if(f!=Facing::North&&f!=Facing::South) box={0,0,0,zs*8-1,ys*4-1,xs*8-1};
        else box={0,0,0,xs*8-1,ys*4-1,zs*8-1};
        switch(f){case Facing::North:box.offset(j*8,yy*4,-(zz+zs)*8+1);break;case Facing::South:box.offset(j*8,yy*4,zz*8);break;case Facing::West:box.offset(-(zz+zs)*8+1,yy*4,j*8);break;case Facing::East:box.offset(zz*8,yy*4,j*8);break;}
    }
    bool place(WorldGenerationContext& c,JavaRandom& random,const Box& clip)override
    {
        switch(kind)
        {
            case RoomKind::DoubleX:return placeDoubleX(c,clip);
            case RoomKind::DoubleY:return placeDoubleY(c,clip);
            case RoomKind::DoubleZ:return placeDoubleZ(c,clip);
            case RoomKind::DoubleXY:return placeDoubleXY(c,clip);
            case RoomKind::DoubleYZ:return placeDoubleYZ(c,clip);
            case RoomKind::Simple:return placeSimple(c,random,clip);
            case RoomKind::SimpleTop:return placeSimpleTop(c,random,clip);
        }
        return true;
    }
private:
    bool placeDoubleX(WorldGenerationContext& c,const Box& clip)
    {
        RoomDefinition* east=room->connections[East];
        if(room->index/25>0){defaultFloor(c,clip,8,0,east->hasOpening[Down]);defaultFloor(c,clip,0,0,room->hasOpening[Down]);}
        if(room->connections[Up]==nullptr)boxOnWater(c,clip,1,4,1,7,4,6,rough());
        if(east->connections[Up]==nullptr)boxOnWater(c,clip,8,4,1,14,4,6,rough());
        fill(c,clip,0,3,0,0,3,7,bricks(),bricks());fill(c,clip,15,3,0,15,3,7,bricks(),bricks());
        fill(c,clip,1,3,0,15,3,0,bricks(),bricks());fill(c,clip,1,3,7,14,3,7,bricks(),bricks());
        fill(c,clip,0,2,0,0,2,7,rough(),rough());fill(c,clip,15,2,0,15,2,7,rough(),rough());
        fill(c,clip,1,2,0,15,2,0,rough(),rough());fill(c,clip,1,2,7,14,2,7,rough(),rough());
        fill(c,clip,0,1,0,0,1,7,bricks(),bricks());fill(c,clip,15,1,0,15,1,7,bricks(),bricks());
        fill(c,clip,1,1,0,15,1,0,bricks(),bricks());fill(c,clip,1,1,7,14,1,7,bricks(),bricks());
        fill(c,clip,5,1,0,10,1,4,bricks(),bricks());fill(c,clip,6,2,0,9,2,3,rough(),rough());
        fill(c,clip,5,3,0,10,3,4,bricks(),bricks());setBlock(c,clip,seaLantern(),6,2,3);setBlock(c,clip,seaLantern(),9,2,3);
        if(room->hasOpening[South])waterBox(c,clip,3,1,0,4,2,0,false);
        if(room->hasOpening[North])waterBox(c,clip,3,1,7,4,2,7,false);
        if(room->hasOpening[West])waterBox(c,clip,0,1,3,0,2,4,false);
        if(east->hasOpening[South])waterBox(c,clip,11,1,0,12,2,0,false);
        if(east->hasOpening[North])waterBox(c,clip,11,1,7,12,2,7,false);
        if(east->hasOpening[East])waterBox(c,clip,15,1,3,15,2,4,false);
        return true;
    }
    bool placeDoubleY(WorldGenerationContext& c,const Box& clip)
    {
        if(room->index/25>0)defaultFloor(c,clip,0,0,room->hasOpening[Down]);
        RoomDefinition* upper=room->connections[Up];
        if(upper->connections[Up]==nullptr)boxOnWater(c,clip,1,8,1,6,8,6,rough());
        fill(c,clip,0,4,0,0,4,7,bricks(),bricks());fill(c,clip,7,4,0,7,4,7,bricks(),bricks());
        fill(c,clip,1,4,0,6,4,0,bricks(),bricks());fill(c,clip,1,4,7,6,4,7,bricks(),bricks());
        fill(c,clip,2,4,1,2,4,2,bricks(),bricks());fill(c,clip,1,4,2,1,4,2,bricks(),bricks());
        fill(c,clip,5,4,1,5,4,2,bricks(),bricks());fill(c,clip,6,4,2,6,4,2,bricks(),bricks());
        fill(c,clip,2,4,5,2,4,6,bricks(),bricks());fill(c,clip,1,4,5,1,4,5,bricks(),bricks());
        fill(c,clip,5,4,5,5,4,6,bricks(),bricks());fill(c,clip,6,4,5,6,4,5,bricks(),bricks());
        RoomDefinition* current=room;
        for(int y=1;y<=5;y+=4)
        {
            const auto wall=[&](Direction d,int fixed,bool xAxis)
            {
                if(current->hasOpening[d])
                {
                    if(xAxis){fill(c,clip,2,y,fixed,2,y+2,fixed,bricks(),bricks());fill(c,clip,5,y,fixed,5,y+2,fixed,bricks(),bricks());fill(c,clip,3,y+2,fixed,4,y+2,fixed,bricks(),bricks());}
                    else{fill(c,clip,fixed,y,2,fixed,y+2,2,bricks(),bricks());fill(c,clip,fixed,y,5,fixed,y+2,5,bricks(),bricks());fill(c,clip,fixed,y+2,3,fixed,y+2,4,bricks(),bricks());}
                }
                else
                {
                    if(xAxis){fill(c,clip,0,y,fixed,7,y+2,fixed,bricks(),bricks());fill(c,clip,0,y+1,fixed,7,y+1,fixed,rough(),rough());}
                    else{fill(c,clip,fixed,y,0,fixed,y+2,7,bricks(),bricks());fill(c,clip,fixed,y+1,0,fixed,y+1,7,rough(),rough());}
                }
            };
            wall(South,0,true);wall(North,7,true);wall(West,0,false);wall(East,7,false);
            current=upper;
        }
        return true;
    }
    bool placeDoubleZ(WorldGenerationContext& c,const Box& clip)
    {
        RoomDefinition* north=room->connections[North];
        if(room->index/25>0){defaultFloor(c,clip,0,8,north->hasOpening[Down]);defaultFloor(c,clip,0,0,room->hasOpening[Down]);}
        if(room->connections[Up]==nullptr)boxOnWater(c,clip,1,4,1,6,4,7,rough());
        if(north->connections[Up]==nullptr)boxOnWater(c,clip,1,4,8,6,4,14,rough());
        fill(c,clip,0,3,0,0,3,15,bricks(),bricks());fill(c,clip,7,3,0,7,3,15,bricks(),bricks());
        fill(c,clip,1,3,0,7,3,0,bricks(),bricks());fill(c,clip,1,3,15,6,3,15,bricks(),bricks());
        fill(c,clip,0,2,0,0,2,15,rough(),rough());fill(c,clip,7,2,0,7,2,15,rough(),rough());
        fill(c,clip,1,2,0,7,2,0,rough(),rough());fill(c,clip,1,2,15,6,2,15,rough(),rough());
        fill(c,clip,0,1,0,0,1,15,bricks(),bricks());fill(c,clip,7,1,0,7,1,15,bricks(),bricks());
        fill(c,clip,1,1,0,7,1,0,bricks(),bricks());fill(c,clip,1,1,15,6,1,15,bricks(),bricks());
        for(const int z:{1,13}){fill(c,clip,1,1,z,1,1,z+1,bricks(),bricks());fill(c,clip,6,1,z,6,1,z+1,bricks(),bricks());fill(c,clip,1,3,z,1,3,z+1,bricks(),bricks());fill(c,clip,6,3,z,6,3,z+1,bricks(),bricks());}
        for(const int z:{6,9}){fill(c,clip,2,1,z,2,3,z,bricks(),bricks());fill(c,clip,5,1,z,5,3,z,bricks(),bricks());fill(c,clip,3,2,z,4,2,z,bricks(),bricks());}
        fill(c,clip,2,2,7,2,2,8,bricks(),bricks());fill(c,clip,5,2,7,5,2,8,bricks(),bricks());
        for(const int z:{5,10}){setBlock(c,clip,seaLantern(),2,2,z);setBlock(c,clip,seaLantern(),5,2,z);setBlock(c,clip,bricks(),2,3,z);setBlock(c,clip,bricks(),5,3,z);}
        if(room->hasOpening[South])waterBox(c,clip,3,1,0,4,2,0,false);
        if(room->hasOpening[East])waterBox(c,clip,7,1,3,7,2,4,false);
        if(room->hasOpening[West])waterBox(c,clip,0,1,3,0,2,4,false);
        if(north->hasOpening[North])waterBox(c,clip,3,1,15,4,2,15,false);
        if(north->hasOpening[West])waterBox(c,clip,0,1,11,0,2,12,false);
        if(north->hasOpening[East])waterBox(c,clip,7,1,11,7,2,12,false);
        return true;
    }
    bool placeDoubleXY(WorldGenerationContext& c,const Box& clip)
    {
        RoomDefinition* east=room->connections[East];
        RoomDefinition* upper=room->connections[Up];
        RoomDefinition* eastUpper=east->connections[Up];
        if(room->index/25>0){defaultFloor(c,clip,8,0,east->hasOpening[Down]);defaultFloor(c,clip,0,0,room->hasOpening[Down]);}
        if(upper->connections[Up]==nullptr)boxOnWater(c,clip,1,8,1,7,8,6,rough());
        if(eastUpper->connections[Up]==nullptr)boxOnWater(c,clip,8,8,1,14,8,6,rough());
        for(int y=1;y<=7;++y)
        {
            const auto s=(y==2||y==6)?rough():bricks();
            fill(c,clip,0,y,0,0,y,7,s,s);fill(c,clip,15,y,0,15,y,7,s,s);
            fill(c,clip,1,y,0,15,y,0,s,s);fill(c,clip,1,y,7,14,y,7,s,s);
        }
        fill(c,clip,2,1,3,2,7,4,bricks(),bricks());fill(c,clip,3,1,2,4,7,2,bricks(),bricks());fill(c,clip,3,1,5,4,7,5,bricks(),bricks());
        fill(c,clip,13,1,3,13,7,4,bricks(),bricks());fill(c,clip,11,1,2,12,7,2,bricks(),bricks());fill(c,clip,11,1,5,12,7,5,bricks(),bricks());
        fill(c,clip,5,1,3,5,3,4,bricks(),bricks());fill(c,clip,10,1,3,10,3,4,bricks(),bricks());
        fill(c,clip,5,7,2,10,7,5,bricks(),bricks());fill(c,clip,5,5,2,5,7,2,bricks(),bricks());fill(c,clip,10,5,2,10,7,2,bricks(),bricks());
        fill(c,clip,5,5,5,5,7,5,bricks(),bricks());fill(c,clip,10,5,5,10,7,5,bricks(),bricks());
        for(const auto [x,z]:std::array<std::pair<int,int>,4>{{{6,2},{9,2},{6,5},{9,5}}})setBlock(c,clip,bricks(),x,6,z);
        fill(c,clip,5,4,3,6,4,4,bricks(),bricks());fill(c,clip,9,4,3,10,4,4,bricks(),bricks());
        for(const auto [x,z]:std::array<std::pair<int,int>,4>{{{5,2},{5,5},{10,2},{10,5}}})setBlock(c,clip,seaLantern(),x,4,z);
        if(room->hasOpening[South])waterBox(c,clip,3,1,0,4,2,0,false);
        if(room->hasOpening[North])waterBox(c,clip,3,1,7,4,2,7,false);
        if(room->hasOpening[West])waterBox(c,clip,0,1,3,0,2,4,false);
        if(east->hasOpening[South])waterBox(c,clip,11,1,0,12,2,0,false);
        if(east->hasOpening[North])waterBox(c,clip,11,1,7,12,2,7,false);
        if(east->hasOpening[East])waterBox(c,clip,15,1,3,15,2,4,false);
        if(upper->hasOpening[South])waterBox(c,clip,3,5,0,4,6,0,false);
        if(upper->hasOpening[North])waterBox(c,clip,3,5,7,4,6,7,false);
        if(upper->hasOpening[West])waterBox(c,clip,0,5,3,0,6,4,false);
        if(eastUpper->hasOpening[South])waterBox(c,clip,11,5,0,12,6,0,false);
        if(eastUpper->hasOpening[North])waterBox(c,clip,11,5,7,12,6,7,false);
        if(eastUpper->hasOpening[East])waterBox(c,clip,15,5,3,15,6,4,false);
        return true;
    }
    bool placeDoubleYZ(WorldGenerationContext& c,const Box& clip)
    {
        RoomDefinition* north=room->connections[North];
        RoomDefinition* upper=room->connections[Up];
        RoomDefinition* northUpper=north->connections[Up];
        if(room->index/25>0){defaultFloor(c,clip,0,8,north->hasOpening[Down]);defaultFloor(c,clip,0,0,room->hasOpening[Down]);}
        if(upper->connections[Up]==nullptr)boxOnWater(c,clip,1,8,1,6,8,7,rough());
        if(northUpper->connections[Up]==nullptr)boxOnWater(c,clip,1,8,8,6,8,14,rough());
        for(int y=1;y<=7;++y)
        {
            const auto s=(y==2||y==6)?rough():bricks();
            fill(c,clip,0,y,0,0,y,15,s,s);fill(c,clip,7,y,0,7,y,15,s,s);
            fill(c,clip,1,y,0,6,y,0,s,s);fill(c,clip,1,y,15,6,y,15,s,s);
        }
        for(int y=1;y<=7;++y)
        {
            const auto s=(y==2||y==6)?seaLantern():dark();
            fill(c,clip,3,y,7,4,y,8,s,s);
        }
        if(room->hasOpening[South])waterBox(c,clip,3,1,0,4,2,0,false);
        if(room->hasOpening[East])waterBox(c,clip,7,1,3,7,2,4,false);
        if(room->hasOpening[West])waterBox(c,clip,0,1,3,0,2,4,false);
        if(north->hasOpening[North])waterBox(c,clip,3,1,15,4,2,15,false);
        if(north->hasOpening[West])waterBox(c,clip,0,1,11,0,2,12,false);
        if(north->hasOpening[East])waterBox(c,clip,7,1,11,7,2,12,false);
        if(upper->hasOpening[South])waterBox(c,clip,3,5,0,4,6,0,false);
        if(upper->hasOpening[East]){waterBox(c,clip,7,5,3,7,6,4,false);fill(c,clip,5,4,2,6,4,5,bricks(),bricks());fill(c,clip,6,1,2,6,3,2,bricks(),bricks());fill(c,clip,6,1,5,6,3,5,bricks(),bricks());}
        if(upper->hasOpening[West]){waterBox(c,clip,0,5,3,0,6,4,false);fill(c,clip,1,4,2,2,4,5,bricks(),bricks());fill(c,clip,1,1,2,1,3,2,bricks(),bricks());fill(c,clip,1,1,5,1,3,5,bricks(),bricks());}
        if(northUpper->hasOpening[North])waterBox(c,clip,3,5,15,4,6,15,false);
        if(northUpper->hasOpening[West]){waterBox(c,clip,0,5,11,0,6,12,false);fill(c,clip,1,4,10,2,4,13,bricks(),bricks());fill(c,clip,1,1,10,1,3,10,bricks(),bricks());fill(c,clip,1,1,13,1,3,13,bricks(),bricks());}
        if(northUpper->hasOpening[East]){waterBox(c,clip,7,5,11,7,6,12,false);fill(c,clip,5,4,10,6,4,13,bricks(),bricks());fill(c,clip,6,1,10,6,3,10,bricks(),bricks());fill(c,clip,6,1,13,6,3,13,bricks(),bricks());}
        return true;
    }
    bool placeSimple(WorldGenerationContext& c,JavaRandom& random,const Box& clip)
    {
        if(room->index/25>0)defaultFloor(c,clip,0,0,room->hasOpening[Down]);
        if(room->connections[Up]==nullptr)boxOnWater(c,clip,1,4,1,6,4,6,rough());
        const bool pillar=design!=0&&random.nextBoolean()&&!room->hasOpening[Down]&&!room->hasOpening[Up]&&room->openings()>1;
        if(design==0)
        {
            const std::array<std::pair<int,int>,4> corners{{{0,0},{5,0},{0,5},{5,5}}};
            for(auto [x,z]:corners)
            {
                fill(c,clip,x,1,z,x+2,1,z+2,bricks(),bricks());fill(c,clip,x,3,z,x+2,3,z+2,bricks(),bricks());
                const int edgeX=(x==0?x:x+2), edgeZ=(z==0?z:z+2);
                fill(c,clip,edgeX,2,z,edgeX,2,z+2,rough(),rough());fill(c,clip,x,2,edgeZ,x+2,2,edgeZ,rough(),rough());
                setBlock(c,clip,seaLantern(),x+(x==0?1:1),2,z+(z==0?1:1));
            }
            if(room->hasOpening[South])fill(c,clip,3,3,0,4,3,0,bricks(),bricks());else{fill(c,clip,3,3,0,4,3,1,bricks(),bricks());fill(c,clip,3,2,0,4,2,0,rough(),rough());fill(c,clip,3,1,0,4,1,1,bricks(),bricks());}
            if(room->hasOpening[North])fill(c,clip,3,3,7,4,3,7,bricks(),bricks());else{fill(c,clip,3,3,6,4,3,7,bricks(),bricks());fill(c,clip,3,2,7,4,2,7,rough(),rough());fill(c,clip,3,1,6,4,1,7,bricks(),bricks());}
            if(room->hasOpening[West])fill(c,clip,0,3,3,0,3,4,bricks(),bricks());else{fill(c,clip,0,3,3,1,3,4,bricks(),bricks());fill(c,clip,0,2,3,0,2,4,rough(),rough());fill(c,clip,0,1,3,1,1,4,bricks(),bricks());}
            if(room->hasOpening[East])fill(c,clip,7,3,3,7,3,4,bricks(),bricks());else{fill(c,clip,6,3,3,7,3,4,bricks(),bricks());fill(c,clip,7,2,3,7,2,4,rough(),rough());fill(c,clip,6,1,3,7,1,4,bricks(),bricks());}
        }
        else if(design==1)
        {
            for(auto [x,z]:std::array<std::pair<int,int>,4>{{{2,2},{2,5},{5,5},{5,2}}}){fill(c,clip,x,1,z,x,3,z,bricks(),bricks());setBlock(c,clip,seaLantern(),x,2,z);}
            fill(c,clip,0,1,0,1,3,0,bricks(),bricks());fill(c,clip,0,1,1,0,3,1,bricks(),bricks());fill(c,clip,0,1,7,1,3,7,bricks(),bricks());fill(c,clip,0,1,6,0,3,6,bricks(),bricks());
            fill(c,clip,6,1,7,7,3,7,bricks(),bricks());fill(c,clip,7,1,6,7,3,6,bricks(),bricks());fill(c,clip,6,1,0,7,3,0,bricks(),bricks());fill(c,clip,7,1,1,7,3,1,bricks(),bricks());
            for(auto [x,z]:std::array<std::pair<int,int>,8>{{{1,0},{0,1},{1,7},{0,6},{6,7},{7,6},{6,0},{7,1}}})setBlock(c,clip,rough(),x,2,z);
            if(!room->hasOpening[South]){fill(c,clip,1,3,0,6,3,0,bricks(),bricks());fill(c,clip,1,2,0,6,2,0,rough(),rough());fill(c,clip,1,1,0,6,1,0,bricks(),bricks());}
            if(!room->hasOpening[North]){fill(c,clip,1,3,7,6,3,7,bricks(),bricks());fill(c,clip,1,2,7,6,2,7,rough(),rough());fill(c,clip,1,1,7,6,1,7,bricks(),bricks());}
            if(!room->hasOpening[West]){fill(c,clip,0,3,1,0,3,6,bricks(),bricks());fill(c,clip,0,2,1,0,2,6,rough(),rough());fill(c,clip,0,1,1,0,1,6,bricks(),bricks());}
            if(!room->hasOpening[East]){fill(c,clip,7,3,1,7,3,6,bricks(),bricks());fill(c,clip,7,2,1,7,2,6,rough(),rough());fill(c,clip,7,1,1,7,1,6,bricks(),bricks());}
        }
        else
        {
            fill(c,clip,0,1,0,0,1,7,bricks(),bricks());fill(c,clip,7,1,0,7,1,7,bricks(),bricks());fill(c,clip,1,1,0,6,1,0,bricks(),bricks());fill(c,clip,1,1,7,6,1,7,bricks(),bricks());
            fill(c,clip,0,2,0,0,2,7,dark(),dark());fill(c,clip,7,2,0,7,2,7,dark(),dark());fill(c,clip,1,2,0,6,2,0,dark(),dark());fill(c,clip,1,2,7,6,2,7,dark(),dark());
            fill(c,clip,0,3,0,0,3,7,bricks(),bricks());fill(c,clip,7,3,0,7,3,7,bricks(),bricks());fill(c,clip,1,3,0,6,3,0,bricks(),bricks());fill(c,clip,1,3,7,6,3,7,bricks(),bricks());
            fill(c,clip,0,1,3,0,2,4,dark(),dark());fill(c,clip,7,1,3,7,2,4,dark(),dark());fill(c,clip,3,1,0,4,2,0,dark(),dark());fill(c,clip,3,1,7,4,2,7,dark(),dark());
            if(room->hasOpening[South])waterBox(c,clip,3,1,0,4,2,0,false);if(room->hasOpening[North])waterBox(c,clip,3,1,7,4,2,7,false);if(room->hasOpening[West])waterBox(c,clip,0,1,3,0,2,4,false);if(room->hasOpening[East])waterBox(c,clip,7,1,3,7,2,4,false);
        }
        if(pillar){fill(c,clip,3,1,3,4,1,4,bricks(),bricks());fill(c,clip,3,2,3,4,2,4,rough(),rough());fill(c,clip,3,3,3,4,3,4,bricks(),bricks());}
        return true;
    }
    bool placeSimpleTop(WorldGenerationContext& c,JavaRandom& random,const Box& clip)
    {
        if(room->index/25>0)defaultFloor(c,clip,0,0,room->hasOpening[Down]);
        if(room->connections[Up]==nullptr)boxOnWater(c,clip,1,4,1,6,4,6,rough());
        for(int x=1;x<=6;++x)for(int z=1;z<=6;++z)if(random.nextInt(3)!=0)
        {
            const int y=2+(random.nextInt(4)==0?0:1);fill(c,clip,x,y,z,x,3,z,wetSponge(),wetSponge());
        }
        fill(c,clip,0,1,0,0,1,7,bricks(),bricks());fill(c,clip,7,1,0,7,1,7,bricks(),bricks());fill(c,clip,1,1,0,6,1,0,bricks(),bricks());fill(c,clip,1,1,7,6,1,7,bricks(),bricks());
        fill(c,clip,0,2,0,0,2,7,dark(),dark());fill(c,clip,7,2,0,7,2,7,dark(),dark());fill(c,clip,1,2,0,6,2,0,dark(),dark());fill(c,clip,1,2,7,6,2,7,dark(),dark());
        fill(c,clip,0,3,0,0,3,7,bricks(),bricks());fill(c,clip,7,3,0,7,3,7,bricks(),bricks());fill(c,clip,1,3,0,6,3,0,bricks(),bricks());fill(c,clip,1,3,7,6,3,7,bricks(),bricks());
        fill(c,clip,0,1,3,0,2,4,dark(),dark());fill(c,clip,7,1,3,7,2,4,dark(),dark());fill(c,clip,3,1,0,4,2,0,dark(),dark());fill(c,clip,3,1,7,4,2,7,dark(),dark());
        if(room->hasOpening[South])waterBox(c,clip,3,1,0,4,2,0,false);
        return true;
    }
};

class WingRoom final:public MonumentPiece
{
public:
    int design=0;
    WingRoom(Facing f,const Box& b,int seed):MonumentPiece(f,b),design(seed&1){}
    bool place(WorldGenerationContext& c,JavaRandom&,const Box& clip)override
    {
        if(design==0)
        {
            for(int i=0;i<4;++i)fill(c,clip,10-i,3-i,20-i,12+i,3-i,20,bricks(),bricks());
            fill(c,clip,7,0,6,15,0,16,bricks(),bricks());fill(c,clip,6,0,6,6,3,20,bricks(),bricks());fill(c,clip,16,0,6,16,3,20,bricks(),bricks());
            fill(c,clip,7,1,7,7,1,20,bricks(),bricks());fill(c,clip,15,1,7,15,1,20,bricks(),bricks());
            fill(c,clip,7,1,6,9,3,6,bricks(),bricks());fill(c,clip,13,1,6,15,3,6,bricks(),bricks());
            fill(c,clip,8,1,7,9,1,7,bricks(),bricks());fill(c,clip,13,1,7,14,1,7,bricks(),bricks());
            fill(c,clip,9,0,5,13,0,5,bricks(),bricks());fill(c,clip,10,0,7,12,0,7,dark(),dark());fill(c,clip,8,0,10,8,0,12,dark(),dark());fill(c,clip,14,0,10,14,0,12,dark(),dark());
            for(int z=18;z>=7;z-=3){setBlock(c,clip,seaLantern(),6,3,z);setBlock(c,clip,seaLantern(),16,3,z);}
            for(auto [x,z]:std::array<std::pair<int,int>,4>{{{10,10},{12,10},{10,12},{12,12}}})setBlock(c,clip,seaLantern(),x,0,z);
            setBlock(c,clip,seaLantern(),8,3,6);setBlock(c,clip,seaLantern(),14,3,6);
            for(auto [x,z]:std::array<std::pair<int,int>,4>{{{4,4},{18,4},{4,18},{18,18}}}){setBlock(c,clip,bricks(),x,2,z);setBlock(c,clip,seaLantern(),x,1,z);setBlock(c,clip,bricks(),x,0,z);}
            setBlock(c,clip,bricks(),9,7,20);setBlock(c,clip,bricks(),13,7,20);
            fill(c,clip,6,0,21,7,4,21,bricks(),bricks());fill(c,clip,15,0,21,16,4,21,bricks(),bricks());
            {const int wx=worldX(11,16),wy=worldY(2),wz=worldZ(11,16);if(clip.contains(wx,wy,wz))c.spawnStructureMob("elder_guardian",wx,wy,wz);}
        }
        else
        {
            fill(c,clip,9,3,18,13,3,20,bricks(),bricks());fill(c,clip,9,0,18,9,2,18,bricks(),bricks());fill(c,clip,13,0,18,13,2,18,bricks(),bricks());
            for(int x:{9,13}){setBlock(c,clip,bricks(),x,6,20);setBlock(c,clip,seaLantern(),x,5,20);setBlock(c,clip,bricks(),x,4,20);}
            fill(c,clip,7,3,7,15,3,14,bricks(),bricks());
            for(int x:{10,12}){fill(c,clip,x,0,10,x,6,10,bricks(),bricks());fill(c,clip,x,0,12,x,6,12,bricks(),bricks());setBlock(c,clip,seaLantern(),x,0,10);setBlock(c,clip,seaLantern(),x,0,12);setBlock(c,clip,seaLantern(),x,4,10);setBlock(c,clip,seaLantern(),x,4,12);}
            for(int x:{8,14}){fill(c,clip,x,0,7,x,2,7,bricks(),bricks());fill(c,clip,x,0,14,x,2,14,bricks(),bricks());}
            fill(c,clip,8,3,8,8,3,13,dark(),dark());fill(c,clip,14,3,8,14,3,13,dark(),dark());
            {const int wx=worldX(11,13),wy=worldY(5),wz=worldZ(11,13);if(clip.contains(wx,wy,wz))c.spawnStructureMob("elder_guardian",wx,wy,wz);}
        }
        return true;
    }
};
class Penthouse final:public MonumentPiece
{
public:
    Penthouse(Facing f,const Box& b):MonumentPiece(f,b){}
    bool place(WorldGenerationContext& c,JavaRandom&,const Box& clip)override
    {
        fill(c,clip,2,-1,2,11,-1,11,bricks(),bricks());fill(c,clip,0,-1,0,1,-1,11,rough(),rough());fill(c,clip,12,-1,0,13,-1,11,rough(),rough());fill(c,clip,2,-1,0,11,-1,1,rough(),rough());fill(c,clip,2,-1,12,11,-1,13,rough(),rough());
        fill(c,clip,0,0,0,0,0,13,bricks(),bricks());fill(c,clip,13,0,0,13,0,13,bricks(),bricks());fill(c,clip,1,0,0,12,0,0,bricks(),bricks());fill(c,clip,1,0,13,12,0,13,bricks(),bricks());
        for(int i=2;i<=11;i+=3){setBlock(c,clip,seaLantern(),0,0,i);setBlock(c,clip,seaLantern(),13,0,i);setBlock(c,clip,seaLantern(),i,0,0);}
        fill(c,clip,2,0,3,4,0,9,bricks(),bricks());fill(c,clip,9,0,3,11,0,9,bricks(),bricks());fill(c,clip,4,0,9,9,0,11,bricks(),bricks());
        setBlock(c,clip,bricks(),5,0,8);setBlock(c,clip,bricks(),8,0,8);setBlock(c,clip,bricks(),10,0,10);setBlock(c,clip,bricks(),3,0,10);
        fill(c,clip,3,0,3,3,0,7,dark(),dark());fill(c,clip,10,0,3,10,0,7,dark(),dark());fill(c,clip,6,0,10,7,0,10,dark(),dark());
        for(int x:{3,10})for(int z=2;z<=8;z+=3)fill(c,clip,x,0,z,x,2,z,bricks(),bricks());
        fill(c,clip,5,0,10,5,2,10,bricks(),bricks());fill(c,clip,8,0,10,8,2,10,bricks(),bricks());fill(c,clip,6,-1,7,7,-1,8,dark(),dark());waterBox(c,clip,6,-1,3,7,-1,4,false);
        {const int wx=worldX(6,6),wy=worldY(1),wz=worldZ(6,6);if(clip.contains(wx,wy,wz))c.spawnStructureMob("elder_guardian",wx,wy,wz);}
        return true;
    }
};

class MonumentBuilding final:public MonumentPiece
{
public:
    std::array<std::unique_ptr<RoomDefinition>,78> ownedRooms{};
    std::vector<std::unique_ptr<MonumentPiece>> children;
    RoomDefinition* sourceRoom=nullptr;
    RoomDefinition* coreRoom=nullptr;

    MonumentBuilding(JavaRandom& random,int x,int z,Facing f):MonumentPiece(f,{x,39,z,x+57,61,z+57})
    {
        auto rooms=generateRoomGraph(random);
        sourceRoom->claimed=true;
        children.push_back(std::make_unique<EntryRoom>(f,sourceRoom));
        children.push_back(std::make_unique<CoreRoom>(f,coreRoom));
        for(RoomDefinition* r:rooms)
        {
            if(r->claimed||r->special())continue;
            if(fitsXY(r)){claimXY(r);children.push_back(std::make_unique<DeferredRoom>(RoomKind::DoubleXY,f,r,random));continue;}
            if(fitsYZ(r)){claimYZ(r);children.push_back(std::make_unique<DeferredRoom>(RoomKind::DoubleYZ,f,r,random));continue;}
            if(fitsZ(r)){r->claimed=r->connections[North]->claimed=true;children.push_back(std::make_unique<DeferredRoom>(RoomKind::DoubleZ,f,r,random));continue;}
            if(fitsX(r)){r->claimed=r->connections[East]->claimed=true;children.push_back(std::make_unique<DeferredRoom>(RoomKind::DoubleX,f,r,random));continue;}
            if(fitsY(r)){r->claimed=r->connections[Up]->claimed=true;children.push_back(std::make_unique<DeferredRoom>(RoomKind::DoubleY,f,r,random));continue;}
            if(fitsTop(r)){r->claimed=true;children.push_back(std::make_unique<DeferredRoom>(RoomKind::SimpleTop,f,r,random));continue;}
            r->claimed=true;children.push_back(std::make_unique<DeferredRoom>(RoomKind::Simple,f,r,random));
        }
        const int oy=box.minY,ox=worldX(9,22),oz=worldZ(9,22);
        for(auto& child:children)child->offset(ox,oy,oz);
        const auto proper=[this](int x0,int y0,int z0,int x1,int y1,int z1)
        {
            return Box{std::min(worldX(x0,z0),worldX(x1,z1)),std::min(worldY(y0),worldY(y1)),std::min(worldZ(x0,z0),worldZ(x1,z1)),
                       std::max(worldX(x0,z0),worldX(x1,z1)),std::max(worldY(y0),worldY(y1)),std::max(worldZ(x0,z0),worldZ(x1,z1))};
        };
        const Box left=proper(1,1,1,23,8,21),right=proper(34,1,1,56,8,21),top=proper(22,13,22,35,17,35);
        int seed=random.nextInt();
        children.push_back(std::make_unique<WingRoom>(f,left,seed++));
        children.push_back(std::make_unique<WingRoom>(f,right,seed++));
        children.push_back(std::make_unique<Penthouse>(f,top));
    }

    std::vector<RoomDefinition*> generateRoomGraph(JavaRandom& random)
    {
        auto make=[this](int idx)->RoomDefinition*{ownedRooms[static_cast<std::size_t>(idx)]=std::make_unique<RoomDefinition>(idx);return ownedRooms[static_cast<std::size_t>(idx)].get();};
        std::array<RoomDefinition*,75> grid{};
        for(int x=0;x<5;++x)for(int z=0;z<4;++z)grid[roomIndex(x,0,z)]=make(roomIndex(x,0,z));
        for(int x=0;x<5;++x)for(int z=0;z<4;++z)grid[roomIndex(x,1,z)]=make(roomIndex(x,1,z));
        for(int x=1;x<4;++x)for(int z=0;z<2;++z)grid[roomIndex(x,2,z)]=make(roomIndex(x,2,z));
        sourceRoom=grid[SourceIndex];
        const std::array<std::array<int,3>,6> step{{{{0,-1,0}},{{0,1,0}},{{0,0,-1}},{{0,0,1}},{{-1,0,0}},{{1,0,0}}}};
        for(int x=0;x<5;++x)for(int z=0;z<5;++z)for(int y=0;y<3;++y)
        {
            RoomDefinition* r=grid[roomIndex(x,y,z)];if(!r)continue;
            for(int di=0;di<6;++di)
            {
                const int nx=x+step[di][0],ny=y+step[di][1],nz=z+step[di][2];
                if(nx<0||nx>=5||ny<0||ny>=3||nz<0||nz>=5)continue;
                RoomDefinition* n=grid[roomIndex(nx,ny,nz)];if(!n)continue;
                const Direction d=static_cast<Direction>(di);
                r->connect(nz==z?d:oppositeDirection(d),n);
            }
        }
        auto top=std::make_unique<RoomDefinition>(1003),left=std::make_unique<RoomDefinition>(1001),right=std::make_unique<RoomDefinition>(1002);
        RoomDefinition* topP=top.get(),*leftP=left.get(),*rightP=right.get();
        ownedRooms[75]=std::move(top);ownedRooms[76]=std::move(left);ownedRooms[77]=std::move(right);
        grid[TopConnectIndex]->connect(Up,topP);grid[LeftWingConnectIndex]->connect(South,leftP);grid[RightWingConnectIndex]->connect(South,rightP);
        topP->claimed=leftP->claimed=rightP->claimed=true;sourceRoom->source=true;
        coreRoom=grid[roomIndex(random.nextInt(4),0,2)];coreRoom->claimed=true;
        auto claim=[](RoomDefinition* r){if(r)r->claimed=true;};
        claim(coreRoom->connections[East]);claim(coreRoom->connections[North]);claim(coreRoom->connections[East]->connections[North]);
        claim(coreRoom->connections[Up]);claim(coreRoom->connections[East]->connections[Up]);claim(coreRoom->connections[North]->connections[Up]);
        claim(coreRoom->connections[East]->connections[North]->connections[Up]);
        std::vector<RoomDefinition*> list;
        for(RoomDefinition* r:grid)if(r){r->updateOpenings();list.push_back(r);}topP->updateOpenings();
        for(std::size_t i=list.size();i>1;--i)std::swap(list[i-1],list[static_cast<std::size_t>(random.nextInt(static_cast<int>(i)))]);
        int scan=1;
        for(RoomDefinition* r:list)
        {
            int removed=0,attempts=0;
            while(removed<2&&attempts<5)
            {
                ++attempts;const int d=random.nextInt(6);if(!r->hasOpening[static_cast<std::size_t>(d)])continue;
                RoomDefinition* n=r->connections[static_cast<std::size_t>(d)];const int od=static_cast<int>(oppositeDirection(static_cast<Direction>(d)));
                r->hasOpening[static_cast<std::size_t>(d)]=false;n->hasOpening[static_cast<std::size_t>(od)]=false;
                if(r->findSource(scan++)&&n->findSource(scan++))++removed;
                else{r->hasOpening[static_cast<std::size_t>(d)]=true;n->hasOpening[static_cast<std::size_t>(od)]=true;}
            }
        }
        list.push_back(topP);list.push_back(leftP);list.push_back(rightP);return list;
    }

    static bool fitsXY(RoomDefinition* r){return r->hasOpening[East]&&r->connections[East]&&!r->connections[East]->claimed&&r->hasOpening[Up]&&r->connections[Up]&&!r->connections[Up]->claimed&&r->connections[East]->hasOpening[Up]&&r->connections[East]->connections[Up]&&!r->connections[East]->connections[Up]->claimed;}
    static bool fitsYZ(RoomDefinition* r){return r->hasOpening[North]&&r->connections[North]&&!r->connections[North]->claimed&&r->hasOpening[Up]&&r->connections[Up]&&!r->connections[Up]->claimed&&r->connections[North]->hasOpening[Up]&&r->connections[North]->connections[Up]&&!r->connections[North]->connections[Up]->claimed;}
    static bool fitsZ(RoomDefinition* r){return r->hasOpening[North]&&r->connections[North]&&!r->connections[North]->claimed;}
    static bool fitsX(RoomDefinition* r){return r->hasOpening[East]&&r->connections[East]&&!r->connections[East]->claimed;}
    static bool fitsY(RoomDefinition* r){return r->hasOpening[Up]&&r->connections[Up]&&!r->connections[Up]->claimed;}
    static bool fitsTop(RoomDefinition* r){return !r->hasOpening[West]&&!r->hasOpening[East]&&!r->hasOpening[North]&&!r->hasOpening[South]&&!r->hasOpening[Up];}
    static void claimXY(RoomDefinition* r){r->claimed=true;r->connections[East]->claimed=true;r->connections[Up]->claimed=true;r->connections[East]->connections[Up]->claimed=true;}
    static void claimYZ(RoomDefinition* r){r->claimed=true;r->connections[North]->claimed=true;r->connections[Up]->claimed=true;r->connections[North]->connections[Up]->claimed=true;}

    void generateWing(bool mirrored,int offset,WorldGenerationContext& c,const Box& clip)
    {
        fill(c,clip,offset,0,0,offset+24,0,20,rough(),rough());waterBox(c,clip,offset,1,0,offset+24,10,20,false);
        for(int j=0;j<4;++j)
        {
            fill(c,clip,offset+j,j+1,j,offset+j,j+1,20,bricks(),bricks());fill(c,clip,offset+j+7,j+5,j+7,offset+j+7,j+5,20,bricks(),bricks());
            fill(c,clip,offset+17-j,j+5,j+7,offset+17-j,j+5,20,bricks(),bricks());fill(c,clip,offset+24-j,j+1,j,offset+24-j,j+1,20,bricks(),bricks());
            fill(c,clip,offset+j+1,j+1,j,offset+23-j,j+1,j,bricks(),bricks());fill(c,clip,offset+j+8,j+5,j+7,offset+16-j,j+5,j+7,bricks(),bricks());
        }
        fill(c,clip,offset+4,4,4,offset+6,4,20,rough(),rough());fill(c,clip,offset+7,4,4,offset+17,4,6,rough(),rough());fill(c,clip,offset+18,4,4,offset+20,4,20,rough(),rough());
        fill(c,clip,offset+11,8,11,offset+13,8,20,rough(),rough());for(int z:{12,15,18})setBlock(c,clip,bricks(),offset+12,9,z);
        const int a=offset+(mirrored?19:5),b=offset+(mirrored?5:19);for(int z=20;z>=5;z-=3)setBlock(c,clip,bricks(),a,5,z);for(int z=19;z>=7;z-=3)setBlock(c,clip,bricks(),b,5,z);
        for(int n=0;n<4;++n)setBlock(c,clip,bricks(),mirrored?offset+(24-(17-n*3)):offset+17-n*3,5,5);setBlock(c,clip,bricks(),b,5,5);
        fill(c,clip,offset+11,1,12,offset+13,7,12,rough(),rough());fill(c,clip,offset+12,1,11,offset+12,7,13,rough(),rough());
    }
    void generateEntranceArchs(WorldGenerationContext& c,const Box& clip)
    {
        waterBox(c,clip,25,0,0,32,8,20,false);
        for(int i=0;i<4;++i){const int z=5+i*4;fill(c,clip,24,2,z,24,4,z,bricks(),bricks());fill(c,clip,22,4,z,23,4,z,bricks(),bricks());setBlock(c,clip,bricks(),25,5,z);setBlock(c,clip,bricks(),26,6,z);setBlock(c,clip,seaLantern(),26,5,z);fill(c,clip,33,2,z,33,4,z,bricks(),bricks());fill(c,clip,34,4,z,35,4,z,bricks(),bricks());setBlock(c,clip,bricks(),32,5,z);setBlock(c,clip,bricks(),31,6,z);setBlock(c,clip,seaLantern(),31,5,z);fill(c,clip,27,6,z,30,6,z,rough(),rough());}
    }
    void generateEntranceWall(WorldGenerationContext& c,const Box& clip)
    {
        fill(c,clip,15,0,21,42,0,21,rough(),rough());waterBox(c,clip,26,1,21,31,3,21,false);
        fill(c,clip,21,12,21,36,12,21,rough(),rough());fill(c,clip,17,11,21,40,11,21,rough(),rough());fill(c,clip,16,10,21,41,10,21,rough(),rough());fill(c,clip,15,7,21,42,9,21,rough(),rough());fill(c,clip,16,6,21,41,6,21,rough(),rough());fill(c,clip,17,5,21,40,5,21,rough(),rough());fill(c,clip,21,4,21,36,4,21,rough(),rough());
        fill(c,clip,22,3,21,26,3,21,rough(),rough());fill(c,clip,31,3,21,35,3,21,rough(),rough());fill(c,clip,23,2,21,25,2,21,rough(),rough());fill(c,clip,32,2,21,34,2,21,rough(),rough());fill(c,clip,28,4,20,29,4,21,bricks(),bricks());
        for(auto [x,y]:std::array<std::pair<int,int>,6>{{{27,3},{30,3},{26,2},{31,2},{25,1},{32,1}}})setBlock(c,clip,bricks(),x,y,21);
        for(int i=0;i<7;++i){setBlock(c,clip,dark(),28-i,6+i,21);setBlock(c,clip,dark(),29+i,6+i,21);}for(int i=0;i<4;++i){setBlock(c,clip,dark(),28-i,9+i,21);setBlock(c,clip,dark(),29+i,9+i,21);}setBlock(c,clip,dark(),28,12,21);setBlock(c,clip,dark(),29,12,21);
        for(int k=0;k<3;++k){for(int y:{8,9}){setBlock(c,clip,dark(),22-k*2,y,21);setBlock(c,clip,dark(),35+k*2,y,21);}}
        waterBox(c,clip,15,13,21,42,15,21,false);waterBox(c,clip,15,1,21,15,6,21,false);waterBox(c,clip,16,1,21,16,5,21,false);waterBox(c,clip,17,1,21,20,4,21,false);waterBox(c,clip,21,1,21,21,3,21,false);waterBox(c,clip,22,1,21,22,2,21,false);waterBox(c,clip,23,1,21,24,1,21,false);waterBox(c,clip,42,1,21,42,6,21,false);waterBox(c,clip,41,1,21,41,5,21,false);waterBox(c,clip,37,1,21,40,4,21,false);waterBox(c,clip,36,1,21,36,3,21,false);waterBox(c,clip,33,1,21,34,1,21,false);waterBox(c,clip,35,1,21,35,2,21,false);
    }
    void generateRoofPiece(WorldGenerationContext& c,const Box& clip)
    {
        fill(c,clip,21,0,22,36,0,36,rough(),rough());waterBox(c,clip,21,1,22,36,23,36,false);
        for(int i=0;i<4;++i){fill(c,clip,21+i,13+i,21+i,36-i,13+i,21+i,bricks(),bricks());fill(c,clip,21+i,13+i,36-i,36-i,13+i,36-i,bricks(),bricks());fill(c,clip,21+i,13+i,22+i,21+i,13+i,35-i,bricks(),bricks());fill(c,clip,36-i,13+i,22+i,36-i,13+i,35-i,bricks(),bricks());}
        fill(c,clip,25,16,25,32,16,32,rough(),rough());for(auto [x,z]:std::array<std::pair<int,int>,4>{{{25,25},{32,25},{25,32},{32,32}}})fill(c,clip,x,17,z,x,19,z,bricks(),bricks());
        for(auto [x,z,dx,dz]:std::array<std::array<int,4>,4>{{{{26,26,27,27}},{{26,31,27,30}},{{31,31,30,30}},{{31,26,30,27}}}}){setBlock(c,clip,bricks(),x,20,z);setBlock(c,clip,bricks(),dx,21,dz);setBlock(c,clip,seaLantern(),dx,20,dz);}
        fill(c,clip,28,21,27,29,21,27,rough(),rough());fill(c,clip,27,21,28,27,21,29,rough(),rough());fill(c,clip,28,21,30,29,21,30,rough(),rough());fill(c,clip,30,21,28,30,21,29,rough(),rough());
    }
    void generateLowerWall(WorldGenerationContext& c,const Box& clip)
    {
        fill(c,clip,0,0,21,6,0,57,rough(),rough());waterBox(c,clip,0,1,21,6,7,57,false);fill(c,clip,4,4,21,6,4,53,rough(),rough());for(int i=0;i<4;++i)fill(c,clip,i,i+1,21,i,i+1,57-i,bricks(),bricks());for(int z=23;z<53;z+=3)setBlock(c,clip,bricks(),5,5,z);setBlock(c,clip,bricks(),5,5,52);fill(c,clip,4,1,52,6,3,52,rough(),rough());fill(c,clip,5,1,51,5,3,53,rough(),rough());
        fill(c,clip,51,0,21,57,0,57,rough(),rough());waterBox(c,clip,51,1,21,57,7,57,false);fill(c,clip,51,4,21,53,4,53,rough(),rough());for(int i=0;i<4;++i)fill(c,clip,57-i,i+1,21,57-i,i+1,57-i,bricks(),bricks());for(int z=23;z<53;z+=3)setBlock(c,clip,bricks(),52,5,z);setBlock(c,clip,bricks(),52,5,52);fill(c,clip,51,1,52,53,3,52,rough(),rough());fill(c,clip,52,1,51,52,3,53,rough(),rough());
        fill(c,clip,7,0,51,50,0,57,rough(),rough());waterBox(c,clip,7,1,51,50,10,57,false);for(int i=0;i<4;++i)fill(c,clip,i+1,i+1,57-i,56-i,i+1,57-i,bricks(),bricks());
    }
    void generateMiddleWall(WorldGenerationContext& c,const Box& clip)
    {
        fill(c,clip,7,0,21,13,0,50,rough(),rough());waterBox(c,clip,7,1,21,13,10,50,false);fill(c,clip,11,8,21,13,8,53,rough(),rough());for(int i=0;i<4;++i)fill(c,clip,i+7,i+5,21,i+7,i+5,54,bricks(),bricks());for(int z=21;z<=45;z+=3)setBlock(c,clip,bricks(),12,9,z);
        fill(c,clip,44,0,21,50,0,50,rough(),rough());waterBox(c,clip,44,1,21,50,10,50,false);fill(c,clip,44,8,21,46,8,53,rough(),rough());for(int i=0;i<4;++i)fill(c,clip,50-i,i+5,21,50-i,i+5,54,bricks(),bricks());for(int z=21;z<=45;z+=3)setBlock(c,clip,bricks(),45,9,z);
        fill(c,clip,14,0,44,43,0,50,rough(),rough());waterBox(c,clip,14,1,44,43,10,50,false);for(int x=12;x<=45;x+=3){setBlock(c,clip,bricks(),x,9,45);setBlock(c,clip,bricks(),x,9,52);if(x==12||x==18||x==24||x==33||x==39||x==45){setBlock(c,clip,bricks(),x,9,47);setBlock(c,clip,bricks(),x,9,50);setBlock(c,clip,bricks(),x,10,45);setBlock(c,clip,bricks(),x,10,46);setBlock(c,clip,bricks(),x,10,51);setBlock(c,clip,bricks(),x,10,52);setBlock(c,clip,bricks(),x,11,47);setBlock(c,clip,bricks(),x,11,50);setBlock(c,clip,bricks(),x,12,48);setBlock(c,clip,bricks(),x,12,49);}}
        for(int i=0;i<3;++i)fill(c,clip,8+i,5+i,54,49-i,5+i,54,rough(),rough());fill(c,clip,11,8,54,46,8,54,bricks(),bricks());fill(c,clip,14,8,44,43,8,53,rough(),rough());
    }
    void generateUpperWall(WorldGenerationContext& c,const Box& clip)
    {
        fill(c,clip,14,0,21,20,0,43,rough(),rough());waterBox(c,clip,14,1,22,20,14,43,false);fill(c,clip,18,12,22,20,12,39,rough(),rough());fill(c,clip,18,12,21,20,12,21,bricks(),bricks());for(int i=0;i<4;++i)fill(c,clip,i+14,i+9,21,i+14,i+9,43-i,bricks(),bricks());for(int z=23;z<=39;z+=3)setBlock(c,clip,bricks(),19,13,z);
        fill(c,clip,37,0,21,43,0,43,rough(),rough());waterBox(c,clip,37,1,22,43,14,43,false);fill(c,clip,37,12,22,39,12,39,rough(),rough());fill(c,clip,37,12,21,39,12,21,bricks(),bricks());for(int i=0;i<4;++i)fill(c,clip,43-i,i+9,21,43-i,i+9,43-i,bricks(),bricks());for(int z=23;z<=39;z+=3)setBlock(c,clip,bricks(),38,13,z);
        fill(c,clip,21,0,37,36,0,43,rough(),rough());waterBox(c,clip,21,1,37,36,14,43,false);fill(c,clip,21,12,37,36,12,39,rough(),rough());for(int i=0;i<4;++i)fill(c,clip,15+i,i+9,43-i,42-i,i+9,43-i,bricks(),bricks());for(int x=21;x<=36;x+=3)setBlock(c,clip,bricks(),x,13,38);
    }

    bool place(WorldGenerationContext& c,JavaRandom& random,const Box& clip)override
    {
        const int height=64-box.minY;
        waterBox(c,clip,0,0,0,58,height,58,false);
        generateWing(false,0,c,clip);generateWing(true,33,c,clip);generateEntranceArchs(c,clip);generateEntranceWall(c,clip);generateRoofPiece(c,clip);generateLowerWall(c,clip);generateMiddleWall(c,clip);generateUpperWall(c,clip);
        for(int j=0;j<7;++j){int k=0;while(k<7){if(k==0&&j==3)k=6;const int x=j*9,z=k*9;for(int dx=0;dx<4;++dx)for(int dz=0;dz<4;++dz){setBlock(c,clip,bricks(),x+dx,0,z+dz);replaceAirAndLiquidDownwards(c,clip,bricks(),x+dx,-1,z+dz);}if(j!=0&&j!=6)k+=6;else ++k;}}
        for(int i=0;i<5;++i){waterBox(c,clip,-1-i,i*2,-1-i,-1-i,23,58+i,false);waterBox(c,clip,58+i,i*2,-1-i,58+i,23,58+i,false);waterBox(c,clip,-i,i*2,-1-i,57+i,23,-1-i,false);waterBox(c,clip,-i,i*2,58+i,57+i,23,58+i,false);}
        for(auto& child:children)if(child->box.intersects(clip))child->place(c,random,clip);
        return true;
    }
};

std::uint64_t chunkKey(int x,int z) noexcept
{return (static_cast<std::uint64_t>(static_cast<std::uint32_t>(x))<<32U)|static_cast<std::uint32_t>(z);}

Facing horizontalFacing(JavaRandom& random)
{
    // EnumFacing.HORIZONTALS is S-W-N-E in 1.12.2.
    switch(random.nextInt(4)){case 0:return Facing::South;case 1:return Facing::West;case 2:return Facing::North;default:return Facing::East;}
}
}

OceanMonumentStructure::Start OceanMonumentStructure::create(
    std::int64_t worldSeed,int chunkX,int chunkZ,JavaRandom& random)
{
    // StartMonument#create discards MapGenStructure's current RNG state and
    // performs its own world-seed-based reseed before selecting orientation.
    random.setSeed(worldSeed);
    const std::int64_t a=random.nextLong(),b=random.nextLong();
    const std::uint64_t mixed=
        static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkX)*a)^
        static_cast<std::uint64_t>(static_cast<std::int64_t>(chunkZ)*b)^
        static_cast<std::uint64_t>(worldSeed);
    random.setSeed(std::bit_cast<std::int64_t>(mixed));
    const int x=chunkX*16+8-29,z=chunkZ*16+8-29;
    Start result;
    result.pieces.push_back(std::make_unique<MonumentBuilding>(random,x,z,horizontalFacing(random)));
    result.bounds=boundsOf(result.pieces);result.sizeable=true;return result;
}

void OceanMonumentStructure::place(Start& start,WorldGenerationContext& context,JavaRandom& random,
                                   int sourceChunkX,int sourceChunkZ,const Box& clip)
{
    const auto key=chunkKey(sourceChunkX,sourceChunkZ);
    if(start.processedChunks.contains(key))return;
    for(auto& p:start.pieces)if(p->box.intersects(clip))p->place(context,random,clip);
    start.processedChunks.insert(key);
}
}
