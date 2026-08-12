#include "content/ContentCatalog.h"
#include "content/ContentModule.h"
#include "core/Registry.h"
#include "engine/FixedStepClock.h"
#include "game/GameBootstrap.h"

#include <cassert>
#include <array>
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
    // Resource-pack and structure-compatibility blocks may legitimately
    // extend the registry beyond the legacy BlockType range. Require the
    // complete legacy set plus the test module rather than an exact total.
    assert(content.blocks().size() >=
           static_cast<std::size_t>(BlockType::Cobweb) + 2U);
    assert(content.blocks().find(
        ResourceLocation("test:decorative_block")
    ) != nullptr);
    const mc::content::BlockState modState = content.defaultState(
        ResourceLocation("test:decorative_block")
    );
    assert(modState.blockRuntimeId() >
           static_cast<mc::core::RuntimeId>(BlockType::Cobweb));
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

    mc::game::GameBootstrap resourceBootstrap(std::filesystem::path("assets"));
    resourceBootstrap.loadContentModules();
    resourceBootstrap.freezeRegistries();
    const mc::content::ContentCatalog& complete = resourceBootstrap.content();
    assert(complete.blocks().size() >= 407U);
    assert(complete.items().size() >= 717U);
    assert(complete.lootTables().size() >= 83U);
    assert(resourceBootstrap.gameplay().mobs().size() >= 49U);
    assert(complete.entityTypes().size() ==
           resourceBootstrap.gameplay().mobs().size() + 1U);
    const auto* zombie = resourceBootstrap.gameplay().mobs().find(
        ResourceLocation("minecraft:zombie")
    );
    assert(zombie != nullptr);
    assert(std::abs(zombie->maximumHealth - 20.0f) < 0.001f);
    assert(std::abs(zombie->movementSpeed - 0.23f) < 0.001f);
    assert(zombie->burnsInDaylight);
    assert(zombie->texture.toString() ==
           "minecraft:entity/zombie/zombie");
    const auto* skeleton = resourceBootstrap.gameplay().mobs().find(
        ResourceLocation("minecraft:skeleton")
    );
    assert(skeleton != nullptr);
    assert(skeleton->model == mc::gameplay::MobModelKind::Skeleton);
    assert(skeleton->attackKind == mc::gameplay::MobAttackKind::Ranged);
    assert(mc::gameplay::hasAiGoal(
        skeleton->aiGoals, mc::gameplay::MobAiGoal::AvoidSun
    ));
    const auto* enderman = resourceBootstrap.gameplay().mobs().find(
        ResourceLocation("minecraft:enderman")
    );
    assert(enderman != nullptr);
    assert(!mc::gameplay::hasAiGoal(
        enderman->aiGoals, mc::gameplay::MobAiGoal::AttackPlayer
    ));
    assert(mc::gameplay::hasAiGoal(
        enderman->aiGoals, mc::gameplay::MobAiGoal::HurtByTarget
    ));
    for (const auto& entry : resourceBootstrap.gameplay().mobs().entries())
    {
        constexpr std::string_view prefix = "entity/";
        assert(entry.value.texture.nameSpace() == "minecraft");
        assert(entry.value.texture.path().starts_with(prefix));
        const std::filesystem::path texture =
            std::filesystem::path("assets") / "minecraft" / "textures" /
            (entry.value.texture.path() + ".png");
        assert(std::filesystem::is_regular_file(texture));
        if (entry.value.overlayTexture.path() != "entity/empty")
        {
            const std::filesystem::path overlay =
                std::filesystem::path("assets") / "minecraft" / "textures" /
                (entry.value.overlayTexture.path() + ".png");
            assert(std::filesystem::is_regular_file(overlay));
        }
        for (const ResourceLocation& variant : entry.value.variantTextures)
        {
            const std::filesystem::path variantFile =
                std::filesystem::path("assets") / "minecraft" / "textures" /
                (variant.path() + ".png");
            assert(std::filesystem::is_regular_file(variantFile));
        }
        for (const ResourceLocation& variant :
             entry.value.variantOverlayTextures)
        {
            const std::filesystem::path variantFile =
                std::filesystem::path("assets") / "minecraft" / "textures" /
                (variant.path() + ".png");
            assert(std::filesystem::is_regular_file(variantFile));
        }
        assert(entry.value.model != mc::gameplay::MobModelKind::Count);
        assert(entry.value.width > 0.0f);
        assert(entry.value.height > 0.0f);
        assert(entry.value.eyeHeight > 0.0f);
        assert(entry.value.maximumHealth > 0.0f);
    }
    const auto oakFence = complete.state(
        ResourceLocation("minecraft:fence"),
        std::array<std::pair<std::string, std::string>, 2>{
            std::pair{"north", "true"}, std::pair{"east", "true"}
        }
    );
    assert(oakFence);
    assert(complete.serializeStateProperties(*oakFence).size() == 4U);

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
