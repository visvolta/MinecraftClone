#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <imgui.h>

#include "AssetPaths.h"
#include "Atmosphere.h"
#include "BlockOutline.h"
#include "BlockDamageOverlay.h"
#include "BlockBreakingController.h"
#include "BlockShape.h"
#include "Camera.h"
#include "CameraEffects.h"
#include "DeathScreen.h"
#include "DebugOverlay.h"
#include "Frustum.h"
#include "FluidTextures.h"
#include "Inventory.h"
#include "InventoryUI.h"
#include "ItemAtlas.h"
#include "ItemEntityManager.h"
#include "Shader.h"
#include "SkyRenderer.h"
#include "Player.h"
#include "PlayerHUD.h"
#include "PostProcessor.h"
#include "Raycast.h"
#include "SaveGame.h"
#include "Texture2D.h"
#include "World.h"
#include "TextureAtlas.h"
#include "client/render/RuntimeTextureAtlas.h"
#include "client/ClientApplication.h"
#include "content/resources/ResourcePack.h"
#include "engine/FixedStepClock.h"
#include "game/GameBootstrap.h"

namespace
{
    constexpr int initialWindowWidth = 1280;
    constexpr int initialWindowHeight = 720;

    void framebufferSizeCallback(GLFWwindow*, int width, int height)
    {
        glViewport(0, 0, width, height);
    }

    void mouseCallback(GLFWwindow* window, double xPosition, double yPosition)
    {
        if (glfwGetInputMode(window, GLFW_CURSOR) != GLFW_CURSOR_DISABLED)
            return;

        auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
        if (camera != nullptr)
            camera->processMousePosition(xPosition, yPosition);
    }

    void focusCallback(GLFWwindow* window, int focused)
    {
        if (focused == GLFW_TRUE)
        {
            auto* camera = static_cast<Camera*>(glfwGetWindowUserPointer(window));
            if (camera != nullptr)
                camera->resetMouseTracking();
        }
    }

