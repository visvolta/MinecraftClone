#include "Chunk.h"
#include "BlockShape.h"
#include "client/render/ModelBakery.h"
#include "client/render/MobModel.h"
#include "content/BlockStateLogic.h"
#include "content/resources/ResourcePack.h"
#include "game/GameBootstrap.h"
#include "gameplay/SurvivalStats.h"
#include "worldgen/BetaSimplexNoise.h"
#include "worldgen/Biome.h"
#include "worldgen/BiomeMap.h"
#include "worldgen/JavaRandom.h"

#include <cassert>
#include <array>
#include <cmath>
#include <filesystem>
#include <vector>
#include <string>

int main()
{
    static_assert(Chunk::HEIGHT == 256);
    static_assert(Chunk::SECTION_COUNT == 16);

    JavaRandom simplexRandom(2345LL);
    const BetaSimplexNoise simplex(simplexRandom);
    std::vector<double> simplexOutput;
    simplex.add(simplexOutput, 0.0, 0.0, 1, 1, 0.0225, 0.0225, 1.0);
    assert(simplexOutput.size() == 1U);
    assert(std::isfinite(simplexOutput.front()));

    Chunk highChunk(4, -2);
    assert(highChunk.setBlock(3, 220, 7, BlockType::Stone));
    assert(highChunk.getWorldSurfaceHeight(3, 7) == 221);
    const ChunkSnapshot highSnapshot = highChunk.snapshot();
    assert(highSnapshot.paletteIndices.size() == Chunk::BLOCK_COUNT);
    Chunk restored;
    restored.restore(highSnapshot);
    assert(restored.getBlock(3, 220, 7) == BlockType::Stone);

    const BiomeRegistry& vanilla = BiomeRegistry::vanilla();
    assert(vanilla.find(VanillaBiomes::Jungle)->treeFeature == TreeFeature::Jungle);
    assert(vanilla.find(VanillaBiomes::Jungle)->treesPerChunk == 50);
    assert(vanilla.find(VanillaBiomes::JungleEdge)->treeFeature == TreeFeature::JungleEdge);
    assert(vanilla.find(VanillaBiomes::JungleEdge)->treesPerChunk == 2);
    assert(vanilla.find(VanillaBiomes::Savanna)->treeFeature == TreeFeature::Savanna);
    assert(vanilla.find(VanillaBiomes::Savanna)->treesPerChunk == 1);
    assert(vanilla.find(VanillaBiomes::Swampland)->treeFeature == TreeFeature::Swamp);
    assert(vanilla.find(VanillaBiomes::Swampland)->treesPerChunk == 2);
    assert(vanilla.find(VanillaBiomes::ExtremeHills)->treeFeature == TreeFeature::Hills);
    assert(vanilla.find(VanillaBiomes::RoofedForest)->roofedForestDecoration);
    assert(vanilla.find(VanillaBiomes::Forest)->treeFeature == TreeFeature::Forest);
    assert(vanilla.find(VanillaBiomes::Forest)->treesPerChunk == 10);
    assert(vanilla.find(VanillaBiomes::FlowerForest)->treesPerChunk == 6);
    assert(vanilla.find(VanillaBiomes::IcePlains)->treeFeature == TreeFeature::Spruce);
    assert(vanilla.find(VanillaBiomes::MesaPlateauF)->treeFeature == TreeFeature::OakOnly);
    assert(vanilla.find(VanillaBiomes::MegaSpruceTaiga)->treeFeature == TreeFeature::MegaSpruceTaiga);
    assert(vanilla.find(VanillaBiomes::BirchForestMountains)->treeFeature == TreeFeature::TallBirch);
    assert(vanilla.find(VanillaBiomes::Desert)->generationWeights[0] == 3);
    assert(vanilla.find(VanillaBiomes::Savanna)->generationWeights[0] == 2);
    assert(vanilla.find(VanillaBiomes::Plains)->generationWeights[0] == 1);
    assert(vanilla.find(VanillaBiomes::Forest)->generationWeights[1] == 1);
    assert(vanilla.find(VanillaBiomes::ColdTaiga)->generationWeights[3] == 1);
    assert(vanilla.find(VanillaBiomes::IcePlains)->generationWeights[3] == 3);
    assert(vanilla.find(VanillaBiomes::MesaBryce)->mutationOf == VanillaBiomes::Mesa);
    const auto hasSpawn = [](const std::vector<BiomeMobSpawn>& spawns,
                             const char* entity, int weight)
    {
        for (const BiomeMobSpawn& spawn : spawns)
            if (spawn.entity == mc::core::ResourceLocation(entity) &&
                spawn.weight == weight)
                return true;
        return false;
    };
    assert(hasSpawn(
        vanilla.find(VanillaBiomes::Plains)->creatureSpawns,
        "minecraft:horse", 5
    ));
    assert(hasSpawn(
        vanilla.find(VanillaBiomes::Desert)->monsterSpawns,
        "minecraft:husk", 80
    ));
    assert(hasSpawn(
        vanilla.find(VanillaBiomes::Jungle)->creatureSpawns,
        "minecraft:parrot", 40
    ));
    assert(hasSpawn(
        vanilla.find(VanillaBiomes::Jungle)->monsterSpawns,
        "minecraft:ocelot", 2
    ));
    assert(!hasSpawn(
        vanilla.find(VanillaBiomes::Jungle)->creatureSpawns,
        "minecraft:ocelot", 2
    ));
    assert(hasSpawn(
        vanilla.find(VanillaBiomes::Desert)->monsterSpawns,
        "minecraft:zombie_villager", 1
    ));
    assert(!hasSpawn(
        vanilla.find(VanillaBiomes::Desert)->monsterSpawns,
        "minecraft:zombie_villager", 5
    ));
    assert(hasSpawn(
        vanilla.find(VanillaBiomes::IcePlains)->monsterSpawns,
        "minecraft:skeleton", 20
    ));
    assert(hasSpawn(
        vanilla.find(VanillaBiomes::IcePlains)->monsterSpawns,
        "minecraft:stray", 80
    ));
    assert(vanilla.find(VanillaBiomes::Ocean)->creatureSpawns.empty());
    assert(hasSpawn(
        vanilla.find(VanillaBiomes::Swampland)->monsterSpawns,
        "minecraft:slime", 1
    ));
    assert(vanilla.find(VanillaBiomes::Void)->monsterSpawns.empty());

    const BiomeMap biomeMap(123456789);
    const auto firstSamples = biomeMap.sampleArea(-32, 48, 16, 16);
    const auto secondSamples = biomeMap.sampleArea(-32, 48, 16, 16);
    assert(firstSamples.size() == 256U);
    assert(secondSamples.size() == firstSamples.size());
    for (std::size_t index = 0; index < firstSamples.size(); ++index)
    {
        assert(firstSamples[index].biome == secondSamples[index].biome);
        assert(vanilla.find(firstSamples[index].biome) != nullptr);
    }
    for (int x = 0; x < 16; ++x)
    {
        for (int z = 0; z < 16; ++z)
        {
            const ClimateSample direct = biomeMap.sample(-32 + x, 48 + z);
            const ClimateSample& batched = firstSamples[static_cast<std::size_t>(
                x * 16 + z
            )];
            assert(direct.biome == batched.biome);
        }
    }
    const auto spawnBiome = biomeMap.findSpawnBiomePosition();
    assert(spawnBiome);
    const BiomeId spawnBiomeId = biomeMap.sample(
        spawnBiome->first, spawnBiome->second
    ).biome;
    assert(spawnBiomeId == VanillaBiomes::Forest ||
           spawnBiomeId == VanillaBiomes::Plains ||
           spawnBiomeId == VanillaBiomes::Taiga ||
           spawnBiomeId == VanillaBiomes::TaigaHills ||
           spawnBiomeId == VanillaBiomes::ForestHills ||
           spawnBiomeId == VanillaBiomes::Jungle ||
           spawnBiomeId == VanillaBiomes::JungleHills);
    BiomeRegistry custom = BiomeRegistry::mutableVanilla();
    const BiomeId customId = custom.nextCustomId();
    assert(customId >= 256);
    BiomeDefinition customBiome;
    customBiome.id = customId;
    customBiome.name = mc::core::ResourceLocation("test:crystal_fields");
    customBiome.displayName = "Crystal Fields";
    custom.registerBiome(customBiome);
    custom.freeze();
    assert(custom.find(mc::core::ResourceLocation("test:crystal_fields")) != nullptr);

    mc::gameplay::SurvivalStats survival;
    int health = 10;
    for (int tick = 0; tick < 80; ++tick)
        survival.tick(true, true, health, 20);
    assert(health == 11);
    survival.addExperience(100);
    assert(survival.experienceLevel() > 0);
    survival.resetAttackCooldown();
    assert(survival.attackStrength() == 0.0f);
    for (int tick = 0; tick < 5; ++tick)
        survival.tick(false, false, health, 20);
    assert(survival.attackStrength() == 1.0f);

    mc::game::GameBootstrap bootstrap(std::filesystem::path("assets"));
    bootstrap.loadContentModules();
    bootstrap.freezeRegistries();
    const mc::content::resources::ResourcePack resources(
        std::filesystem::path("assets")
    );
    const mc::client::ModelBakery bakery(resources, bootstrap.content());
    std::size_t resourceStateCount = 0;
    for (const auto& entry : bootstrap.content().blocks().entries())
    {
        if (entry.value.legacyType)
            continue;
        for (std::size_t properties = 0;
             properties < entry.value.stateSchema.stateCount(); ++properties)
        {
            const auto state = mc::content::BlockState::fromRuntimeId(
                entry.runtimeId, static_cast<std::uint16_t>(properties)
            );
            assert(bootstrap.content().isValidState(state));
            static_cast<void>(bakery.bake(state, 0U));
            ++resourceStateCount;
        }
    }
    assert(resourceStateCount > 2000U);

    for (std::uint8_t rawModel = 0;
         rawModel < static_cast<std::uint8_t>(
             mc::gameplay::MobModelKind::Count
         ); ++rawModel)
    {
        const auto kind = static_cast<mc::gameplay::MobModelKind>(rawModel);
        mc::client::MobModelDefinition mobModel =
            mc::client::createMobModel(kind);
        assert(!mobModel.parts.empty());
        std::size_t cubeCount = 0;
        for (std::size_t partIndex=0; partIndex<mobModel.parts.size();
             ++partIndex)
        {
            const mc::client::MobModelPart& part=mobModel.parts[partIndex];
            assert(part.parent<static_cast<int>(partIndex));
            cubeCount += part.cubes.size();
            for(const mc::client::MobModelCube& cube:part.cubes)
            {
                assert(cube.size.x>0 && cube.size.y>0 && cube.size.z>0);
                assert(cube.textureOffset.x>=0 && cube.textureOffset.y>=0);
                assert(cube.textureOffset.x+cube.size.z*2+
                       cube.size.x*2<=mobModel.textureWidth);
                assert(cube.textureOffset.y+cube.size.z+
                       cube.size.y<=mobModel.textureHeight*2);
            }
        }
        assert(cubeCount > 0);
        mc::client::animateMobModel(
            kind, mobModel,
            {20.0f, 4.0f, 0.8f, 0.2f, -0.1f,
             0.5f, 0.6f, 0.0f, 0.0f, false, false, true}
        );
    }
    const mc::client::BakedModel stone = bakery.bake(
        bootstrap.content().defaultState(BlockType::Stone), 1234U
    );
    assert(stone.quads.size() == 6U);
    for (const mc::client::BakedQuad& quad : stone.quads)
        assert(quad.texture.toString() == "minecraft:blocks/stone");

    const mc::content::BlockState netherrackState =
        bootstrap.content().defaultState(BlockType::Netherrack);
    const mc::client::BakedModel netherrackZero = bakery.bake(
        netherrackState, 0U
    );
    const mc::client::BakedModel netherrackOne = bakery.bake(
        netherrackState, 1U
    );
    assert(netherrackZero.quads.size() == netherrackOne.quads.size());
    bool weightedVariantDiffers = false;
    for (std::size_t quadIndex = 0;
         quadIndex < netherrackZero.quads.size(); ++quadIndex)
    {
        const mc::client::BakedQuad& first = netherrackZero.quads[quadIndex];
        const mc::client::BakedQuad& second = netherrackOne.quads[quadIndex];
        if (first.face != second.face)
            weightedVariantDiffers = true;
        for (std::size_t vertexIndex = 0; vertexIndex < first.positions.size();
             ++vertexIndex)
        {
            const glm::vec3 positionDelta =
                first.positions[vertexIndex] - second.positions[vertexIndex];
            const glm::vec2 uvDelta = first.textureCoordinates[vertexIndex] -
                                      second.textureCoordinates[vertexIndex];
            if (positionDelta.x * positionDelta.x +
                    positionDelta.y * positionDelta.y +
                    positionDelta.z * positionDelta.z > 0.00000001F ||
                uvDelta.x * uvDelta.x + uvDelta.y * uvDelta.y > 0.00000001F)
                weightedVariantDiffers = true;
        }
    }
    assert(weightedVariantDiffers);

    const auto fenceState = bootstrap.content().state(
        mc::core::ResourceLocation("minecraft:fence"),
        std::array<std::pair<std::string, std::string>, 2>{
            std::pair{"north", "true"}, std::pair{"east", "true"}
        }
    );
    assert(fenceState);
    const mc::client::BakedModel fence = bakery.bake(*fenceState, 99U);
    assert(fence.quads.size() > 6U);
    assert(!fence.elementBoxes.empty());
    registerModelBlockShape(
        *fenceState,
        fence.elementBoxes,
        ModelBlockShapeKind::Fence,
        false
    );
    for (const BlockBox& box : getBlockShape(*fenceState).collisionBoxes)
        assert(box.maximum.y == 1.5f);

    const auto stairState = bootstrap.content().state(
        mc::core::ResourceLocation("minecraft:oak_stairs"),
        std::array<std::pair<std::string, std::string>, 3>{
            std::pair{"facing", "east"},
            std::pair{"half", "bottom"},
            std::pair{"shape", "straight"}
        }
    );
    assert(stairState);
    const mc::content::BlockDefinition* stairDefinition =
        bootstrap.content().block(*stairState);
    assert(stairDefinition);
    assert(stairDefinition->behaviour.traits.solid);
    assert(!stairDefinition->behaviour.traits.plant);
    assert(!stairDefinition->behaviour.traits.opaque);
    const mc::client::BakedModel stair = bakery.bake(*stairState, 0U);
    assert(stair.elementBoxes.size() == 2U);
    registerModelBlockShape(
        *stairState,
        stair.elementBoxes,
        ModelBlockShapeKind::Solid,
        false
    );
    assert(getBlockShape(*stairState).collisionBoxes.size() == 2U);
    const auto northStairState = bootstrap.content().state(
        mc::core::ResourceLocation("minecraft:oak_stairs"),
        std::array<std::pair<std::string, std::string>, 3>{
            std::pair{"facing", "north"},
            std::pair{"half", "bottom"},
            std::pair{"shape", "straight"}
        }
    );
    assert(northStairState);
    const mc::content::BlockState cornerStair =
        mc::content::resolveActualBlockState(
            *stairState,
            {{{}, *northStairState, {}, {}}},
            {}
        );
    bool outerLeft = false;
    for (const auto& [name, value] :
         bootstrap.content().serializeStateProperties(cornerStair))
        if (name == "shape")
            outerLeft = value == "outer_left";
    assert(outerLeft);

    const auto disconnectedFence = bootstrap.content().state(
        mc::core::ResourceLocation("minecraft:fence"),
        std::array<std::pair<std::string, std::string>, 4>{
            std::pair{"north", "false"}, std::pair{"east", "false"},
            std::pair{"south", "false"}, std::pair{"west", "false"}
        }
    );
    assert(disconnectedFence);
    const mc::content::BlockState connectedFence =
        mc::content::resolveActualBlockState(
            *disconnectedFence,
            {{bootstrap.content().defaultState(BlockType::Stone), {}, {}, {}}},
            {}
        );
    bool northConnected = false;
    for (const auto& [name, value] :
         bootstrap.content().serializeStateProperties(connectedFence))
    {
        if (name == "north")
            northConnected = value == "true";
    }
    assert(northConnected);

    const mc::content::BlockState fenceAgainstGlass =
        mc::content::resolveActualBlockState(
            *disconnectedFence,
            {{bootstrap.content().defaultState(BlockType::Glass), {}, {}, {}}},
            {}
        );
    for (const auto& [name, value] :
         bootstrap.content().serializeStateProperties(fenceAgainstGlass))
        if (name == "north")
            assert(value == "false");

    const auto disconnectedPane = bootstrap.content().state(
        mc::core::ResourceLocation("minecraft:glass_pane"),
        std::array<std::pair<std::string, std::string>, 4>{
            std::pair{"north", "false"}, std::pair{"east", "false"},
            std::pair{"south", "false"}, std::pair{"west", "false"}
        }
    );
    assert(disconnectedPane);
    const mc::content::BlockState paneAgainstGlass =
        mc::content::resolveActualBlockState(
            *disconnectedPane,
            {{bootstrap.content().defaultState(BlockType::Glass), {}, {}, {}}},
            {}
        );
    for (const auto& [name, value] :
         bootstrap.content().serializeStateProperties(paneAgainstGlass))
        if (name == "north")
            assert(value == "true");
    clearModelBlockShapes();
}
