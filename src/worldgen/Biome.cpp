#include "worldgen/Biome.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
const BiomeRegistry* ActiveRegistry = nullptr;

BiomeDefinition biome(
    BiomeId id,
    const char* name,
    const char* displayName,
    float baseHeight,
    float variation,
    float temperature,
    float rainfall)
{
    BiomeDefinition result;
    result.id = id;
    result.name = mc::core::ResourceLocation("minecraft", name);
    result.displayName = displayName;
    result.baseHeight = baseHeight;
    result.heightVariation = variation;
    result.temperature = temperature;
    result.rainfall = rainfall;
    // Biome.java installs these four lists for every ordinary biome. Derived
    // biomes below replace or extend them exactly where 1.12.2 does.
    result.monsterSpawns = {
        {mc::core::ResourceLocation("minecraft:spider"), 100, 4, 4},
        {mc::core::ResourceLocation("minecraft:zombie"), 95, 4, 4},
        {mc::core::ResourceLocation("minecraft:zombie_villager"), 5, 1, 1},
        {mc::core::ResourceLocation("minecraft:skeleton"), 100, 4, 4},
        {mc::core::ResourceLocation("minecraft:creeper"), 100, 4, 4},
        {mc::core::ResourceLocation("minecraft:slime"), 100, 4, 4},
        {mc::core::ResourceLocation("minecraft:enderman"), 10, 1, 4},
        {mc::core::ResourceLocation("minecraft:witch"), 5, 1, 1}
    };
    result.creatureSpawns = {
        {mc::core::ResourceLocation("minecraft:sheep"), 12, 4, 4},
        {mc::core::ResourceLocation("minecraft:pig"), 10, 4, 4},
        {mc::core::ResourceLocation("minecraft:chicken"), 10, 4, 4},
        {mc::core::ResourceLocation("minecraft:cow"), 8, 4, 4}
    };
    result.waterCreatureSpawns = {
        {mc::core::ResourceLocation("minecraft:squid"), 10, 4, 4}
    };
    result.ambientSpawns = {
        {mc::core::ResourceLocation("minecraft:bat"), 10, 8, 8}
    };
    return result;
}

