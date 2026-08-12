#pragma once

#include "BiomeColorMap.h"
#include "BlockEntity.h"
#include "ChunkManager.h"
#include "ChunkMesh.h"
#include "Frustum.h"
#include "FluidSystem.h"
#include "LightingSystem.h"
#include "worldgen/Biome.h"
#include "worldgen/BiomeMap.h"
#include "worldgen/StructureGenerator.h"
#include "gameplay/FarmingSystem.h"
#include "gameplay/RedstoneSystem.h"
#include "entity/AxisAlignedBB.h"
#include "entity/Difficulty.h"
#include "entity/EnumCreatureType.h"
#include "core/ResourceLocation.h"
#include "worldgen/JavaRandom.h"
#include "Item.h"

#include <glm/glm.hpp>

namespace mc::entity
{
class Entity;
class LivingEntity;
class PlayerEntity;
class Mob;
}

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct ChestInventoryView
{
    ChestBlockEntity* first = nullptr;
    ChestBlockEntity* second = nullptr;

    [[nodiscard]] bool valid() const noexcept { return first != nullptr; }
    [[nodiscard]] bool doubleChest() const noexcept { return second != nullptr; }
};

class World
{
public:
    explicit World(int renderDistance = 7, int seed = 1337);
    ~World();

    World(const World&) = delete;
    World& operator=(const World&) = delete;
    World(World&&) = delete;
    World& operator=(World&&) = delete;

    void update(const glm::vec3& cameraPosition);
    void tick();
    void finishInitialLoad();

    void drawOpaque(const Frustum& frustum);
    void drawCutout() const;
    void drawLeaves(bool fast) const;
    void drawLava() const;
    void drawWater(
        const glm::vec3& cameraPosition
    ) const;

    [[nodiscard]] mc::content::BlockState getBlockState(
        int worldX,
        int worldY,
        int worldZ
    ) const;
    [[nodiscard]] mc::content::BlockState getActualBlockState(
        int worldX,
        int worldY,
        int worldZ
    ) const;
    [[nodiscard]] BlockType getBlock(int worldX, int worldY, int worldZ) const;
    [[nodiscard]] std::uint8_t getBlockMetadata(
        int worldX,
        int worldY,
        int worldZ
    ) const;
    [[nodiscard]] bool isSolidBlock(int worldX, int worldY, int worldZ) const;
    [[nodiscard]] bool isBlockLoaded(int worldX, int worldY, int worldZ) const;
    bool setBlock(int worldX, int worldY, int worldZ, BlockType block);
    bool setBlockState(
        int worldX,
        int worldY,
        int worldZ,
        mc::content::BlockState state
    );
    bool setBlockAndMetadata(
        int worldX,
        int worldY,
        int worldZ,
        BlockType block,
        std::uint8_t metadata
    );
    [[nodiscard]] glm::vec3 getFluidFlowVector(
        int worldX,
        int worldY,
        int worldZ,
        BlockType liquid
    ) const;
    [[nodiscard]] int getHighestSolidBlockY(int worldX, int worldZ) const;
    [[nodiscard]] BiomeId getBiomeAt(int worldX, int worldZ) const;
    [[nodiscard]] float getTemperatureAt(int worldX, int worldZ) const;
    [[nodiscard]] int getSkyLightLevel(int worldX, int worldY, int worldZ) const;
    [[nodiscard]] int getBlockLightLevel(int worldX, int worldY, int worldZ) const;

    [[nodiscard]] FurnaceBlockEntity* getFurnace(
        int worldX, int worldY, int worldZ
    ) noexcept;
    [[nodiscard]] FurnaceBlockEntity& getOrCreateFurnace(
        int worldX, int worldY, int worldZ
    );
    [[nodiscard]] ChestBlockEntity* getChest(
        int worldX, int worldY, int worldZ
    ) noexcept;
    [[nodiscard]] ChestBlockEntity& getOrCreateChest(
        int worldX, int worldY, int worldZ
    );
    [[nodiscard]] bool canPlaceChest(int worldX, int worldY, int worldZ) const;
    [[nodiscard]] bool canOpenChest(int worldX, int worldY, int worldZ) const;
    [[nodiscard]] ChestInventoryView getChestInventory(
        int worldX, int worldY, int worldZ
    );
    [[nodiscard]] std::vector<ItemStack> copyBlockEntityContents(
        int worldX, int worldY, int worldZ
    ) const;

