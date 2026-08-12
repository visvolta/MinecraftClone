#include "client/render/ModelBakery.h"
#include "content/ContentCatalog.h"
#include "content/resources/ResourcePack.h"
#include "game/GameBootstrap.h"

#include <iostream>
#include <stdexcept>

namespace
{
void require(bool value, const char* message)
{
    if (!value) throw std::runtime_error(message);
}
}

int main()
{
    try
    {
        mc::game::GameBootstrap bootstrap("assets");
        bootstrap.loadContentModules();
        bootstrap.freezeRegistries();

        const auto& catalog = bootstrap.content();

        for (const char* name : {
                 "minecraft:double_grass",
                 "minecraft:double_fern"})
        {
            const auto* block = catalog.blocks().find(
                mc::core::ResourceLocation(name)
            );
            require(block != nullptr, "Missing double-plant resource block");
            require(
                block->behaviour.tint == mc::content::BlockTint::Grass,
                "Double grass/fern must use grass biome tint"
            );
        }

        const mc::content::resources::ResourcePack resources("assets");
        const mc::client::ModelBakery bakery(resources, catalog);

        const auto cactus = bakery.bakeModel(
            mc::core::ResourceLocation("minecraft:item/cactus")
        );
        require(!cactus.quads.empty(), "Cactus item model has no quads");
        require(!cactus.elementBoxes.empty(), "Cactus item model has no geometry");

        bool inset = false;
        for (const auto& box : cactus.elementBoxes)
        {
            if (box.minimum.x > 0.0001f || box.minimum.z > 0.0001f ||
                box.maximum.x < 0.9999f || box.maximum.z < 0.9999f)
                inset = true;
        }
        require(inset, "Cactus item model incorrectly became a full cube");

        std::cout
            << "PASS: resource tint and dropped ItemBlock model parity.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Render/item parity failure: " << error.what() << '\n';
        return 1;
    }
}
