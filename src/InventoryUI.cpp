#include "InventoryUI.h"

#include "AssetPaths.h"
#include "ItemAtlas.h"
#include "Texture2D.h"
#include "TextureAtlas.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

namespace
{
    ImTextureID textureId(const Texture2D& texture)
    {
        return static_cast<ImTextureID>(texture.getId());
    }

    ImVec2 uvTopLeft(const AtlasUV& uv)
    {
        return {uv.minU, uv.maxV};
    }

    ImVec2 uvTopRight(const AtlasUV& uv)
    {
        return {uv.maxU, uv.maxV};
    }

    ImVec2 uvBottomRight(const AtlasUV& uv)
    {
        return {uv.maxU, uv.minV};
    }

    ImVec2 uvBottomLeft(const AtlasUV& uv)
    {
        return {uv.minU, uv.minV};
    }

    // 3x5 pixel digits. Rendering counts as rectangles avoids the smoothing
    // built into Dear ImGui's default anti-aliased font atlas.
    constexpr std::array<std::array<unsigned char, 5>, 10> DIGITS = {{
        {{0b111, 0b101, 0b101, 0b101, 0b111}},
        {{0b010, 0b110, 0b010, 0b010, 0b111}},
        {{0b111, 0b001, 0b111, 0b100, 0b111}},
        {{0b111, 0b001, 0b111, 0b001, 0b111}},
        {{0b101, 0b101, 0b111, 0b001, 0b001}},
        {{0b111, 0b100, 0b111, 0b001, 0b111}},
        {{0b111, 0b100, 0b111, 0b101, 0b111}},
        {{0b111, 0b001, 0b001, 0b001, 0b001}},
        {{0b111, 0b101, 0b111, 0b101, 0b111}},
        {{0b111, 0b101, 0b111, 0b001, 0b111}}
    }};

}

float InventoryUI::FramebufferMapping::logicalX(
    int physicalX) const noexcept
{
    // Keep the GUI rendered in a fixed physical-pixel space and snap the
    // converted logical coordinates to whole pixels so the inventory and
    // hotbar do not land on fractional edges and look soft when scaled.
    return std::round(
        static_cast<float>(physicalX) / logicalToPhysicalX
    );
}

float InventoryUI::FramebufferMapping::logicalY(
    int physicalY) const noexcept
{
    // Match the X-axis snapping so the hotbar/inventory quads stay aligned on
    // the same pixel grid in both directions.
    return std::round(
        static_cast<float>(physicalY) / logicalToPhysicalY
    );
}

int InventoryUI::FramebufferMapping::physicalMouseX(
    float logicalXValue) const noexcept
{
    // Mouse hit testing should use the same snapped pixel-space convention as
    // the UI rendering so hover/click targets stay aligned with the drawn slots.
    return static_cast<int>(
        std::lround(logicalXValue * logicalToPhysicalX)
    );
}

int InventoryUI::FramebufferMapping::physicalMouseY(
    float logicalYValue) const noexcept
{
    return static_cast<int>(
        std::lround(logicalYValue * logicalToPhysicalY)
    );
}

InventoryUI::InventoryUI(
    const mc::content::ContentCatalog& content,
    const std::filesystem::path& assetRoot)
    : recipes_(assetRoot, content)
{
    inventoryTexture_ = std::make_unique<Texture2D>(
        AssetPaths::get("gui/inventory.png"),
        256,
        256
    );
    craftingTableTexture_ = std::make_unique<Texture2D>(
        AssetPaths::get("gui/crafting_table.png"),
        256,
        256
    );
    furnaceTexture_ = std::make_unique<Texture2D>(
        AssetPaths::get("gui/furnace.png"),
        256,
        256
    );
    chestTexture_ = std::make_unique<Texture2D>(
        AssetPaths::get("gui/generic_54.png"),
        256,
        256
    );
    hotbarTexture_ = std::make_unique<Texture2D>(
        AssetPaths::get("gui/hotbar.png"),
        182,
        22
    );
    selectedSlotTexture_ = std::make_unique<Texture2D>(
        AssetPaths::get("gui/hotbarslot_selected.png"),
        24,
        24
    );
}

void InventoryUI::toggleInventory(Inventory& inventory)
{
    if (isOpen())
    {
        close(inventory);
        return;
    }

    screen_ = Screen::Inventory;
}

void InventoryUI::openCraftingTable()
{
    screen_ = Screen::CraftingTable;
}

void InventoryUI::openFurnace(FurnaceBlockEntity& furnace)
{
    furnace_ = &furnace;
    screen_ = Screen::Furnace;
}

void InventoryUI::openChest(
    ChestBlockEntity& first,
    ChestBlockEntity* second)
{
    chestFirst_ = &first;
    chestSecond_ = second;
    screen_ = Screen::Chest;
}

void InventoryUI::close(Inventory& inventory)
{
    if (isOpen())
        returnCraftingItems(inventory);

    screen_ = Screen::None;
    furnace_ = nullptr;
    chestFirst_ = nullptr;
    chestSecond_ = nullptr;
}

