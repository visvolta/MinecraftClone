#include "worldgen/BetaNoiseGeneratorOctaves.h"

#include "worldgen/JavaRandom.h"

#include <cmath>
#include <cstddef>
#include <cstdint>

BetaNoiseGeneratorOctaves::BetaNoiseGeneratorOctaves(JavaRandom& random,int octaveCount)
{
    octaves_.reserve(static_cast<std::size_t>(octaveCount));
    for(int i=0;i<octaveCount;++i) octaves_.emplace_back(random);
}

double BetaNoiseGeneratorOctaves::noise2D(double x,double z,double scale) const
{
    std::vector<double> out;
    generateNoise2D(out,static_cast<int>(std::floor(x)),static_cast<int>(std::floor(z)),1,1,scale,scale);
    return out.empty()?0.0:out.front();
}

double BetaNoiseGeneratorOctaves::noise3D(double x,double y,double z,double sx,double sy,double sz) const
{
    std::vector<double> out;
    generateNoiseOctaves(out,x,y,z,1,1,1,sx,sy,sz);
    return out.empty()?0.0:out.front();
}

void BetaNoiseGeneratorOctaves::generateNoiseOctaves(
    std::vector<double>& out,double originX,double originY,double originZ,
    int sizeX,int sizeY,int sizeZ,double scaleX,double scaleY,double scaleZ) const
{
    out.assign(static_cast<std::size_t>(sizeX*sizeY*sizeZ),0.0);
    double octaveScale=1.0;
    for(const auto& octave:octaves_)
    {
        double x=originX*octaveScale*scaleX;
        const double y=originY*octaveScale*scaleY;
        double z=originZ*octaveScale*scaleZ;
        std::int64_t fx=static_cast<std::int64_t>(std::floor(x));
        std::int64_t fz=static_cast<std::int64_t>(std::floor(z));
        x-=static_cast<double>(fx); z-=static_cast<double>(fz);
        fx%=16777216LL; fz%=16777216LL;
        x+=static_cast<double>(fx); z+=static_cast<double>(fz);
        octave.populateNoiseArray(out,x,y,z,sizeX,sizeY,sizeZ,
            scaleX*octaveScale,scaleY*octaveScale,scaleZ*octaveScale,octaveScale);
        octaveScale/=2.0;
    }
}

void BetaNoiseGeneratorOctaves::generateNoise2D(
    std::vector<double>& out,int originX,int originZ,int sizeX,int sizeZ,
    double scaleX,double scaleZ) const
{
    generateNoiseOctaves(out,static_cast<double>(originX),10.0,
        static_cast<double>(originZ),sizeX,1,sizeZ,scaleX,1.0,scaleZ);
}
