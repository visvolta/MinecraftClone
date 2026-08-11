#include "worldgen/BetaNoiseGeneratorPerlin.h"

#include "worldgen/JavaRandom.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace
{
constexpr std::array<double, 16> GradX{
    1,-1,1,-1,1,-1,1,-1,0,0,0,0,1,0,-1,0};
constexpr std::array<double, 16> GradY{
    1,1,-1,-1,0,0,0,0,1,-1,1,-1,1,-1,1,-1};
constexpr std::array<double, 16> GradZ{
    0,0,0,0,1,1,-1,-1,1,1,-1,-1,0,1,0,-1};
}

BetaNoiseGeneratorPerlin::BetaNoiseGeneratorPerlin(JavaRandom& random)
    : offsetX(random.nextDouble() * 256.0),
      offsetY(random.nextDouble() * 256.0),
      offsetZ(random.nextDouble() * 256.0)
{
    for (int i = 0; i < 256; ++i)
        permutations[static_cast<std::size_t>(i)] = i;
    for (int i = 0; i < 256; ++i)
    {
        const int j = random.nextInt(256 - i) + i;
        std::swap(permutations[static_cast<std::size_t>(i)],
                  permutations[static_cast<std::size_t>(j)]);
        permutations[static_cast<std::size_t>(i + 256)] =
            permutations[static_cast<std::size_t>(i)];
    }
}

double BetaNoiseGeneratorPerlin::fade(double v) noexcept
{
    return v*v*v*(v*(v*6.0-15.0)+10.0);
}

double BetaNoiseGeneratorPerlin::lerp(double a,double b,double c) noexcept
{
    return b+a*(c-b);
}

double BetaNoiseGeneratorPerlin::grad(int hash,double x,double y,double z) noexcept
{
    const int i=hash&15;
    return GradX[static_cast<std::size_t>(i)]*x+
           GradY[static_cast<std::size_t>(i)]*y+
           GradZ[static_cast<std::size_t>(i)]*z;
}

double BetaNoiseGeneratorPerlin::grad2(int hash,double x,double z) noexcept
{
    const int i=hash&15;
    return GradX[static_cast<std::size_t>(i)]*x+
           GradZ[static_cast<std::size_t>(i)]*z;
}

double BetaNoiseGeneratorPerlin::noise(double x,double y) const
{
    return noise(x,y,0.0);
}

double BetaNoiseGeneratorPerlin::noise(double x,double y,double z) const
{
    x+=offsetX; y+=offsetY; z+=offsetZ;
    const int ix=static_cast<int>(std::floor(x));
    const int iy=static_cast<int>(std::floor(y));
    const int iz=static_cast<int>(std::floor(z));
    const int X=ix&255,Y=iy&255,Z=iz&255;
    x-=ix; y-=iy; z-=iz;
    const double u=fade(x),v=fade(y),w=fade(z);
    const int a=permutations[static_cast<std::size_t>(X)]+Y;
    const int aa=permutations[static_cast<std::size_t>(a)]+Z;
    const int ab=permutations[static_cast<std::size_t>(a+1)]+Z;
    const int b=permutations[static_cast<std::size_t>(X+1)]+Y;
    const int ba=permutations[static_cast<std::size_t>(b)]+Z;
    const int bb=permutations[static_cast<std::size_t>(b+1)]+Z;
    return lerp(w,
        lerp(v,lerp(u,grad(permutations[aa],x,y,z),grad(permutations[ba],x-1,y,z)),
               lerp(u,grad(permutations[ab],x,y-1,z),grad(permutations[bb],x-1,y-1,z))),
        lerp(v,lerp(u,grad(permutations[aa+1],x,y,z-1),grad(permutations[ba+1],x-1,y,z-1)),
               lerp(u,grad(permutations[ab+1],x,y-1,z-1),grad(permutations[bb+1],x-1,y-1,z-1))));
}

