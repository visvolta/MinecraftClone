#include "worldgen/StructurePrimitives.h"

#include "content/ContentCatalog.h"
#include "worldgen/Vanilla112State.h"
#include "worldgen/WorldGenerationContext.h"

#include <limits>
#include <string>

namespace mc112
{
Box Box::component(int sx,int sy,int sz,int xMin,int yMin,int zMin,
                   int xSize,int ySize,int zSize,Facing f) noexcept
{
    switch(f)
    {
        case Facing::North:return {sx+xMin,sy+yMin,sz-zSize+1+zMin,sx+xSize-1+xMin,sy+ySize-1+yMin,sz+zMin};
        case Facing::South:return {sx+xMin,sy+yMin,sz+zMin,sx+xSize-1+xMin,sy+ySize-1+yMin,sz+zSize-1+zMin};
        case Facing::West:return {sx-zSize+1+zMin,sy+yMin,sz+xMin,sx+zMin,sy+ySize-1+yMin,sz+xSize-1+xMin};
        case Facing::East:return {sx+zMin,sy+yMin,sz+xMin,sx+zSize-1+zMin,sy+ySize-1+yMin,sz+xSize-1+xMin};
    }
    return {};
}

bool Box::intersects(const Box&o)const noexcept{return maxX>=o.minX&&minX<=o.maxX&&maxZ>=o.minZ&&minZ<=o.maxZ&&maxY>=o.minY&&minY<=o.maxY;}
bool Box::intersectsXZ(int x0,int z0,int x1,int z1)const noexcept{return maxX>=x0&&minX<=x1&&maxZ>=z0&&minZ<=z1;}
bool Box::contains(int x,int y,int z)const noexcept{return x>=minX&&x<=maxX&&y>=minY&&y<=maxY&&z>=minZ&&z<=maxZ;}
void Box::expand(const Box&o)noexcept{minX=std::min(minX,o.minX);minY=std::min(minY,o.minY);minZ=std::min(minZ,o.minZ);maxX=std::max(maxX,o.maxX);maxY=std::max(maxY,o.maxY);maxZ=std::max(maxZ,o.maxZ);}
void Box::offset(int x,int y,int z)noexcept{minX+=x;maxX+=x;minY+=y;maxY+=y;minZ+=z;maxZ+=z;}
Facing opposite(Facing f)noexcept{switch(f){case Facing::North:return Facing::South;case Facing::South:return Facing::North;case Facing::West:return Facing::East;case Facing::East:return Facing::West;}return f;}
Facing rotateY(Facing f)noexcept{switch(f){case Facing::North:return Facing::East;case Facing::East:return Facing::South;case Facing::South:return Facing::West;case Facing::West:return Facing::North;}return f;}
Facing rotateYCCW(Facing f)noexcept{switch(f){case Facing::North:return Facing::West;case Facing::West:return Facing::South;case Facing::South:return Facing::East;case Facing::East:return Facing::North;}return f;}
std::pair<int,int> step(Facing f)noexcept{switch(f){case Facing::North:return{0,-1};case Facing::South:return{0,1};case Facing::West:return{-1,0};case Facing::East:return{1,0};}return{0,0};}
// StructureComponent#setCoordBaseMode uses these rotations, but SOUTH and
// WEST also apply Mirror.LEFT_RIGHT before rotation. Use
// transformStateForFacing for actual component state placement.
Rotation rotationForFacing(Facing f)noexcept{switch(f){case Facing::West:case Facing::East:return Rotation::Clockwise90;case Facing::North:case Facing::South:return Rotation::None;}return Rotation::None;}

namespace
{
std::string rotateHorizontalName(std::string value, Rotation rotation)
{
    const auto once=[](std::string current)
    {
        if(current=="north") return std::string("east");
        if(current=="east") return std::string("south");
        if(current=="south") return std::string("west");
        if(current=="west") return std::string("north");
        return current;
    };
    int turns=rotation==Rotation::Clockwise90?1:
        rotation==Rotation::Clockwise180?2:
        rotation==Rotation::CounterClockwise90?3:0;
    while(turns-->0) value=once(std::move(value));
    return value;
}

std::optional<std::string> pistonFacingName(int index)
{
    switch(index)
    {
        case 0:return "down"; case 1:return "up"; case 2:return "north";
        case 3:return "south"; case 4:return "west"; case 5:return "east";
        default:return std::nullopt;
    }
}

int pistonFacingIndex(std::string_view value)
{
    if(value=="down")return 0;if(value=="up")return 1;if(value=="north")return 2;
    if(value=="south")return 3;if(value=="west")return 4;if(value=="east")return 5;
    return -1;
}

std::optional<std::string> repeaterFacingName(int index)
{
    switch(index&3)
    {
        case 0:return "south"; case 1:return "west";
        case 2:return "north"; case 3:return "east";
    }
    return std::nullopt;
}

int repeaterFacingIndex(std::string_view value)
{
    if(value=="south")return 0;if(value=="west")return 1;
    if(value=="north")return 2;if(value=="east")return 3;
    return -1;
}

std::string rotateLeverOrientation(std::string value,Rotation rotation)
{
    const auto once=[](std::string current)
    {
        if(current=="east")return std::string("south");
        if(current=="west")return std::string("north");
        if(current=="south")return std::string("west");
        if(current=="north")return std::string("east");
        if(current=="up_z")return std::string("up_x");
        if(current=="up_x")return std::string("up_z");
        if(current=="down_x")return std::string("down_z");
        if(current=="down_z")return std::string("down_x");
        return current;
    };
    int turns=rotation==Rotation::Clockwise90?1:
        rotation==Rotation::Clockwise180?2:
        rotation==Rotation::CounterClockwise90?3:0;
    while(turns-->0) value=once(std::move(value));
    return value;
}

std::optional<std::string> leverOrientationName(int metadata)
{
    switch(metadata&7)
    {
        case 0:return "down_x";case 1:return "east";case 2:return "west";
        case 3:return "south";case 4:return "north";case 5:return "up_z";
        case 6:return "up_x";case 7:return "down_z";
    }
    return std::nullopt;
}

int leverOrientationIndex(std::string_view value)
{
    if(value=="down_x")return 0;if(value=="east")return 1;if(value=="west")return 2;
    if(value=="south")return 3;if(value=="north")return 4;if(value=="up_z")return 5;
    if(value=="up_x")return 6;if(value=="down_z")return 7;
    return -1;
}

mc::content::BlockState rotateLegacyState(
    mc::content::BlockState value, Rotation rotation)
{
    if(rotation==Rotation::None) return value;
    const std::uint16_t metadata=value.properties();

    if(value.block()==BlockType::Lever)
    {
        auto orientation=leverOrientationName(metadata);
        if(!orientation) return value;
        const int encoded=leverOrientationIndex(
            rotateLeverOrientation(*orientation,rotation));
        return value.withProperties(static_cast<std::uint16_t>(
            encoded | (metadata&8U)));
    }

    if(value.block()==BlockType::Piston || value.block()==BlockType::StickyPiston)
    {
        auto facing=pistonFacingName(metadata&7U);
        if(!facing) return value;
        if(*facing!="up"&&*facing!="down")
            *facing=rotateHorizontalName(std::move(*facing),rotation);
        const int encoded=pistonFacingIndex(*facing);
        return encoded<0 ? value : value.withProperties(static_cast<std::uint16_t>(
            encoded | (metadata&8U)));
    }

    if(value.block()==BlockType::Repeater)
    {
        auto facing=repeaterFacingName(metadata&3U);
        if(!facing) return value;
        *facing=rotateHorizontalName(std::move(*facing),rotation);
        const int encoded=repeaterFacingIndex(*facing);
        return encoded<0 ? value : value.withProperties(static_cast<std::uint16_t>(
            encoded | (metadata&12U)));
    }

    if(value.block()==BlockType::Vine)
    {
        bool south=(metadata&1U)!=0,west=(metadata&2U)!=0,
             north=(metadata&4U)!=0,east=(metadata&8U)!=0;
        int turns=rotation==Rotation::Clockwise90?1:
            rotation==Rotation::Clockwise180?2:3;
        while(turns-->0)
        {
            const bool oldSouth=south,oldWest=west,oldNorth=north,oldEast=east;
            north=oldWest;east=oldNorth;south=oldEast;west=oldSouth;
        }
        return value.withProperties(static_cast<std::uint16_t>(
            (south?1:0)|(west?2:0)|(north?4:0)|(east?8:0)));
    }

    return value;
}

mc::content::BlockState mirrorLegacyLeftRight(mc::content::BlockState value)
{
    const std::uint16_t metadata=value.properties();
    if(value.block()==BlockType::Lever)
    {
        auto orientation=leverOrientationName(metadata);
        if(!orientation) return value;
        if(*orientation=="north") *orientation="south";
        else if(*orientation=="south") *orientation="north";
        const int encoded=leverOrientationIndex(*orientation);
        return value.withProperties(static_cast<std::uint16_t>(encoded|(metadata&8U)));
    }
    if(value.block()==BlockType::Piston || value.block()==BlockType::StickyPiston)
    {
        auto facing=pistonFacingName(metadata&7U);
        if(!facing) return value;
        if(*facing=="north") *facing="south";
        else if(*facing=="south") *facing="north";
        const int encoded=pistonFacingIndex(*facing);
        return encoded<0?value:value.withProperties(static_cast<std::uint16_t>(encoded|(metadata&8U)));
    }
    if(value.block()==BlockType::Repeater)
    {
        auto facing=repeaterFacingName(metadata&3U);
        if(!facing) return value;
        if(*facing=="north") *facing="south";
        else if(*facing=="south") *facing="north";
        const int encoded=repeaterFacingIndex(*facing);
        return encoded<0?value:value.withProperties(static_cast<std::uint16_t>(encoded|(metadata&12U)));
    }
    if(value.block()==BlockType::Vine)
    {
        const bool south=(metadata&1U)!=0,west=(metadata&2U)!=0,
                   north=(metadata&4U)!=0,east=(metadata&8U)!=0;
        return value.withProperties(static_cast<std::uint16_t>(
            (north?1:0)|(west?2:0)|(south?4:0)|(east?8:0)));
    }
    return value;
}
}

mc::content::BlockState rotateState(mc::content::BlockState value,Rotation rotation)
{
    if(rotation==Rotation::None)return value;
    const auto legacyRotated=rotateLegacyState(value,rotation);
    if(legacyRotated!=value || value.block()==BlockType::Lever ||
       value.block()==BlockType::Piston || value.block()==BlockType::StickyPiston ||
       value.block()==BlockType::Repeater || value.block()==BlockType::Vine)
        return legacyRotated;
    const auto* active=mc::content::ContentCatalog::active();
    if(active==nullptr)return value;
    const auto* name=active->blockName(value); if(name==nullptr)return value;
    auto props=active->serializeStateProperties(value);
    auto rotateFacing=[rotation](std::string v){
        const auto once=[](std::string s){if(s=="north")return std::string("east");if(s=="east")return std::string("south");if(s=="south")return std::string("west");if(s=="west")return std::string("north");return s;};
        int n=rotation==Rotation::Clockwise90?1:rotation==Rotation::Clockwise180?2:3;
        while(n-->0)v=once(std::move(v));return v;};
    auto rotateRail=[rotation](std::string s){
        const auto once=[](std::string v){
            if(v=="north_south")return std::string("east_west"); if(v=="east_west")return std::string("north_south");
            if(v=="ascending_east")return std::string("ascending_south");if(v=="ascending_south")return std::string("ascending_west");
            if(v=="ascending_west")return std::string("ascending_north");if(v=="ascending_north")return std::string("ascending_east");
            if(v=="south_east")return std::string("south_west");if(v=="south_west")return std::string("north_west");
            if(v=="north_west")return std::string("north_east");if(v=="north_east")return std::string("south_east");return v;};
        int n=rotation==Rotation::Clockwise90?1:rotation==Rotation::Clockwise180?2:3;while(n-->0)s=once(std::move(s));return s;};
    for(auto& [key,val]:props)
    {
        if(key=="facing" && (val=="north"||val=="south"||val=="east"||val=="west")) val=rotateFacing(val);
        else if(key=="axis" && (rotation==Rotation::Clockwise90||rotation==Rotation::CounterClockwise90) && (val=="x"||val=="z")) val=val=="x"?"z":"x";
        else if(key=="shape" && (val.find("north")!=std::string::npos||val.find("south")!=std::string::npos||val.find("east")!=std::string::npos||val.find("west")!=std::string::npos)) val=rotateRail(val);
        else if(key=="rotation")
        {
            try{int i=std::stoi(val);const int add=rotation==Rotation::Clockwise90?4:rotation==Rotation::Clockwise180?8:12;val=std::to_string((i+add)&15);}catch(...){ }
        }
    }
    const auto result=active->state(*name,std::span<const std::pair<std::string,std::string>>(props));
    return result.value_or(value);
}

mc::content::BlockState transformStateForFacing(
    mc::content::BlockState value, Facing facing)
{
    // Vanilla StructureComponent#setCoordBaseMode:
    // NORTH = NONE/NONE, SOUTH = LEFT_RIGHT/NONE,
    // WEST = LEFT_RIGHT/CW90, EAST = NONE/CW90.
    if(facing==Facing::North) return value;

    if(value.block()==BlockType::Lever || value.block()==BlockType::Piston ||
       value.block()==BlockType::StickyPiston || value.block()==BlockType::Repeater ||
       value.block()==BlockType::Vine)
    {
        if(facing==Facing::South || facing==Facing::West)
            value=mirrorLegacyLeftRight(value);
        return (facing==Facing::West || facing==Facing::East)
            ? rotateLegacyState(value,Rotation::Clockwise90)
            : value;
    }

    const auto* active=mc::content::ContentCatalog::active();
    if(active==nullptr) return value;
    const auto* name=active->blockName(value);
    if(name==nullptr) return value;
    auto props=active->serializeStateProperties(value);

    if(facing==Facing::South || facing==Facing::West)
    {
        // Mirror.LEFT_RIGHT reflects the Z axis. The generic state container
        // representation lets us preserve the vanilla directional result for
        // blocks used by structures without reducing them to legacy metadata.
        for(auto& [key,val]:props)
        {
            if(key=="facing")
            {
                if(val=="north") val="south";
                else if(val=="south") val="north";
            }
            else if(key=="shape")
            {
                if(val=="ascending_north") val="ascending_south";
                else if(val=="ascending_south") val="ascending_north";
                else if(val=="south_east") val="north_east";
                else if(val=="south_west") val="north_west";
                else if(val=="north_west") val="south_west";
                else if(val=="north_east") val="south_east";
            }
        }
        // Multipart states such as vines encode cardinal directions as
        // property NAMES, so swap those keys as Mirror.LEFT_RIGHT does.
        auto north=std::find_if(props.begin(),props.end(),[](const auto&p){return p.first=="north";});
        auto south=std::find_if(props.begin(),props.end(),[](const auto&p){return p.first=="south";});
        if(north!=props.end()&&south!=props.end()) std::swap(north->second,south->second);

        if(const auto mirrored=active->state(*name,std::span<const std::pair<std::string,std::string>>(props));mirrored)
            value=*mirrored;
    }

    return (facing==Facing::West || facing==Facing::East)
        ? rotateState(value,Rotation::Clockwise90)
        : value;
}

int Piece::worldX(int x,int z)const noexcept{if(!facing)return x;switch(*facing){case Facing::North:case Facing::South:return box.minX+x;case Facing::West:return box.maxX-z;case Facing::East:return box.minX+z;}return x;}
int Piece::worldY(int y)const noexcept{return facing?y+box.minY:y;}
int Piece::worldZ(int x,int z)const noexcept{if(!facing)return z;switch(*facing){case Facing::North:return box.maxZ-z;case Facing::South:return box.minZ+z;case Facing::West:case Facing::East:return box.minZ+x;}return z;}
void Piece::setBlock(WorldGenerationContext&c,const Box&clip,mc::content::BlockState s,int x,int y,int z)const{const int wx=worldX(x,z),wy=worldY(y),wz=worldZ(x,z);if(!clip.contains(wx,wy,wz))return;if(facing)s=transformStateForFacing(s,*facing);c.setBlockState(wx,wy,wz,s);}
mc::content::BlockState Piece::getBlock(const WorldGenerationContext&c,const Box&clip,int x,int y,int z)const{const int wx=worldX(x,z),wy=worldY(y),wz=worldZ(x,z);return clip.contains(wx,wy,wz)?c.getBlockState(wx,wy,wz):mc112::state("air");}
void Piece::fillAir(WorldGenerationContext&c,const Box&clip,int x0,int y0,int z0,int x1,int y1,int z1)const{const auto air=mc112::state("air");for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)for(int z=z0;z<=z1;++z)setBlock(c,clip,air,x,y,z);}
void Piece::fill(WorldGenerationContext&c,const Box&clip,int x0,int y0,int z0,int x1,int y1,int z1,mc::content::BlockState edge,mc::content::BlockState inside,bool existingOnly)const{for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)for(int z=z0;z<=z1;++z){if(existingOnly&&mc112::isAir(getBlock(c,clip,x,y,z)))continue;setBlock(c,clip,(y!=y0&&y!=y1&&x!=x0&&x!=x1&&z!=z0&&z!=z1)?inside:edge,x,y,z);}}
void Piece::maybeBox(WorldGenerationContext&c,const Box&clip,JavaRandom&r,float chance,int x0,int y0,int z0,int x1,int y1,int z1,mc::content::BlockState edge,mc::content::BlockState inside,bool requireNonAir)const{for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x)for(int z=z0;z<=z1;++z)if(r.nextFloat()<=chance&&(!requireNonAir||!mc112::isAir(getBlock(c,clip,x,y,z))))setBlock(c,clip,(y!=y0&&y!=y1&&x!=x0&&x!=x1&&z!=z0&&z!=z1)?inside:edge,x,y,z);}
void Piece::maybeBlock(WorldGenerationContext&c,const Box&clip,JavaRandom&r,float chance,int x,int y,int z,mc::content::BlockState s)const{if(r.nextFloat()<chance)setBlock(c,clip,s,x,y,z);}
bool Piece::liquidAround(const WorldGenerationContext&c,const Box&clip)const{const int x0=std::max(box.minX-1,clip.minX),y0=std::max(box.minY-1,clip.minY),z0=std::max(box.minZ-1,clip.minZ),x1=std::min(box.maxX+1,clip.maxX),y1=std::min(box.maxY+1,clip.maxY),z1=std::min(box.maxZ+1,clip.maxZ);for(int x=x0;x<=x1;++x)for(int z=z0;z<=z1;++z)if(mc112::isLiquid(c.getBlockState(x,y0,z))||mc112::isLiquid(c.getBlockState(x,y1,z)))return true;for(int x=x0;x<=x1;++x)for(int y=y0;y<=y1;++y)if(mc112::isLiquid(c.getBlockState(x,y,z0))||mc112::isLiquid(c.getBlockState(x,y,z1)))return true;for(int z=z0;z<=z1;++z)for(int y=y0;y<=y1;++y)if(mc112::isLiquid(c.getBlockState(x0,y,z))||mc112::isLiquid(c.getBlockState(x1,y,z)))return true;return false;}