BiomeRegistry makeVanillaRegistry()
{
    BiomeRegistry result;
    const auto add = [&result](BiomeDefinition definition)
    {
        result.registerBiome(std::move(definition));
    };

    auto value = biome(0, "ocean", "Ocean", -1.0f, 0.1f, 0.5f, 0.5f);
    value.treeFeature = TreeFeature::None; add(std::move(value));
    value = biome(1, "plains", "Plains", 0.125f, 0.05f, 0.8f, 0.4f);
    value.treeFeature = TreeFeature::Plains;
    value.treesPerChunk = 0; value.extraTreeChance = 0.05f;
    value.grassPerChunk = 10; value.flowersPerChunk = 4; add(std::move(value));
    value = biome(2, "desert", "Desert", 0.125f, 0.05f, 2.0f, 0.0f);
    value.rain = false; value.topBlock = BlockType::Sand; value.fillerBlock = BlockType::Sand;
    value.treeFeature = TreeFeature::None; value.treesPerChunk = -999;
    value.grassPerChunk = 0; value.flowersPerChunk = 0; add(std::move(value));
    value = biome(3, "extreme_hills", "Extreme Hills", 1.0f, 0.5f, 0.2f, 0.3f);
    value.treeFeature = TreeFeature::Hills; value.treesPerChunk = 0; add(std::move(value));
    value = biome(4, "forest", "Forest", 0.1f, 0.2f, 0.7f, 0.8f);
    value.treeFeature = TreeFeature::Forest; value.treesPerChunk = 10;
    value.grassPerChunk = 2; value.flowersPerChunk = 2; add(std::move(value));
    value = biome(5, "taiga", "Taiga", 0.2f, 0.2f, 0.25f, 0.8f);
    value.treeFeature = TreeFeature::Taiga; value.treesPerChunk = 10; add(std::move(value));
    value = biome(6, "swampland", "Swampland", -0.2f, 0.1f, 0.8f, 0.9f);
    value.waterColor = 14745518U; value.foliageColor = 0x6A7039U;
    value.treeFeature = TreeFeature::Swamp; value.treesPerChunk = 2;
    value.grassPerChunk = 5; value.flowersPerChunk = 1; add(std::move(value));
    value = biome(7, "river", "River", -0.5f, 0.0f, 0.5f, 0.5f);
    value.treeFeature = TreeFeature::None; add(std::move(value));
    value = biome(8, "hell", "Hell", 0.1f, 0.2f, 2.0f, 0.0f);
    value.rain = false; value.treeFeature = TreeFeature::None; add(std::move(value));
    value = biome(9, "sky", "The End", 0.1f, 0.2f, 0.5f, 0.5f);
    value.rain = false; value.treeFeature = TreeFeature::None; add(std::move(value));
    value = biome(10, "frozen_ocean", "Frozen Ocean", -1.0f, 0.1f, 0.0f, 0.5f);
    value.snowy = true; value.treeFeature = TreeFeature::None; add(std::move(value));
    value = biome(11, "frozen_river", "Frozen River", -0.5f, 0.0f, 0.0f, 0.5f);
    value.snowy = true; value.treeFeature = TreeFeature::None; add(std::move(value));
    value = biome(12, "ice_flats", "Ice Plains", 0.125f, 0.05f, 0.0f, 0.5f);
    value.snowy = true; value.treeFeature = TreeFeature::Spruce;
    value.treesPerChunk = 0; add(std::move(value));
    value = biome(13, "ice_mountains", "Ice Mountains", 0.45f, 0.3f, 0.0f, 0.5f);
    value.snowy = true; value.treeFeature = TreeFeature::Spruce;
    add(std::move(value));
    value = biome(14, "mushroom_island", "Mushroom Island", 0.2f, 0.3f, 0.9f, 1.0f);
    value.treeFeature = TreeFeature::None; value.treesPerChunk = -100;
    value.grassPerChunk = -100; value.flowersPerChunk = -100; add(std::move(value));
    value = biome(15, "mushroom_island_shore", "Mushroom Island Shore", 0.0f, 0.025f, 0.9f, 1.0f);
    value.treeFeature = TreeFeature::None; value.treesPerChunk = -100;
    value.grassPerChunk = -100; value.flowersPerChunk = -100; add(std::move(value));
    value = biome(16, "beaches", "Beach", 0.0f, 0.025f, 0.8f, 0.4f);
    value.topBlock = BlockType::Sand; value.fillerBlock = BlockType::Sand;
    value.treeFeature = TreeFeature::None; value.treesPerChunk = -999; add(std::move(value));
    value = biome(17, "desert_hills", "Desert Hills", 0.45f, 0.3f, 2.0f, 0.0f);
    value.rain = false; value.topBlock = BlockType::Sand; value.fillerBlock = BlockType::Sand;
    value.treeFeature = TreeFeature::None; value.treesPerChunk = -999;
    value.grassPerChunk = 0; value.flowersPerChunk = 0; add(std::move(value));
    value = biome(18, "forest_hills", "Forest Hills", 0.45f, 0.3f, 0.7f, 0.8f);
    value.treeFeature = TreeFeature::Forest; value.treesPerChunk = 10;
    value.grassPerChunk = 2; add(std::move(value));
    value = biome(19, "taiga_hills", "Taiga Hills", 0.45f, 0.3f, 0.25f, 0.8f);
    value.treeFeature = TreeFeature::Taiga; value.treesPerChunk = 10; add(std::move(value));
    value = biome(20, "smaller_extreme_hills", "Extreme Hills Edge", 0.8f, 0.3f, 0.2f, 0.3f);
    value.treeFeature = TreeFeature::Hills; add(std::move(value));
    value = biome(21, "jungle", "Jungle", 0.1f, 0.2f, 0.95f, 0.9f);
    value.treeFeature = TreeFeature::Jungle; value.treesPerChunk = 50;
    value.grassPerChunk = 25; value.flowersPerChunk = 4; add(std::move(value));
    value = biome(22, "jungle_hills", "Jungle Hills", 0.45f, 0.3f, 0.95f, 0.9f);
    value.treeFeature = TreeFeature::Jungle; value.treesPerChunk = 50;
    value.grassPerChunk = 25; value.flowersPerChunk = 4; add(std::move(value));
    value = biome(23, "jungle_edge", "Jungle Edge", 0.1f, 0.2f, 0.95f, 0.8f);
    value.treeFeature = TreeFeature::JungleEdge; value.treesPerChunk = 2;
    value.grassPerChunk = 25; value.flowersPerChunk = 4; add(std::move(value));
    value = biome(24, "deep_ocean", "Deep Ocean", -1.8f, 0.1f, 0.5f, 0.5f);
    value.treeFeature = TreeFeature::None; add(std::move(value));
    value = biome(25, "stone_beach", "Stone Beach", 0.1f, 0.8f, 0.2f, 0.3f);
    value.topBlock = BlockType::Stone; value.fillerBlock = BlockType::Stone;
    value.treeFeature = TreeFeature::None; value.treesPerChunk = -999; add(std::move(value));
    value = biome(26, "cold_beach", "Cold Beach", 0.0f, 0.025f, 0.05f, 0.3f);
    value.snowy = true; value.topBlock = BlockType::Sand; value.fillerBlock = BlockType::Sand;
    value.treeFeature = TreeFeature::None; value.treesPerChunk = -999; add(std::move(value));
    value = biome(27, "birch_forest", "Birch Forest", 0.1f, 0.2f, 0.6f, 0.6f);
    value.treeFeature = TreeFeature::Birch; value.treesPerChunk = 10;
    value.grassPerChunk = 2; add(std::move(value));
    value = biome(28, "birch_forest_hills", "Birch Forest Hills", 0.45f, 0.3f, 0.6f, 0.6f);
    value.treeFeature = TreeFeature::Birch; value.treesPerChunk = 10;
    value.grassPerChunk = 2; add(std::move(value));
    value = biome(29, "roofed_forest", "Roofed Forest", 0.1f, 0.2f, 0.7f, 0.8f);
    value.treeFeature = TreeFeature::RoofedForest; value.treesPerChunk = -999;
    value.grassPerChunk = 2; value.roofedForestDecoration = true;
    add(std::move(value));
    value = biome(30, "taiga_cold", "Cold Taiga", 0.2f, 0.2f, -0.5f, 0.4f);
    value.snowy = true; value.treeFeature = TreeFeature::Taiga; value.treesPerChunk = 10; add(std::move(value));
    value = biome(31, "taiga_cold_hills", "Cold Taiga Hills", 0.45f, 0.3f, -0.5f, 0.4f);
    value.snowy = true; value.treeFeature = TreeFeature::Taiga; value.treesPerChunk = 10; add(std::move(value));
    value = biome(32, "redwood_taiga", "Mega Taiga", 0.2f, 0.2f, 0.3f, 0.8f);
    value.treeFeature = TreeFeature::MegaTaiga; value.treesPerChunk = 10;
    value.grassPerChunk = 7; add(std::move(value));
    value = biome(33, "redwood_taiga_hills", "Mega Taiga Hills", 0.45f, 0.3f, 0.3f, 0.8f);
    value.treeFeature = TreeFeature::MegaTaiga; value.treesPerChunk = 10;
    value.grassPerChunk = 7; add(std::move(value));
    value = biome(34, "extreme_hills_with_trees", "Extreme Hills+", 1.0f, 0.5f, 0.2f, 0.3f);
    value.treeFeature = TreeFeature::Hills; value.treesPerChunk = 3; add(std::move(value));
    value = biome(35, "savanna", "Savanna", 0.125f, 0.05f, 1.2f, 0.0f);
    value.rain = false; value.treeFeature = TreeFeature::Savanna;
    value.treesPerChunk = 1; value.grassPerChunk = 20;
    value.flowersPerChunk = 4; add(std::move(value));
    value = biome(36, "savanna_rock", "Savanna Plateau", 1.5f, 0.025f, 1.0f, 0.0f);
    value.rain = false; value.treeFeature = TreeFeature::Savanna;
    value.treesPerChunk = 1; value.grassPerChunk = 20;
    value.flowersPerChunk = 4; add(std::move(value));
    value = biome(37, "mesa", "Mesa", 0.1f, 0.2f, 2.0f, 0.0f);
    value.rain = false; value.topBlock = BlockType::Sand; value.fillerBlock = BlockType::Sand;
    value.treeFeature = TreeFeature::None; value.treesPerChunk = -999;
    value.grassPerChunk = 0; value.flowersPerChunk = 0; add(std::move(value));
    value = biome(38, "mesa_rock", "Mesa Plateau F", 1.5f, 0.025f, 2.0f, 0.0f);
    value.rain = false; value.topBlock = BlockType::Sand; value.fillerBlock = BlockType::Sand;
    value.treeFeature = TreeFeature::OakOnly; value.treesPerChunk = 5;
    value.grassPerChunk = 0; value.flowersPerChunk = 0; add(std::move(value));
    value = biome(39, "mesa_clear_rock", "Mesa Plateau", 1.5f, 0.025f, 2.0f, 0.0f);
    value.rain = false; value.topBlock = BlockType::Sand; value.fillerBlock = BlockType::Sand;
    value.treeFeature = TreeFeature::None; value.treesPerChunk = -999;
    value.grassPerChunk = 0; value.flowersPerChunk = 0; add(std::move(value));

    const auto weight = [&result](
        BiomeId id,
        BiomeClimateCategory climate,
        int amount)
    {
        BiomeDefinition* definition = result.findMutable(id);
        if (definition == nullptr)
            throw std::logic_error("Missing weighted biome");
        if (climate == BiomeClimateCategory::None)
            throw std::logic_error("Biome generation climate is required");
        definition->generationWeights[
            static_cast<std::size_t>(climate) - 1U
        ] = amount;
    };
    weight(VanillaBiomes::Desert, BiomeClimateCategory::Warm, 3);
    weight(VanillaBiomes::Savanna, BiomeClimateCategory::Warm, 2);
    weight(VanillaBiomes::Plains, BiomeClimateCategory::Warm, 1);
    weight(VanillaBiomes::Forest, BiomeClimateCategory::Medium, 1);
    weight(VanillaBiomes::RoofedForest, BiomeClimateCategory::Medium, 1);
    weight(VanillaBiomes::ExtremeHills, BiomeClimateCategory::Medium, 1);
    weight(VanillaBiomes::Plains, BiomeClimateCategory::Medium, 1);
    weight(VanillaBiomes::BirchForest, BiomeClimateCategory::Medium, 1);
    weight(VanillaBiomes::Swampland, BiomeClimateCategory::Medium, 1);
    weight(VanillaBiomes::Forest, BiomeClimateCategory::Cold, 1);
    weight(VanillaBiomes::ExtremeHills, BiomeClimateCategory::Cold, 1);
    weight(VanillaBiomes::Taiga, BiomeClimateCategory::Cold, 1);
    weight(VanillaBiomes::Plains, BiomeClimateCategory::Cold, 1);
    weight(VanillaBiomes::IcePlains, BiomeClimateCategory::Ice, 3);
    weight(VanillaBiomes::ColdTaiga, BiomeClimateCategory::Ice, 1);

    const auto mutation = [&result, &add](
        BiomeId parentId,
        BiomeId id,
        const char* name,
        const char* displayName,
        std::optional<float> baseHeight = std::nullopt,
        std::optional<float> variation = std::nullopt)
    {
        const BiomeDefinition* parent = result.find(parentId);
        if (parent == nullptr)
            throw std::logic_error("Missing parent biome for mutation");
        BiomeDefinition mutated = *parent;
        mutated.id = id;
        mutated.name = mc::core::ResourceLocation("minecraft", name);
        mutated.displayName = displayName;
        mutated.baseHeight = baseHeight.value_or(parent->baseHeight + 0.1f);
        mutated.heightVariation = variation.value_or(
            parent->heightVariation + 0.2f
        );
        mutated.mutationOf = parentId;
        mutated.generationWeights.fill(0);
        add(std::move(mutated));
    };
    mutation(1, 129, "mutated_plains", "Sunflower Plains", 0.125f, 0.05f);
    mutation(2, 130, "mutated_desert", "Desert M");
    mutation(3, 131, "mutated_extreme_hills", "Extreme Hills M", 1.0f, 0.5f);
    mutation(4, 132, "mutated_forest", "Flower Forest", 0.1f, 0.4f);
    mutation(5, 133, "mutated_taiga", "Taiga M");
    mutation(6, 134, "mutated_swampland", "Swampland M", -0.1f, 0.3f);
    mutation(12, 140, "mutated_ice_flats", "Ice Plains Spikes", 0.425f, 0.45f);
    mutation(21, 149, "mutated_jungle", "Jungle M");
    mutation(23, 151, "mutated_jungle_edge", "Jungle Edge M");
    mutation(27, 155, "mutated_birch_forest", "Birch Forest M");
    mutation(28, 156, "mutated_birch_forest_hills", "Birch Forest Hills M");
    mutation(29, 157, "mutated_roofed_forest", "Roofed Forest M");
    mutation(30, 158, "mutated_taiga_cold", "Cold Taiga M");
    mutation(32, 160, "mutated_redwood_taiga", "Mega Spruce Taiga", 0.2f, 0.2f);
    mutation(33, 161, "mutated_redwood_taiga_hills", "Mega Spruce Taiga Hills", 0.45f, 0.3f);
    mutation(34, 162, "mutated_extreme_hills_with_trees", "Extreme Hills+ M", 1.0f, 0.5f);
    mutation(35, 163, "mutated_savanna", "Savanna M", 0.3625f, 1.225f);
    mutation(36, 164, "mutated_savanna_rock", "Savanna Plateau M", 1.05f, 1.2125f);
    mutation(37, 165, "mutated_mesa", "Mesa (Bryce)", 0.1f, 0.2f);
    mutation(38, 166, "mutated_mesa_rock", "Mesa Plateau F M", 0.45f, 0.3f);
    mutation(39, 167, "mutated_mesa_clear_rock", "Mesa Plateau M", 0.45f, 0.3f);

    if (BiomeDefinition* flowerForest =
            result.findMutable(VanillaBiomes::FlowerForest))
    {
        flowerForest->treesPerChunk = 6;
        flowerForest->flowersPerChunk = 100;
        flowerForest->grassPerChunk = 1;
    }
    if (BiomeDefinition* megaSpruce =
            result.findMutable(VanillaBiomes::MegaSpruceTaiga))
    {
        megaSpruce->temperature = 0.25f;
        megaSpruce->treeFeature = TreeFeature::MegaSpruceTaiga;
    }
    if (BiomeDefinition* megaSpruceHills =
            result.findMutable(VanillaBiomes::MegaSpruceTaigaHills))
    {
        megaSpruceHills->baseHeight = 0.2f;
        megaSpruceHills->heightVariation = 0.2f;
        megaSpruceHills->temperature = 0.25f;
        megaSpruceHills->treeFeature = TreeFeature::MegaSpruceTaiga;
    }
    for (const BiomeId birchMutation : {
             VanillaBiomes::BirchForestMountains,
             VanillaBiomes::BirchForestHillsMountains})
    {
        if (BiomeDefinition* birch = result.findMutable(birchMutation))
            birch->treeFeature = TreeFeature::TallBirch;
    }
    for (const BiomeId savannaMutation : {
             VanillaBiomes::SavannaMountains,
             VanillaBiomes::SavannaPlateauMountains})
    {
        if (BiomeDefinition* savanna = result.findMutable(savannaMutation))
        {
            savanna->treesPerChunk = 2;
            savanna->flowersPerChunk = 2;
            savanna->grassPerChunk = 5;
        }
    }

    const auto configure = [&result](BiomeId id, auto action)
    {
        if (BiomeDefinition* definition = result.findMutable(id))
            action(*definition);
    };
    for (const BiomeId id : {
             VanillaBiomes::Desert,
             VanillaBiomes::DesertHills,
             VanillaBiomes::DesertMountains})
    {
        configure(id, [](BiomeDefinition& definition)
        {
            definition.deadBushesPerChunk = 2;
            definition.reedsPerChunk = 50;
            definition.cactiPerChunk = 10;
        });
    }
    for (const BiomeId id : {
             VanillaBiomes::Mesa,
             VanillaBiomes::MesaPlateauF,
             VanillaBiomes::MesaPlateau,
             VanillaBiomes::MesaBryce,
             VanillaBiomes::MesaPlateauFMountains,
             VanillaBiomes::MesaPlateauMountains})
    {
        configure(id, [](BiomeDefinition& definition)
        {
            definition.deadBushesPerChunk = 20;
            definition.reedsPerChunk = 3;
            definition.cactiPerChunk = 5;
        });
    }
    for (const BiomeId id : {
             VanillaBiomes::Swampland,
             VanillaBiomes::SwamplandMountains})
    {
        configure(id, [](BiomeDefinition& definition)
        {
            definition.deadBushesPerChunk = 1;
            definition.mushroomsPerChunk = 8;
        });
    }
    for (const BiomeId id : {
             VanillaBiomes::Taiga,
             VanillaBiomes::TaigaHills,
             VanillaBiomes::TaigaMountains,
             VanillaBiomes::ColdTaiga,
             VanillaBiomes::ColdTaigaHills,
             VanillaBiomes::ColdTaigaMountains})
    {
        configure(id, [](BiomeDefinition& definition)
        {
            definition.mushroomsPerChunk = 1;
        });
    }
    for (const BiomeId id : {
             VanillaBiomes::MegaTaiga,
             VanillaBiomes::MegaTaigaHills,
             VanillaBiomes::MegaSpruceTaiga,
             VanillaBiomes::MegaSpruceTaigaHills})
    {
        configure(id, [](BiomeDefinition& definition)
        {
            definition.deadBushesPerChunk = 1;
            definition.mushroomsPerChunk = 3;
        });
    }
    for (const BiomeId id : {
             VanillaBiomes::Jungle,
             VanillaBiomes::JungleHills,
             VanillaBiomes::JungleMountains})
    {
        configure(id, [](BiomeDefinition& definition)
        {
            definition.melonsPerChunk = 1;
            definition.vinesPerChunk = 50;
        });
    }
    for (const BiomeId id : {
             VanillaBiomes::MushroomIsland,
             VanillaBiomes::MushroomShore})
    {
        configure(id, [](BiomeDefinition& definition)
        {
            definition.bigMushroomsPerChunk = 1;
            definition.mushroomsPerChunk = 1;
        });
    }

    value = biome(127, "void", "The Void", 0.1f, 0.2f, 0.5f, 0.5f);
    value.rain = false; value.treeFeature = TreeFeature::None; add(std::move(value));

    const auto addCreature = [&result](BiomeId id, BiomeMobSpawn spawn)
    {
        if (BiomeDefinition* definition = result.findMutable(id))
            definition->creatureSpawns.push_back(std::move(spawn));
    };
    const auto clearCreatures = [&result](BiomeId id)
    {
        if (BiomeDefinition* definition = result.findMutable(id))
            definition->creatureSpawns.clear();
    };
    for (const BiomeId id : {
             VanillaBiomes::Ocean, VanillaBiomes::DeepOcean,
             VanillaBiomes::FrozenOcean, VanillaBiomes::River,
             VanillaBiomes::FrozenRiver, VanillaBiomes::Beach,
             VanillaBiomes::ColdBeach, VanillaBiomes::StoneBeach,
             VanillaBiomes::Mesa, VanillaBiomes::MesaPlateauF,
             VanillaBiomes::MesaPlateau, VanillaBiomes::MesaBryce,
             VanillaBiomes::MesaPlateauFMountains,
             VanillaBiomes::MesaPlateauMountains})
        clearCreatures(id);
    for (const BiomeId id : {
             VanillaBiomes::Plains, VanillaBiomes::SunflowerPlains})
    {
        addCreature(id, {mc::core::ResourceLocation("minecraft:horse"), 5, 2, 6});
        addCreature(id, {mc::core::ResourceLocation("minecraft:donkey"), 1, 1, 3});
    }
    for (const BiomeId id : {
             VanillaBiomes::Savanna, VanillaBiomes::SavannaPlateau,
             VanillaBiomes::SavannaMountains,
             VanillaBiomes::SavannaPlateauMountains})
    {
        addCreature(id, {mc::core::ResourceLocation("minecraft:horse"), 1, 2, 6});
        addCreature(id, {mc::core::ResourceLocation("minecraft:donkey"), 1, 1, 1});
    }
    for (const BiomeId id : {
             VanillaBiomes::Forest, VanillaBiomes::ForestHills})
        addCreature(id, {mc::core::ResourceLocation("minecraft:wolf"), 5, 4, 4});
    addCreature(
        VanillaBiomes::FlowerForest,
        {mc::core::ResourceLocation("minecraft:rabbit"), 4, 2, 3}
    );
    for (const BiomeId id : {
             VanillaBiomes::Taiga, VanillaBiomes::TaigaHills,
             VanillaBiomes::TaigaMountains, VanillaBiomes::ColdTaiga,
             VanillaBiomes::ColdTaigaHills,
             VanillaBiomes::ColdTaigaMountains,
             VanillaBiomes::MegaTaiga, VanillaBiomes::MegaTaigaHills,
             VanillaBiomes::MegaSpruceTaiga,
             VanillaBiomes::MegaSpruceTaigaHills})
    {
        addCreature(id, {mc::core::ResourceLocation("minecraft:wolf"), 8, 4, 4});
        addCreature(id, {mc::core::ResourceLocation("minecraft:rabbit"), 4, 2, 3});
    }
    for (const BiomeId id : {
             VanillaBiomes::Jungle, VanillaBiomes::JungleHills,
             VanillaBiomes::JungleMountains})
    {
        if (BiomeDefinition* definition = result.findMutable(id))
            definition->monsterSpawns.push_back({
                mc::core::ResourceLocation("minecraft:ocelot"), 2, 1, 1
            });
        addCreature(id, {mc::core::ResourceLocation("minecraft:parrot"), 40, 1, 2});
        addCreature(id, {mc::core::ResourceLocation("minecraft:chicken"), 10, 4, 4});
    }
    for (const BiomeId id : {
             VanillaBiomes::JungleEdge,
             VanillaBiomes::JungleEdgeMountains})
    {
        addCreature(id, {mc::core::ResourceLocation("minecraft:parrot"), 40, 1, 2});
        addCreature(id, {mc::core::ResourceLocation("minecraft:chicken"), 10, 4, 4});
    }
    for (const BiomeId id : {
             VanillaBiomes::IcePlains, VanillaBiomes::IceMountains,
             VanillaBiomes::IcePlainsSpikes})
    {
        addCreature(id, {mc::core::ResourceLocation("minecraft:rabbit"), 10, 2, 3});
        addCreature(id, {mc::core::ResourceLocation("minecraft:polar_bear"), 1, 1, 2});
    }
    for (const BiomeId id : {
             VanillaBiomes::MushroomIsland, VanillaBiomes::MushroomShore})
    {
        if (BiomeDefinition* definition = result.findMutable(id))
        {
            definition->monsterSpawns.clear();
            definition->waterCreatureSpawns.clear();
            definition->creatureSpawns = {{
                mc::core::ResourceLocation("minecraft:mushroom_cow"), 8, 4, 8
            }};
        }
    }
    for (const BiomeId id : {
             VanillaBiomes::Desert, VanillaBiomes::DesertHills,
             VanillaBiomes::DesertMountains})
    {
        if (BiomeDefinition* definition = result.findMutable(id))
        {
            definition->creatureSpawns.clear();
            definition->creatureSpawns.push_back({
                mc::core::ResourceLocation("minecraft:rabbit"), 4, 2, 3
            });
            std::erase_if(
                definition->monsterSpawns,
                [](const BiomeMobSpawn& spawn)
                {
                    return spawn.entity ==
                               mc::core::ResourceLocation("minecraft:zombie") ||
                           spawn.entity == mc::core::ResourceLocation(
                               "minecraft:zombie_villager"
                           );
                }
            );
            definition->monsterSpawns.push_back({
                mc::core::ResourceLocation("minecraft:zombie"), 19, 4, 4
            });
            definition->monsterSpawns.push_back({
                mc::core::ResourceLocation("minecraft:zombie_villager"), 1, 1, 1
            });
            definition->monsterSpawns.push_back({
                mc::core::ResourceLocation("minecraft:husk"), 80, 4, 4
            });
        }
    }
    for (const BiomeId id : {
             VanillaBiomes::IcePlains, VanillaBiomes::IceMountains,
             VanillaBiomes::IcePlainsSpikes})
    {
        if (BiomeDefinition* definition = result.findMutable(id))
        {
            std::erase_if(
                definition->monsterSpawns,
                [](const BiomeMobSpawn& spawn)
                {
                    return spawn.entity ==
                        mc::core::ResourceLocation("minecraft:skeleton");
                }
            );
            definition->monsterSpawns.push_back({
                mc::core::ResourceLocation("minecraft:skeleton"), 20, 4, 4
            });
            definition->monsterSpawns.push_back({
                mc::core::ResourceLocation("minecraft:stray"), 80, 4, 4
            });
        }
    }
    if (BiomeDefinition* swamp = result.findMutable(VanillaBiomes::Swampland))
        swamp->monsterSpawns.push_back({
            mc::core::ResourceLocation("minecraft:slime"), 1, 1, 1
        });
    for (const BiomeId id : {
             VanillaBiomes::ExtremeHills, VanillaBiomes::ExtremeHillsEdge,
             VanillaBiomes::ExtremeHillsPlus,
             VanillaBiomes::ExtremeHillsMountains,
             VanillaBiomes::ExtremeHillsPlusMountains})
        addCreature(id, {mc::core::ResourceLocation("minecraft:llama"), 5, 4, 6});
    for (const BiomeId id : {
             VanillaBiomes::SavannaMountains,
             VanillaBiomes::SavannaPlateauMountains})
        addCreature(id, {mc::core::ResourceLocation("minecraft:llama"), 8, 4, 4});
    if (BiomeDefinition* nether = result.findMutable(VanillaBiomes::Hell))
    {
        nether->creatureSpawns.clear();
        nether->waterCreatureSpawns.clear();
        nether->ambientSpawns.clear();
        nether->monsterSpawns = {
            {mc::core::ResourceLocation("minecraft:ghast"), 50, 4, 4},
            {mc::core::ResourceLocation("minecraft:zombie_pigman"), 100, 4, 4},
            {mc::core::ResourceLocation("minecraft:magma_cube"), 2, 4, 4},
            {mc::core::ResourceLocation("minecraft:enderman"), 1, 4, 4}
        };
    }
    if (BiomeDefinition* end = result.findMutable(VanillaBiomes::Sky))
    {
        end->creatureSpawns.clear();
        end->waterCreatureSpawns.clear();
        end->ambientSpawns.clear();
        end->monsterSpawns = {{
            mc::core::ResourceLocation("minecraft:enderman"), 10, 4, 4
        }};
    }
    if (BiomeDefinition* empty = result.findMutable(VanillaBiomes::Void))
    {
        empty->monsterSpawns.clear();
        empty->creatureSpawns.clear();
        empty->waterCreatureSpawns.clear();
        empty->ambientSpawns.clear();
    }

    result.freeze();
    return result;
}
}