bool InventoryUI::isOpen() const noexcept
{
    return screen_ != Screen::None;
}

bool InventoryUI::isCraftingTableOpen() const noexcept
{
    return screen_ == Screen::CraftingTable;
}

bool InventoryUI::isFurnaceOpen() const noexcept
{
    return screen_ == Screen::Furnace;
}

bool InventoryUI::isChestOpen() const noexcept
{
    return screen_ == Screen::Chest;
}

InventoryUI::FramebufferMapping InventoryUI::makeMapping(
    int framebufferWidth,
    int framebufferHeight) noexcept
{
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    FramebufferMapping mapping;
    mapping.widthPixels = std::max(framebufferWidth, 1);
    mapping.heightPixels = std::max(framebufferHeight, 1);

    // ScaledResolution in 1.12 chooses the largest integral scale that keeps
    // the logical GUI at least 320x240. This prevents a fixed 3x HUD from
    // overlapping in short or resized windows.
    UI_PIXEL_SCALE = 1;
    while (UI_PIXEL_SCALE < 1000 &&
           mapping.widthPixels / (UI_PIXEL_SCALE + 1) >= 320 &&
           mapping.heightPixels / (UI_PIXEL_SCALE + 1) >= 240)
    {
        ++UI_PIXEL_SCALE;
    }
    ICON_SIZE_PIXELS = 16 * UI_PIXEL_SCALE;

    // Use the framebuffer dimensions passed by main.cpp as the physical source
    // of truth. The inventory and hotbar are drawn in framebuffer pixels and
    // only converted to ImGui logical coordinates right before submission so
    // their edges stay crisp when the window is scaled.
    mapping.logicalToPhysicalX =
        displaySize.x > 0.0f
            ? static_cast<float>(mapping.widthPixels) / displaySize.x
            : 1.0f;
    mapping.logicalToPhysicalY =
        displaySize.y > 0.0f
            ? static_cast<float>(mapping.heightPixels) / displaySize.y
            : 1.0f;

    mapping.logicalToPhysicalX =
        std::max(mapping.logicalToPhysicalX, 0.001f);
    mapping.logicalToPhysicalY =
        std::max(mapping.logicalToPhysicalY, 0.001f);

    return mapping;
}

void InventoryUI::drawTexture(
    const Texture2D& texture,
    const FramebufferMapping& mapping,
    int xPixels,
    int yPixels,
    int widthPixels,
    int heightPixels)
{
    const int rightPixels = xPixels + widthPixels;
    const int bottomPixels = yPixels + heightPixels;

    // The rectangle is defined entirely in physical integer pixels. It is
    // converted to ImGui logical coordinates only once, immediately before
    // submitting the quad, which keeps the GUI aligned to a real pixel grid.
    ImGui::GetBackgroundDrawList()->AddImage(
        textureId(texture),
        ImVec2(
            mapping.logicalX(xPixels),
            mapping.logicalY(yPixels)
        ),
        ImVec2(
            mapping.logicalX(rightPixels),
            mapping.logicalY(bottomPixels)
        ),
        ImVec2(0.0f, 1.0f),
        ImVec2(1.0f, 0.0f)
    );
}

void InventoryUI::drawTextureRegion(
    const Texture2D& texture,
    const FramebufferMapping& mapping,
    int xPixels,
    int yPixels,
    int sourceWidthPixels,
    int sourceHeightPixels)
{
    drawTextureSubRegion(
        texture,
        mapping,
        xPixels,
        yPixels,
        0,
        0,
        sourceWidthPixels,
        sourceHeightPixels
    );
}

void InventoryUI::drawTextureSubRegion(
    const Texture2D& texture,
    const FramebufferMapping& mapping,
    int xPixels,
    int yPixels,
    int sourceX,
    int sourceY,
    int sourceWidthPixels,
    int sourceHeightPixels)
{
    const int widthPixels = sourceWidthPixels * UI_PIXEL_SCALE;
    const int heightPixels = sourceHeightPixels * UI_PIXEL_SCALE;
    const float minU = static_cast<float>(sourceX) /
        static_cast<float>(texture.getWidth());
    const float maxU = static_cast<float>(sourceX + sourceWidthPixels) /
        static_cast<float>(texture.getWidth());
    const float maxV = 1.0f - static_cast<float>(sourceY) /
        static_cast<float>(texture.getHeight());
    const float minV = 1.0f - static_cast<float>(sourceY + sourceHeightPixels) /
        static_cast<float>(texture.getHeight());

    ImGui::GetBackgroundDrawList()->AddImage(
        textureId(texture),
        ImVec2(mapping.logicalX(xPixels), mapping.logicalY(yPixels)),
        ImVec2(
            mapping.logicalX(xPixels + widthPixels),
            mapping.logicalY(yPixels + heightPixels)
        ),
        ImVec2(minU, maxV),
        ImVec2(maxU, minV)
    );
}

bool InventoryUI::rendersAs3DBlock(
    BlockType block) noexcept
{
    return block != BlockType::Air &&
           !isLiquid(block) &&
           !isPlant(block) &&
           !isLadder(block);
}