    [[nodiscard]] int getSeed() const noexcept;
    [[nodiscard]] std::uint32_t getGenerationVersion() const noexcept;
    [[nodiscard]] std::optional<StructureLocation> findNearestStructure(
        WorldStructure structure,
        int worldX,
        int worldZ,
        int maximumRegionRadius = 100) const;
    [[nodiscard]] std::vector<ChunkSnapshot> getModifiedChunkSnapshots() const;
    [[nodiscard]] std::vector<BlockEntityRecord> getBlockEntitySnapshots() const;
    [[nodiscard]] std::vector<FluidScheduledTickSnapshot>
        getScheduledFluidTickSnapshots() const;
    void restorePersistentState(
        std::vector<ChunkSnapshot> chunks,
        std::vector<BlockEntityRecord> blockEntities,
        const std::vector<FluidScheduledTickSnapshot>& fluidTicks
    );

    [[nodiscard]] bool isSmoothLightingEnabled() const noexcept;
    void setSmoothLightingEnabled(bool enabled);
    [[nodiscard]] bool isFastLeavesEnabled() const noexcept;
    void setFastLeavesEnabled(bool enabled);

    void setRenderDistance(int renderDistance);
    [[nodiscard]] int getRenderDistance() const noexcept;

    [[nodiscard]] const ChunkManager& getChunkManager() const noexcept;
    [[nodiscard]] ChunkManager& getChunkManager() noexcept;

    [[nodiscard]] std::size_t getLoadedChunkCount() const noexcept;
    [[nodiscard]] int getVisibleFaceCount() const noexcept;
    [[nodiscard]] int getVertexCount() const noexcept;
    [[nodiscard]] std::size_t getPendingChunkCount() const;
    [[nodiscard]] std::size_t getPendingMeshCount() const;
    [[nodiscard]] int getDrawnChunkCount() const noexcept;
    [[nodiscard]] int getCulledChunkCount() const noexcept;
    [[nodiscard]] double getTerrainMilliseconds() const noexcept;
    [[nodiscard]] double getMeshMilliseconds() const noexcept;
    [[nodiscard]] double getUploadMilliseconds() const noexcept;
    [[nodiscard]] double getWorldUpdateMilliseconds() const noexcept;
    [[nodiscard]] std::size_t getPendingLightingCount() const noexcept;
    [[nodiscard]] double getLightingMilliseconds() const noexcept;
    [[nodiscard]] std::size_t getPendingFluidTickCount() const noexcept;

    void setPlayer(mc::entity::PlayerEntity* player) noexcept;
    [[nodiscard]] mc::entity::PlayerEntity* getPlayer() const noexcept;
    mc::entity::Entity* spawnEntity(std::unique_ptr<mc::entity::Entity> entity);
    void tickEntities();
    [[nodiscard]] std::vector<mc::entity::Entity*> getEntitiesInAABB(
        const mc::entity::AxisAlignedBB& box,
        const mc::entity::Entity* exclude = nullptr) const;
    [[nodiscard]] std::vector<mc::entity::Mob*> getMobs() const;
    [[nodiscard]] std::vector<mc::entity::Entity*> getEntities() const;
    [[nodiscard]] mc::entity::PlayerEntity* getClosestPlayer(
        double x, double y, double z, double maxDistance) const;
    [[nodiscard]] bool isAnyPlayerWithinRangeAt(
        double x, double y, double z, double range) const;
    [[nodiscard]] std::vector<mc::entity::AxisAlignedBB> getCollisionBoxes(
        const mc::entity::Entity* entity,
        const mc::entity::AxisAlignedBB& area) const;
    [[nodiscard]] bool canSeeSky(int x, int y, int z) const;
    [[nodiscard]] float getLightBrightness(int x, int y, int z) const;
    [[nodiscard]] bool isDaytime() const;
    [[nodiscard]] mc::entity::Difficulty getDifficulty() const noexcept;
    void setDifficulty(mc::entity::Difficulty difficulty) noexcept;
    [[nodiscard]] std::uint64_t getWorldTime() const noexcept;
    void setWorldTime(std::uint64_t time) noexcept;
    void spawnXpOrbs(double x, double y, double z, int amount);
    void dropLootTable(
        const mc::core::ResourceLocation& table,
        double x,
        double y,
        double z,
        bool killedByPlayer
    );
    void spawnItemStack(
        const ItemStack& stack,
        double x,
        double y,
        double z,
        const glm::vec3& velocity = {}
    );
    [[nodiscard]] int countMobs(mc::entity::EnumCreatureType type) const;
    [[nodiscard]] JavaRandom& entityRandom() noexcept;
    void restoreEntities(std::vector<std::unique_ptr<mc::entity::Entity>> entities);
    [[nodiscard]] std::vector<mc::entity::Entity*> takePersistentEntities();

private:
    using ChunkKey = std::uint64_t;

    struct ChunkCoord
    {
        int x = 0;
        int z = 0;
    };