void BetaNoiseGeneratorPerlin::populateNoiseArray(
    std::vector<double>& out,double xOffset,double yOffset,double zOffset,
    int xSize,int ySize,int zSize,double xScale,double yScale,double zScale,
    double noiseScale) const
{
    const std::size_t need=static_cast<std::size_t>(xSize*ySize*zSize);
    if(out.size()<need) out.resize(need,0.0);
    const double inv=1.0/noiseScale;
    std::size_t index=0;

    if(ySize==1)
    {
        for(int x=0;x<xSize;++x)
        {
            double dx=xOffset+x*xScale+offsetX;
            int ix=static_cast<int>(dx); if(dx<ix)--ix;
            const int px=ix&255; dx-=ix; const double fx=fade(dx);
            for(int z=0;z<zSize;++z)
            {
                double dz=zOffset+z*zScale+offsetZ;
                int iz=static_cast<int>(dz); if(dz<iz)--iz;
                const int pz=iz&255; dz-=iz; const double fz=fade(dz);
                const int a=permutations[static_cast<std::size_t>(px)];
                const int aa=permutations[static_cast<std::size_t>(a)]+pz;
                const int b=permutations[static_cast<std::size_t>(px+1)];
                const int ba=permutations[static_cast<std::size_t>(b)]+pz;
                const double n0=lerp(fx,
                    grad2(permutations[static_cast<std::size_t>(aa)],dx,dz),
                    grad(permutations[static_cast<std::size_t>(ba)],dx-1.0,0.0,dz));
                const double n1=lerp(fx,
                    grad(permutations[static_cast<std::size_t>(aa+1)],dx,0.0,dz-1.0),
                    grad(permutations[static_cast<std::size_t>(ba+1)],dx-1.0,0.0,dz-1.0));
                out[index++]+=lerp(fz,n0,n1)*inv;
            }
        }
        return;
    }

    int cachedY=-1;
    int a=0,aa=0,ab=0,b=0,ba=0,bb=0;
    double x00=0,x10=0,x01=0,x11=0;
    for(int x=0;x<xSize;++x)
    {
        double dx=xOffset+x*xScale+offsetX;
        int ix=static_cast<int>(dx); if(dx<ix)--ix;
        const int px=ix&255; dx-=ix; const double fx=fade(dx);
        for(int z=0;z<zSize;++z)
        {
            double dz=zOffset+z*zScale+offsetZ;
            int iz=static_cast<int>(dz); if(dz<iz)--iz;
            const int pz=iz&255; dz-=iz; const double fz=fade(dz);
            for(int y=0;y<ySize;++y)
            {
                double dy=yOffset+y*yScale+offsetY;
                int iy=static_cast<int>(dy); if(dy<iy)--iy;
                const int py=iy&255; dy-=iy; const double fy=fade(dy);
                if(y==0||py!=cachedY)
                {
                    cachedY=py;
                    a=permutations[static_cast<std::size_t>(px)]+py;
                    aa=permutations[static_cast<std::size_t>(a)]+pz;
                    ab=permutations[static_cast<std::size_t>(a+1)]+pz;
                    b=permutations[static_cast<std::size_t>(px+1)]+py;
                    ba=permutations[static_cast<std::size_t>(b)]+pz;
                    bb=permutations[static_cast<std::size_t>(b+1)]+pz;
                    x00=lerp(fx,grad(permutations[aa],dx,dy,dz),grad(permutations[ba],dx-1,dy,dz));
                    x10=lerp(fx,grad(permutations[ab],dx,dy-1,dz),grad(permutations[bb],dx-1,dy-1,dz));
                    x01=lerp(fx,grad(permutations[aa+1],dx,dy,dz-1),grad(permutations[ba+1],dx-1,dy,dz-1));
                    x11=lerp(fx,grad(permutations[ab+1],dx,dy-1,dz-1),grad(permutations[bb+1],dx-1,dy-1,dz-1));
                }
                const double y0=lerp(fy,x00,x10),y1=lerp(fy,x01,x11);
                out[index++]+=lerp(fz,y0,y1)*inv;
            }
        }
    }
}
