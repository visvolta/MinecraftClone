#pragma once

#include "content/BlockState.h"
#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

class WorldGenerationContext;

namespace mc112
{
enum class Facing : std::uint8_t { North, South, West, East };
enum class Rotation : std::uint8_t { None, Clockwise90, Clockwise180, CounterClockwise90 };

struct Box
{
    int minX=0,minY=0,minZ=0,maxX=-1,maxY=-1,maxZ=-1;
    Box()=default;
    Box(int x0,int y0,int z0,int x1,int y1,int z1)
        :minX(x0),minY(y0),minZ(z0),maxX(x1),maxY(y1),maxZ(z1){}
    [[nodiscard]] static Box component(int sx,int sy,int sz,int xMin,int yMin,int zMin,
                                       int xSize,int ySize,int zSize,Facing facing) noexcept;
    [[nodiscard]] bool intersects(const Box& other) const noexcept;
    [[nodiscard]] bool intersectsXZ(int x0,int z0,int x1,int z1) const noexcept;
    [[nodiscard]] bool contains(int x,int y,int z) const noexcept;
    void expand(const Box& other) noexcept;
    void offset(int x,int y,int z) noexcept;
    [[nodiscard]] int xSize()const noexcept{return maxX-minX+1;}
    [[nodiscard]] int ySize()const noexcept{return maxY-minY+1;}
    [[nodiscard]] int zSize()const noexcept{return maxZ-minZ+1;}
};

[[nodiscard]] Facing opposite(Facing facing) noexcept;
[[nodiscard]] Facing rotateY(Facing facing) noexcept;
[[nodiscard]] Facing rotateYCCW(Facing facing) noexcept;
[[nodiscard]] std::pair<int,int> step(Facing facing) noexcept;
[[nodiscard]] Rotation rotationForFacing(Facing facing) noexcept;
[[nodiscard]] mc::content::BlockState rotateState(mc::content::BlockState state, Rotation rotation);
[[nodiscard]] mc::content::BlockState transformStateForFacing(
    mc::content::BlockState state, Facing facing);

class Piece
{
public:
    virtual ~Piece()=default;
    Box box;
    int componentType=0;
    std::optional<Facing> facing;

    virtual void build(std::vector<std::unique_ptr<Piece>>& pieces,JavaRandom& random){}
    virtual bool place(WorldGenerationContext& context,JavaRandom& random,const Box& clip)=0;
    virtual void offset(int x,int y,int z){box.offset(x,y,z);}

    [[nodiscard]] int worldX(int x,int z) const noexcept;
    [[nodiscard]] int worldY(int y) const noexcept;
    [[nodiscard]] int worldZ(int x,int z) const noexcept;
    void setBlock(WorldGenerationContext&,const Box&,mc::content::BlockState,int x,int y,int z) const;
    [[nodiscard]] mc::content::BlockState getBlock(const WorldGenerationContext&,const Box&,int x,int y,int z) const;
    void fillAir(WorldGenerationContext&,const Box&,int x0,int y0,int z0,int x1,int y1,int z1)const;
    void fill(WorldGenerationContext&,const Box&,int x0,int y0,int z0,int x1,int y1,int z1,
              mc::content::BlockState edge,mc::content::BlockState inside,bool existingOnly=false)const;
    void maybeBox(WorldGenerationContext&,const Box&,JavaRandom&,float chance,
                  int x0,int y0,int z0,int x1,int y1,int z1,
                  mc::content::BlockState edge,mc::content::BlockState inside,
                  bool requireNonAir=false) const;
    void maybeBlock(WorldGenerationContext&,const Box&,JavaRandom&,float chance,
                    int x,int y,int z,mc::content::BlockState)const;
    [[nodiscard]] bool liquidAround(const WorldGenerationContext&,const Box&)const;
    [[nodiscard]] int skyBrightness(const WorldGenerationContext&,const Box&,int x,int y,int z)const;
    void rareFill(WorldGenerationContext&,const Box&,int x0,int y0,int z0,int x1,int y1,int z1,
                  mc::content::BlockState state,bool excludeAir=false)const;
    void replaceAirAndLiquidDownwards(WorldGenerationContext&,const Box&,mc::content::BlockState state,
                                      int x,int y,int z)const;
};

[[nodiscard]] Piece* findIntersecting(const std::vector<std::unique_ptr<Piece>>& pieces,const Box& box) noexcept;
[[nodiscard]] Box boundsOf(const std::vector<std::unique_ptr<Piece>>& pieces) noexcept;
void offsetAll(std::vector<std::unique_ptr<Piece>>& pieces,int x,int y,int z);
void markAvailableHeight(std::vector<std::unique_ptr<Piece>>& pieces,JavaRandom& random,int minimumY=10,int seaLevel=63);
}