int Piece::skyBrightness(const WorldGenerationContext& c,const Box& clip,int x,int y,int z)const
{
    const int wx=worldX(x,z),wy=worldY(y+1),wz=worldZ(x,z);
    if(!clip.contains(wx,wy,wz)) return 15;
    return c.getHeightValue(wx,wz)<=wy ? 15 : 0;
}

void Piece::rareFill(WorldGenerationContext& c,const Box& clip,int x0,int y0,int z0,int x1,int y1,int z1,mc::content::BlockState state,bool excludeAir)const
{
    const float sx=static_cast<float>(x1-x0+1), sy=static_cast<float>(y1-y0+1), sz=static_cast<float>(z1-z0+1);
    const float cx=static_cast<float>(x0)+sx/2.0f, cz=static_cast<float>(z0)+sz/2.0f;
    for(int y=y0;y<=y1;++y){const float ny=static_cast<float>(y-y0)/sy;for(int x=x0;x<=x1;++x){const float nx=(static_cast<float>(x)-cx)/(sx*0.5f);for(int z=z0;z<=z1;++z){const float nz=(static_cast<float>(z)-cz)/(sz*0.5f);if((!excludeAir||!mc112::isAir(getBlock(c,clip,x,y,z)))&&nx*nx+ny*ny+nz*nz<=1.05f)setBlock(c,clip,state,x,y,z);}}}
}