void InventoryUI::drawFlatIcon(
    BlockType block,
    const Texture2D& atlas,
    const FramebufferMapping& mapping,
    int xPixels,
    int yPixels)
{
    const AtlasUV uv = TextureAtlas::getBlockUV(block, BlockFace::Front);

    ImGui::GetBackgroundDrawList()->AddImage(
        textureId(atlas),
        ImVec2(
            mapping.logicalX(xPixels),
            mapping.logicalY(yPixels)
        ),
        ImVec2(
            mapping.logicalX(xPixels + ICON_SIZE_PIXELS),
            mapping.logicalY(yPixels + ICON_SIZE_PIXELS)
        ),
        uvTopLeft(uv),
        uvBottomRight(uv)
    );
}

void InventoryUI::draw3DIcon(
    BlockType block,
    const Texture2D& atlas,
    const FramebufferMapping& mapping,
    int xPixels,
    int yPixels)
{
    const AtlasUV topUv = TextureAtlas::getBlockUV(block, BlockFace::Top);
    const AtlasUV leftUv = TextureAtlas::getBlockUV(block, BlockFace::Left);
    const AtlasUV rightUv = TextureAtlas::getBlockUV(block, BlockFace::Front);

    const auto point =
        [&mapping, xPixels, yPixels](
            int sourceX,
            int sourceY)
        {
            const int physicalX =
                xPixels + sourceX * UI_PIXEL_SCALE;
            const int physicalY =
                yPixels + sourceY * UI_PIXEL_SCALE;

            return ImVec2(
                mapping.logicalX(physicalX),
                mapping.logicalY(physicalY)
            );
        };

    const ImVec2 topBack = point(8, 0);
    const ImVec2 topRight = point(15, 4);
    const ImVec2 topFront = point(8, 8);
    const ImVec2 topLeft = point(1, 4);
    const ImVec2 bottomFront = point(8, 16);
    const ImVec2 bottomLeft = point(1, 12);
    const ImVec2 bottomRight = point(15, 12);

    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    const ImTextureID atlasId = textureId(atlas);

    draw->AddImageQuad(
        atlasId,
        topBack,
        topRight,
        topFront,
        topLeft,
        uvTopLeft(topUv),
        uvTopRight(topUv),
        uvBottomRight(topUv),
        uvBottomLeft(topUv),
        IM_COL32(255, 255, 255, 255)
    );

    draw->AddImageQuad(
        atlasId,
        topLeft,
        topFront,
        bottomFront,
        bottomLeft,
        uvTopLeft(leftUv),
        uvTopRight(leftUv),
        uvBottomRight(leftUv),
        uvBottomLeft(leftUv),
        IM_COL32(153, 153, 153, 255)
    );

    draw->AddImageQuad(
        atlasId,
        topFront,
        topRight,
        bottomRight,
        bottomFront,
        uvTopLeft(rightUv),
        uvTopRight(rightUv),
        uvBottomRight(rightUv),
        uvBottomLeft(rightUv),
        IM_COL32(204, 204, 204, 255)
    );
}

void InventoryUI::drawItemIcon(
    ItemType item,
    const ItemAtlas& atlas,
    const FramebufferMapping& mapping,
    int xPixels,
    int yPixels)
{
    const AtlasUV uv = atlas.getItemUV(item);
    ImGui::GetBackgroundDrawList()->AddImage(
        textureId(atlas.texture()),
        ImVec2(mapping.logicalX(xPixels), mapping.logicalY(yPixels)),
        ImVec2(
            mapping.logicalX(xPixels + ICON_SIZE_PIXELS),
            mapping.logicalY(yPixels + ICON_SIZE_PIXELS)
        ),
        uvTopLeft(uv),
        uvBottomRight(uv)
    );
}

void InventoryUI::drawStackCount(
    int count,
    const FramebufferMapping& mapping,
    int iconX,
    int iconY)
{
    if (count <= 1)
        return;

    const std::string text = std::to_string(count);

    constexpr int glyphWidth = 3;
    constexpr int glyphHeight = 5;
    constexpr int glyphSpacing = 1;
    const int pixelSize = UI_PIXEL_SCALE;

    const int textWidth =
        static_cast<int>(text.size()) *
            glyphWidth * pixelSize +
        (static_cast<int>(text.size()) - 1) *
            glyphSpacing * pixelSize;
    const int textHeight = glyphHeight * pixelSize;

    const int textX =
        iconX + ICON_SIZE_PIXELS - textWidth;
    const int textY =
        iconY + ICON_SIZE_PIXELS - textHeight;

    ImDrawList* draw = ImGui::GetBackgroundDrawList();

    const auto drawPass =
        [&](int offsetX, int offsetY, ImU32 colour)
        {
            int cursorX = textX;

            for (const char character : text)
            {
                const int digit = character - '0';
                if (digit < 0 || digit > 9)
                    continue;

                for (int row = 0; row < glyphHeight; ++row)
                {
                    const unsigned char rowBits =
                        DIGITS[static_cast<std::size_t>(digit)]
                              [static_cast<std::size_t>(row)];

                    for (int column = 0;
                         column < glyphWidth;
                         ++column)
                    {
                        const int mask =
                            1 << (glyphWidth - 1 - column);

                        if ((rowBits & mask) == 0)
                            continue;

                        const int left =
                            cursorX +
                            column * pixelSize +
                            offsetX;
                        const int top =
                            textY +
                            row * pixelSize +
                            offsetY;

                        draw->AddRectFilled(
                            ImVec2(
                                mapping.logicalX(left),
                                mapping.logicalY(top)
                            ),
                            ImVec2(
                                mapping.logicalX(left + pixelSize),
                                mapping.logicalY(top + pixelSize)
                            ),
                            colour
                        );
                    }
                }

                cursorX +=
                    (glyphWidth + glyphSpacing) * pixelSize;
            }
        };

    drawPass(
        UI_PIXEL_SCALE,
        UI_PIXEL_SCALE,
        IM_COL32(63, 63, 63, 255)
    );
    drawPass(
        0,
        0,
        IM_COL32(255, 255, 255, 255)
    );
}

