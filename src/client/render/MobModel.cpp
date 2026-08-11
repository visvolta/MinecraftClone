#include "client/render/MobModel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace mc::client
{
namespace
{
using gameplay::MobModelKind;

int part(
    MobModelDefinition& model,
    const char* name,
    glm::vec3 pivot = {},
    int parent = -1,
    glm::vec3 rotation = {})
{
    model.parts.push_back({name, pivot, rotation, parent, {}});
    return static_cast<int>(model.parts.size() - 1U);
}

void cube(
    MobModelDefinition& model,
    int partIndex,
    glm::vec3 origin,
    glm::ivec3 size,
    glm::ivec2 uv,
    float inflate = 0.0f,
    bool mirror = false)
{
    model.parts[static_cast<std::size_t>(partIndex)].cubes.push_back(
        {origin, size, uv, inflate, mirror, 0}
    );
}

int find(const MobModelDefinition& model, const char* name)
{
    for (std::size_t index = 0; index < model.parts.size(); ++index)
        if (model.parts[index].name == name)
            return static_cast<int>(index);
    return -1;
}

MobModelPart& named(MobModelDefinition& model, const char* name)
{
    const int index = find(model, name);
    if (index < 0)
        throw std::logic_error(std::string("Missing mob model part: ") + name);
    return model.parts[static_cast<std::size_t>(index)];
}

MobModelDefinition biped(bool skeleton, bool enderman, bool zombieVillager)
{
    MobModelDefinition model;
    model.textureWidth = zombieVillager ? 64 : 64;
    model.textureHeight = zombieVillager ? 64 : 32;
    const float yOffset = enderman ? -14.0f : 0.0f;
    const int head = part(model, "head", {0, yOffset, 0});
    cube(model, head, {-4,-8,-4}, {8,8,8}, {0,0}, enderman ? -0.5f : 0.0f);
    if (zombieVillager)
        cube(model, head, {-1,-3,-6}, {2,4,2}, {24,0});
    const int headwear = part(model, "headwear", {0,yOffset,0});
    cube(model, headwear, {-4,-8,-4}, {8,8,8}, {32,0}, 0.5f);
    const int body = part(model, "body", {0,yOffset,0});
    cube(model, body, {-4,0,-2}, {8,12,4}, {16,16});
    const glm::ivec3 limbSize = skeleton ? glm::ivec3(2,12,2) :
                              enderman ? glm::ivec3(2,30,2) :
                                         glm::ivec3(4,12,4);
    const glm::vec3 rightArmOrigin = skeleton || enderman
        ? glm::vec3(-1,-2,-1) : glm::vec3(-3,-2,-2);
    const glm::vec3 leftArmOrigin = skeleton || enderman
        ? glm::vec3(-1,-2,-1) : glm::vec3(-1,-2,-2);
    const float armX = enderman ? 5.0f : 5.0f;
    const int rightArm = part(model, "right_arm", {-armX,2+yOffset,0});
    cube(model, rightArm, rightArmOrigin, limbSize, {40,16});
    const int leftArm = part(model, "left_arm", {armX,2+yOffset,0});
    cube(model, leftArm, leftArmOrigin, limbSize, {40,16}, 0.0f, true);
    const float legY = enderman ? -5.0f : 12.0f;
    const int rightLeg = part(model, "right_leg", {-1.9f,legY,0});
    cube(model, rightLeg, skeleton || enderman ? glm::vec3(-1,0,-1) :
                                                glm::vec3(-2,0,-2),
         limbSize, {0,16});
    const int leftLeg = part(model, "left_leg", {1.9f,legY,0});
    cube(model, leftLeg, skeleton || enderman ? glm::vec3(-1,0,-1) :
                                               glm::vec3(-2,0,-2),
         limbSize, {0,16}, 0.0f, true);
    if(enderman)
    {
        model.parts[static_cast<std::size_t>(headwear)].cubes.clear();
        cube(model,headwear,{-4,-8,-4},{8,8,8},{0,16},-0.5f);
        model.parts[static_cast<std::size_t>(body)].cubes.clear();
        cube(model,body,{-4,0,-2},{8,12,4},{32,16});
        model.parts[static_cast<std::size_t>(rightArm)].pivot.x=-3.0f;
        model.parts[static_cast<std::size_t>(rightArm)].cubes.clear();
        cube(model,rightArm,{-1,-2,-1},{2,30,2},{56,0});
        model.parts[static_cast<std::size_t>(leftArm)].cubes.clear();
        cube(model,leftArm,{-1,-2,-1},{2,30,2},{56,0},0,true);
        model.parts[static_cast<std::size_t>(rightLeg)].pivot={-2,-5,0};
        model.parts[static_cast<std::size_t>(rightLeg)].cubes.clear();
        cube(model,rightLeg,{-1,0,-1},{2,30,2},{56,0});
        model.parts[static_cast<std::size_t>(leftLeg)].pivot={2,-5,0};
        model.parts[static_cast<std::size_t>(leftLeg)].cubes.clear();
        cube(model,leftLeg,{-1,0,-1},{2,30,2},{56,0},0,true);
    }
    return model;
}

MobModelDefinition quadruped(int legHeight, bool pig, bool cow, bool sheep)
{
    MobModelDefinition model;
    model.textureWidth = 64;
    model.textureHeight = 32;
    int head = part(model, "head", {0,static_cast<float>(18-legHeight),-6});
    cube(model, head, {-4,-4,-8}, {8,8,8}, {0,0});
    if (pig)
        cube(model, head, {-2,0,-9}, {4,3,1}, {16,16});
    if (cow)
    {
        model.parts[head].cubes.clear();
        cube(model, head, {-4,-4,-6}, {8,8,6}, {0,0});
        cube(model, head, {-5,-5,-4}, {1,3,1}, {22,0});
        cube(model, head, {4,-5,-4}, {1,3,1}, {22,0});
        model.parts[head].pivot = {0,4,-8};
    }
    if (sheep)
    {
        model.parts[head].cubes.clear();
        cube(model, head, {-3,-4,-6}, {6,6,8}, {0,0});
        model.parts[head].pivot = {0,6,-8};
    }
    int body = part(model, "body", {0,static_cast<float>(17-legHeight),2},
                    -1, {std::numbers::pi_v<float>/2,0,0});
    cube(model, body, cow ? glm::vec3(-6,-10,-7) : glm::vec3(-5,-10,-7),
         cow ? glm::ivec3(12,18,10) :
         sheep ? glm::ivec3(8,16,6) : glm::ivec3(10,16,8),
         cow ? glm::ivec2(18,4) : glm::ivec2(28,8));
    if (cow)
        cube(model, body, {-2,2,-8}, {4,6,1}, {52,0});
    const std::array<glm::vec3,4> pivots{{
        {-3,static_cast<float>(24-legHeight),7},
        {3,static_cast<float>(24-legHeight),7},
        {-3,static_cast<float>(24-legHeight),-5},
        {3,static_cast<float>(24-legHeight),-5}
    }};
    for (int index = 0; index < 4; ++index)
    {
        const std::string name = "leg" + std::to_string(index + 1);
        const int leg = part(model, name.c_str(), pivots[index]);
        cube(model, leg, {-2,0,-2}, {4,legHeight,4}, {0,16});
    }
    return model;
}

MobModelDefinition spider()
{
    MobModelDefinition model;
    int head = part(model, "head", {0,15,-3});
    cube(model, head, {-4,-4,-8}, {8,8,8}, {32,4});
    int neck = part(model, "neck", {0,15,0});
    cube(model, neck, {-3,-3,-3}, {6,6,6}, {0,0});
    int body = part(model, "body", {0,15,9});
    cube(model, body, {-5,-4,-6}, {10,8,12}, {0,12});
    for (int index = 0; index < 8; ++index)
    {
        const bool left = index % 2 == 0;
        const int pair = index / 2;
        const std::string name = "leg" + std::to_string(index + 1);
        const int leg = part(model, name.c_str(),
            {left ? -4.0f : 4.0f,15.0f,2.0f-static_cast<float>(pair)});
        cube(model, leg, left ? glm::vec3(-15,-1,-1) : glm::vec3(-1,-1,-1),
             {16,2,2}, {18,0});
    }
    return model;
}

MobModelDefinition creeper()
{
    MobModelDefinition model;
    int head = part(model,"head",{0,6,0});
    cube(model,head,{-4,-8,-4},{8,8,8},{0,0});
    int body = part(model,"body",{0,6,0});
    cube(model,body,{-4,0,-2},{8,12,4},{16,16});
    const std::array<glm::vec3,4> pivots{{{-2,18,4},{2,18,4},{-2,18,-4},{2,18,-4}}};
    for (int index=0; index<4; ++index)
    {
        const std::string name="leg"+std::to_string(index+1);
        int leg=part(model,name.c_str(),pivots[index]);
        cube(model,leg,{-2,0,-2},{4,6,4},{0,16});
    }
    return model;
}

MobModelDefinition chicken()
{
    MobModelDefinition model;
    int head=part(model,"head",{0,15,-4});
    cube(model,head,{-2,-6,-2},{4,6,3},{0,0});
    int bill=part(model,"bill",{0,15,-4});
    cube(model,bill,{-2,-4,-4},{4,2,2},{14,0});
    int chin=part(model,"chin",{0,15,-4});
    cube(model,chin,{-1,-2,-3},{2,2,2},{14,4});
    int body=part(model,"body",{0,16,0},-1,{std::numbers::pi_v<float>/2,0,0});
    cube(model,body,{-3,-4,-3},{6,8,6},{0,9});
    int rightLeg=part(model,"right_leg",{-2,19,1});
    cube(model,rightLeg,{-1,0,-3},{3,5,3},{26,0});
    int leftLeg=part(model,"left_leg",{1,19,1});
    cube(model,leftLeg,{-1,0,-3},{3,5,3},{26,0});
    int rightWing=part(model,"right_wing",{-4,13,0});
    cube(model,rightWing,{0,0,-3},{1,4,6},{24,13});
    int leftWing=part(model,"left_wing",{4,13,0});
    cube(model,leftWing,{-1,0,-3},{1,4,6},{24,13});
    return model;
}

MobModelDefinition vex()
{
    MobModelDefinition model=biped(false,false,false);
    model.textureHeight=64;
    named(model,"left_leg").cubes.clear();
    named(model,"headwear").cubes.clear();
    named(model,"right_leg").cubes.clear();
    cube(model,find(model,"right_leg"),{-1,-1,-2},{6,10,4},{32,0});
    int rightWing=part(model,"right_wing",{0,1,2},-1,{.471239f,0,.471239f});
    cube(model,rightWing,{-20,0,0},{20,12,1},{0,32});
    int leftWing=part(model,"left_wing",{0,1,2},-1,{.471239f,0,-.471239f});
    cube(model,leftWing,{0,0,0},{20,12,1},{0,32},0,true);
    return model;
}

MobModelDefinition bat()
{
    MobModelDefinition model; model.textureHeight=64;
    int head=part(model,"head",{0,0,0});
    cube(model,head,{-3,-3,-3},{6,6,6},{0,0});
    cube(model,head,{-4,-6,-2},{3,4,1},{24,0});
    cube(model,head,{1,-6,-2},{3,4,1},{24,0});
    int body=part(model,"body",{0,0,0});
    cube(model,body,{-3,4,-3},{6,12,6},{0,16});
    cube(model,body,{-5,16,0},{10,6,1},{0,34});
    int rw=part(model,"right_wing",{0,0,0},body);
    cube(model,rw,{-12,1,1.5f},{10,16,1},{42,0});
    int rwo=part(model,"right_outer_wing",{-12,1,1.5f},rw);
    cube(model,rwo,{-8,1,0},{8,12,1},{24,16});
    int lw=part(model,"left_wing",{0,0,0},body);
    cube(model,lw,{2,1,1.5f},{10,16,1},{42,0},0,true);
    int lwo=part(model,"left_outer_wing",{12,1,1.5f},lw);
    cube(model,lwo,{0,1,0},{8,12,1},{24,16},0,true);
    return model;
}

MobModelDefinition blaze()
{
    MobModelDefinition model;
    int head=part(model,"head",{0,0,0});
    cube(model,head,{-4,-4,-4},{8,8,8},{0,0});
    for(int i=0;i<12;++i)
    {
        const std::string name="rod"+std::to_string(i);
        int rod=part(model,name.c_str());
        cube(model,rod,{0,0,0},{2,8,2},{0,16});
    }
    return model;
}

MobModelDefinition ghast()
{
    MobModelDefinition model; model.textureWidth=64; model.textureHeight=32;
    int body=part(model,"body",{0,8,0});
    cube(model,body,{-8,-8,-8},{16,16,16},{0,0});
    for(int i=0;i<9;++i)
    {
        const int x=(i%3-1)*5; const int z=(i/3-1)*5;
        const std::string name="tentacle"+std::to_string(i);
        int tentacle=part(model,name.c_str(),{static_cast<float>(x),15,static_cast<float>(z)});
        cube(model,tentacle,{-1,0,-1},{2,8+(i%3)*2,2},{0,0});
    }
    return model;
}

MobModelDefinition segmented(MobModelKind kind)
{
    MobModelDefinition model;
    if(kind==MobModelKind::Silverfish)
    {
        constexpr int sizes[7][3]={{3,2,2},{4,3,2},{6,4,3},{3,3,3},{2,2,3},{2,2,3},{1,2,3}};
        float z=-3.5f;
        for(int i=0;i<7;++i)
        {
            const std::string name="segment"+std::to_string(i);
            int segment=part(model,name.c_str(),{0,static_cast<float>(24-sizes[i][1]),z});
            cube(model,segment,{-sizes[i][0]*.5f,0,-sizes[i][2]*.5f},
                 {sizes[i][0],sizes[i][1],sizes[i][2]}, {0,i<3?0:11});
            z+=(sizes[i][2]+(i<6?sizes[i+1][2]:0))*.5f;
        }
    }
    else
    {
        for(int i=0;i<4;++i)
        {
            const std::string name="segment"+std::to_string(i);
            int segment=part(model,name.c_str(),{0,22,static_cast<float>((i-1)*3)});
            cube(model,segment,{-2-i*.25f,-2,-2},{4+i/2,4,4},{0,i*4});
        }
    }
    return model;
}

MobModelDefinition slime(bool magma)
{
    MobModelDefinition model;
    if(magma)
    {
        for(int i=0;i<8;++i)
        {
            const std::string name="slice"+std::to_string(i);
            int slice=part(model,name.c_str(),{0,static_cast<float>(16+i),0});
            cube(model,slice,{-4,0,-4},{8,1,8},{0,i});
        }
        int core=part(model,"core",{0,18,0});
        cube(model,core,{-2,0,-2},{4,4,4},{0,16});
    }
    else
    {
        int outer=part(model,"outer");
        cube(model,outer,{-4,16,-4},{8,8,8},{0,16});
        int body=part(model,"body");
        cube(model,body,{-3,17,-3},{6,6,6},{0,0});
        cube(model,body,{-3.25f,18,-3.5f},{2,2,2},{32,0});
        cube(model,body,{1.25f,18,-3.5f},{2,2,2},{32,4});
        cube(model,body,{0,21,-3.5f},{1,1,1},{32,8});
    }
    return model;
}

MobModelDefinition squid()
{
    MobModelDefinition model;
    int body=part(model,"body",{0,8,0});
    cube(model,body,{-6,-8,-6},{12,16,12},{0,0});
    for(int i=0;i<8;++i)
    {
        const float a=static_cast<float>(i)*std::numbers::pi_v<float>/4.0f;
        const std::string name="tentacle"+std::to_string(i);
        int tentacle=part(model,name.c_str(),{std::cos(a)*5,15,std::sin(a)*5},-1,{0,a,0});
        cube(model,tentacle,{-1,0,-1},{2,18,2},{48,0});
    }
    return model;
}

MobModelDefinition villager(bool witch, bool illager)
{
    MobModelDefinition model; model.textureHeight=witch?128:64;
    int head=part(model,"head",{0,0,0});
    cube(model,head,{-4,-10,-4},{8,10,8},{0,0});
    int nose=part(model,"nose",{0,-2,0},head);
    cube(model,nose,{-1,-1,-6},{2,4,2},{24,0});
    int body=part(model,"body",{0,0,0});
    cube(model,body,{-4,0,-3},{8,12,6},{16,20});
    cube(model,body,{-4,0,-3},{8,18,6},{0,38},.5f);
    int arms=part(model,"arms",{0,2,0},-1,{-0.75f,0,0});
    cube(model,arms,{-8,-2,-2},{4,8,4},{44,22});
    cube(model,arms,{4,-2,-2},{4,8,4},{44,22});
    cube(model,arms,{-4,2,-2},{8,4,4},{40,38});
    int rightLeg=part(model,"right_leg",{-2,12,0});
    cube(model,rightLeg,{-2,0,-2},{4,12,4},{0,22});
    int leftLeg=part(model,"left_leg",{2,12,0});
    cube(model,leftLeg,{-2,0,-2},{4,12,4},{0,22},0,true);
    if(illager)
    {
        model.textureHeight=64;
        model.parts[head].cubes.clear();
        cube(model,head,{-4,-10,-4},{8,10,8},{0,0});
        cube(model,head,{-4,-10,-4},{8,12,8},{32,0},.45f);
        int rightArm=part(model,"right_arm",{-5,2,0});
        cube(model,rightArm,{-3,-2,-2},{4,12,4},{40,46});
        int leftArm=part(model,"left_arm",{5,2,0});
        cube(model,leftArm,{-1,-2,-2},{4,12,4},{40,46},0,true);
    }
    if(witch)
    {
        cube(model,nose,{0,3,-6.75f},{1,1,1},{0,0},-.25f);
        int hat=part(model,"hat",{-5,-10.03125f,-5},head);
        cube(model,hat,{0,0,0},{10,2,10},{0,64});
        int hat2=part(model,"hat2",{1.75f,-4,2},hat,{-0.05236f,0,0.02618f});
        cube(model,hat2,{0,0,0},{7,4,7},{0,76});
        int hat3=part(model,"hat3",{1.75f,-4,2},hat2,{-0.10472f,0,0.05236f});
        cube(model,hat3,{0,0,0},{4,4,4},{0,87});
        int hat4=part(model,"hat4",{1.75f,-2,2},hat3,{-0.20944f,0,0.10472f});
        cube(model,hat4,{0,0,0},{1,2,1},{0,95},.25f);
    }
    return model;
}

MobModelDefinition wolf()
{
    MobModelDefinition model;
    int head=part(model,"head",{-1,13.5f,-7});
    cube(model,head,{-2,-3,-2},{6,6,4},{0,0});
    cube(model,head,{-2,-5,0},{2,2,1},{16,14});
    cube(model,head,{2,-5,0},{2,2,1},{16,14});
    cube(model,head,{-.5f,0,-5},{3,3,4},{0,10});
    int body=part(model,"body",{0,14,2},-1,{std::numbers::pi_v<float>/2,0,0});
    cube(model,body,{-3,-2,-3},{6,9,6},{18,14});
    int mane=part(model,"mane",{-1,14,2},-1,{std::numbers::pi_v<float>/2,0,0});
    cube(model,mane,{-3,-3,-3},{8,6,7},{21,0});
    const std::array<glm::vec3,4> pivots{{{-2.5f,16,7},{.5f,16,7},{-2.5f,16,-4},{.5f,16,-4}}};
    for(int i=0;i<4;++i)
    {
        const std::string name="leg"+std::to_string(i+1);
        int leg=part(model,name.c_str(),pivots[i]);
        cube(model,leg,{0,0,-1},{2,8,2},{0,18});
    }
    int tail=part(model,"tail",{-1,12,8},-1,{0.7f,0,0});
    cube(model,tail,{0,0,-1},{2,8,2},{9,18});
    return model;
}

MobModelDefinition genericSpecial(MobModelKind kind)
{
    MobModelDefinition model;
    switch(kind)
    {
        case MobModelKind::Guardian:
        {
            model.textureHeight=64;
            int body=part(model,"body",{0,16,0});
            cube(model,body,{-6,-6,-8},{12,12,16},{0,0});
            cube(model,body,{-8,-6,-6},{16,12,12},{0,28});
            for(int i=0;i<12;++i)
            {
                const float a=static_cast<float>(i)*std::numbers::pi_v<float>/6;
                const std::string name="spine"+std::to_string(i);
                int spine=part(model,name.c_str(),{std::cos(a)*6,16+std::sin(a)*6,0});
                cube(model,spine,{-1,-1,-4},{2,2,4},{0,0});
            }
            int tail=part(model,"tail",{0,16,8});
            cube(model,tail,{-2,-2,0},{4,4,8},{40,0});
            int tail2=part(model,"tail2",{0,0,8},tail);
            cube(model,tail2,{-1.5f,-1.5f,0},{3,3,7},{0,54});
            break;
        }
        case MobModelKind::Horse:
        {
            model.textureWidth=128; model.textureHeight=128;
            int body=part(model,"body",{0,11,5}); cube(model,body,{-5,-8,-17},{10,10,22},{0,32});
            int neck=part(model,"neck",{0,4,-10},-1,{0.5236f,0,0}); cube(model,neck,{-2.5f,-8,-4},{5,14,8},{0,35});
            int head=part(model,"head",{0,4,-10},neck); cube(model,head,{-3,-11,-2},{6,5,7},{0,0});
            cube(model,head,{-2,-11,-7},{4,3,5},{0,12});
            for(int i=0;i<4;++i){const bool back=i<2; const bool left=i%2==0; const std::string n="leg"+std::to_string(i+1); int l=part(model,n.c_str(),{left?-4.0f:4.0f,14,back?7.0f:-10.0f}); cube(model,l,{-1.5f,0,-1.5f},{3,10,3},{48,21}); int shin=part(model,(n+"_shin").c_str(),{0,10,0},l); cube(model,shin,{-1,0,-1},{2,8,2},{48,21});}
            int tail=part(model,"tail",{0,3,14},-1,{0.8f,0,0}); cube(model,tail,{-1,-1,0},{2,3,12},{42,36});
            break;
        }
        case MobModelKind::IronGolem:
        {
            model.textureWidth=128; model.textureHeight=128;
            int head=part(model,"head",{0,-7,-2}); cube(model,head,{-4,-12,-5.5f},{8,10,8},{0,0}); cube(model,head,{-1,-5.5f,-7.5f},{2,4,2},{24,0});
            int body=part(model,"body",{0,-7,0}); cube(model,body,{-9,-2,-6},{18,12,11},{0,40}); cube(model,body,{-5,10,-4},{10,5,7},{0,70});
            for(int i=0;i<2;++i){const std::string n=i?"left_arm":"right_arm"; int a=part(model,n.c_str(),{i?9.0f:-9.0f,-5,0}); cube(model,a,{i?-3.0f:-1.0f,-2,-2},{4,30,6},{60,21});}
            for(int i=0;i<2;++i){const std::string n=i?"left_leg":"right_leg"; int l=part(model,n.c_str(),{i?5.0f:-4.0f,11,0}); cube(model,l,{-3,0,-3},{6,16,5},{37,0});}
            break;
        }
        case MobModelKind::Llama:
        {
            model.textureWidth=128; model.textureHeight=64;
            int head=part(model,"head",{0,7,-6}); cube(model,head,{-2,-14,-10},{4,4,9},{0,0}); cube(model,head,{-4,-16,-6},{8,8,8},{0,14}); cube(model,head,{-4,-19,-4},{3,3,2},{17,0}); cube(model,head,{1,-19,-4},{3,3,2},{17,0});
            int body=part(model,"body",{0,5,2},-1,{std::numbers::pi_v<float>/2,0,0}); cube(model,body,{-6,-10,-7},{12,18,10},{29,0});
            for(int i=0;i<4;++i){const std::string n="leg"+std::to_string(i+1); int l=part(model,n.c_str(),{i%2?-3.5f:3.5f,14,i<2?6.0f:-5.0f}); cube(model,l,{-2,0,-2},{4,10,4},{29,29});}
            break;
        }
        case MobModelKind::Ocelot:
        {
            model.textureWidth=64; model.textureHeight=32;
            int head=part(model,"head",{0,15,-9}); cube(model,head,{-2.5f,-2,-3},{5,4,5},{0,0}); cube(model,head,{-2,-3,-1},{1,1,2},{0,0}); cube(model,head,{1,-3,-1},{1,1,2},{0,0}); cube(model,head,{-.5f,0,-4},{1,1,2},{0,0});
            int body=part(model,"body",{0,12,-10},-1,{std::numbers::pi_v<float>/2,0,0}); cube(model,body,{-2,-3,-8},{4,16,6},{20,0});
            for(int i=0;i<4;++i){const std::string n="leg"+std::to_string(i+1); int l=part(model,n.c_str(),{i%2?-1.1f:1.1f,i<2?18.0f:14.1f,i<2?5.0f:-5.0f}); cube(model,l,{-1,0,-1},{2,i<2?6:10,2},{0,16});}
            int tail=part(model,"tail",{0,15,8},-1,{0.9f,0,0}); cube(model,tail,{-1,0,-1},{2,8,2},{0,15}); int tail2=part(model,"tail2",{0,8,0},tail,{0.5f,0,0}); cube(model,tail2,{-1,0,-1},{2,8,2},{0,15});
            break;
        }
        case MobModelKind::Parrot:
        {
            model.textureWidth=32; model.textureHeight=32;
            int body=part(model,"body",{0,16.5f,-3},-1,{0.4937f,0,0}); cube(model,body,{-1.5f,0,-1.5f},{3,6,3},{2,8});
            int head=part(model,"head",{0,15.69f,-2.76f}); cube(model,head,{-2,-3,-2},{4,3,3},{2,2}); cube(model,head,{-.5f,-4,-1},{1,2,1},{10,0}); cube(model,head,{-.5f,-2,-3},{1,1,2},{11,7});
            int rw=part(model,"right_wing",{-1.5f,16.94f,-2.76f},-1,{-0.698f,-std::numbers::pi_v<float>,0}); cube(model,rw,{-.5f,0,-1.5f},{1,5,3},{19,8}); int lw=part(model,"left_wing",{1.5f,16.94f,-2.76f},-1,{-0.698f,-std::numbers::pi_v<float>,0}); cube(model,lw,{-.5f,0,-1.5f},{1,5,3},{19,8});
            for(int i=0;i<2;++i){const std::string n=i?"left_leg":"right_leg"; int l=part(model,n.c_str(),{i?.9f:-.9f,22, -1.05f}); cube(model,l,{-.5f,0,-.5f},{1,2,1},{14,18});}
            break;
        }
        case MobModelKind::PolarBear:
        {
            model=quadruped(12,false,false,false); model.textureWidth=128; model.textureHeight=64;
            auto& h=named(model,"head"); h.pivot={0,10,-16}; h.cubes.clear(); cube(model,find(model,"head"),{-3.5f,-3,-3},{7,7,7},{0,0}); cube(model,find(model,"head"),{-2.5f,1,-6},{5,3,3},{0,44}); cube(model,find(model,"head"),{-4.5f,-4,-1},{3,3,1},{26,0}); cube(model,find(model,"head"),{1.5f,-4,-1},{3,3,1},{26,0});
            auto& b=named(model,"body"); b.pivot={0,9,2}; b.cubes.clear(); cube(model,find(model,"body"),{-7,-8,-19},{14,14,28},{0,19}); cube(model,find(model,"body"),{-6,-10,-7},{12,12,10},{39,0});
            break;
        }
        case MobModelKind::Rabbit:
        {
            model.textureWidth=64; model.textureHeight=32;
            int body=part(model,"body",{0,19,-2},-1,{-0.349f,0,0}); cube(model,body,{-3,-2,-10},{6,5,10},{0,0});
            int head=part(model,"head",{0,16,-1},-1,{-0.175f,0,0}); cube(model,head,{-2.5f,-4,-5},{5,4,5},{32,0}); cube(model,head,{-.5f,-2.5f,-5.5f},{1,1,1},{32,9});
            for(int i=0;i<2;++i){const std::string n=i?"left_ear":"right_ear"; int e=part(model,n.c_str(),{i?1.0f:-1.0f,-4,0},head,{0,i?.2618f:-.2618f,0}); cube(model,e,{-1,-5,-1},{2,5,1},{i?58:52,0});}
            for(int i=0;i<2;++i){const std::string n=i?"left_haunch":"right_haunch"; int h=part(model,n.c_str(),{i?3.0f:-3.0f,17.5f,3.7f},-1,{-0.349f,0,0}); cube(model,h,{-2.5f,-4.5f,-2.5f},{5,5,5},{30,15});}
            for(int i=0;i<2;++i){const std::string n=i?"left_foot":"right_foot"; int f=part(model,n.c_str(),{i?3.0f:-3.0f,17.5f,3.7f},-1,{-0.349f,0,0}); cube(model,f,{-2,-1,-7},{4,2,7},{26,24});}
            int tail=part(model,"tail",{0,20,7}); cube(model,tail,{-1.5f,-1.5f,0},{3,3,2},{52,6});
            break;
        }
        case MobModelKind::Shulker:
        {
            model.textureWidth=64; model.textureHeight=64;
            int base=part(model,"base",{0,24,0}); cube(model,base,{-8,-8,-8},{16,8,16},{0,28});
            int lid=part(model,"lid",{0,16,0}); cube(model,lid,{-8,-8,-8},{16,12,16},{0,0});
            int head=part(model,"head",{0,12,0}); cube(model,head,{-3,-3,-3},{6,6,6},{0,52});
            break;
        }
        case MobModelKind::SnowGolem:
        {
            model.textureWidth=64; model.textureHeight=64;
            int head=part(model,"head",{0,4,0}); cube(model,head,{-4,-8,-4},{8,8,8},{0,0},-.5f);
            int body=part(model,"body",{0,13,0}); cube(model,body,{-5,-10,-5},{10,10,10},{0,16},-.5f);
            int bottom=part(model,"bottom",{0,24,0}); cube(model,bottom,{-6,-12,-6},{12,12,12},{0,36},-.5f);
            int ra=part(model,"right_arm",{0,6,0}); cube(model,ra,{-1,0,-1},{12,2,2},{32,0},-.5f);
            int la=part(model,"left_arm",{0,6,0}); cube(model,la,{-1,0,-1},{12,2,2},{32,0},-.5f);
            break;
        }
        case MobModelKind::Wither:
        {
            model.textureWidth=64; model.textureHeight=64;
            int shoulders=part(model,"shoulders"); cube(model,shoulders,{-10,3.9f,-.5f},{20,3,3},{0,16});
            int spine=part(model,"spine",{-2,6.9f,-.5f}); cube(model,spine,{0,0,0},{3,10,3},{0,22}); cube(model,spine,{-4,1.5f,.5f},{11,2,2},{24,22}); cube(model,spine,{-4,4,.5f},{11,2,2},{24,22}); cube(model,spine,{-4,6.5f,.5f},{11,2,2},{24,22});
            int lower=part(model,"lower",{-2,16.9f,2}); cube(model,lower,{0,0,0},{3,6,3},{12,22});
            int head=part(model,"head",{0,4,0}); cube(model,head,{-4,-4,-4},{8,8,8},{0,0});
            int rh=part(model,"right_head",{-8,4,0}); cube(model,rh,{-4,-4,-4},{6,6,6},{32,0}); int lh=part(model,"left_head",{10,4,0}); cube(model,lh,{-4,-4,-4},{6,6,6},{32,0});
            break;
        }
        case MobModelKind::Dragon:
        {
            model.textureWidth=256; model.textureHeight=256;
            int head=part(model,"head",{0,0,-16}); cube(model,head,{-6,-1,-8},{12,5,16},{176,44}); cube(model,head,{-8,-8,-10},{16,16,16},{112,30}); cube(model,head,{-5,-12,-4},{2,4,6},{112,0}); cube(model,head,{3,-12,-4},{2,4,6},{112,0});
            int neck=part(model,"neck",{0,0,0}); cube(model,neck,{-5,-5,-5},{10,10,10},{192,104});
            int body=part(model,"body",{0,4,8}); cube(model,body,{-12,0,-16},{24,24,64},{0,0});
            for(int side=0;side<2;++side){const float sign=side?1.0f:-1.0f; const std::string n=side?"left_wing":"right_wing"; int wing=part(model,n.c_str(),{sign*12,5,2}); cube(model,wing,{sign<0?-56.0f:0.0f,0,-4},{56,4,8},{112,88}); cube(model,wing,{sign<0?-56.0f:0.0f,0,4},{56,1,56},{0,0});}
            for(int i=0;i<5;++i){const std::string n="tail"+std::to_string(i); int tail=part(model,n.c_str(),{0,10,40.0f+i*8}); cube(model,tail,{-4,-4,-4},{8,8,8},{192,104});}
            break;
        }
        default:
            throw std::logic_error("Unhandled special 1.12 mob model");
    }
    return model;
}
}

MobModelDefinition createMobModel(MobModelKind kind)
{
    switch(kind)
    {
        case MobModelKind::Biped: return biped(false,false,false);
        case MobModelKind::Skeleton: return biped(true,false,false);
        case MobModelKind::Enderman: return biped(false,true,false);
        case MobModelKind::ZombieVillager: return biped(false,false,true);
        case MobModelKind::Quadruped: return quadruped(6,false,false,false);
        case MobModelKind::Cow: return quadruped(12,false,true,false);
        case MobModelKind::Pig: return quadruped(6,true,false,false);
        case MobModelKind::Sheep: return quadruped(6,false,false,true);
        case MobModelKind::Spider: return spider();
        case MobModelKind::Creeper: return creeper();
        case MobModelKind::Chicken: return chicken();
        case MobModelKind::Vex: return vex();
        case MobModelKind::Bat: return bat();
        case MobModelKind::Blaze: return blaze();
        case MobModelKind::Ghast: return ghast();
        case MobModelKind::Silverfish:
        case MobModelKind::Endermite: return segmented(kind);
        case MobModelKind::Slime: return slime(false);
        case MobModelKind::MagmaCube: return slime(true);
        case MobModelKind::Squid: return squid();
        case MobModelKind::Villager: return villager(false,false);
        case MobModelKind::Illager: return villager(false,true);
        case MobModelKind::Witch: return villager(true,false);
        case MobModelKind::Wolf: return wolf();
        case MobModelKind::Guardian:
        case MobModelKind::Dragon:
        case MobModelKind::Horse:
        case MobModelKind::IronGolem:
        case MobModelKind::Llama:
        case MobModelKind::Ocelot:
        case MobModelKind::Parrot:
        case MobModelKind::PolarBear:
        case MobModelKind::Rabbit:
        case MobModelKind::Shulker:
        case MobModelKind::Wither:
        case MobModelKind::SnowGolem:
            return genericSpecial(kind);
        case MobModelKind::Count: break;
    }
    throw std::logic_error("Invalid 1.12 mob model kind");
}

void animateMobModel(
    MobModelKind kind,
    MobModelDefinition& model,
    const MobPoseState& state)
{
    const float pi=std::numbers::pi_v<float>;
    const float walk=state.limbSwing*0.6662f;
    const float amount=std::clamp(state.limbSwingAmount,0.0f,1.0f);
    const auto rotateHead=[&]()
    {
        if(find(model,"head")>=0)
            named(model,"head").rotation={state.headPitch,state.headYaw,0};
        if(find(model,"headwear")>=0)
            named(model,"headwear").rotation={state.headPitch,state.headYaw,0};
    };
    rotateHead();
    const auto animateBiped=[&]()
    {
        named(model,"right_arm").rotation.x=std::cos(walk+pi)*amount;
        named(model,"left_arm").rotation.x=std::cos(walk)*amount;
        named(model,"right_leg").rotation.x=std::cos(walk)*1.4f*amount;
        named(model,"left_leg").rotation.x=std::cos(walk+pi)*1.4f*amount;
        named(model,"right_arm").rotation.z+=std::cos(state.age*.09f)*.05f+.05f;
        named(model,"left_arm").rotation.z-=std::cos(state.age*.09f)*.05f+.05f;
        named(model,"right_arm").rotation.x+=std::sin(state.age*.067f)*.05f;
        named(model,"left_arm").rotation.x-=std::sin(state.age*.067f)*.05f;
    };
    const auto animateZombieArms=[&]()
    {
        const float swing=std::sin(state.attackProgress*pi);
        const float eased=std::sin(
            (1.0f-(1.0f-state.attackProgress)*(1.0f-state.attackProgress))*pi
        );
        auto& right=named(model,"right_arm");
        auto& left=named(model,"left_arm");
        right.rotation.z=0.0f;
        left.rotation.z=0.0f;
        right.rotation.y=-(0.1f-swing*0.6f);
        left.rotation.y=0.1f-swing*0.6f;
        const float raised=state.aggressive ? -pi/1.5f : -pi/2.25f;
        right.rotation.x=raised+swing*1.2f-eased*.4f;
        left.rotation.x=raised+swing*1.2f-eased*.4f;
        right.rotation.z+=std::cos(state.age*.09f)*.05f+.05f;
        left.rotation.z-=std::cos(state.age*.09f)*.05f+.05f;
        right.rotation.x+=std::sin(state.age*.067f)*.05f;
        left.rotation.x-=std::sin(state.age*.067f)*.05f;
    };
    switch(kind)
    {
        case MobModelKind::Biped:
        case MobModelKind::ZombieVillager:
            animateBiped();
            animateZombieArms();
            break;
        case MobModelKind::Skeleton:
            animateBiped();
            if(state.aggressive)
            {
                auto& right=named(model,"right_arm");
                auto& left=named(model,"left_arm");
                right.rotation.y=-.1f+state.headYaw;
                left.rotation.y=.1f+state.headYaw+.4f;
                right.rotation.x=-pi/2+state.headPitch;
                left.rotation.x=-pi/2+state.headPitch;
            }
            break;
        case MobModelKind::Enderman:
            animateBiped();
            for(const char* limb:{"right_arm","left_arm","right_leg","left_leg"})
                named(model,limb).rotation.x=std::clamp(
                    named(model,limb).rotation.x*.5f,-.4f,.4f
                );
            if(state.aggressive)
                named(model,"head").pivot.y=-13.0f;
            break;
        case MobModelKind::Vex:
            animateBiped();
            named(model,"right_leg").rotation.x+=pi/5.0f;
            named(model,"right_wing").rotation.y=.471239f+
                std::cos(state.age*.8f)*pi*.05f;
            named(model,"left_wing").rotation.y=-
                named(model,"right_wing").rotation.y;
            if(state.aggressive)
                named(model,"right_arm").rotation.x=3.7699115f;
            break;
        case MobModelKind::Quadruped:
        case MobModelKind::Cow:
        case MobModelKind::Pig:
        case MobModelKind::Sheep:
        case MobModelKind::PolarBear:
            named(model,"leg1").rotation.x=std::cos(walk)*1.4f*amount;
            named(model,"leg2").rotation.x=std::cos(walk+pi)*1.4f*amount;
            named(model,"leg3").rotation.x=std::cos(walk+pi)*1.4f*amount;
            named(model,"leg4").rotation.x=std::cos(walk)*1.4f*amount;
            if(kind==MobModelKind::Sheep && state.attackProgress>0.0f)
            {
                named(model,"head").pivot.y=6.0f+state.attackProgress*4.0f;
                named(model,"head").rotation.x=state.attackProgress*1.1f;
            }
            break;
        case MobModelKind::Creeper:
            for(int i=0;i<4;++i) named(model,("leg"+std::to_string(i+1)).c_str()).rotation.x=std::cos(walk+(i%2?pi:0))*1.4f*amount;
            break;
        case MobModelKind::Chicken:
            named(model,"right_leg").rotation.x=std::cos(walk)*1.4f*amount;
            named(model,"left_leg").rotation.x=std::cos(walk+pi)*1.4f*amount;
            named(model,"right_wing").rotation.z=state.age;
            named(model,"left_wing").rotation.z=-state.age;
            break;
        case MobModelKind::Spider:
        {
            const std::array<float,8> baseZ{
                -pi/4,pi/4,-.58119464f,.58119464f,
                -.58119464f,.58119464f,-pi/4,pi/4
            };
            const std::array<float,8> baseY{
                pi/4,-pi/4,.3926991f,-.3926991f,
                -.3926991f,.3926991f,-pi/4,pi/4
            };
            const std::array<float,4> yawDelta{
                -std::cos(walk*2.0f)*.4f*amount,
                -std::cos(walk*2.0f+pi)*.4f*amount,
                -std::cos(walk*2.0f+pi/2)*.4f*amount,
                -std::cos(walk*2.0f+pi*1.5f)*.4f*amount
            };
            const std::array<float,4> rollDelta{
                std::abs(std::sin(walk)*.4f)*amount,
                std::abs(std::sin(walk+pi)*.4f)*amount,
                std::abs(std::sin(walk+pi/2)*.4f)*amount,
                std::abs(std::sin(walk+pi*1.5f)*.4f)*amount
            };
            for(int pair=0;pair<4;++pair)
            {
                auto& first=named(model,("leg"+std::to_string(pair*2+1)).c_str());
                auto& second=named(model,("leg"+std::to_string(pair*2+2)).c_str());
                first.rotation={0,baseY[pair*2]+yawDelta[pair],
                                baseZ[pair*2]+rollDelta[pair]};
                second.rotation={0,baseY[pair*2+1]-yawDelta[pair],
                                 baseZ[pair*2+1]-rollDelta[pair]};
            }
            break;
        }
        case MobModelKind::Bat:
            named(model,"body").rotation.x=pi/4+std::cos(state.age*.1f)*.15f;
            named(model,"right_wing").rotation.y=std::cos(state.age*1.3f)*pi*.25f;
            named(model,"left_wing").rotation.y=-named(model,"right_wing").rotation.y;
            named(model,"right_outer_wing").rotation.y=named(model,"right_wing").rotation.y*.5f;
            named(model,"left_outer_wing").rotation.y=-named(model,"right_outer_wing").rotation.y;
            break;
        case MobModelKind::Blaze:
            for(int i=0;i<12;++i)
            {
                auto& rod=named(model,("rod"+std::to_string(i)).c_str());
                const int ring=i/4; const float a=state.age*(ring==1?-.03f:.03f)+i*pi/2;
                const float radius=ring==1?7.0f:9.0f;
                rod.pivot={std::cos(a)*radius,(ring==0? -2.0f:ring==1?8.0f:16.0f)+std::cos(a*2),std::sin(a)*radius};
            }
            break;
        case MobModelKind::Ghast:
            for(int i=0;i<9;++i) named(model,("tentacle"+std::to_string(i)).c_str()).rotation.x=.2f*std::sin(state.age*.3f+i)+.4f;
            break;
        case MobModelKind::Silverfish:
        case MobModelKind::Endermite:
            for(std::size_t i=0;i<model.parts.size();++i) model.parts[i].rotation.y=std::cos(state.age*.9f+static_cast<float>(i)*.15f*pi)*pi*.05f*(1+std::abs(static_cast<int>(i)-2));
            break;
        case MobModelKind::Squid:
            for(int i=0;i<8;++i) named(model,("tentacle"+std::to_string(i)).c_str()).rotation.x=.6f+.35f*std::sin(state.age*.25f);
            break;
        case MobModelKind::Slime:
        case MobModelKind::MagmaCube:
            for(auto& p:model.parts) p.pivot.y+=std::sin(state.age*.5f)*state.jumpProgress;
            break;
        case MobModelKind::Horse:
        case MobModelKind::Llama:
        case MobModelKind::Ocelot:
            for(int i=0;i<4;++i) if(find(model,("leg"+std::to_string(i+1)).c_str())>=0) named(model,("leg"+std::to_string(i+1)).c_str()).rotation.x=std::cos(walk+(i==0||i==3?0:pi))*1.4f*amount;
            if(find(model,"tail")>=0) named(model,"tail").rotation.y=std::cos(walk)*.4f*amount;
            break;
        case MobModelKind::Wolf:
            if(state.sitting)
            {
                named(model,"mane").pivot={-1,16,-3};
                named(model,"mane").rotation.x=pi*2/5;
                named(model,"body").pivot={0,18,0};
                named(model,"body").rotation.x=pi/4;
                named(model,"tail").pivot={-1,21,6};
                named(model,"leg1").pivot={-2.5f,22,2};
                named(model,"leg2").pivot={.5f,22,2};
                named(model,"leg1").rotation.x=pi*1.5f;
                named(model,"leg2").rotation.x=pi*1.5f;
                named(model,"leg3").pivot={-2.49f,17,-4};
                named(model,"leg4").pivot={.51f,17,-4};
                named(model,"leg3").rotation.x=5.811947f;
                named(model,"leg4").rotation.x=5.811947f;
            }
            else
            {
                for(int i=0;i<4;++i)
                    named(model,("leg"+std::to_string(i+1)).c_str()).rotation.x=
                        std::cos(walk+(i==0||i==3?0:pi))*1.4f*amount;
                named(model,"tail").rotation.y=state.aggressive
                    ? 0.0f : std::cos(walk)*1.4f*amount;
            }
            named(model,"head").rotation.z=state.begging ? .2f : 0.0f;
            named(model,"tail").rotation.x=.8f;
            break;
        case MobModelKind::Rabbit:
            for(const char* n:{"right_haunch","left_haunch"}) named(model,n).rotation.x=-.349f+state.jumpProgress*.45f;
            for(const char* n:{"right_foot","left_foot"}) named(model,n).rotation.x=-.349f+state.jumpProgress*.9f;
            break;
        case MobModelKind::Parrot:
            named(model,"right_wing").rotation.z=-.0873f-state.jumpProgress;
            named(model,"left_wing").rotation.z=.0873f+state.jumpProgress;
            break;
        case MobModelKind::Guardian:
            for(int i=0;i<12;++i){auto& s=named(model,("spine"+std::to_string(i)).c_str()); const float scale=1.0f+.1f*std::cos(state.age*1.5f+i); s.pivot*=scale;}
            named(model,"tail").rotation.y=std::sin(state.age*.3f)*.3f;
            named(model,"tail2").rotation.y=std::sin(state.age*.3f)*.6f;
            break;
        case MobModelKind::Shulker:
            named(model,"lid").pivot.y=16.0f-std::abs(std::sin(state.age*.05f))*5.0f;
            named(model,"lid").rotation.y=std::sin(state.age*.05f)*.25f;
            break;
        case MobModelKind::Wither:
            named(model,"spine").rotation.x=(.065f+.05f*std::cos(state.age*.1f))*pi;
            named(model,"lower").rotation.x=(.265f+.1f*std::cos(state.age*.1f))*pi;
            break;
        case MobModelKind::Dragon:
            named(model,"left_wing").rotation.z=.25f+std::sin(state.age*.2f)*.35f;
            named(model,"right_wing").rotation.z=-named(model,"left_wing").rotation.z;
            for(int i=0;i<5;++i) named(model,("tail"+std::to_string(i)).c_str()).rotation.y=std::sin(state.age*.1f+i*.45f)*.2f;
            break;
        case MobModelKind::IronGolem:
            named(model,"right_leg").rotation.x=-1.5f*std::cos(walk)*amount;
            named(model,"left_leg").rotation.x=1.5f*std::cos(walk)*amount;
            named(model,"right_arm").rotation.x=state.aggressive?-2.0f+1.5f*std::sin(state.attackProgress*pi):std::cos(walk+pi)*amount;
            named(model,"left_arm").rotation.x=named(model,"right_arm").rotation.x;
            break;
        case MobModelKind::SnowGolem:
            named(model,"body").rotation.y=state.headYaw*.25f;
            named(model,"right_arm").rotation.z=1.0f;
            named(model,"left_arm").rotation.z=-1.0f;
            named(model,"right_arm").rotation.y=named(model,"body").rotation.y;
            named(model,"left_arm").rotation.y=pi+named(model,"body").rotation.y;
            break;
        case MobModelKind::Villager:
        case MobModelKind::Illager:
        case MobModelKind::Witch:
            named(model,"right_leg").rotation.x=std::cos(walk)*.7f*amount;
            named(model,"left_leg").rotation.x=std::cos(walk+pi)*.7f*amount;
            if(kind==MobModelKind::Illager)
            {
                if(state.aggressive)
                {
                    named(model,"arms").cubes.clear();
                    named(model,"right_arm").rotation.x=-1.8849558f+
                        std::cos(state.age*.09f)*.15f;
                    named(model,"left_arm").rotation.x=
                        std::cos(state.age*.19f)*.5f;
                }
                else
                {
                    named(model,"right_arm").cubes.clear();
                    named(model,"left_arm").cubes.clear();
                }
            }
            if(kind==MobModelKind::Witch){named(model,"nose").rotation.x=std::sin(state.age*.06f)*.0785f; named(model,"nose").rotation.z=std::cos(state.age*.06f)*.0436f;}
            break;
        case MobModelKind::Count: break;
    }
}
}
