#include "content/ContentCatalog.h"
#include "content/ContentModule.h"
#include "core/Registry.h"
#include "engine/FixedStepClock.h"
#include "game/GameBootstrap.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <memory>
#include <stdexcept>

namespace
{
struct TestValue
{
    int number = 0;
};

class TestContentModule final : public mc::content::ContentModule
{
public:
    [[nodiscard]] mc::core::ResourceLocation id() const override
    {
        return mc::core::ResourceLocation("test:module");
    }

    void registerContent(mc::content::ContentCatalog& catalog) override
    {
        catalog.registerBlock(
            mc::core::ResourceLocation("test:decorative_block"),
            {std::nullopt, "Test Decorative Block", {}, {}, {}}
        );
        registered = true;
    }

    bool registered = false;
};
}

int main()
{
    using mc::core::Registry;
    using mc::core::ResourceLocation;

    assert(ResourceLocation("stone").toString() == "minecraft:stone");
    assert(ResourceLocation("examplemod", "machine/frame").toString() ==
           "examplemod:machine/frame");

    bool invalidRejected = false;
    try
    {
        [[maybe_unused]] ResourceLocation invalid("Bad Namespace:block");
    }
    catch (const std::invalid_argument&)
    {
        invalidRejected = true;
    }
    assert(invalidRejected);

    Registry<TestValue> registry(ResourceLocation("test:values"));
    registry.registerValue(ResourceLocation("test:first"), {11});
    registry.registerValue(ResourceLocation("test:second"), {22});
    registry.freeze();
    assert(registry.frozen());
    assert(registry.runtimeId(ResourceLocation("test:first")) == 0);
    assert(registry.entry(1)->value.number == 22);

    bool frozenRejected = false;
    try
    {
        registry.registerValue(ResourceLocation("test:late"), {33});
    }
    catch (const std::logic_error&)
    {
        frozenRejected = true;
    }
    assert(frozenRejected);

    mc::engine::FixedStepClock clock(20.0, 5);
    assert(clock.advance(0.124) == 2);
    assert(clock.tickCount() == 2);
    assert(std::abs(clock.partialTick() - 0.48f) < 0.0001f);

    mc::game::GameBootstrap bootstrap;
    auto testModule = std::make_unique<TestContentModule>();
    TestContentModule* testModuleView = testModule.get();
    bootstrap.addContentModule(std::move(testModule));
    bootstrap.loadContentModules();
    assert(testModuleView->registered);

    bool lateModuleRejected = false;
    try
    {
        bootstrap.addContentModule(std::make_unique<TestContentModule>());
    }
    catch (const std::logic_error&)
    {
        lateModuleRejected = true;
    }
    assert(lateModuleRejected);

    bootstrap.freezeRegistries();
    const mc::content::ContentCatalog& content = bootstrap.content();
    assert(content.frozen());
    assert(content.blocks().size() ==
           static_cast<std::size_t>(BlockType::TNT) + 2U);
    assert(content.blocks().find(
        ResourceLocation("test:decorative_block")
    ) != nullptr);
    const mc::content::BlockState modState = content.defaultState(
        ResourceLocation("test:decorative_block")
    );
    assert(modState.blockRuntimeId() >
           static_cast<mc::core::RuntimeId>(BlockType::TNT));
    assert(bootstrap.gameplay().dimensions().size() == 3U);
    assert(bootstrap.gameplay().mobs().size() >= 8U);
    assert(bootstrap.gameplay().structures().size() >= 6U);
    assert(!content.legacyBlock(modState));
    assert(content.block(BlockType::Stone)->displayName == "Stone");
    assert(content.item(ItemType::DiamondPickaxe)->displayName ==
           "Diamond Pickaxe");
    const auto* stoneTexture = content.block(BlockType::Stone)
        ->textures.resolve(BlockFace::Top, 0);
    assert(stoneTexture != nullptr);
    assert(stoneTexture->toString() == "minecraft:blocks/stone");

    const mc::content::BlockState furnace =
        content.defaultState(BlockType::Furnace);
    assert(furnace.properties() == 3);
    assert(content.isValidState(furnace));
    assert(!content.isValidState(
        mc::content::BlockState(BlockType::Furnace, 0)
    ));
    assert(content.isValidState(
        mc::content::BlockState(BlockType::Water, 15)
    ));
    const auto furnaceProperties = content.serializeStateProperties(furnace);
    assert(furnaceProperties.size() == 1);
    assert(furnaceProperties.front().first == "facing");
    assert(furnaceProperties.front().second == "south");
    const auto decodedFurnace = content.state(
        ResourceLocation("minecraft:furnace"), furnaceProperties
    );
    assert(decodedFurnace && *decodedFurnace == furnace);
    assert(!content.isValidState(
        mc::content::BlockState(BlockType::Stone, 1)
    ));

    const std::filesystem::path textureRoot =
        std::filesystem::path("assets") / "textures" / "blocks";
    for (const auto& entry : content.blocks().entries())
    {
        if (entry.value.legacyType)
        {
            assert(entry.value.stateSchema.accepts(content.defaultState(
                *entry.value.legacyType
            )));
        }
        const auto verifyTexture = [&](const auto& textureName)
        {
            if (!textureName)
                return;
            assert(textureName->nameSpace() == "minecraft");
            constexpr std::string_view prefix = "blocks/";
            assert(textureName->path().starts_with(prefix));
            const std::filesystem::path file = textureRoot /
                (textureName->path().substr(prefix.size()) + ".png");
            assert(std::filesystem::is_regular_file(file));
        };
        const mc::content::BlockTextures& textures = entry.value.textures;
        verifyTexture(textures.all);
        verifyTexture(textures.side);
        verifyTexture(textures.top);
        verifyTexture(textures.bottom);
        verifyTexture(textures.front);
        verifyTexture(textures.back);
        verifyTexture(textures.left);
        verifyTexture(textures.right);
        verifyTexture(textures.sideOverlay);
    }
}