    BlockType selectedBlockFromNumberKeys(GLFWwindow* window, BlockType current)
    {
        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) return BlockType::Dirt;
        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) return BlockType::Grass;
        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) return BlockType::Stone;
        if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) return BlockType::Cobblestone;
        if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) return BlockType::Gravel;
        if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) return BlockType::Water;
        if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) return BlockType::Bedrock;
        if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) return BlockType::OakLog;
        if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) return BlockType::OakLeaves;
        if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) return BlockType::Sand;
        return current;
    }

    std::uint8_t furnacePlacementMetadata(const glm::vec3& lookDirection)
    {
        if (std::abs(lookDirection.x) > std::abs(lookDirection.z))
            return lookDirection.x > 0.0f ? 4U : 5U;
        return lookDirection.z > 0.0f ? 2U : 3U;
    }

    const char* blockName(BlockType block)
    {
        switch (block)
        {
            case BlockType::Dirt: return "Dirt";
            case BlockType::Grass: return "Grass";
            case BlockType::Stone: return "Stone";
            case BlockType::Cobblestone: return "Cobblestone";
            case BlockType::Gravel: return "Gravel";
            case BlockType::Water: return "Water";
            case BlockType::Bedrock: return "Bedrock";
            case BlockType::OakLog: return "Oak Log";
            case BlockType::OakLeaves: return "Oak Leaves";
            case BlockType::Sand: return "Sand";
            case BlockType::Clay: return "Clay";
            case BlockType::IronOre: return "Iron Ore";
            case BlockType::GoldOre: return "Gold Ore";
            case BlockType::RedstoneOre: return "Redstone Ore";
            case BlockType::DiamondOre: return "Diamond Ore";
            case BlockType::CoalOre: return "Coal Ore";
            case BlockType::SpruceLeaves: return "Spruce Leaves";
            case BlockType::BirchLeaves: return "Birch Leaves";
            case BlockType::SpruceLog: return "Spruce Log";
            case BlockType::BirchLog: return "Birch Log";
            case BlockType::BrownMushroom: return "Brown Mushroom";
            case BlockType::RedMushroom: return "Red Mushroom";
            case BlockType::TallGrass: return "Tall Grass";
            case BlockType::Rose: return "Rose";
            case BlockType::Dandelion: return "Dandelion";
            case BlockType::MossyCobblestone: return "Mossy Cobblestone";
            case BlockType::Spawner: return "Spawner";
            case BlockType::Chest: return "Chest";
            case BlockType::Pumpkin: return "Pumpkin";
            case BlockType::CraftingTable: return "Crafting Table";
            case BlockType::OakPlanks: return "Oak Planks";
            case BlockType::SprucePlanks: return "Spruce Planks";
            case BlockType::BirchPlanks: return "Birch Planks";
            case BlockType::Sandstone: return "Sandstone";
            case BlockType::Bricks: return "Bricks";
            case BlockType::HayBale: return "Hay Bale";
            case BlockType::Ladder: return "Ladder";
            case BlockType::LapisBlock: return "Lapis Block";
            case BlockType::LapisOre: return "Lapis Ore";
            case BlockType::IronBlock: return "Iron Block";
            case BlockType::GoldBlock: return "Gold Block";
            case BlockType::WhiteWool: return "White Wool";
            case BlockType::OrangeWool: return "Orange Wool";
            case BlockType::MagentaWool: return "Magenta Wool";
            case BlockType::LightBlueWool: return "Light Blue Wool";
            case BlockType::YellowWool: return "Yellow Wool";
            case BlockType::LimeWool: return "Lime Wool";
            case BlockType::PinkWool: return "Pink Wool";
            case BlockType::GrayWool: return "Gray Wool";
            case BlockType::LightGrayWool: return "Light Gray Wool";
            case BlockType::CyanWool: return "Cyan Wool";
            case BlockType::PurpleWool: return "Purple Wool";
            case BlockType::BlueWool: return "Blue Wool";
            case BlockType::BrownWool: return "Brown Wool";
            case BlockType::GreenWool: return "Green Wool";
            case BlockType::RedWool: return "Red Wool";
            case BlockType::BlackWool: return "Black Wool";
            case BlockType::Obsidian: return "Obsidian";
            case BlockType::Furnace: return "Furnace";
            case BlockType::LitFurnace: return "Lit Furnace";
            case BlockType::Lava: return "Lava";
            case BlockType::Air: return "Air";
            default: return "Unknown";
        }
    }

    GameSaveData captureSaveData(
        const World& world,
        const Player& player,
        const Inventory& inventory,
        const Atmosphere& atmosphere)
    {
        GameSaveData data;
        data.seed = world.getSeed();
        data.worldTime = atmosphere.getWorldTime();
        data.player = player.persistentState();
        data.inventory = inventory.getSlots();
        data.selectedHotbarSlot = inventory.getSelectedHotbarSlot();
        data.modifiedChunks = world.getModifiedChunkSnapshots();
        data.blockEntities = world.getBlockEntitySnapshots();
        data.fluidTicks = world.getScheduledFluidTickSnapshots();
        return data;
    }
}

