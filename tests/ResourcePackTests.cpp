#include "content/resources/ResourcePack.h"

#include <cassert>
#include <filesystem>
#include <random>

int main()
{
    using mc::content::resources::ResourcePack;
    using mc::core::ResourceLocation;

    const ResourcePack resources(std::filesystem::path("assets"));

    assert(resources.blockStateNames().size() == 407U);
    assert(resources.itemModelNames().size() == 717U);
    assert(resources.lootTableNames().size() >= 83U);
    const auto wireStates = resources.blockStateCombinations(
        ResourceLocation("minecraft:redstone_wire")
    );
    assert(wireStates.size() == 81U);
    assert(resources.blockStateCombinations(
        ResourceLocation("minecraft:fence")).size() == 16U);
    assert(resources.blockStateCombinations(
        ResourceLocation("minecraft:cobblestone_wall")).size() == 32U);
    assert(resources.blockStateCombinations(
        ResourceLocation("minecraft:glass_pane")).size() == 16U);

    const auto furnace = resources.loadBlockState(
        ResourceLocation("minecraft:furnace")
    );
    assert(furnace.variants.size() == 4);
    assert(furnace.variants.at("facing=east").front().rotationY == 90);

    const auto stone = resources.resolveModel(
        ResourceLocation("minecraft:block/stone")
    );
    assert(stone.elements.size() == 1);
    assert(stone.elements.front().faces.size() == 6);
    const auto textures = resources.textureDependencies(stone);
    assert(textures.contains(ResourceLocation("minecraft:blocks/stone")));

    const auto item = resources.resolveModel(
        ResourceLocation("minecraft:item/stone")
    );
    assert(item.elements.size() == 1);
    assert(item.display.contains("gui"));
    assert(item.display.at("gui").rotation[0] == 30.0f);

    const auto mirrored = resources.resolveModel(
        ResourceLocation("minecraft:block/stone_mirrored")
    );
    const auto& mirroredDown = mirrored.elements.front().faces.at("down");
    assert(mirroredDown.uv);
    assert((*mirroredDown.uv)[0] == 16.0f);
    assert((*mirroredDown.uv)[2] == 0.0f);

    std::mt19937 random(1122U);
    bool cookedBeef = false;
    bool leather = false;
    for (int sample = 0; sample < 32; ++sample)
    {
        for (const auto& drop : resources.rollLootTable(
                 ResourceLocation("minecraft:entities/cow"),
                 {.killedByPlayer = true, .onFire = true, .lootingLevel = 3},
                 random))
        {
            cookedBeef = cookedBeef || drop.item ==
                ResourceLocation("minecraft:cooked_beef");
            leather = leather || drop.item ==
                ResourceLocation("minecraft:leather");
        }
    }
    assert(cookedBeef);
    assert(leather);
}