void InventoryUI::drawDurabilityBar(
    const ItemStack& stack,
    const FramebufferMapping& mapping,
    int iconX,
    int iconY)
{
    const int maximumDamage = getItemProperties(stack.item).maximumDamage;
    if (maximumDamage <= 0 || stack.damage == 0)
        return;

    // RenderItem.renderItemOverlayIntoGUI from Beta 1.7.3.
    const int remainingPixels = static_cast<int>(std::lround(
        13.0 - static_cast<double>(stack.damage) * 13.0 /
                   static_cast<double>(maximumDamage)
    ));
    const int green = std::clamp(static_cast<int>(std::lround(
        255.0 - static_cast<double>(stack.damage) * 255.0 /
                    static_cast<double>(maximumDamage)
    )), 0, 255);
    const int red = 255 - green;
    const int scale = UI_PIXEL_SCALE;
    const int x = iconX + 2 * scale;
    const int y = iconY + 13 * scale;
    ImDrawList* draw = ImGui::GetBackgroundDrawList();

    const auto rectangle =
        [&mapping, draw](int left, int top, int width, int height, ImU32 colour)
        {
            draw->AddRectFilled(
                ImVec2(mapping.logicalX(left), mapping.logicalY(top)),
                ImVec2(
                    mapping.logicalX(left + width),
                    mapping.logicalY(top + height)
                ),
                colour
            );
        };

    rectangle(x, y, 13 * scale, 2 * scale, IM_COL32(0, 0, 0, 255));
    rectangle(
        x, y, 12 * scale, scale,
        IM_COL32(red / 4, 63, 0, 255)
    );
    rectangle(
        x, y, std::max(0, remainingPixels) * scale, scale,
        IM_COL32(red, green, 0, 255)
    );
}

void InventoryUI::drawStack(
    const ItemStack& stack,
    const Texture2D& blockAtlas,
    const ItemAtlas& itemAtlas,
    const FramebufferMapping& mapping,
    int xPixels,
    int yPixels)
{
    if (stack.empty())
        return;

    if (isBlockItem(stack.item))
    {
        const BlockType block = blockFromItem(stack.item);
        if (rendersAs3DBlock(block))
        {
            draw3DIcon(
                block, blockAtlas, mapping, xPixels, yPixels
            );
        }
        else
        {
            drawFlatIcon(
                block, blockAtlas, mapping, xPixels, yPixels
            );
        }
    }
    else
    {
        drawItemIcon(stack.item, itemAtlas, mapping, xPixels, yPixels);
    }

    drawStackCount(
        stack.count,
        mapping,
        xPixels,
        yPixels
    );
    drawDurabilityBar(stack, mapping, xPixels, yPixels);
}

void InventoryUI::drawHotbar(
    const Inventory& inventory,
    const Texture2D& blockAtlas,
    const ItemAtlas& itemAtlas,
    const FramebufferMapping& mapping)
{
    constexpr int sourceWidth = 182;
    constexpr int sourceHeight = 22;
    constexpr int bottomMarginSourcePixels = 22;

    const int widthPixels = sourceWidth * UI_PIXEL_SCALE;
    const int heightPixels = sourceHeight * UI_PIXEL_SCALE;

    // Keep the hotbar origin in framebuffer pixels so the texture placement and
    // the selected-slot highlight stay aligned with the same pixel grid.
    const int originX =
        (mapping.widthPixels - widthPixels) / 2;
    const int originY =
        mapping.heightPixels -
        bottomMarginSourcePixels * UI_PIXEL_SCALE;

    drawTexture(
        *hotbarTexture_,
        mapping,
        originX,
        originY,
        widthPixels,
        heightPixels
    );

    const int selected =
        inventory.getSelectedHotbarSlot();

    drawTexture(
        *selectedSlotTexture_,
        mapping,
        originX +
            (selected * 20 - 1) * UI_PIXEL_SCALE,
        originY - UI_PIXEL_SCALE,
        24 * UI_PIXEL_SCALE,
        24 * UI_PIXEL_SCALE
    );

    for (int slot = 0;
         slot < Inventory::HOTBAR_SIZE;
         ++slot)
    {
        drawStack(
            inventory.getSlot(slot),
            blockAtlas,
            itemAtlas,
            mapping,
            originX +
                (3 + slot * 20) * UI_PIXEL_SCALE,
            originY + 3 * UI_PIXEL_SCALE
        );
    }
}