int mc::client::ClientApplication::run(int argc, char** argv)
{
    try
    {
        AssetPaths::initialize(argc > 0 ? argv[0] : nullptr);
        std::cout << "Asset directory: " << AssetPaths::root().string() << '\n';
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }
    if (glfwInit() == GLFW_FALSE)
    {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        initialWindowWidth,
        initialWindowHeight,
        "Minecraft Clone",
        nullptr,
        nullptr
    );

    if (window == nullptr)
    {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)))
    {
        std::cerr << "Failed to initialize GLAD\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    Camera camera;
    glfwSetWindowUserPointer(window, &camera);
    glfwSetCursorPosCallback(window, mouseCallback);
    glfwSetWindowFocusCallback(window, focusCallback);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (glfwRawMouseMotionSupported() == GLFW_TRUE)
        glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    // GLFW window dimensions are logical screen coordinates. On Windows
    // display scaling, the OpenGL framebuffer can be larger. Rendering into a
    // viewport based on the logical size makes the entire final framebuffer,
    // including ImGui, get stretched and blurred.
    int initialFramebufferWidth = 0;
    int initialFramebufferHeight = 0;
    glfwGetFramebufferSize(
        window,
        &initialFramebufferWidth,
        &initialFramebufferHeight
    );
    glViewport(
        0,
        0,
        initialFramebufferWidth,
        initialFramebufferHeight
    );

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    try
    {
        Shader blockShader(AssetPaths::get("shaders/block.vert"), AssetPaths::get("shaders/block.frag"));
        Shader outlineShader(AssetPaths::get("shaders/outline.vert"), AssetPaths::get("shaders/outline.frag"));
        mc::game::GameBootstrap gameBootstrap;
        gameBootstrap.loadContentModules();
        gameBootstrap.freezeRegistries();
        FurnaceRecipeRegistry::initialize(
            AssetPaths::root(), gameBootstrap.content()
        );

        mc::client::RuntimeTextureAtlas runtimeBlockAtlas(
            AssetPaths::root() / "textures" / "blocks"
        );
        const mc::content::resources::ResourcePack resourcePack(
            AssetPaths::root()
        );
        const auto stoneBlockState = resourcePack.loadBlockState(
            mc::core::ResourceLocation("minecraft:stone")
        );
        const auto stoneModel = resourcePack.resolveModel(
            mc::core::ResourceLocation("minecraft:block/stone")
        );
        const auto stoneTextures = resourcePack.textureDependencies(stoneModel);
        TextureAtlas::initialize(
            runtimeBlockAtlas,
            gameBootstrap.content(),
            resourcePack
        );
        const Texture2D& blockAtlas = runtimeBlockAtlas.texture();
        std::cout
            << "Registered " << gameBootstrap.content().blocks().size()
            << " blocks and " << gameBootstrap.content().items().size()
            << " items; stitched " << runtimeBlockAtlas.textureCount()
            << " block textures at runtime; loaded "
            << stoneBlockState.variants.size() << " stone state variant and "
            << stoneTextures.size() << " model texture dependency\n";
        ItemAtlas itemAtlas(
            AssetPaths::root() / "textures"
        );
        FluidTextures fluidTextures;
        BlockOutline blockOutline;
        BlockDamageOverlay blockDamageOverlay;
        BlockBreakingController blockBreakingController;
        Inventory inventory;
        InventoryUI inventoryUI(gameBootstrap.content(), AssetPaths::root());
        PlayerHUD playerHUD;
        DeathScreen deathScreen;
        ItemEntityManager itemEntities;
        SkyRenderer skyRenderer;
        Atmosphere atmosphere;
        PostProcessor postProcessor;
        DebugOverlay debugOverlay(window);

        const std::filesystem::path savePath = SaveGame::defaultPath();
        std::string saveMessage;
        std::optional<GameSaveData> loadedSave = SaveGame::load(
            savePath, saveMessage, gameBootstrap.content()
        );
        std::cout << saveMessage << '\n';

        blockShader.use();
        blockShader.setInt("blockTexture", 0);
        fluidTextures.configureShader(blockShader);

        constexpr int startupRenderDistance = 1;
        constexpr int gameplayRenderDistance = 10;

        std::cout << "Loading spawn area...\n";
        World world(
            startupRenderDistance,
            loadedSave ? loadedSave->seed : 1337
        );
        if (loadedSave)
        {
            world.restorePersistentState(
                std::move(loadedSave->modifiedChunks),
                std::move(loadedSave->blockEntities),
                loadedSave->fluidTicks
            );
            inventory.restore(
                loadedSave->inventory,
                loadedSave->selectedHotbarSlot
            );
            atmosphere.setWorldTime(loadedSave->worldTime);
        }

        // Spawn in the centre of a block column, one block above its surface.
        // X/Z use +0.5 because Minecraft block coordinates describe minimum corners.
        const int spawnBlockX = 0;
        const int spawnBlockZ = 20;
        const glm::vec3 spawnColumn(
            static_cast<float>(spawnBlockX) + 0.5f,
            0.0f,
            static_cast<float>(spawnBlockZ) + 0.5f
        );

        const glm::vec3 initialLoadPosition = loadedSave
            ? loadedSave->player.position
            : spawnColumn;
        world.update(initialLoadPosition);
        world.finishInitialLoad();

        // Start rendering after only the 3x3 spawn area is ready. The full
        // seven-chunk distance then streams in asynchronously during gameplay.
        world.setRenderDistance(gameplayRenderDistance);

        const float spawnFeetY =
            static_cast<float>(world.getHighestSolidBlockY(spawnBlockX, spawnBlockZ) + 1);

        const glm::vec3 spawnFeetPosition(
            spawnColumn.x,
            spawnFeetY,
            spawnColumn.z
        );
        Player player(initialLoadPosition.y > 0.0f
            ? initialLoadPosition
            : spawnFeetPosition);
        if (loadedSave)
            player.restorePersistentState(loadedSave->player);
        camera.setPosition(player.getEyePosition());

        std::cout << "Chunks: " << world.getLoadedChunkCount()
                  << ", visible faces: " << world.getVisibleFaceCount()
                  << ", vertices: " << world.getVertexCount() << '\n';

        const glm::mat4 model(1.0f);
        constexpr float blockReach = 5.0f;
        bool rightMouseWasPressed = false;
        bool escapeWasPressed = false;
        bool inventoryWasPressed = false;
        bool dropWasPressed = false;
        bool cursorCaptured = true;
        bool deathScreenActive = false;
        bool fastLeaves = false;
        AntiAliasingMode antiAliasingMode = AntiAliasingMode::Off;
        double previousFrameTime = glfwGetTime();
        mc::engine::FixedStepClock gameClock(20.0, 5);

        while (!glfwWindowShouldClose(window))
        {
            // Process mouse and keyboard events before simulation/rendering to
            // avoid an extra frame of input latency.
            glfwPollEvents();

            debugOverlay.beginFrame();

            const double currentFrameTime = glfwGetTime();
            const double rawDeltaTime = currentFrameTime - previousFrameTime;
            previousFrameTime = currentFrameTime;

            // Rendering may run at any FPS, but simulation-facing time is
            // clamped and all classic animations advance from a 20 Hz clock.
            const double clampedDeltaTime =
                std::clamp(rawDeltaTime, 0.0, 0.1);
            const float deltaTime =
                static_cast<float>(clampedDeltaTime);

            const int processedGameTicks = gameClock.advance(clampedDeltaTime);
            for (int tickIndex = 0;
                 tickIndex < processedGameTicks;
                 ++tickIndex)
            {
                const std::uint64_t gameTick =
                    gameClock.tickCount() -
                    static_cast<std::uint64_t>(processedGameTicks) +
                    static_cast<std::uint64_t>(tickIndex) + 1U;

                world.tick();
                itemEntities.tick(
                    world,
                    player,
                    inventory
                );
                atmosphere.tick(world, player.getEyePosition());

                if (gameTick % 600 == 0)
                {
                    const GameSaveData data = captureSaveData(
                        world, player, inventory, atmosphere
                    );
                    if (!SaveGame::save(
                            savePath,
                            data,
                            saveMessage,
                            gameBootstrap.content()))
                        std::cerr << "Autosave failed: " << saveMessage << '\n';
                }
            }

            const bool escapePressed =
                glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;

            if (player.isAlive() && escapePressed && !escapeWasPressed)
            {
                if (inventoryUI.isOpen())
                {
                    inventoryUI.close(inventory);
                    cursorCaptured = true;
                }
                else
                {
                    cursorCaptured = false;
                }

                glfwSetInputMode(
                    window,
                    GLFW_CURSOR,
                    cursorCaptured
                        ? GLFW_CURSOR_DISABLED
                        : GLFW_CURSOR_NORMAL
                );
                if (glfwRawMouseMotionSupported() == GLFW_TRUE)
                {
                    glfwSetInputMode(
                        window,
                        GLFW_RAW_MOUSE_MOTION,
                        cursorCaptured ? GLFW_TRUE : GLFW_FALSE
                    );
                }
                camera.resetMouseTracking();
            }
            escapeWasPressed = escapePressed;

            const bool inventoryPressed =
                glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
            if (player.isAlive() &&
                inventoryPressed && !inventoryWasPressed)
            {
                inventoryUI.toggleInventory(inventory);
                cursorCaptured = !inventoryUI.isOpen();
                glfwSetInputMode(
                    window,
                    GLFW_CURSOR,
                    cursorCaptured
                        ? GLFW_CURSOR_DISABLED
                        : GLFW_CURSOR_NORMAL
                );
                if (glfwRawMouseMotionSupported() == GLFW_TRUE)
                {
                    glfwSetInputMode(
                        window,
                        GLFW_RAW_MOUSE_MOTION,
                        cursorCaptured ? GLFW_TRUE : GLFW_FALSE
                    );
                }
                camera.resetMouseTracking();
                blockBreakingController.reset();
            }
            inventoryWasPressed = inventoryPressed;

            // Click outside an ImGui control to return to mouse-look.
            if (!cursorCaptured &&
                player.isAlive() &&
                !inventoryUI.isOpen() &&
                glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS &&
                !ImGui::GetIO().WantCaptureMouse)
            {
                cursorCaptured = true;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                if (glfwRawMouseMotionSupported() == GLFW_TRUE)
                    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);
                camera.resetMouseTracking();
            }

            int armorPoints = 0;
            float armorToughness = 0.0f;
            constexpr ArmorSlot expectedArmorSlots[Inventory::ARMOR_SLOT_COUNT] = {
                ArmorSlot::Head, ArmorSlot::Chest, ArmorSlot::Legs, ArmorSlot::Feet
            };
            for (int armor = 0; armor < Inventory::ARMOR_SLOT_COUNT; ++armor)
            {
                const ItemProperties& properties = getItemProperties(
                    inventory.getSlot(Inventory::ARMOR_SLOT_START + armor).item
                );
                if (properties.armorSlot == expectedArmorSlots[armor])
                {
                    armorPoints += properties.armorPoints;
                    armorToughness += properties.armorToughness;
                }
            }
            player.survival().setArmor(armorPoints, armorToughness);
            player.update(window, deltaTime, world, camera);
            world.setFastLeavesEnabled(fastLeaves);
            world.update(player.getPosition());

            if (!player.isAlive() && !deathScreenActive)
            {
                deathScreenActive = true;
                if (inventoryUI.isOpen())
                    inventoryUI.close(inventory);

                cursorCaptured = false;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                if (glfwRawMouseMotionSupported() == GLFW_TRUE)
                {
                    glfwSetInputMode(
                        window,
                        GLFW_RAW_MOUSE_MOTION,
                        GLFW_FALSE
                    );
                }
                camera.resetMouseTracking();
                blockBreakingController.reset();
            }

            for (int slot = 0; slot < Inventory::HOTBAR_SIZE; ++slot)
            {
                if (glfwGetKey(window, GLFW_KEY_1 + slot) == GLFW_PRESS)
                    inventory.setSelectedHotbarSlot(slot);
            }

            const bool dropPressed =
                player.isAlive() &&
                cursorCaptured &&
                glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS;

            if (dropPressed && !dropWasPressed)
            {
                ItemStack& selectedStack = inventory.getSlot(
                    inventory.getSelectedHotbarSlot()
                );

                if (!selectedStack.empty())
                {
                    const ItemStack dropped{
                        selectedStack.item,
                        1,
                        selectedStack.damage
                    };

                    --selectedStack.count;
                    if (selectedStack.count == 0)
                        selectedStack.clear();

                    itemEntities.spawnPlayerDrop(
                        dropped,
                        player.getEyePosition(),
                        camera.getForward()
                    );
                }
            }
            dropWasPressed = dropPressed;

            RaycastHit targetedBlock = Raycast::cast(
                world,
                camera.getPosition(),
                camera.getForward(),
                blockReach
            );

            const bool allowWorldMouseInput =
                player.isAlive() &&
                cursorCaptured && !ImGui::GetIO().WantCaptureMouse;

            const bool leftMousePressed =
                allowWorldMouseInput &&
                glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            const bool rightMousePressed =
                allowWorldMouseInput &&
                glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;

            const ItemProperties& mainHandProperties =
                getItemProperties(inventory.getSelectedItem());
            const bool offhandShield = getItemProperties(
                inventory.getSlot(Inventory::OFFHAND_SLOT).item
            ).shield;
            bool targetedInteraction = false;
            if (targetedBlock.hit)
            {
                const BlockType target = world.getBlock(
                    targetedBlock.blockPosition.x,
                    targetedBlock.blockPosition.y,
                    targetedBlock.blockPosition.z
                );
                targetedInteraction = target == BlockType::CraftingTable ||
                    isFurnace(target) || target == BlockType::Chest ||
                    target == BlockType::Lever || target == BlockType::Farmland;
            }
            const bool mainHandHasUse =
                inventory.getSelectedBlock() != BlockType::Air ||
                mainHandProperties.foodPoints > 0;
            const bool usingShield = mainHandProperties.shield ||
                (offhandShield && !mainHandHasUse && !targetedInteraction);
            player.setBlocking(rightMousePressed && usingShield);

            blockBreakingController.update(
                leftMousePressed,
                targetedBlock,
                deltaTime,
                inventory.getSlot(inventory.getSelectedHotbarSlot()),
                player.isGrounded(),
                player.isHeadUnderwater(),
                player.survival().effectAmplifier(
                    mc::gameplay::StatusEffectType::Haste
                ),
                player.survival().effectAmplifier(
                    mc::gameplay::StatusEffectType::MiningFatigue
                ),
                false,
                world,
                itemEntities
            );

            bool consumedFood = false;
            if (rightMousePressed && !rightMouseWasPressed)
            {
                ItemStack& held = inventory.getSlot(
                    inventory.getSelectedHotbarSlot()
                );
                const ItemProperties& item = getItemProperties(held.item);
                if (!held.empty() && item.foodPoints > 0 &&
                    player.survival().foodLevel() < 20)
                {
                    player.eat(item.foodPoints, item.saturationModifier);
                    --held.count;
                    if (held.count == 0)
                        held.clear();
                    consumedFood = true;
                }
            }

            if (!consumedFood && !usingShield && rightMousePressed &&
                !rightMouseWasPressed && targetedBlock.hit)
            {
                const BlockType targetedType = world.getBlockState(
                    targetedBlock.blockPosition.x,
                    targetedBlock.blockPosition.y,
                    targetedBlock.blockPosition.z
                ).block();

                const glm::ivec3 placementPosition =
                    targetedBlock.previousPosition;
                ItemStack& selectedStack = inventory.getSlot(
                    inventory.getSelectedHotbarSlot()
                );
                BlockType plantedCrop = BlockType::Air;
                if (selectedStack.item == ItemType::Seeds)
                    plantedCrop = BlockType::Wheat;
                else if (selectedStack.item == ItemType::Carrot)
                    plantedCrop = BlockType::Carrots;
                else if (selectedStack.item == ItemType::Potato)
                    plantedCrop = BlockType::Potatoes;

                if (targetedType == BlockType::Farmland &&
                    plantedCrop != BlockType::Air &&
                    placementPosition.y == targetedBlock.blockPosition.y + 1 &&
                    world.getBlock(
                        placementPosition.x,
                        placementPosition.y,
                        placementPosition.z
                    ) == BlockType::Air)
                {
                    if (world.setBlock(
                            placementPosition.x,
                            placementPosition.y,
                            placementPosition.z,
                            plantedCrop))
                    {
                        --selectedStack.count;
                        if (selectedStack.count == 0)
                            selectedStack.clear();
                    }
                }
                else if (targetedType == BlockType::CraftingTable ||
                    isFurnace(targetedType) ||
                    targetedType == BlockType::Chest)
                {
                    bool opened = true;
                    if (isFurnace(targetedType))
                    {
                        inventoryUI.openFurnace(
                            world.getOrCreateFurnace(
                                targetedBlock.blockPosition.x,
                                targetedBlock.blockPosition.y,
                                targetedBlock.blockPosition.z
                            )
                        );
                    }
                    else if (targetedType == BlockType::Chest)
                    {
                        const ChestInventoryView chest = world.getChestInventory(
                            targetedBlock.blockPosition.x,
                            targetedBlock.blockPosition.y,
                            targetedBlock.blockPosition.z
                        );
                        opened = chest.valid();
                        if (opened)
                            inventoryUI.openChest(*chest.first, chest.second);
                    }
                    else
                    {
                        inventoryUI.openCraftingTable();
                    }
                    if (opened)
                        cursorCaptured = false;
                    glfwSetInputMode(
                        window,
                        GLFW_CURSOR,
                        opened ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED
                    );
                    if (glfwRawMouseMotionSupported() == GLFW_TRUE)
                    {
                        glfwSetInputMode(
                            window,
                            GLFW_RAW_MOUSE_MOTION,
                            opened ? GLFW_FALSE : GLFW_TRUE
                        );
                    }
                    if (opened)
                    {
                        camera.resetMouseTracking();
                        blockBreakingController.reset();
                    }
                }
                else if (targetedType == BlockType::Lever)
                {
                    const auto leverState = world.getBlockState(
                        targetedBlock.blockPosition.x,
                        targetedBlock.blockPosition.y,
                        targetedBlock.blockPosition.z
                    );
                    world.setBlockAndMetadata(
                        targetedBlock.blockPosition.x,
                        targetedBlock.blockPosition.y,
                        targetedBlock.blockPosition.z,
                        BlockType::Lever,
                        static_cast<std::uint8_t>(
                            (leverState.properties() & 1U) == 0U ? 1U : 0U
                        )
                    );
                }
                else
                {
                    const BlockType selectedBlock =
                        inventory.getSelectedBlock();
                    if (world.getBlock(
                            placementPosition.x,
                            placementPosition.y,
                            placementPosition.z) == BlockType::Air &&
                        !player.overlapsBlock(
                            placementPosition,
                            selectedBlock) &&
                        selectedBlock != BlockType::Air &&
                        (selectedBlock != BlockType::Chest ||
                         world.canPlaceChest(
                             placementPosition.x,
                             placementPosition.y,
                             placementPosition.z)))
                    {
                        const std::uint8_t metadata =
                            (selectedBlock == BlockType::Furnace ||
                             selectedBlock == BlockType::Chest)
                                ? furnacePlacementMetadata(camera.getForward())
                                : 0U;
                        if (world.setBlockAndMetadata(
                            placementPosition.x,
                            placementPosition.y,
                            placementPosition.z,
                            selectedBlock,
                            metadata))
                        {
                            --selectedStack.count;
                            if (selectedStack.count == 0)
                                selectedStack.clear();
                        }
                    }
                }
            }

            rightMouseWasPressed = rightMousePressed;

            // Refresh the selection immediately after an edit.
            targetedBlock = Raycast::cast(
                world,
                camera.getPosition(),
                camera.getForward(),
                blockReach
            );

            int framebufferWidth = 0;
            int framebufferHeight = 0;
            glfwGetFramebufferSize(
                window,
                &framebufferWidth,
                &framebufferHeight
            );
            if (framebufferHeight == 0)
                continue;

            glViewport(
                0,
                0,
                framebufferWidth,
                framebufferHeight
            );
            postProcessor.beginFrame(
                framebufferWidth,
                framebufferHeight,
                antiAliasingMode
            );

            const float partialGameTick = gameClock.partialTick();
            const glm::mat4 view = CameraEffects::apply(
                camera.getViewMatrix(),
                player,
                partialGameTick
            );
            const AtmosphereState atmosphereState = atmosphere.sample(
                world,
                player,
                camera.getPosition(),
                world.getRenderDistance(),
                partialGameTick
            );
            const float verticalFieldOfView = glm::radians(
                CameraEffects::getFieldOfView(
                    camera.getZoom(),
                    player,
                    partialGameTick
                )
            );
            const float aspectRatio =
                static_cast<float>(framebufferWidth) /
                static_cast<float>(framebufferHeight);
            const glm::mat4 projection = postProcessor.jitterProjection(
                glm::perspective(
                    verticalFieldOfView,
                    aspectRatio,
                    0.05f,
                    atmosphereState.farPlaneDistance * 2.0f
                ),
                antiAliasingMode
            );
            const glm::mat4 skyProjection = postProcessor.jitterProjection(
                glm::perspective(
                    verticalFieldOfView,
                    aspectRatio,
                    0.05f,
                    512.0f
                ),
                antiAliasingMode
            );

            glClearColor(
                atmosphereState.fogColour.r,
                atmosphereState.fogColour.g,
                atmosphereState.fogColour.b,
                1.0f
            );
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            skyRenderer.draw(view, skyProjection, atmosphereState);

            blockShader.use();
            blockShader.setMat4("model", model);
            blockShader.setMat4("view", view);
            blockShader.setMat4("projection", projection);
            blockShader.setBool("fastLeaves", fastLeaves);
            blockShader.setFloat(
                "daylightBrightness",
                atmosphereState.daylightBrightness
            );
            blockShader.setInt(
                "fogMode",
                static_cast<int>(atmosphereState.fogMode)
            );
            blockShader.setVec3("fogColour", atmosphereState.fogColour);
            blockShader.setFloat("fogStart", atmosphereState.fogStart);
            blockShader.setFloat("fogEnd", atmosphereState.fogEnd);
            blockShader.setFloat("fogDensity", atmosphereState.fogDensity);

            blockAtlas.bind(0);
            fluidTextures.bindFrame(blockShader, gameClock.tickCount());

            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            const Frustum frustum(projection * view);
            world.drawOpaque(frustum);

            // Both leaf modes are alpha-tested, depth-writing geometry. Fast
            // mode uses its culled opaque-volume mesh; fancy mode preserves
            // faces between neighbouring transparent leaf blocks.
            blockShader.setBool("fastLeaves", fastLeaves);
            world.drawLeaves(fastLeaves);

            // Plants and ladders remain in the normal cutout pass.
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            world.drawCutout();

            itemEntities.draw(
                partialGameTick,
                view,
                projection,
                blockAtlas,
                itemAtlas,
                atmosphereState
            );

            // Interaction overlays are drawn before fluids. Water blends over
            // them and opaque lava subsequently depth-occludes them, so a
            // selected solid can no longer show through a liquid surface.
            if (blockBreakingController.isBreaking())
            {
                blockDamageOverlay.draw(
                    blockBreakingController.getBlockPosition(),
                    blockBreakingController.getDestroyStage(),
                    view,
                    projection
                );
            }

            if (targetedBlock.hit)
            {
                const BlockType targetedType = world.getBlock(
                    targetedBlock.blockPosition.x,
                    targetedBlock.blockPosition.y,
                    targetedBlock.blockPosition.z
                );

                outlineShader.use();
                outlineShader.setMat4("view", view);
                outlineShader.setMat4("projection", projection);
                glLineWidth(1.0f);
                glDepthMask(GL_FALSE);
                for (const BlockBox& box :
                     getBlockShape(targetedType).selectionBoxes)
                {
                    const glm::vec3 scale = box.maximum - box.minimum;
                    const glm::vec3 centre =
                        (box.minimum + box.maximum) * 0.5f;
                    glm::mat4 outlineModel(1.0f);
                    outlineModel = glm::translate(
                        outlineModel,
                        glm::vec3(targetedBlock.blockPosition) + centre
                    );
                    outlineModel = glm::scale(outlineModel, scale);
                    outlineShader.setMat4("model", outlineModel);
                    blockOutline.draw();
                }
                glDepthMask(GL_TRUE);
            }

            // Item rendering binds its own shader. Explicitly restore the
            // block shader before fluid meshes; relying on the previous
            // program made updated water draw with the item vertex layout and
            // intermittently disappear.
            blockShader.use();
            blockShader.setBool("fastLeaves", fastLeaves);

            // Lava's texture is fully opaque. Give it a dedicated depth-
            // writing pass instead of treating it as blended water.
            glDisable(GL_BLEND);
            glDepthMask(GL_TRUE);
            glDisable(GL_CULL_FACE);
            world.drawLava();

            // Water is the only blended block pass. Chunks are rendered back
            // to front and depth writes remain disabled, while lava/terrain
            // depth still reject hidden water faces.
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            world.drawWater(camera.getPosition());

            glEnable(GL_CULL_FACE);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);

            postProcessor.resolve(
                antiAliasingMode,
                projection * view
            );

            if (player.isAlive())
            {
                debugOverlay.draw(
                    world,
                    player,
                    camera,
                    atmosphere,
                    antiAliasingMode,
                    player.getRenderPosition(),
                    fastLeaves
                );
                playerHUD.draw(
                    player,
                    camera,
                    framebufferWidth,
                    framebufferHeight,
                    !inventoryUI.isOpen()
                );
                inventoryUI.draw(
                    inventory,
                    blockAtlas,
                    itemAtlas,
                    framebufferWidth,
                    framebufferHeight
                );
            }
            else
            {
                const DeathScreenAction action = deathScreen.draw(
                    framebufferWidth,
                    framebufferHeight
                );

                if (action == DeathScreenAction::Respawn)
                {
                    player.respawn(spawnFeetPosition);
                    camera.setPosition(player.getEyePosition());
                    postProcessor.invalidateHistory();
                    deathScreenActive = false;
                    cursorCaptured = true;
                    glfwSetInputMode(
                        window,
                        GLFW_CURSOR,
                        GLFW_CURSOR_DISABLED
                    );
                    if (glfwRawMouseMotionSupported() == GLFW_TRUE)
                    {
                        glfwSetInputMode(
                            window,
                            GLFW_RAW_MOUSE_MOTION,
                            GLFW_TRUE
                        );
                    }
                    camera.resetMouseTracking();
                    rightMouseWasPressed = false;
                    dropWasPressed = false;
                    blockBreakingController.reset();
                }
                else if (action == DeathScreenAction::TitleMenu)
                {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
            }
            debugOverlay.render();

            glfwSwapBuffers(window);
        }

        const GameSaveData finalSave = captureSaveData(
            world, player, inventory, atmosphere
        );
        if (!SaveGame::save(
                savePath,
                finalSave,
                saveMessage,
                gameBootstrap.content()))
            std::cerr << "Final save failed: " << saveMessage << '\n';
    }
catch (const std::exception& error)
{
    std::cerr << "\nFatal error:\n" << error.what() << "\n";
    std::cerr << "Press Enter to close...\n";
    std::cin.get();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
}

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
