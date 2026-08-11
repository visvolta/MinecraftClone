#include "SaveGame.h"
#include "content/ContentModule.h"
#include "game/GameBootstrap.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>

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
    source.spawnPosition = glm::ivec3(84, 71, -36);
    source.player.uuid = {0x1234U, 0x5678U};
    source.player.survival.foodLevel = 13;
    source.player.survival.saturation = 2.5f;
    source.player.survival.experienceLevel = 7;
    source.inventory[Inventory::OFFHAND_SLOT] = {ItemType::Shield, 1};
    source.inventory[0] = {ItemType::RawBeef, 17};
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
           static_cast<mc::core::RuntimeId>(BlockType::Cobweb));
    chunk.setBlockState(3, 62, 4, modState);
    chunk.setBlockState(5, 220, 6, mc::content::BlockState(BlockType::Stone));
    source.modifiedChunks.push_back(chunk.snapshot());
    BlockEntityStore sourceEntities;
    FurnaceBlockEntity& sourceFurnace =
        sourceEntities.getOrCreateFurnace({2, 61, 3});
    sourceFurnace.getSlot(FurnaceBlockEntity::Input) =
        {BlockType::IronOre, 4};
    sourceEntities.getOrCreateSpawner({6, 22, 7}).setMobId(1);
    source.blockEntities = sourceEntities.snapshot();
    mc::entity::MobPersistentState wolf;
    wolf.type = mc::core::ResourceLocation("minecraft:wolf");
    wolf.uuid = {11U, 22U};
    wolf.ownerUuid = source.player.uuid;
    wolf.position = {82.5f, 71.0f, -34.5f};
    wolf.health = 17.0f;
    wolf.ticksExisted = 123;
    wolf.growingAge = -12000;
    wolf.variant = 2;
    wolf.tamed = true;
    wolf.sitting = true;
    source.mobs.push_back(wolf);

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
    assert(version == 7);
    raw.close();

    const auto loaded = SaveGame::load(path, message, bootstrap.content());
    assert(loaded);
    assert(loaded->seed == source.seed);
    assert(loaded->worldTime == source.worldTime);
    assert(loaded->generationVersion == source.generationVersion);
    assert(loaded->spawnPosition == source.spawnPosition);
    assert(loaded->player.survival.foodLevel == 13);
    assert(loaded->player.survival.experienceLevel == 7);
    assert(loaded->player.uuid == source.player.uuid);
    assert(loaded->inventory[Inventory::OFFHAND_SLOT].item == ItemType::Shield);
    assert(loaded->inventory[0].item == ItemType::RawBeef);
    assert(loaded->inventory[0].count == 17);
    assert(loaded->modifiedChunks.size() == 1);
    Chunk restored;
    restored.restore(loaded->modifiedChunks.front());
    assert(restored.getBlockState(1, 60, 2).block() == BlockType::Stone);
    assert(restored.getBlockState(2, 61, 3) ==
           mc::content::BlockState(BlockType::Furnace, 5));
    assert(restored.getBlockState(3, 62, 4) == modState);
    assert(restored.getBlockState(5, 220, 6).block() == BlockType::Stone);
    assert(loaded->blockEntities.size() == 2);
    BlockEntityStore loadedEntities;
    loadedEntities.restore(loaded->blockEntities);
    assert(loadedEntities.getFurnace({2, 61, 3})->getSlot(
        FurnaceBlockEntity::Input).count == 4);
    assert(loadedEntities.getSpawner({6, 22, 7})->mobId() == 1);
    assert(loaded->mobs.size() == 1U);
    assert(loaded->mobs.front().type.toString() == "minecraft:wolf");
    assert(loaded->mobs.front().ownerUuid == source.player.uuid);
    assert(loaded->mobs.front().growingAge == -12000);
    assert(loaded->mobs.front().tamed);
    assert(loaded->mobs.front().sitting);

    // Version 5 includes the persisted spawn but predates generator
    // versioning. It must remain readable and opt into the legacy generator
    // marker without shifting the following player data.
    const std::filesystem::path versionFivePath =
        std::filesystem::temp_directory_path() /
        "minecraftclone-version-five-save-test.dat";
    std::ifstream versionSixInput(path, std::ios::binary);
    std::vector<char> versionFiveBytes(
        (std::istreambuf_iterator<char>(versionSixInput)),
        std::istreambuf_iterator<char>()
    );
    const std::uint32_t versionFive = 5;
    std::memcpy(versionFiveBytes.data() + 8, &versionFive, sizeof(versionFive));
    constexpr std::size_t generationOffset = 8 + 4 + 4 + 8;
    versionFiveBytes.erase(
        versionFiveBytes.begin() + static_cast<std::ptrdiff_t>(generationOffset),
        versionFiveBytes.begin() + static_cast<std::ptrdiff_t>(generationOffset + 4)
    );
    std::ofstream versionFiveOutput(versionFivePath, std::ios::binary);
    versionFiveOutput.write(
        versionFiveBytes.data(),
        static_cast<std::streamsize>(versionFiveBytes.size())
    );
    versionFiveOutput.close();
    const auto migratedVersionFive = SaveGame::load(
        versionFivePath, message, bootstrap.content()
    );
    assert(migratedVersionFive);
    assert(migratedVersionFive->generationVersion == 1);
    assert(migratedVersionFive->spawnPosition == source.spawnPosition);

    // Version 4 had all 256-high/survival data but no generation version or
    // persisted world spawn. Removing those four integers creates an exact
    // migration case.
    const std::filesystem::path versionFourPath =
        std::filesystem::temp_directory_path() /
        "minecraftclone-version-four-save-test.dat";
    std::ifstream versionFiveInput(path, std::ios::binary);
    std::vector<char> versionFourBytes(
        (std::istreambuf_iterator<char>(versionFiveInput)),
        std::istreambuf_iterator<char>()
    );
    const std::uint32_t versionFour = 4;
    std::memcpy(versionFourBytes.data() + 8, &versionFour, sizeof(versionFour));
    versionFourBytes.erase(
        versionFourBytes.begin() + static_cast<std::ptrdiff_t>(generationOffset),
        versionFourBytes.begin() + static_cast<std::ptrdiff_t>(generationOffset + 16)
    );
    std::ofstream versionFourOutput(versionFourPath, std::ios::binary);
    versionFourOutput.write(
        versionFourBytes.data(),
        static_cast<std::streamsize>(versionFourBytes.size())
    );
    versionFourOutput.close();
    const auto migratedVersionFour = SaveGame::load(
        versionFourPath, message, bootstrap.content()
    );
    assert(migratedVersionFour);
    assert(migratedVersionFour->generationVersion == 1);
    assert(!migratedVersionFour->spawnPosition);
    assert(migratedVersionFour->player.survival.foodLevel == 13);
    assert(migratedVersionFour->modifiedChunks.front().paletteIndices.size() ==
           Chunk::BLOCK_COUNT);

    const std::filesystem::path legacyPath =
        std::filesystem::temp_directory_path() /
        "minecraftclone-legacy-save-test.dat";
    std::filesystem::remove(legacyPath, ignored);
    std::filesystem::remove(legacyPath.string() + ".bak", ignored);
    std::filesystem::remove(versionFourPath, ignored);
    std::filesystem::remove(versionFourPath.string() + ".bak", ignored);
    std::filesystem::remove(versionFivePath, ignored);
    std::filesystem::remove(versionFivePath.string() + ".bak", ignored);
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
