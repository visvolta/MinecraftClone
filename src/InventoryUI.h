#pragma once

#include "CraftingRecipe.h"
#include "Inventory.h"
#include "Furnace.h"
#include "BlockEntity.h"

#include <filesystem>
#include <memory>

namespace mc::content { class ContentCatalog; }

class Texture2D;
class ItemAtlas;

class InventoryUI
{
public:
    InventoryUI(
        const mc::content::ContentCatalog& content,
        const std::filesystem::path& assetRoot
    );

    void toggleInventory(Inventory& inventory);
    void openCraftingTable();
    void openFurnace(FurnaceBlockEntity& furnace);
    void openChest(
        ChestBlockEntity& first,
        ChestBlockEntity* second = nullptr
    );
    void close(Inventory& inventory);
    [[nodiscard]] bool isOpen() const noexcept;
    [[nodiscard]] bool isCraftingTableOpen() const noexcept;
    [[nodiscard]] bool isFurnaceOpen() const noexcept;
    [[nodiscard]] bool isChestOpen() const noexcept;

    void draw(
        Inventory& inventory,
        const Texture2D& blockAtlas,
        const ItemAtlas& itemAtlas,
        int framebufferWidth,
        int framebufferHeight
    );

private:
    // Selected per frame with the 1.12 ScaledResolution rules. Rendering is
    // single-threaded, so the helpers can share the scale selected by draw().
    inline static int UI_PIXEL_SCALE = 1;
    inline static int ICON_SIZE_PIXELS = 16;

    enum class Screen
    {
        None,
        Inventory,
        CraftingTable,
        Furnace,
        Chest
    };

    struct FramebufferMapping
    {
        int widthPixels = 1;
        int heightPixels = 1;
        float logicalToPhysicalX = 1.0f;
        float logicalToPhysicalY = 1.0f;

        [[nodiscard]] float logicalX(int physicalX) const noexcept;
        [[nodiscard]] float logicalY(int physicalY) const noexcept;
        [[nodiscard]] int physicalMouseX(float logicalX) const noexcept;
        [[nodiscard]] int physicalMouseY(float logicalY) const noexcept;
    };

    Screen screen_ = Screen::None;
    ItemStack cursorStack_{};
    CraftingGrid craftingGrid_{};
    RecipeRegistry recipes_;
    FurnaceBlockEntity* furnace_ = nullptr;
    ChestBlockEntity* chestFirst_ = nullptr;
    ChestBlockEntity* chestSecond_ = nullptr;

    std::unique_ptr<Texture2D> inventoryTexture_;
    std::unique_ptr<Texture2D> craftingTableTexture_;
    std::unique_ptr<Texture2D> furnaceTexture_;
    std::unique_ptr<Texture2D> chestTexture_;
    std::unique_ptr<Texture2D> hotbarTexture_;
    std::unique_ptr<Texture2D> selectedSlotTexture_;

    [[nodiscard]] static FramebufferMapping makeMapping(
        int framebufferWidth,
        int framebufferHeight
    ) noexcept;

    static void drawTexture(
        const Texture2D& texture,
        const FramebufferMapping& mapping,
        int xPixels,
        int yPixels,
        int widthPixels,
        int heightPixels
    );

    static void drawTextureRegion(
        const Texture2D& texture,
        const FramebufferMapping& mapping,
        int xPixels,
        int yPixels,
        int sourceWidthPixels,
        int sourceHeightPixels
    );

    static void drawTextureSubRegion(
        const Texture2D& texture,
        const FramebufferMapping& mapping,
        int xPixels,
        int yPixels,
        int sourceX,
        int sourceY,
        int sourceWidthPixels,
        int sourceHeightPixels
    );

    [[nodiscard]] static bool rendersAs3DBlock(
        BlockType block
    ) noexcept;

    static void drawFlatIcon(
        BlockType block,
        const Texture2D& atlas,
        const FramebufferMapping& mapping,
        int xPixels,
        int yPixels
    );

    static void drawItemIcon(
        ItemType item,
        const ItemAtlas& atlas,
        const FramebufferMapping& mapping,
        int xPixels,
        int yPixels
    );

    static void draw3DIcon(
        BlockType block,
        const Texture2D& atlas,
        const FramebufferMapping& mapping,
        int xPixels,
        int yPixels
    );

    static void drawStackCount(
        int count,
        const FramebufferMapping& mapping,
        int iconX,
        int iconY
    );

    static void drawDurabilityBar(
        const ItemStack& stack,
        const FramebufferMapping& mapping,
        int iconX,
        int iconY
    );

    static void drawStack(
        const ItemStack& stack,
        const Texture2D& blockAtlas,
        const ItemAtlas& itemAtlas,
        const FramebufferMapping& mapping,
        int xPixels,
        int yPixels
    );

    void drawHotbar(
        const Inventory& inventory,
        const Texture2D& blockAtlas,
        const ItemAtlas& itemAtlas,
        const FramebufferMapping& mapping
    );

    void drawContainer(
        Inventory& inventory,
        const Texture2D& blockAtlas,
        const ItemAtlas& itemAtlas,
        const FramebufferMapping& mapping,
        bool craftingTable
    );

    void drawFurnace(
        Inventory& inventory,
        const Texture2D& blockAtlas,
        const ItemAtlas& itemAtlas,
        const FramebufferMapping& mapping
    );
    void drawChest(
        Inventory& inventory,
        const Texture2D& blockAtlas,
        const ItemAtlas& itemAtlas,
        const FramebufferMapping& mapping
    );

    void returnCraftingItems(Inventory& inventory);
    void handleLeftClick(ItemStack& slot);
    void handleRightClick(ItemStack& slot);
    void takeFurnaceOutput(bool rightClick);
    [[nodiscard]] bool canTakeCraftingResult(
        const CraftingResult& result
    ) const noexcept;
    void takeCraftingResult(const CraftingRecipe& recipe);
};
