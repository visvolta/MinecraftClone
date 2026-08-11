#include "worldgen/BetaSimplexNoise.h"
#include "worldgen/JavaRandom.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>
namespace{
constexpr std::array<std::array<int,3>,12> gradients{{{{1,1,0}},{{-1,1,0}},{{1,-1,0}},{{-1,-1,0}},{{1,0,1}},{{-1,0,1}},{{1,0,-1}},{{-1,0,-1}},{{0,1,1}},{{0,-1,1}},{{0,1,-1}},{{0,-1,-1}}}};
const double F2=.5*(std::sqrt(3.0)-1.0),G2=(3.0-std::sqrt(3.0))/6.0;
}
BetaSimplexNoise::BetaSimplexNoise(JavaRandom& r):offsetX_(r.nextDouble()*256.0),offsetZ_(r.nextDouble()*256.0),offsetY_(r.nextDouble()*256.0){std::array<int,256> p{};for(int i=0;i<256;++i)p[i]=i;for(int i=0;i<256;++i){int j=i+r.nextInt(256-i);std::swap(p[i],p[j]);permutations_[i]=p[i];permutations_[i+256]=p[i];}}
int BetaSimplexNoise::fastFloor(double v) noexcept{return v>0.0?static_cast<int>(v):static_cast<int>(v)-1;}
double BetaSimplexNoise::gradientDot(int g,double x,double z) noexcept{return gradients[static_cast<std::size_t>(g)][0]*x+gradients[static_cast<std::size_t>(g)][1]*z;}
double BetaSimplexNoise::value(double x,double z) const
{
    const double s=(x+z)*F2;const int i=fastFloor(x+s),j=fastFloor(z+s);const double t=(i+j)*G2;
    const double x0=x-(i-t),z0=z-(j-t);const int i1=x0>z0?1:0,j1=x0>z0?0:1;
    const double x1=x0-i1+G2,z1=z0-j1+G2,x2=x0-1.0+2.0*G2,z2=z0-1.0+2.0*G2;
    const int ii=i&255,jj=j&255;const int g0=permutations_[ii+permutations_[jj]]%12,g1=permutations_[ii+i1+permutations_[jj+j1]]%12,g2=permutations_[ii+1+permutations_[jj+1]]%12;
    auto contribution=[](double a,double gx,double gz,double dot){if(a<0.0)return 0.0;a*=a;return a*a*dot;};
    const double n0=contribution(.5-x0*x0-z0*z0,x0,z0,gradientDot(g0,x0,z0));
    const double n1=contribution(.5-x1*x1-z1*z1,x1,z1,gradientDot(g1,x1,z1));
    const double n2=contribution(.5-x2*x2-z2*z2,x2,z2,gradientDot(g2,x2,z2));
    return 70.0*(n0+n1+n2);
}
void BetaSimplexNoise::add(std::vector<double>& out,double ox,double oz,int sx,int sz,double scaleX,double scaleZ,double amp) const
{
    if(sx<=0||sz<=0)return;const std::size_t need=static_cast<std::size_t>(sx)*sz;if(out.size()<need)out.resize(need,0.0);std::size_t idx=0;
    for(int x=0;x<sx;++x){const double inputX=(ox+x)*scaleX+offsetX_;for(int z=0;z<sz;++z){const double inputZ=(oz+z)*scaleZ+offsetZ_;const double s=(inputX+inputZ)*F2;const int i=fastFloor(inputX+s),j=fastFloor(inputZ+s);const double t=(i+j)*G2;const double x0=inputX-(i-t),z0=inputZ-(j-t);const int i1=x0>z0?1:0,j1=x0>z0?0:1;const double x1=x0-i1+G2,z1=z0-j1+G2,x2=x0-1.0+2.0*G2,z2=z0-1.0+2.0*G2;const int ii=i&255,jj=j&255;const int g0=permutations_[ii+permutations_[jj]]%12,g1=permutations_[ii+i1+permutations_[jj+j1]]%12,g2=permutations_[ii+1+permutations_[jj+1]]%12;double a=.5-x0*x0-z0*z0,n0=0,n1=0,n2=0;if(a>=0){a*=a;n0=a*a*gradientDot(g0,x0,z0);}a=.5-x1*x1-z1*z1;if(a>=0){a*=a;n1=a*a*gradientDot(g1,x1,z1);}a=.5-x2*x2-z2*z2;if(a>=0){a*=a;n2=a*a*gradientDot(g2,x2,z2);}out[idx++]+=70.0*(n0+n1+n2)*amp;}}
}
