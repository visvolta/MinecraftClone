#include "worldgen/BetaSimplexOctaves.h"
#include "worldgen/JavaRandom.h"
#include <cstddef>
BetaSimplexOctaves::BetaSimplexOctaves(JavaRandom& r,int count){octaves_.reserve(static_cast<std::size_t>(count));for(int i=0;i<count;++i)octaves_.emplace_back(r);}
double BetaSimplexOctaves::value(double x,double z) const{double result=0.0,frequency=1.0;for(const auto& o:octaves_){result+=o.value(x*frequency,z*frequency)/frequency;frequency/=2.0;}return result;}
void BetaSimplexOctaves::generate(std::vector<double>& out,double ox,double oz,int sx,int sz,double scaleX,double scaleZ,double freqMul,double ampMul) const{scaleX/=1.5;scaleZ/=1.5;out.assign(static_cast<std::size_t>(sx*sz),0.0);double amplitudeScale=1.0,frequencyScale=1.0;for(const auto& o:octaves_){o.add(out,ox,oz,sx,sz,scaleX*frequencyScale,scaleZ*frequencyScale,.55/amplitudeScale);frequencyScale*=freqMul;amplitudeScale*=ampMul;}}