void InventoryUI::drawContainer(
    Inventory& inventory,
    const Texture2D& blockAtlas,
    const ItemAtlas& itemAtlas,
    const FramebufferMapping& mapping,
    bool craftingTable)
{
    constexpr int sourceWidth = 176;
    constexpr int sourceHeight = 166;
    const int widthPixels = sourceWidth * UI_PIXEL_SCALE;
    const int heightPixels = sourceHeight * UI_PIXEL_SCALE;
    const int originX = (mapping.widthPixels - widthPixels) / 2;
    const int originY = (mapping.heightPixels - heightPixels) / 2;

    if (craftingTable)
    {
        drawTextureRegion(
            *craftingTableTexture_,
            mapping,
            originX,
            originY,
            sourceWidth,
            sourceHeight
        );
    }
    else
    {
        drawTextureRegion(
            *inventoryTexture_,
            mapping,
            originX,
            originY,
            sourceWidth,
            sourceHeight
        );
    }

    const ImVec2 logicalMouse = ImGui::GetIO().MousePos;
    const int mouseX = mapping.physicalMouseX(logicalMouse.x);
    const int mouseY = mapping.physicalMouseY(logicalMouse.y);

    ItemStack* hoveredStack = nullptr;
    int hoveredX = 0;
    int hoveredY = 0;
    bool hoveringResult = false;

    const auto drawInteractiveStack =
        [&](ItemStack& stack, int slotX, int slotY)
        {
            if (mouseX >= slotX &&
                mouseX < slotX + ICON_SIZE_PIXELS &&
                mouseY >= slotY &&
                mouseY < slotY + ICON_SIZE_PIXELS)
            {
                hoveredStack = &stack;
                hoveredX = slotX;
                hoveredY = slotY;
            }

            drawStack(
                stack, blockAtlas, itemAtlas, mapping, slotX, slotY
            );
        };

    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 9; ++column)
        {
            const int slot = 9 + row * 9 + column;
            drawInteractiveStack(
                inventory.getSlot(slot),
                originX + (8 + column * 18) * UI_PIXEL_SCALE,
                originY + (84 + row * 18) * UI_PIXEL_SCALE
            );
        }
    }

    for (int column = 0; column < 9; ++column)
    {
        drawInteractiveStack(
            inventory.getSlot(column),
            originX + (8 + column * 18) * UI_PIXEL_SCALE,
            originY + 142 * UI_PIXEL_SCALE
        );
    }

    if (!craftingTable)
    {
        for (int armor = 0; armor < Inventory::ARMOR_SLOT_COUNT; ++armor)
        {
            drawInteractiveStack(
                inventory.getSlot(Inventory::ARMOR_SLOT_START + armor),
                originX + 8 * UI_PIXEL_SCALE,
                originY + (8 + armor * 18) * UI_PIXEL_SCALE
            );
        }
        drawInteractiveStack(
            inventory.getSlot(Inventory::OFFHAND_SLOT),
            originX + 77 * UI_PIXEL_SCALE,
            originY + 62 * UI_PIXEL_SCALE
        );
    }

    const CraftingRecipe* matchedRecipe = nullptr;
    CraftingResult craftingResult;

    const int craftingGridSize = craftingTable ? 3 : 2;
    const int craftingGridX = craftingTable ? 30 : 98;
    const int craftingGridY = craftingTable ? 17 : 18;
    const int craftingResultX = craftingTable ? 124 : 154;
    const int craftingResultY = craftingTable ? 35 : 28;

    for (int row = 0; row < craftingGridSize; ++row)
    {
        for (int column = 0; column < craftingGridSize; ++column)
        {
            drawInteractiveStack(
                craftingGrid_[static_cast<std::size_t>(
                    row * 3 + column
                )],
                originX +
                    (craftingGridX + column * 18) * UI_PIXEL_SCALE,
                originY +
                    (craftingGridY + row * 18) * UI_PIXEL_SCALE
            );
        }
    }

    matchedRecipe = recipes_.findMatch(craftingGrid_);
    if (matchedRecipe != nullptr)
    {
        craftingResult = RecipeRegistry::getResult(*matchedRecipe);
        const ItemStack resultStack = craftingResult;
        const int resultX =
            originX + craftingResultX * UI_PIXEL_SCALE;
        const int resultY =
            originY + craftingResultY * UI_PIXEL_SCALE;

        drawStack(
            resultStack,
            blockAtlas,
            itemAtlas,
            mapping,
            resultX,
            resultY
        );

        if (mouseX >= resultX &&
            mouseX < resultX + ICON_SIZE_PIXELS &&
            mouseY >= resultY &&
            mouseY < resultY + ICON_SIZE_PIXELS)
        {
            hoveringResult = true;
            hoveredStack = nullptr;
            hoveredX = resultX;
            hoveredY = resultY;
        }
    }

    if (hoveredStack != nullptr || hoveringResult)
    {
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2(
                mapping.logicalX(hoveredX),
                mapping.logicalY(hoveredY)
            ),
            ImVec2(
                mapping.logicalX(hoveredX + ICON_SIZE_PIXELS),
                mapping.logicalY(hoveredY + ICON_SIZE_PIXELS)
            ),
            IM_COL32(255, 255, 255, 96)
        );

        const bool leftClicked =
            ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        const bool rightClicked =
            ImGui::IsMouseClicked(ImGuiMouseButton_Right);

        if (leftClicked || rightClicked)
        {
            if (hoveringResult && matchedRecipe != nullptr &&
                canTakeCraftingResult(craftingResult))
            {
                takeCraftingResult(*matchedRecipe);
            }
            else if (hoveredStack != nullptr)
            {
                if (leftClicked)
                    handleLeftClick(*hoveredStack);
                else
                    handleRightClick(*hoveredStack);
            }
        }
    }

    drawStack(
        cursorStack_,
        blockAtlas,
        itemAtlas,
        mapping,
        mouseX - ICON_SIZE_PIXELS / 2,
        mouseY - ICON_SIZE_PIXELS / 2
    );
}

