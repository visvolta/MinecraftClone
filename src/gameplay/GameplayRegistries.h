#pragma once

#include "core/Registry.h"

#include <cstdint>
#include <string>

namespace mc::gameplay
{
struct DimensionDefinition
{
    int legacyId = 0;
    std::string displayName;
    int minimumY = 0;
    int height = 256;
    double coordinateScale = 1.0;
    bool hasSkyLight = true;
    bool ultrawarm = false;
    bool natural = true;
};

enum class MobCategory : std::uint8_t
{
    Monster,
    Creature,
    Ambient,
    WaterCreature
};

struct MobDefinition
{
    std::string displayName;
    MobCategory category = MobCategory::Creature;
    float width = 0.6f;
    float height = 1.8f;
    int trackingRange = 80;
    int spawnWeight = 10;
    int minimumGroup = 1;
    int maximumGroup = 4;
};

struct StructureDefinition
{
    std::string displayName;
    int spacing = 32;
    int separation = 8;
    std::uint32_t salt = 0;
    bool terrainAdapted = true;
};

struct AdvancementDefinition
{
    std::string displayName;
    core::ResourceLocation parent{"minecraft:root"};
    core::ResourceLocation trigger{"minecraft:impossible"};
};

class GameplayRegistries
{
public:
    GameplayRegistries();

    DimensionDefinition& registerDimension(
        core::ResourceLocation name,
        DimensionDefinition definition
    );
    MobDefinition& registerMob(
        core::ResourceLocation name,
        MobDefinition definition
    );
    StructureDefinition& registerStructure(
        core::ResourceLocation name,
        StructureDefinition definition
    );
    AdvancementDefinition& registerAdvancement(
        core::ResourceLocation name,
        AdvancementDefinition definition
    );
    void freeze();

    [[nodiscard]] const core::Registry<DimensionDefinition>& dimensions() const noexcept;
    [[nodiscard]] const core::Registry<MobDefinition>& mobs() const noexcept;
    [[nodiscard]] const core::Registry<StructureDefinition>& structures() const noexcept;
    [[nodiscard]] const core::Registry<AdvancementDefinition>& advancements() const noexcept;

private:
    core::Registry<DimensionDefinition> dimensions_;
    core::Registry<MobDefinition> mobs_;
    core::Registry<StructureDefinition> structures_;
    core::Registry<AdvancementDefinition> advancements_;
};

void registerVanillaGameplay(GameplayRegistries& registries);
}