void BiomeRegistry::registerBiome(BiomeDefinition definition)
{
    if (frozen_)
        throw std::logic_error("Biome registry is frozen");
    if (byId_.contains(definition.id) || byName_.contains(definition.name))
        throw std::logic_error("Duplicate biome registration");
    const std::size_t index = entries_.size();
    byId_.emplace(definition.id, index);
    byName_.emplace(definition.name, index);
    entries_.push_back(std::move(definition));
}

void BiomeRegistry::freeze() { frozen_ = true; }

const BiomeDefinition* BiomeRegistry::find(BiomeId id) const noexcept
{
    const auto found = byId_.find(id);
    return found == byId_.end() ? nullptr : &entries_[found->second];
}

BiomeDefinition* BiomeRegistry::findMutable(BiomeId id) noexcept
{
    if (frozen_)
        return nullptr;
    const auto found = byId_.find(id);
    return found == byId_.end() ? nullptr : &entries_[found->second];
}

const BiomeDefinition* BiomeRegistry::find(
    const mc::core::ResourceLocation& name) const noexcept
{
    const auto found = byName_.find(name);
    return found == byName_.end() ? nullptr : &entries_[found->second];
}

BiomeId BiomeRegistry::nextCustomId() const noexcept
{
    BiomeId candidate = 256;
    while (byId_.contains(candidate) && candidate != 0xFFFFU)
        ++candidate;
    return candidate;
}