void InventoryUI::drawFurnace(
    Inventory& inventory,
    const Texture2D& blockAtlas,
    const ItemAtlas& itemAtlas,
    const FramebufferMapping& mapping)
{
    if (furnace_ == nullptr)
        return;

    constexpr int sourceWidth = 176;
    constexpr int sourceHeight = 166;
    const int widthPixels = sourceWidth * UI_PIXEL_SCALE;
    const int heightPixels = sourceHeight * UI_PIXEL_SCALE;
    const int originX = (mapping.widthPixels - widthPixels) / 2;
    const int originY = (mapping.heightPixels - heightPixels) / 2;

    drawTextureRegion(
        *furnaceTexture_,
        mapping,
        originX,
        originY,
        sourceWidth,
        sourceHeight
    );

    if (furnace_->isBurning())
    {
        const int flameHeight = furnace_->getBurnTimeScaled(12);
        drawTextureSubRegion(
            *furnaceTexture_,
            mapping,
            originX + 56 * UI_PIXEL_SCALE,
            originY + (36 + 12 - flameHeight) * UI_PIXEL_SCALE,
            176,
            12 - flameHeight,
            14,
            flameHeight + 2
        );
    }

    const int progress = furnace_->getCookProgressScaled(24);
    drawTextureSubRegion(
        *furnaceTexture_,
        mapping,
        originX + 79 * UI_PIXEL_SCALE,
        originY + 34 * UI_PIXEL_SCALE,
        176,
        14,
        progress + 1,
        16
    );

    const ImVec2 logicalMouse = ImGui::GetIO().MousePos;
    const int mouseX = mapping.physicalMouseX(logicalMouse.x);
    const int mouseY = mapping.physicalMouseY(logicalMouse.y);
    ItemStack* hoveredStack = nullptr;
    int hoveredX = 0;
    int hoveredY = 0;
    bool hoveringOutput = false;

    const auto drawInteractiveStack =
        [&](ItemStack& stack, int slotX, int slotY, bool output)
        {
            if (mouseX >= slotX &&
                mouseX < slotX + ICON_SIZE_PIXELS &&
                mouseY >= slotY &&
                mouseY < slotY + ICON_SIZE_PIXELS)
            {
                hoveredStack = output ? nullptr : &stack;
                hoveringOutput = output;
                hoveredX = slotX;
                hoveredY = slotY;
            }
            drawStack(
                stack, blockAtlas, itemAtlas, mapping, slotX, slotY
            );
        };

    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 9; ++column)
        {
            const int slot = 9 + row * 9 + column;
            drawInteractiveStack(
                inventory.getSlot(slot),
                originX + (8 + column * 18) * UI_PIXEL_SCALE,
                originY + (84 + row * 18) * UI_PIXEL_SCALE,
                false
            );
        }
    }
    for (int column = 0; column < 9; ++column)
    {
        drawInteractiveStack(
            inventory.getSlot(column),
            originX + (8 + column * 18) * UI_PIXEL_SCALE,
            originY + 142 * UI_PIXEL_SCALE,
            false
        );
    }

    drawInteractiveStack(
        furnace_->getSlot(FurnaceBlockEntity::Input),
        originX + 56 * UI_PIXEL_SCALE,
        originY + 17 * UI_PIXEL_SCALE,
        false
    );
    drawInteractiveStack(
        furnace_->getSlot(FurnaceBlockEntity::Fuel),
        originX + 56 * UI_PIXEL_SCALE,
        originY + 53 * UI_PIXEL_SCALE,
        false
    );
    drawInteractiveStack(
        furnace_->getSlot(FurnaceBlockEntity::Output),
        originX + 116 * UI_PIXEL_SCALE,
        originY + 35 * UI_PIXEL_SCALE,
        true
    );

    if (hoveredStack != nullptr || hoveringOutput)
    {
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2(mapping.logicalX(hoveredX), mapping.logicalY(hoveredY)),
            ImVec2(
                mapping.logicalX(hoveredX + ICON_SIZE_PIXELS),
                mapping.logicalY(hoveredY + ICON_SIZE_PIXELS)
            ),
            IM_COL32(255, 255, 255, 96)
        );

        const bool leftClicked =
            ImGui::IsMouseClicked(ImGuiMouseButton_Left);
        const bool rightClicked =
            ImGui::IsMouseClicked(ImGuiMouseButton_Right);
        if (hoveringOutput && (leftClicked || rightClicked))
            takeFurnaceOutput(rightClicked);
        else if (hoveredStack != nullptr)
        {
            if (leftClicked)
                handleLeftClick(*hoveredStack);
            else if (rightClicked)
                handleRightClick(*hoveredStack);
        }
    }

    drawStack(
        cursorStack_,
        blockAtlas,
        itemAtlas,
        mapping,
        mouseX - ICON_SIZE_PIXELS / 2,
        mouseY - ICON_SIZE_PIXELS / 2
    );
}