    struct GeneratedChunk
    {
        std::unique_ptr<Chunk> chunk;
        double milliseconds = 0.0;
    };

    struct VisibleMesh
    {
        const ChunkMesh* mesh = nullptr;
        std::uint16_t sectionMask = 0;
    };

    ChunkManager chunkManager_;
    std::unordered_map<ChunkKey, std::unique_ptr<ChunkMesh>> chunkMeshes_;
    std::vector<VisibleMesh> visibleMeshes_;
    BiomeColorMap biomeColours_;
    LightingSystem lighting_;
    FluidSystem fluidSystem_;
    mc::gameplay::FarmingSystem farmingSystem_;
    mc::gameplay::RedstoneSystem redstoneSystem_;
    BlockEntityStore blockEntities_;
    std::unordered_set<ChunkKey> modifiedChunks_;
    std::unordered_map<ChunkKey, ChunkSnapshot> persistedChunkSnapshots_;
    bool smoothLighting_ = true;
    bool fastLeaves_ = false;

    int seed_ = 1337;
    StructureGenerator structureLocator_;
    BiomeMap biomeLocator_;
    int renderDistance_ = 3;
    int unloadDistance_ = 4;

    std::unordered_set<ChunkKey> desiredChunks_;
    std::unordered_set<ChunkKey> generationInFlight_;
    std::unordered_set<ChunkKey> meshInFlight_;
    std::unordered_map<ChunkKey, std::uint64_t> meshVersions_;
    std::unordered_map<ChunkKey, std::uint64_t> uploadedMeshVersions_;
    std::unordered_set<ChunkKey> highPriorityMeshKeys_;
    std::unordered_set<ChunkKey> initialMeshPending_;

    std::deque<ChunkCoord> generationRequests_;
    std::deque<GeneratedChunk> generationResults_;
    std::deque<ChunkMeshInput> meshRequests_;
    std::deque<ChunkMeshData> meshResults_;

    mutable std::mutex generationMutex_;
    mutable std::mutex meshMutex_;
    std::condition_variable generationCv_;
    std::condition_variable meshCv_;

    std::vector<std::thread> generationThreads_;
    std::vector<std::thread> meshThreads_;
    std::atomic<bool> stopping_{false};

    std::exception_ptr workerException_;
    mutable std::mutex workerExceptionMutex_;

    int cameraChunkX_ = 0;
    int cameraChunkZ_ = 0;
    bool initialized_ = false;

    int visibleFaceCount_ = 0;
    int vertexCount_ = 0;
    int drawnChunks_ = 0;
    int culledChunks_ = 0;

    double terrainMilliseconds_ = 0.0;
    double meshMilliseconds_ = 0.0;
    double uploadMilliseconds_ = 0.0;
    double worldUpdateMilliseconds_ = 0.0;

    std::vector<std::unique_ptr<mc::entity::Entity>> entities_;
    mc::entity::PlayerEntity* player_ = nullptr;
    mc::entity::Difficulty difficulty_ = mc::entity::Difficulty::Normal;
    std::uint64_t worldTime_ = 0;
    JavaRandom entityRandom_{1337};
    int spawnHostileTimer_ = 0;

    void refreshDesiredChunks(int centerChunkX, int centerChunkZ);
    void initializeGeneratedBlockEntities(const Chunk& chunk);
    void queueChunkGeneration(int chunkX, int chunkZ);
    void queueChunkMesh(
        int chunkX,
        int chunkZ,
        bool highPriority = false
    );
    void queueChunkMeshAndNeighbours(
        int chunkX,
        int chunkZ,
        bool highPriority = false
    );
    void queueChunkMeshArea(
        int chunkX,
        int chunkZ,
        bool highPriority = false
    );
    void queueMeshesForBlockEdit(
        int worldX,
        int worldZ,
        bool highPriority = true
    );
    void handleLightingUpdate(const LightingSystem::UpdatedChunk& update);
    void queueMeshesForLightingUpdate(
        const LightingSystem::UpdatedChunk& update
    );

    [[nodiscard]] ChunkMeshInput createMeshInput(
        int chunkX,
        int chunkZ,
        std::uint64_t version) const;

    void integrateCompletedJobs(double budgetMilliseconds);
    void removeDistantChunks();

    void storeWorkerException() noexcept;
    void rethrowWorkerException();

    void generationWorker();
    void meshWorker();

    [[nodiscard]] static ChunkKey makeKey(int chunkX, int chunkZ) noexcept;
    [[nodiscard]] static int worldToChunk(float coordinate, int chunkSize) noexcept;
    [[nodiscard]] static int floorDivide(int value, int divisor) noexcept;
    [[nodiscard]] static int positiveModulo(int value, int divisor) noexcept;
};
