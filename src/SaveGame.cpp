#include "SaveGame.h"

#include "content/ContentCatalog.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace
{
constexpr std::array<char, 8> MAGIC{{'M','C','B','1','7','3','S','V'}};
constexpr std::uint32_t LEGACY_VERSION = 1;
constexpr std::uint32_t NAMESPACED_BLOCK_VERSION = 2;
constexpr std::uint32_t REGISTRY_ENTITY_VERSION = 3;
constexpr std::uint32_t SURVIVAL_HEIGHT_VERSION = 4;
constexpr std::uint32_t SPAWN_VERSION = 5;
constexpr std::uint32_t GENERATOR_VERSION = 6;
constexpr std::uint32_t VERSION = 7;
constexpr int LEGACY_HEIGHT = 128;
constexpr std::size_t LEGACY_BLOCK_COUNT =
    static_cast<std::size_t>(Chunk::WIDTH * LEGACY_HEIGHT * Chunk::DEPTH);
constexpr std::uint32_t MAX_CHUNKS = 16384;
constexpr std::uint32_t MAX_BLOCK_ENTITIES = 1'000'000;
constexpr std::uint32_t MAX_FLUID_TICKS = 2'000'000;
constexpr std::uint32_t MAX_MOBS = 4096;

class Writer
{
public:
    explicit Writer(const std::filesystem::path& path)
        : stream_(path, std::ios::binary | std::ios::trunc)
    {
        if (!stream_)
            throw std::runtime_error("Could not open temporary world save");
    }

    template<typename T>
    void value(const T& input)
    {
        static_assert(std::is_trivially_copyable_v<T>);
        stream_.write(reinterpret_cast<const char*>(&input), sizeof(T));
        if (!stream_)
            throw std::runtime_error("Failed while writing world save");
    }

    void bytes(const void* data, std::size_t size)
    {
        stream_.write(static_cast<const char*>(data),
                      static_cast<std::streamsize>(size));
        if (!stream_)
            throw std::runtime_error("Failed while writing world save");
    }

    void string(std::string_view input)
    {
        if (input.size() > std::numeric_limits<std::uint16_t>::max())
            throw std::runtime_error("Save string is too long");
        value(static_cast<std::uint16_t>(input.size()));
        bytes(input.data(), input.size());
    }

    void finish()
    {
        stream_.flush();
        if (!stream_)
            throw std::runtime_error("Failed to flush world save");
        stream_.close();
    }

private:
    std::ofstream stream_;
};

class Reader
{
public:
    explicit Reader(const std::filesystem::path& path)
        : stream_(path, std::ios::binary)
    {
        if (!stream_)
            throw std::runtime_error("Could not open world save");
    }

    template<typename T>
    T value()
    {
        static_assert(std::is_trivially_copyable_v<T>);
        T result{};
        stream_.read(reinterpret_cast<char*>(&result), sizeof(T));
        if (!stream_)
            throw std::runtime_error("World save is truncated");
        return result;
    }

    void bytes(void* data, std::size_t size)
    {
        stream_.read(static_cast<char*>(data), static_cast<std::streamsize>(size));
        if (!stream_)
            throw std::runtime_error("World save is truncated");
    }

    [[nodiscard]] std::string string()
    {
        const std::uint16_t size = value<std::uint16_t>();
        std::string result(size, '\0');
        bytes(result.data(), result.size());
        return result;
    }

private:
    std::ifstream stream_;
};

bool validItemValue(std::uint16_t value)
{
    return (value > 0 && value <= static_cast<std::uint16_t>(BlockType::Cobweb)) ||
           (value >= static_cast<std::uint16_t>(ItemType::Stick) &&
            value <= static_cast<std::uint16_t>(ItemType::Lead));
}

void writeStack(Writer& writer, const ItemStack& stack)
{
    writer.value(static_cast<std::uint16_t>(stack.item));
    writer.value(stack.count);
    writer.value(stack.damage);
}

void writeUuid(Writer& writer, const mc::entity::EntityUuid& uuid)
{
    writer.value(uuid.most);
    writer.value(uuid.least);
}

mc::entity::EntityUuid readUuid(Reader& reader)
{
    return {
        reader.value<std::uint64_t>(), reader.value<std::uint64_t>()
    };
}

ItemStack readStack(Reader& reader)
{
    const std::uint16_t itemValue = reader.value<std::uint16_t>();
    const std::uint8_t count = reader.value<std::uint8_t>();
    const std::uint16_t damage = reader.value<std::uint16_t>();
    if (itemValue == 0 && count == 0)
        return {};
    if (!validItemValue(itemValue) || count == 0)
        throw std::runtime_error("World save contains an invalid item stack");
    ItemStack stack{static_cast<ItemType>(itemValue), count, damage};
    if (count > stack.maximumStackSize())
        throw std::runtime_error("World save contains an oversized item stack");
    return stack;
}

void writeChunk(
    Writer& writer,
    const ChunkSnapshot& chunk,
    const mc::content::ContentCatalog& content)
{
    writer.value(chunk.chunkX);
    writer.value(chunk.chunkZ);
    writer.value(static_cast<std::uint32_t>(chunk.palette.size()));
    for (const mc::content::BlockState state : chunk.palette)
    {
        const mc::core::ResourceLocation* name = content.blockName(state);
        if (name == nullptr || !content.isValidState(state))
            throw std::runtime_error("Cannot save an unregistered block state");
        writer.string(name->toString());
        const auto properties = content.serializeStateProperties(state);
        if (properties.size() > std::numeric_limits<std::uint8_t>::max())
            throw std::runtime_error("Block state has too many properties");
        writer.value(static_cast<std::uint8_t>(properties.size()));
        for (const auto& [propertyName, propertyValue] : properties)
        {
            writer.string(propertyName);
            writer.string(propertyValue);
        }
    }
    writer.value(static_cast<std::uint16_t>(Chunk::HEIGHT));
    writer.bytes(
        chunk.paletteIndices.data(),
        chunk.paletteIndices.size() * sizeof(std::uint16_t)
    );
    writer.bytes(chunk.temperatures.data(), chunk.temperatures.size() * sizeof(float));
    writer.bytes(chunk.humidities.data(), chunk.humidities.size() * sizeof(float));
    writer.bytes(chunk.biomeIds.data(), chunk.biomeIds.size() * sizeof(BiomeId));
    writer.bytes(
        chunk.worldSurfaceHeight.data(),
        chunk.worldSurfaceHeight.size() * sizeof(std::uint16_t)
    );
    writer.bytes(
        chunk.motionBlockingHeight.data(),
        chunk.motionBlockingHeight.size() * sizeof(std::uint16_t)
    );
}

ChunkSnapshot readLegacyChunk(Reader& reader)
{
    const int chunkX = reader.value<int>();
    const int chunkZ = reader.value<int>();
    std::array<BlockType, LEGACY_BLOCK_COUNT> blocks{};
    std::array<std::uint8_t, LEGACY_BLOCK_COUNT> properties{};
    for (BlockType& block : blocks)
    {
        const std::uint8_t value = reader.value<std::uint8_t>();
        if (value > static_cast<std::uint8_t>(BlockType::Lava))
            throw std::runtime_error("World save contains an invalid block ID");
        block = static_cast<BlockType>(value);
    }
    reader.bytes(properties.data(), properties.size());

    Chunk migrated(chunkX, chunkZ);
    for (std::size_t index = 0; index < LEGACY_BLOCK_COUNT; ++index)
    {
        const int x = static_cast<int>(index % Chunk::WIDTH);
        const int z = static_cast<int>((index / Chunk::WIDTH) % Chunk::DEPTH);
        const int y = static_cast<int>(index / (Chunk::WIDTH * Chunk::DEPTH));
        migrated.setBlockState(
            x, y, z,
            mc::content::BlockState(blocks[index], properties[index])
        );
    }
    ChunkSnapshot chunk = migrated.snapshot();
    reader.bytes(chunk.temperatures.data(), chunk.temperatures.size() * sizeof(float));
    reader.bytes(chunk.humidities.data(), chunk.humidities.size() * sizeof(float));
    for (std::size_t column = 0; column < chunk.biomeIds.size(); ++column)
    {
        chunk.biomeIds[column] = classifyReleaseBiome(
            chunk.temperatures[column], chunk.humidities[column], 0.0
        );
    }
    return chunk;
}

ChunkSnapshot readNamespacedChunk(
    Reader& reader,
    const mc::content::ContentCatalog& content,
    std::uint32_t version)
{
    ChunkSnapshot chunk;
    chunk.chunkX = reader.value<int>();
    chunk.chunkZ = reader.value<int>();
    const std::uint32_t paletteSize = reader.value<std::uint32_t>();
    if (paletteSize == 0 || paletteSize > 65'536U)
        throw std::runtime_error("World save contains an invalid chunk palette");
    chunk.palette.reserve(paletteSize);
    for (std::uint32_t index = 0; index < paletteSize; ++index)
    {
        const mc::core::ResourceLocation name(reader.string());
        const std::uint8_t propertyCount = reader.value<std::uint8_t>();
        std::vector<std::pair<std::string, std::string>> properties;
        properties.reserve(propertyCount);
        for (std::uint8_t property = 0; property < propertyCount; ++property)
        {
            // Function argument evaluation order is not guaranteed to match the
            // on-disk order, so read the name and value as separate operations.
            std::string propertyName = reader.string();
            std::string propertyValue = reader.string();
            properties.emplace_back(
                std::move(propertyName),
                std::move(propertyValue)
            );
        }
        const std::optional<mc::content::BlockState> state =
            content.state(name, properties);
        if (!state)
        {
            throw std::runtime_error(
                "World save requires unknown block state: " + name.toString()
            );
        }
        chunk.palette.push_back(*state);
    }
    if (version >= SURVIVAL_HEIGHT_VERSION)
    {
        const std::uint16_t height = reader.value<std::uint16_t>();
        if (height != Chunk::HEIGHT)
            throw std::runtime_error("World save has an unsupported height");
        chunk.paletteIndices.resize(Chunk::BLOCK_COUNT);
        reader.bytes(
            chunk.paletteIndices.data(),
            chunk.paletteIndices.size() * sizeof(std::uint16_t)
        );
    }
    else
    {
        std::array<std::uint16_t, LEGACY_BLOCK_COUNT> legacyIndices{};
        reader.bytes(
            legacyIndices.data(),
            legacyIndices.size() * sizeof(std::uint16_t)
        );
        Chunk migrated(chunk.chunkX, chunk.chunkZ);
        for (std::size_t index = 0; index < legacyIndices.size(); ++index)
        {
            if (legacyIndices[index] >= chunk.palette.size())
                throw std::runtime_error(
                    "World save contains an invalid palette index"
                );
            const int x = static_cast<int>(index % Chunk::WIDTH);
            const int z = static_cast<int>(
                (index / Chunk::WIDTH) % Chunk::DEPTH
            );
            const int y = static_cast<int>(
                index / (Chunk::WIDTH * Chunk::DEPTH)
            );
            migrated.setBlockState(
                x, y, z, chunk.palette[legacyIndices[index]]
            );
        }
        chunk = migrated.snapshot();
    }
    for (const std::uint16_t paletteIndex : chunk.paletteIndices)
    {
        if (paletteIndex >= chunk.palette.size())
            throw std::runtime_error("World save contains an invalid palette index");
    }
    reader.bytes(chunk.temperatures.data(), chunk.temperatures.size() * sizeof(float));
    reader.bytes(chunk.humidities.data(), chunk.humidities.size() * sizeof(float));
    if (version >= SURVIVAL_HEIGHT_VERSION)
    {
        reader.bytes(
            chunk.biomeIds.data(),
            chunk.biomeIds.size() * sizeof(BiomeId)
        );
        reader.bytes(
            chunk.worldSurfaceHeight.data(),
            chunk.worldSurfaceHeight.size() * sizeof(std::uint16_t)
        );
        reader.bytes(
            chunk.motionBlockingHeight.data(),
            chunk.motionBlockingHeight.size() * sizeof(std::uint16_t)
        );
    }
    else
    {
        for (std::size_t column = 0; column < chunk.biomeIds.size(); ++column)
        {
            chunk.biomeIds[column] = classifyReleaseBiome(
                chunk.temperatures[column], chunk.humidities[column], 0.0
            );
        }
    }
    return chunk;
}

void writeData(
    Writer& writer,
    const GameSaveData& data,
    const mc::content::ContentCatalog& content)
{
    writer.bytes(MAGIC.data(), MAGIC.size());
    writer.value(VERSION);
    writer.value(data.seed);
    writer.value(data.worldTime);
    writer.value(data.generationVersion);
    const glm::ivec3 spawn = data.spawnPosition.value_or(glm::ivec3(0, 64, 0));
    writer.value(spawn.x);
    writer.value(spawn.y);
    writer.value(spawn.z);
    writer.value(data.player.position.x);
    writer.value(data.player.position.y);
    writer.value(data.player.position.z);
    writer.value(data.player.health);
    writer.value(data.player.previousHealth);
    writer.value(data.player.air);
    writer.value(data.player.fireTicks);
    writer.value(data.player.ticksExisted);
    writer.value(data.player.survival.foodLevel);
    writer.value(data.player.survival.saturation);
    writer.value(data.player.survival.exhaustion);
    writer.value(data.player.survival.foodTickTimer);
    writer.value(data.player.survival.experienceLevel);
    writer.value(data.player.survival.experienceTotal);
    writer.value(data.player.survival.experienceProgress);
    writer.value(data.player.survival.armorPoints);
    writer.value(data.player.survival.armorToughness);
    writer.value(data.selectedHotbarSlot);
    for (const ItemStack& stack : data.inventory)
        writeStack(writer, stack);

    writer.value(static_cast<std::uint32_t>(data.modifiedChunks.size()));
    for (const ChunkSnapshot& chunk : data.modifiedChunks)
        writeChunk(writer, chunk, content);

    writer.value(static_cast<std::uint32_t>(data.blockEntities.size()));
    for (const BlockEntityRecord& record : data.blockEntities)
    {
        writer.value(record.position.x);
        writer.value(record.position.y);
        writer.value(record.position.z);
        writer.string(record.type.toString());
        writer.value(record.data.version);
        writer.value(static_cast<std::uint32_t>(record.data.items.size()));
        for (const ItemStack& stack : record.data.items)
            writeStack(writer, stack);
        writer.value(static_cast<std::uint32_t>(record.data.integers.size()));
        for (const auto& [name, value] : record.data.integers)
        {
            writer.string(name);
            writer.value(value);
        }
    }

    writer.value(static_cast<std::uint32_t>(data.fluidTicks.size()));
    for (const FluidScheduledTickSnapshot& tick : data.fluidTicks)
    {
        writer.value(tick.x);
        writer.value(tick.y);
        writer.value(tick.z);
        writer.value(static_cast<std::uint8_t>(tick.liquid));
        writer.value(tick.remainingTicks);
    }

    writeUuid(writer, data.player.uuid);
    if (data.mobs.size() > MAX_MOBS)
        throw std::runtime_error("Cannot save more than 4096 mobs");
    writer.value(static_cast<std::uint32_t>(data.mobs.size()));
    for (const mc::entity::MobPersistentState& mob : data.mobs)
    {
        writer.string(mob.type.toString());
        writeUuid(writer, mob.uuid);
        writeUuid(writer, mob.ownerUuid);
        writeUuid(writer, mob.loveCauseUuid);
        writeUuid(writer, mob.leashHolderUuid);
        writer.value(mob.position.x);
        writer.value(mob.position.y);
        writer.value(mob.position.z);
        writer.value(mob.velocity.x);
        writer.value(mob.velocity.y);
        writer.value(mob.velocity.z);
        writer.value(mob.yaw);
        writer.value(mob.health);
        writer.value(mob.ticksExisted);
        writer.value(mob.growingAge);
        writer.value(mob.forcedAge);
        writer.value(mob.inLove);
        writer.value(mob.variant);
        writer.value(mob.temper);
        writer.value(mob.tamed);
        writer.value(mob.sitting);
        writer.value(mob.sheared);
        writer.value(mob.saddled);
        writer.value(mob.leashed);
        writeStack(writer, mob.armor);
    }
}

GameSaveData readData(
    const std::filesystem::path& path,
    const mc::content::ContentCatalog& content)
{
    Reader reader(path);
    std::array<char, MAGIC.size()> magic{};
    reader.bytes(magic.data(), magic.size());
    if (magic != MAGIC)
        throw std::runtime_error("Not a MinecraftClone world save");
    const std::uint32_t version = reader.value<std::uint32_t>();
    if (version != LEGACY_VERSION &&
        version != NAMESPACED_BLOCK_VERSION &&
        version != REGISTRY_ENTITY_VERSION &&
        version != SURVIVAL_HEIGHT_VERSION &&
        version != SPAWN_VERSION &&
        version != GENERATOR_VERSION &&
        version != VERSION)
        throw std::runtime_error("Unsupported world save version");

    GameSaveData data;
    data.seed = reader.value<int>();
    data.worldTime = reader.value<std::uint64_t>();
    data.generationVersion = version >= GENERATOR_VERSION
        ? reader.value<std::uint32_t>()
        : 1U;
    if (version >= SPAWN_VERSION)
    {
        const int spawnX = reader.value<int>();
        const int spawnY = reader.value<int>();
        const int spawnZ = reader.value<int>();
        data.spawnPosition = glm::ivec3(spawnX, spawnY, spawnZ);
        if (data.spawnPosition->y < 0 ||
            data.spawnPosition->y >= Chunk::HEIGHT)
        {
            throw std::runtime_error("World save contains an invalid spawn point");
        }
    }
    data.player.position.x = reader.value<float>();
    data.player.position.y = reader.value<float>();
    data.player.position.z = reader.value<float>();
    data.player.health = reader.value<int>();
    data.player.previousHealth = reader.value<int>();
    data.player.air = reader.value<int>();
    data.player.fireTicks = reader.value<int>();
    data.player.ticksExisted = reader.value<int>();
    if (version >= SURVIVAL_HEIGHT_VERSION)
    {
        data.player.survival.foodLevel = reader.value<int>();
        data.player.survival.saturation = reader.value<float>();
        data.player.survival.exhaustion = reader.value<float>();
        data.player.survival.foodTickTimer = reader.value<int>();
        data.player.survival.experienceLevel = reader.value<int>();
        data.player.survival.experienceTotal = reader.value<int>();
        data.player.survival.experienceProgress = reader.value<float>();
        data.player.survival.armorPoints = reader.value<int>();
        data.player.survival.armorToughness = reader.value<float>();
    }
    data.selectedHotbarSlot = reader.value<int>();
    if (data.selectedHotbarSlot < 0 || data.selectedHotbarSlot >= Inventory::HOTBAR_SIZE)
        throw std::runtime_error("World save contains an invalid selected slot");
    const int savedInventorySlots = version >= SURVIVAL_HEIGHT_VERSION
        ? Inventory::SLOT_COUNT
        : Inventory::MAIN_SLOT_COUNT;
    for (int slot = 0; slot < savedInventorySlots; ++slot)
        data.inventory[static_cast<std::size_t>(slot)] = readStack(reader);

    const std::uint32_t chunkCount = reader.value<std::uint32_t>();
    if (chunkCount > MAX_CHUNKS)
        throw std::runtime_error("World save contains too many chunks");
    data.modifiedChunks.reserve(chunkCount);
    for (std::uint32_t i = 0; i < chunkCount; ++i)
    {
        data.modifiedChunks.push_back(
            version == LEGACY_VERSION
                ? readLegacyChunk(reader)
                : readNamespacedChunk(reader, content, version)
        );
    }

    const std::uint32_t entityCount = reader.value<std::uint32_t>();
    if (entityCount > MAX_BLOCK_ENTITIES)
        throw std::runtime_error("World save contains too many block entities");
    data.blockEntities.reserve(entityCount);
    for (std::uint32_t i = 0; i < entityCount; ++i)
    {
        BlockEntityRecord record;
        record.position.x = reader.value<int>();
        record.position.y = reader.value<int>();
        record.position.z = reader.value<int>();
        if (record.position.y < 0 || record.position.y >= Chunk::HEIGHT)
            throw std::runtime_error("World save contains an invalid block entity position");
        if (version < REGISTRY_ENTITY_VERSION)
        {
            const std::uint8_t type = reader.value<std::uint8_t>();
            if (type == 1)
            {
                FurnacePersistentState state;
                for (ItemStack& stack : state.slots)
                    stack = readStack(reader);
                state.burnTime = reader.value<int>();
                state.currentFuelBurnTime = reader.value<int>();
                state.cookTime = reader.value<int>();
                FurnaceBlockEntity furnace;
                furnace.restorePersistentState(state);
                record.type = furnace.typeId();
                record.data = furnace.savePersistentData();
            }
            else if (type == 2)
            {
                ChestBlockEntity chest;
                for (ItemStack& stack : chest.getSlots())
                    stack = readStack(reader);
                record.type = chest.typeId();
                record.data = chest.savePersistentData();
            }
            else
            {
                throw std::runtime_error("World save contains an unknown block entity");
            }
        }
        else
        {
            record.type = mc::core::ResourceLocation(reader.string());
            record.data.version = reader.value<std::uint32_t>();
            const std::uint32_t itemCount = reader.value<std::uint32_t>();
            if (itemCount > 4096U)
                throw std::runtime_error("Block entity contains too many items");
            record.data.items.reserve(itemCount);
            for (std::uint32_t item = 0; item < itemCount; ++item)
                record.data.items.push_back(readStack(reader));
            const std::uint32_t integerCount = reader.value<std::uint32_t>();
            if (integerCount > 256U)
                throw std::runtime_error("Block entity contains too many fields");
            record.data.integers.reserve(integerCount);
            for (std::uint32_t field = 0; field < integerCount; ++field)
            {
                std::string name = reader.string();
                const std::int64_t value = reader.value<std::int64_t>();
                record.data.integers.emplace_back(std::move(name), value);
            }
        }
        data.blockEntities.push_back(std::move(record));
    }

    const std::uint32_t fluidCount = reader.value<std::uint32_t>();
    if (fluidCount > MAX_FLUID_TICKS)
        throw std::runtime_error("World save contains too many fluid ticks");
    data.fluidTicks.reserve(fluidCount);
    for (std::uint32_t i = 0; i < fluidCount; ++i)
    {
        FluidScheduledTickSnapshot tick;
        tick.x = reader.value<int>();
        tick.y = reader.value<int>();
        tick.z = reader.value<int>();
        const std::uint8_t liquid = reader.value<std::uint8_t>();
        if (liquid != static_cast<std::uint8_t>(BlockType::Water) &&
            liquid != static_cast<std::uint8_t>(BlockType::Lava))
        {
            throw std::runtime_error("World save contains an invalid fluid tick");
        }
        tick.liquid = static_cast<BlockType>(liquid);
        tick.remainingTicks = reader.value<std::uint64_t>();
        data.fluidTicks.push_back(tick);
    }
    if (version >= VERSION)
    {
        data.player.uuid = readUuid(reader);
        const std::uint32_t mobCount = reader.value<std::uint32_t>();
        if (mobCount > MAX_MOBS)
            throw std::runtime_error("World save contains too many mobs");
        data.mobs.reserve(mobCount);
        for (std::uint32_t index = 0; index < mobCount; ++index)
        {
            mc::entity::MobPersistentState mob;
            mob.type = mc::core::ResourceLocation(reader.string());
            mob.uuid = readUuid(reader);
            mob.ownerUuid = readUuid(reader);
            mob.loveCauseUuid = readUuid(reader);
            mob.leashHolderUuid = readUuid(reader);
            mob.position.x = reader.value<float>();
            mob.position.y = reader.value<float>();
            mob.position.z = reader.value<float>();
            mob.velocity.x = reader.value<float>();
            mob.velocity.y = reader.value<float>();
            mob.velocity.z = reader.value<float>();
            mob.yaw = reader.value<float>();
            mob.health = reader.value<float>();
            mob.ticksExisted = reader.value<int>();
            mob.growingAge = reader.value<int>();
            mob.forcedAge = reader.value<int>();
            mob.inLove = reader.value<int>();
            mob.variant = reader.value<int>();
            mob.temper = reader.value<int>();
            mob.tamed = reader.value<bool>();
            mob.sitting = reader.value<bool>();
            mob.sheared = reader.value<bool>();
            mob.saddled = reader.value<bool>();
            mob.leashed = reader.value<bool>();
            mob.armor = readStack(reader);
            if (!std::isfinite(mob.position.x) ||
                !std::isfinite(mob.position.y) ||
                !std::isfinite(mob.position.z) ||
                mob.position.y < -64.0f ||
                mob.position.y > static_cast<float>(Chunk::HEIGHT + 64) ||
                !std::isfinite(mob.health) || mob.health < 0.0f)
                throw std::runtime_error(
                    "World save contains invalid mob data"
                );
            data.mobs.push_back(std::move(mob));
        }
    }
    return data;
}
}

std::filesystem::path SaveGame::defaultPath()
{
    return std::filesystem::path("saves") / "BetaWorld" / "world.dat";
}

std::optional<GameSaveData> SaveGame::load(
    const std::filesystem::path& path,
    std::string& message,
    const mc::content::ContentCatalog& content)
{
    if (!std::filesystem::exists(path) &&
        !std::filesystem::exists(path.string() + ".bak"))
    {
        message = "Starting a new world";
        return std::nullopt;
    }
    try
    {
        GameSaveData data = readData(path, content);
        message = "Loaded world save";
        return data;
    }
    catch (const std::exception& primaryError)
    {
        try
        {
            GameSaveData data = readData(path.string() + ".bak", content);
            message = std::string("Primary save was invalid; loaded backup: ") +
                      primaryError.what();
            return data;
        }
        catch (const std::exception& backupError)
        {
            message = std::string("Could not load world or backup: ") +
                      primaryError.what() + "; " + backupError.what();
            return std::nullopt;
        }
    }
}

bool SaveGame::save(
    const std::filesystem::path& path,
    const GameSaveData& data,
    std::string& message,
    const mc::content::ContentCatalog& content)
{
    const std::filesystem::path temporary = path.string() + ".tmp";
    const std::filesystem::path backup = path.string() + ".bak";
    try
    {
        std::filesystem::create_directories(path.parent_path());
        Writer writer(temporary);
        writeData(writer, data, content);
        writer.finish();

        std::error_code error;
        std::filesystem::remove(backup, error);
        error.clear();
        if (std::filesystem::exists(path))
        {
            std::filesystem::rename(path, backup, error);
            if (error)
                throw std::runtime_error("Could not rotate the previous save: " + error.message());
        }
        std::filesystem::rename(temporary, path, error);
        if (error)
        {
            if (std::filesystem::exists(backup) && !std::filesystem::exists(path))
            {
                std::error_code restoreError;
                std::filesystem::rename(backup, path, restoreError);
            }
            throw std::runtime_error("Could not install the new save: " + error.message());
        }
        message = "World saved";
        return true;
    }
    catch (const std::exception& error)
    {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        message = error.what();
        return false;
    }
}

bool SaveGame::wipeAll(std::string& message)
{
    const std::filesystem::path saveRoot =
        defaultPath().parent_path().parent_path();
    if (saveRoot.empty() || saveRoot == "." ||
        saveRoot == saveRoot.root_path())
    {
        message = "Refusing to wipe an unsafe save path";
        return false;
    }

    std::error_code error;
    const std::uintmax_t removed =
        std::filesystem::remove_all(saveRoot, error);
    if (error)
    {
        message = "Could not wipe saves: " + error.message();
        return false;
    }

    message = "Deleted " + std::to_string(removed) +
        " save files and directories";
    return true;
}
