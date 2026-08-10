#include "content/resources/ResourcePack.h"

#include <cassert>
#include <filesystem>

int main()
{
    using mc::content::resources::ResourcePack;
    using mc::core::ResourceLocation;

    const ResourcePack resources(std::filesystem::path("assets"));

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
}
