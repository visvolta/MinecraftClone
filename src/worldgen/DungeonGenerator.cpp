#include "worldgen/DungeonGenerator.h"
#include "worldgen/JavaRandom.h"
#include "worldgen/WorldGenerationContext.h"
#include "Block.h"

bool DungeonGenerator::generate(
    WorldGenerationContext& c,JavaRandom& r,int x,int y,int z) const
{
    const int height=3, rx=r.nextInt(2)+2, rz=r.nextInt(2)+2;
    int openings=0;
    for(int px=x-rx-1;px<=x+rx+1;++px)
      for(int py=y-1;py<=y+height+1;++py)
        for(int pz=z-rz-1;pz<=z+rz+1;++pz)
        {
            if((py==y-1||py==y+height+1) &&
               !isSolid(c.getBlock(px,py,pz))) return false;
            if((px==x-rx-1||px==x+rx+1||pz==z-rz-1||pz==z+rz+1) &&
               py==y && c.getBlock(px,py,pz)==BlockType::Air &&
               c.getBlock(px,py+1,pz)==BlockType::Air) ++openings;
        }
    if(openings<1||openings>5) return false;

    for(int px=x-rx-1;px<=x+rx+1;++px)
      for(int py=y+height;py>=y-1;--py)
        for(int pz=z-rz-1;pz<=z+rz+1;++pz)
        {
            const bool shell=px==x-rx-1||px==x+rx+1||pz==z-rz-1||
                             pz==z+rz+1||py==y-1||py==y+height;
            if(!shell) c.setBlock(px,py,pz,BlockType::Air);
            else if(py==y-1)
                c.setBlock(px,py,pz,r.nextInt(4)!=0?
                           BlockType::MossyCobblestone:BlockType::Cobblestone);
            else c.setBlock(px,py,pz,BlockType::Cobblestone);
        }

    for(int chest=0;chest<2;++chest)
      for(int attempt=0;attempt<3;++attempt)
      {
        const int px=x+r.nextInt(rx*2+1)-rx;
        const int pz=z+r.nextInt(rz*2+1)-rz;
        if(c.getBlock(px,y,pz)!=BlockType::Air) continue;
        int solidSides=0;
        solidSides+=isSolid(c.getBlock(px-1,y,pz));
        solidSides+=isSolid(c.getBlock(px+1,y,pz));
        solidSides+=isSolid(c.getBlock(px,y,pz-1));
        solidSides+=isSolid(c.getBlock(px,y,pz+1));
        if(solidSides==1){ c.setBlock(px,y,pz,BlockType::Chest); break; }
      }
    c.setBlock(x,y,z,BlockType::Spawner);
    return true;
}
