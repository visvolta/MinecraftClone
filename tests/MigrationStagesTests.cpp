#include "Chunk.h"
#include "client/render/ModelBakery.h"
#include "content/resources/ResourcePack.h"
#include "game/GameBootstrap.h"
#include "gameplay/SurvivalStats.h"
#include "worldgen/Biome.h"

#include <cassert>
#include <filesystem>

int main()
{
    static_assert(Chunk::HEIGHT == 256);
    static_assert(Chunk::SECTION_COUNT == 16);

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
    assert(vanilla.find(VanillaBiomes::Savanna)->treeFeature == TreeFeature::Savanna);
    assert(vanilla.find(VanillaBiomes::MesaBryce)->mutationOf == VanillaBiomes::Mesa);
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

    mc::game::GameBootstrap bootstrap;
    bootstrap.loadContentModules();
    bootstrap.freezeRegistries();
    const mc::content::resources::ResourcePack resources(
        std::filesystem::path("assets")
    );
    const mc::client::ModelBakery bakery(resources, bootstrap.content());
    const mc::client::BakedModel stone = bakery.bake(
        bootstrap.content().defaultState(BlockType::Stone), 1234U
    );
    assert(stone.quads.size() == 6U);
    for (const mc::client::BakedQuad& quad : stone.quads)
        assert(quad.texture.toString() == "minecraft:blocks/stone");
}