bool BiomeRegistry::frozen() const noexcept { return frozen_; }
const std::vector<BiomeDefinition>& BiomeRegistry::entries() const noexcept
{
    return entries_;
}

void BiomeRegistry::activate() const noexcept { ActiveRegistry = this; }

const BiomeRegistry& BiomeRegistry::active() noexcept
{
    return ActiveRegistry == nullptr ? vanilla() : *ActiveRegistry;
}

const BiomeRegistry& BiomeRegistry::vanilla()
{
    static const BiomeRegistry registry = makeVanillaRegistry();
    return registry;
}

BiomeRegistry BiomeRegistry::mutableVanilla()
{
    BiomeRegistry registry = vanilla();
    registry.frozen_ = false;
    return registry;
}

BiomeId classifyReleaseBiome(
    double temperature,
    double humidity,
    double continentalness) noexcept
{
    if (continentalness < -0.72)
        return VanillaBiomes::DeepOcean;
    if (continentalness < -0.48)
        return temperature < 0.15 ? VanillaBiomes::FrozenOcean
                                  : VanillaBiomes::Ocean;
    if (continentalness < -0.38)
        return temperature < 0.15 ? VanillaBiomes::ColdBeach
                                  : VanillaBiomes::Beach;
    if (temperature < 0.12)
        return continentalness > 0.45 ? VanillaBiomes::IceMountains
                                      : VanillaBiomes::IcePlains;
    if (temperature < 0.32)
        return continentalness > 0.48 ? VanillaBiomes::TaigaHills
                                      : VanillaBiomes::Taiga;
    if (continentalness > 0.68)
        return humidity > 0.55 ? VanillaBiomes::ExtremeHillsPlus
                               : VanillaBiomes::ExtremeHills;
    if (temperature > 0.82 && humidity < 0.24)
        return continentalness > 0.45 ? VanillaBiomes::DesertHills
                                      : VanillaBiomes::Desert;
    if (temperature > 0.75 && humidity > 0.72)
        return continentalness > 0.45 ? VanillaBiomes::JungleHills
                                      : VanillaBiomes::Jungle;
    if (temperature > 0.70 && humidity < 0.42)
        return VanillaBiomes::Savanna;
    if (humidity > 0.82 && continentalness < 0.08)
        return VanillaBiomes::Swampland;
    if (humidity > 0.68)
        return continentalness > 0.45 ? VanillaBiomes::ForestHills
                                      : VanillaBiomes::Forest;
    if (humidity > 0.52)
        return VanillaBiomes::BirchForest;
    return VanillaBiomes::Plains;
}

const char* biomeName(BiomeId biome) noexcept
{
    const BiomeDefinition* definition = BiomeRegistry::active().find(biome);
    return definition == nullptr ? "Unknown" : definition->displayName.c_str();
}