void Piece::replaceAirAndLiquidDownwards(WorldGenerationContext& c,const Box& clip,mc::content::BlockState state,int x,int y,int z)const
{
    int wx=worldX(x,z),wy=worldY(y),wz=worldZ(x,z);
    if(wx<clip.minX||wx>clip.maxX||wz<clip.minZ||wz>clip.maxZ)return;
    while(wy>1){const auto current=c.getBlockState(wx,wy,wz);if(!mc112::isAir(current)&&!mc112::isLiquid(current))break;c.setBlockState(wx,wy,wz,state);--wy;}
}
Piece* findIntersecting(const std::vector<std::unique_ptr<Piece>>&pieces,const Box&b)noexcept{for(auto& p:pieces)if(p&&p->box.intersects(b))return p.get();return nullptr;}
Box boundsOf(const std::vector<std::unique_ptr<Piece>>&pieces)noexcept{Box b{std::numeric_limits<int>::max(),std::numeric_limits<int>::max(),std::numeric_limits<int>::max(),std::numeric_limits<int>::min(),std::numeric_limits<int>::min(),std::numeric_limits<int>::min()};for(auto& p:pieces)if(p)b.expand(p->box);return b;}
void offsetAll(std::vector<std::unique_ptr<Piece>>&pieces,int x,int y,int z){for(auto& p:pieces)if(p)p->offset(x,y,z);}
void markAvailableHeight(std::vector<std::unique_ptr<Piece>>&pieces,JavaRandom&r,int minimumY,int seaLevel){Box b=boundsOf(pieces);const int target=seaLevel-minimumY;const int room=target-b.maxY;const int shift=room>1?r.nextInt(room):0;const int dy=shift+minimumY-b.minY;offsetAll(pieces,0,dy,0);}
}