void InventoryUI::drawChest(
    Inventory& inventory,
    const Texture2D& blockAtlas,
    const ItemAtlas& itemAtlas,
    const FramebufferMapping& mapping)
{
    if (chestFirst_ == nullptr)
        return;

    const int rows = chestSecond_ == nullptr ? 3 : 6;
    const int sourceWidth = 176;
    const int sourceHeight = 114 + rows * 18;
    const int widthPixels = sourceWidth * UI_PIXEL_SCALE;
    const int heightPixels = sourceHeight * UI_PIXEL_SCALE;
    const int originX = (mapping.widthPixels - widthPixels) / 2;
    const int originY = (mapping.heightPixels - heightPixels) / 2;

    // GuiChest in Beta builds both chest sizes from container.png: the chest
    // section grows by 18 pixels per row and the player inventory section is
    // copied immediately below it. generic_54.png uses the same 256x256 layout.
    drawTextureSubRegion(
        *chestTexture_, mapping, originX, originY,
        0, 0, sourceWidth, rows * 18 + 17
    );
    drawTextureSubRegion(
        *chestTexture_, mapping,
        originX, originY + (rows * 18 + 17) * UI_PIXEL_SCALE,
        0, 126, sourceWidth, 96
    );

    const ImVec2 logicalMouse = ImGui::GetIO().MousePos;
    const int mouseX = mapping.physicalMouseX(logicalMouse.x);
    const int mouseY = mapping.physicalMouseY(logicalMouse.y);
    ItemStack* hoveredStack = nullptr;
    int hoveredX = 0;
    int hoveredY = 0;

    const auto drawInteractiveStack =
        [&](ItemStack& stack, int slotX, int slotY)
        {
            if (mouseX >= slotX && mouseX < slotX + ICON_SIZE_PIXELS &&
                mouseY >= slotY && mouseY < slotY + ICON_SIZE_PIXELS)
            {
                hoveredStack = &stack;
                hoveredX = slotX;
                hoveredY = slotY;
            }
            drawStack(stack, blockAtlas, itemAtlas, mapping, slotX, slotY);
        };

    for (int row = 0; row < rows; ++row)
    {
        ChestBlockEntity* chest = row < 3 ? chestFirst_ : chestSecond_;
        const int localRow = row % 3;
        for (int column = 0; column < 9; ++column)
        {
            drawInteractiveStack(
                chest->getSlot(static_cast<std::size_t>(localRow * 9 + column)),
                originX + (8 + column * 18) * UI_PIXEL_SCALE,
                originY + (18 + row * 18) * UI_PIXEL_SCALE
            );
        }
    }

    const int playerMainY = 103 + (rows - 4) * 18;
    for (int row = 0; row < 3; ++row)
    {
        for (int column = 0; column < 9; ++column)
        {
            drawInteractiveStack(
                inventory.getSlot(9 + row * 9 + column),
                originX + (8 + column * 18) * UI_PIXEL_SCALE,
                originY + (playerMainY + row * 18) * UI_PIXEL_SCALE
            );
        }
    }
    const int hotbarY = 161 + (rows - 4) * 18;
    for (int column = 0; column < 9; ++column)
    {
        drawInteractiveStack(
            inventory.getSlot(column),
            originX + (8 + column * 18) * UI_PIXEL_SCALE,
            originY + hotbarY * UI_PIXEL_SCALE
        );
    }

    if (hoveredStack != nullptr)
    {
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            ImVec2(mapping.logicalX(hoveredX), mapping.logicalY(hoveredY)),
            ImVec2(
                mapping.logicalX(hoveredX + ICON_SIZE_PIXELS),
                mapping.logicalY(hoveredY + ICON_SIZE_PIXELS)
            ),
            IM_COL32(255, 255, 255, 96)
        );
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            handleLeftClick(*hoveredStack);
        else if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            handleRightClick(*hoveredStack);
    }

    drawStack(
        cursorStack_, blockAtlas, itemAtlas, mapping,
        mouseX - ICON_SIZE_PIXELS / 2,
        mouseY - ICON_SIZE_PIXELS / 2
    );
}

