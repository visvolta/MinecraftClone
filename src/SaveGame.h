#pragma once

#include "BlockEntity.h"
#include "Chunk.h"
#include "FluidSystem.h"
#include "Inventory.h"
#include "Player.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace mc::content { class ContentCatalog; }

struct GameSaveData
{
    int seed = 1337;
    std::uint64_t worldTime = 0;
    PlayerPersistentState player;
    std::array<ItemStack, Inventory::SLOT_COUNT> inventory{};
    int selectedHotbarSlot = 0;
    std::vector<ChunkSnapshot> modifiedChunks;
    std::vector<BlockEntityRecord> blockEntities;
    std::vector<FluidScheduledTickSnapshot> fluidTicks;
};

namespace SaveGame
{
[[nodiscard]] std::filesystem::path defaultPath();
[[nodiscard]] std::optional<GameSaveData> load(
    const std::filesystem::path& path,
    std::string& message,
    const mc::content::ContentCatalog& content
);
[[nodiscard]] bool save(
    const std::filesystem::path& path,
    const GameSaveData& data,
    std::string& message,
    const mc::content::ContentCatalog& content
);
[[nodiscard]] bool wipeAll(std::string& message);
}
