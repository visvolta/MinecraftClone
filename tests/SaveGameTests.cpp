#include "SaveGame.h"
#include "content/ContentModule.h"
#include "game/GameBootstrap.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>

namespace
{
class TestContentModule final : public mc::content::ContentModule
{
public:
    [[nodiscard]] mc::core::ResourceLocation id() const override
    {
        return mc::core::ResourceLocation("save_test:module");
    }

    void registerContent(mc::content::ContentCatalog& catalog) override
    {
        catalog.registerBlock(
            mc::core::ResourceLocation("save_test:machine_casing"),
            {std::nullopt, "Machine Casing", {}, {}, {}}
        );
    }
};

template<typename T>
void writeValue(std::ofstream& output, const T& value)
{
    output.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

void writeEmptyStack(std::ofstream& output)
{
    writeValue<std::uint16_t>(output, 0);
    writeValue<std::uint8_t>(output, 0);
    writeValue<std::uint16_t>(output, 0);
}

void writeStack(std::ofstream& output, ItemType item, std::uint8_t count)
{
    writeValue<std::uint16_t>(output, static_cast<std::uint16_t>(item));
    writeValue<std::uint8_t>(output, count);
    writeValue<std::uint16_t>(output, 0);
}

void writeLegacyV1Fixture(const std::filesystem::path& path)
{
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    const std::array<char, 8> magic{{'M','C','B','1','7','3','S','V'}};
    output.write(magic.data(), static_cast<std::streamsize>(magic.size()));
    writeValue<std::uint32_t>(output, 1);
    writeValue<int>(output, 2468);
    writeValue<std::uint64_t>(output, 9001);
    writeValue<float>(output, 0.0F);
    writeValue<float>(output, 64.0F);
    writeValue<float>(output, 0.0F);
    writeValue<int>(output, 20);
    writeValue<int>(output, 20);
    writeValue<int>(output, 300);
    writeValue<int>(output, 0);
    writeValue<int>(output, 0);
    writeValue<int>(output, 0);
    for (int slot = 0; slot < Inventory::MAIN_SLOT_COUNT; ++slot)
        writeEmptyStack(output);

    writeValue<std::uint32_t>(output, 1);
    writeValue<int>(output, -3);
    writeValue<int>(output, 7);
    constexpr std::size_t legacyBlockCount = 16U * 128U * 16U;
    std::array<std::uint8_t, legacyBlockCount> blocks{};
    std::array<std::uint8_t, legacyBlockCount> properties{};
    const std::size_t stoneIndex = 1 + Chunk::WIDTH * (2 + Chunk::DEPTH * 60);
    const std::size_t furnaceIndex = 2 + Chunk::WIDTH * (3 + Chunk::DEPTH * 61);
    blocks[stoneIndex] = static_cast<std::uint8_t>(BlockType::Stone);
    blocks[furnaceIndex] = static_cast<std::uint8_t>(BlockType::Furnace);
    properties[furnaceIndex] = 5;
    output.write(
        reinterpret_cast<const char*>(blocks.data()),
        static_cast<std::streamsize>(blocks.size())
    );
    output.write(
        reinterpret_cast<const char*>(properties.data()),
        static_cast<std::streamsize>(properties.size())
    );
    std::array<float, Chunk::COLUMN_COUNT> climate{};
    output.write(
        reinterpret_cast<const char*>(climate.data()),
        static_cast<std::streamsize>(climate.size() * sizeof(float))
    );
    output.write(
        reinterpret_cast<const char*>(climate.data()),
        static_cast<std::streamsize>(climate.size() * sizeof(float))
    );
    writeValue<std::uint32_t>(output, 1);
    writeValue<int>(output, 2);
    writeValue<int>(output, 61);
    writeValue<int>(output, 3);
    writeValue<std::uint8_t>(output, 1);
    writeStack(output, itemFromBlock(BlockType::IronOre), 2);
    writeEmptyStack(output);
    writeEmptyStack(output);
    writeValue<int>(output, 20);
    writeValue<int>(output, 1600);
    writeValue<int>(output, 40);
    writeValue<std::uint32_t>(output, 0);
    assert(output);
}
}

int main()
{
    mc::game::GameBootstrap bootstrap;
    bootstrap.addContentModule(std::make_unique<TestContentModule>());
    bootstrap.loadContentModules();
    bootstrap.freezeRegistries();

    GameSaveData source;
    source.seed = 987654;
    source.worldTime = 12'345;
    source.player.survival.foodLevel = 13;
    source.player.survival.saturation = 2.5f;
    source.player.survival.experienceLevel = 7;
    source.inventory[Inventory::OFFHAND_SLOT] = {ItemType::Shield, 1};
    Chunk chunk(4, -2);
    chunk.setBlockState(
        1, 60, 2,
        bootstrap.content().defaultState(BlockType::Stone)
    );
    chunk.setBlockState(
        2, 61, 3,
        mc::content::BlockState(BlockType::Furnace, 5)
    );
    const mc::content::BlockState modState = bootstrap.content().defaultState(
        mc::core::ResourceLocation("save_test:machine_casing")
    );
    assert(modState.blockRuntimeId() >
           static_cast<mc::core::RuntimeId>(BlockType::TNT));
    chunk.setBlockState(3, 62, 4, modState);
    chunk.setBlockState(5, 220, 6, mc::content::BlockState(BlockType::Stone));
    source.modifiedChunks.push_back(chunk.snapshot());
    BlockEntityStore sourceEntities;
    FurnaceBlockEntity& sourceFurnace =
        sourceEntities.getOrCreateFurnace({2, 61, 3});
    sourceFurnace.getSlot(FurnaceBlockEntity::Input) =
        {BlockType::IronOre, 4};
    source.blockEntities = sourceEntities.snapshot();

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() /
        "minecraftclone-namespaced-save-test.dat";
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    std::filesystem::remove(path.string() + ".bak", ignored);

    std::string message;
    assert(SaveGame::save(path, source, message, bootstrap.content()));

    std::ifstream raw(path, std::ios::binary);
    raw.seekg(8);
    std::uint32_t version = 0;
    raw.read(reinterpret_cast<char*>(&version), sizeof(version));
    assert(version == 4);

    const auto loaded = SaveGame::load(path, message, bootstrap.content());
    assert(loaded);
    assert(loaded->seed == source.seed);
    assert(loaded->worldTime == source.worldTime);
    assert(loaded->player.survival.foodLevel == 13);
    assert(loaded->player.survival.experienceLevel == 7);
    assert(loaded->inventory[Inventory::OFFHAND_SLOT].item == ItemType::Shield);
    assert(loaded->modifiedChunks.size() == 1);
    Chunk restored;
    restored.restore(loaded->modifiedChunks.front());
    assert(restored.getBlockState(1, 60, 2).block() == BlockType::Stone);
    assert(restored.getBlockState(2, 61, 3) ==
           mc::content::BlockState(BlockType::Furnace, 5));
    assert(restored.getBlockState(3, 62, 4) == modState);
    assert(restored.getBlockState(5, 220, 6).block() == BlockType::Stone);
    assert(loaded->blockEntities.size() == 1);
    assert(loaded->blockEntities.front().type.toString() ==
           "minecraft:furnace");
    BlockEntityStore loadedEntities;
    loadedEntities.restore(loaded->blockEntities);
    assert(loadedEntities.getFurnace({2, 61, 3})->getSlot(
        FurnaceBlockEntity::Input).count == 4);

    const std::filesystem::path legacyPath =
        std::filesystem::temp_directory_path() /
        "minecraftclone-legacy-save-test.dat";
    std::filesystem::remove(legacyPath, ignored);
    std::filesystem::remove(legacyPath.string() + ".bak", ignored);
    writeLegacyV1Fixture(legacyPath);
    const auto migrated = SaveGame::load(
        legacyPath,
        message,
        bootstrap.content()
    );
    assert(migrated);
    assert(migrated->seed == 2468);
    assert(migrated->worldTime == 9001);
    assert(migrated->modifiedChunks.size() == 1);
    Chunk migratedChunk;
    migratedChunk.restore(migrated->modifiedChunks.front());
    assert(migratedChunk.getBlockState(1, 60, 2).block() == BlockType::Stone);
    assert(migratedChunk.getBlockState(2, 61, 3) ==
           mc::content::BlockState(BlockType::Furnace, 5));
    assert(migrated->blockEntities.size() == 1);
    assert(migrated->blockEntities.front().type.toString() ==
           "minecraft:furnace");

    std::filesystem::remove(path, ignored);
    std::filesystem::remove(path.string() + ".bak", ignored);
    std::filesystem::remove(legacyPath, ignored);
    std::filesystem::remove(legacyPath.string() + ".bak", ignored);

    const std::filesystem::path wipeSandbox =
        std::filesystem::temp_directory_path() /
        "minecraftclone-save-wipe-test";
    std::filesystem::remove_all(wipeSandbox, ignored);
    std::filesystem::create_directories(
        wipeSandbox / "saves" / "WorldOne"
    );
    std::filesystem::create_directories(
        wipeSandbox / "saves" / "WorldTwo"
    );
    std::ofstream(wipeSandbox / "saves" / "WorldOne" / "world.dat")
        << "save one";
    std::ofstream(wipeSandbox / "saves" / "WorldTwo" / "world.dat")
        << "save two";

    const std::filesystem::path originalDirectory =
        std::filesystem::current_path();
    std::filesystem::current_path(wipeSandbox);
    assert(SaveGame::wipeAll(message));
    assert(!std::filesystem::exists("saves"));
    std::filesystem::current_path(originalDirectory);
    std::filesystem::remove_all(wipeSandbox, ignored);
}
