#pragma once

#include "gameplay/GameplayRegistries.h"

#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace mc::client
{
struct MobModelCube
{
    glm::vec3 origin{};
    glm::ivec3 size{};
    glm::ivec2 textureOffset{};
    float inflate = 0.0f;
    bool mirror = false;
    std::uint8_t textureLayer = 0;
};

struct MobModelPart
{
    std::string name;
    glm::vec3 pivot{};
    glm::vec3 rotation{};
    int parent = -1;
    std::vector<MobModelCube> cubes;
};

struct MobModelDefinition
{
    int textureWidth = 64;
    int textureHeight = 32;
    std::vector<MobModelPart> parts;
};

struct MobPoseState
{
    float age = 0.0f;
    float limbSwing = 0.0f;
    float limbSwingAmount = 0.0f;
    float headYaw = 0.0f;
    float headPitch = 0.0f;
    float attackProgress = 0.0f;
    float jumpProgress = 0.0f;
    float hurtProgress = 0.0f;
    float deathProgress = 0.0f;
    bool onGround = true;
    bool inWater = false;
    bool aggressive = false;
};

[[nodiscard]] MobModelDefinition createMobModel(
    gameplay::MobModelKind kind
);
void animateMobModel(
    gameplay::MobModelKind kind,
    MobModelDefinition& model,
    const MobPoseState& state
);
}