void InventoryUI::returnCraftingItems(Inventory& inventory)
{
    for (ItemStack& stack : craftingGrid_)
        inventory.addStack(stack);
}

void InventoryUI::handleLeftClick(ItemStack& slot)
{
    if (cursorStack_.empty() || slot.empty() ||
        !cursorStack_.canStackWith(slot))
    {
        std::swap(slot, cursorStack_);
        return;
    }

    const int room = slot.maximumStackSize() - slot.count;
    const int moved = std::min<int>(room, cursorStack_.count);
    slot.count = static_cast<std::uint8_t>(slot.count + moved);
    cursorStack_.count = static_cast<std::uint8_t>(
        cursorStack_.count - moved
    );

    if (cursorStack_.count == 0)
        cursorStack_.clear();
}

void InventoryUI::handleRightClick(ItemStack& slot)
{
    if (cursorStack_.empty())
    {
        if (slot.empty())
            return;

        const int taken = (static_cast<int>(slot.count) + 1) / 2;
        cursorStack_ = {
            slot.item,
            static_cast<std::uint8_t>(taken),
            slot.damage
        };
        slot.count = static_cast<std::uint8_t>(slot.count - taken);

        if (slot.count == 0)
            slot.clear();
        return;
    }

    if (slot.empty())
    {
        slot = {cursorStack_.item, 1, cursorStack_.damage};
        --cursorStack_.count;
        if (cursorStack_.count == 0)
            cursorStack_.clear();
        return;
    }

    if (slot.canStackWith(cursorStack_))
    {
        if (slot.count >= slot.maximumStackSize())
            return;

        ++slot.count;
        --cursorStack_.count;
        if (cursorStack_.count == 0)
            cursorStack_.clear();
        return;
    }

    std::swap(slot, cursorStack_);
}

void InventoryUI::takeFurnaceOutput(bool rightClick)
{
    if (furnace_ == nullptr)
        return;

    ItemStack& output = furnace_->getSlot(FurnaceBlockEntity::Output);
    if (output.empty())
        return;
    if (!cursorStack_.empty() && !cursorStack_.canStackWith(output))
        return;

    const int room = cursorStack_.empty()
        ? output.maximumStackSize()
        : cursorStack_.maximumStackSize() - cursorStack_.count;
    if (room <= 0)
        return;

    const int requested = rightClick
        ? (static_cast<int>(output.count) + 1) / 2
        : static_cast<int>(output.count);
    const int moved = std::min(room, requested);
    if (cursorStack_.empty())
    {
        cursorStack_ = {
            output.item,
            static_cast<std::uint8_t>(moved),
            output.damage
        };
    }
    else
    {
        cursorStack_.count = static_cast<std::uint8_t>(
            cursorStack_.count + moved
        );
    }

    output.count = static_cast<std::uint8_t>(output.count - moved);
    if (output.count == 0)
        output.clear();
}

bool InventoryUI::canTakeCraftingResult(
    const CraftingResult& result) const noexcept
{
    if (result.empty())
        return false;
    if (cursorStack_.empty())
        return true;

    return cursorStack_.canStackWith(result) &&
           static_cast<int>(cursorStack_.count) + result.count <=
               cursorStack_.maximumStackSize();
}

void InventoryUI::takeCraftingResult(
    const CraftingRecipe& recipe)
{
    const CraftingResult& result = RecipeRegistry::getResult(recipe);

    if (cursorStack_.empty())
    {
        cursorStack_ = result;
    }
    else
    {
        cursorStack_.count = static_cast<std::uint8_t>(
            cursorStack_.count + result.count
        );
    }

    for (ItemStack& stack : craftingGrid_)
    {
        if (stack.empty())
            continue;

        --stack.count;
        if (stack.count == 0)
            stack.clear();
    }
}

void InventoryUI::draw(
    Inventory& inventory,
    const Texture2D& blockAtlas,
    const ItemAtlas& itemAtlas,
    int framebufferWidth,
    int framebufferHeight)
{
    const FramebufferMapping mapping =
        makeMapping(
            framebufferWidth,
            framebufferHeight
        );
        
    // Some graphics drivers leave a linear sampler bound for ImGui. Explicitly
    // remove it before the pixel-art draw commands so GL_NEAREST on each game
    // texture remains authoritative.
    ImGui::GetBackgroundDrawList()->AddCallback(
        [](const ImDrawList*, const ImDrawCmd*) { glBindSampler(0, 0); },
        nullptr
    );

    drawHotbar(
        inventory,
        blockAtlas,
        itemAtlas,
        mapping
    );

    if (!isOpen())
        return;

    if (isFurnaceOpen())
        drawFurnace(inventory, blockAtlas, itemAtlas, mapping);
    else if (isChestOpen())
        drawChest(inventory, blockAtlas, itemAtlas, mapping);
    else
    {
        drawContainer(
            inventory,
            blockAtlas,
            itemAtlas,
            mapping,
            isCraftingTableOpen()
        );
    }
}
