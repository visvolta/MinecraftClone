#include "DeathScreen.h"

#include "AssetPaths.h"
#include "Texture2D.h"

#include <algorithm>
#include <cmath>

#include <glad/gl.h>
#include <imgui.h>

namespace
{
ImTextureID textureId(const Texture2D& texture)
{
    return static_cast<ImTextureID>(texture.getId());
}
}

DeathScreen::DeathScreen()
    : widgets_(std::make_unique<Texture2D>(
          AssetPaths::get("gui/widgets.png"),
          256,
          256))
{
}

float DeathScreen::FramebufferMapping::logicalX(
    int physicalX) const noexcept
{
    return std::round(
        static_cast<float>(physicalX) / logicalToPhysicalX
    );
}

float DeathScreen::FramebufferMapping::logicalY(
    int physicalY) const noexcept
{
    return std::round(
        static_cast<float>(physicalY) / logicalToPhysicalY
    );
}

int DeathScreen::FramebufferMapping::physicalMouseX(
    float logicalXValue) const noexcept
{
    return static_cast<int>(
        std::lround(logicalXValue * logicalToPhysicalX)
    );
}

int DeathScreen::FramebufferMapping::physicalMouseY(
    float logicalYValue) const noexcept
{
    return static_cast<int>(
        std::lround(logicalYValue * logicalToPhysicalY)
    );
}

DeathScreen::FramebufferMapping DeathScreen::makeMapping(
    int framebufferWidth,
    int framebufferHeight) noexcept
{
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    FramebufferMapping mapping;
    mapping.widthPixels = std::max(framebufferWidth, 1);
    mapping.heightPixels = std::max(framebufferHeight, 1);
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

DeathScreenAction DeathScreen::draw(
    int framebufferWidth,
    int framebufferHeight) const
{
    const FramebufferMapping mapping = makeMapping(
        framebufferWidth,
        framebufferHeight
    );
    ImDrawList* draw = ImGui::GetForegroundDrawList();

    draw->AddCallback(
        [](const ImDrawList*, const ImDrawCmd*) { glBindSampler(0, 0); },
        nullptr
    );

    draw->AddRectFilledMultiColor(
        ImVec2(mapping.logicalX(0), mapping.logicalY(0)),
        ImVec2(
            mapping.logicalX(mapping.widthPixels),
            mapping.logicalY(mapping.heightPixels)
        ),
        IM_COL32(80, 0, 0, 96),
        IM_COL32(80, 0, 0, 96),
        IM_COL32(128, 48, 48, 160),
        IM_COL32(128, 48, 48, 160)
    );

    drawCenteredText(
        mapping,
        "Game over!",
        mapping.widthPixels / 2,
        30 * UI_PIXEL_SCALE,
        16 * UI_PIXEL_SCALE,
        IM_COL32(255, 255, 255, 255)
    );
    drawCenteredText(
        mapping,
        "Score: 0",
        mapping.widthPixels / 2,
        100 * UI_PIXEL_SCALE,
        8 * UI_PIXEL_SCALE,
        IM_COL32(255, 255, 255, 255)
    );

    const int buttonX =
        mapping.widthPixels / 2 - 100 * UI_PIXEL_SCALE;
    const int firstButtonY =
        mapping.heightPixels / 4 + 72 * UI_PIXEL_SCALE;

    if (drawButton(mapping, buttonX, firstButtonY, "Respawn"))
        return DeathScreenAction::Respawn;

    if (drawButton(
            mapping,
            buttonX,
            mapping.heightPixels / 4 + 96 * UI_PIXEL_SCALE,
            "Title menu"))
    {
        return DeathScreenAction::TitleMenu;
    }

    return DeathScreenAction::None;
}

void DeathScreen::drawRegion(
    const FramebufferMapping& mapping,
    int destinationX,
    int destinationY,
    int sourceX,
    int sourceY,
    int sourceWidth,
    int sourceHeight) const
{
    const float textureWidth = static_cast<float>(widgets_->getWidth());
    const float textureHeight = static_cast<float>(widgets_->getHeight());
    const float minU = static_cast<float>(sourceX) / textureWidth;
    const float maxU = static_cast<float>(sourceX + sourceWidth) / textureWidth;
    const float maxV = 1.0f - static_cast<float>(sourceY) / textureHeight;
    const float minV =
        1.0f - static_cast<float>(sourceY + sourceHeight) / textureHeight;

    ImGui::GetForegroundDrawList()->AddImage(
        textureId(*widgets_),
        ImVec2(
            mapping.logicalX(destinationX),
            mapping.logicalY(destinationY)
        ),
        ImVec2(
            mapping.logicalX(destinationX + sourceWidth * UI_PIXEL_SCALE),
            mapping.logicalY(destinationY + sourceHeight * UI_PIXEL_SCALE)
        ),
        ImVec2(minU, maxV),
        ImVec2(maxU, minV)
    );
}

bool DeathScreen::drawButton(
    const FramebufferMapping& mapping,
    int x,
    int y,
    std::string_view label) const
{
    constexpr int sourceWidth = 200;
    constexpr int sourceHeight = 20;
    const int width = sourceWidth * UI_PIXEL_SCALE;
    const int height = sourceHeight * UI_PIXEL_SCALE;

    const ImVec2 logicalMouse = ImGui::GetIO().MousePos;
    const int mouseX = mapping.physicalMouseX(logicalMouse.x);
    const int mouseY = mapping.physicalMouseY(logicalMouse.y);
    const bool hovered =
        mouseX >= x && mouseX < x + width &&
        mouseY >= y && mouseY < y + height;

    // GuiButton selects y=66 for the normal grey state and y=86 for the
    // highlighted blue state, drawing the texture as two stretchable halves.
    const int sourceY = hovered ? 86 : 66;
    drawRegion(mapping, x, y, 0, sourceY, 100, sourceHeight);
    drawRegion(
        mapping,
        x + 100 * UI_PIXEL_SCALE,
        y,
        100,
        sourceY,
        100,
        sourceHeight
    );

    drawCenteredText(
        mapping,
        label,
        x + width / 2,
        y + 6 * UI_PIXEL_SCALE,
        8 * UI_PIXEL_SCALE,
        hovered
            ? IM_COL32(255, 255, 160, 255)
            : IM_COL32(224, 224, 224, 255)
    );

    return hovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
}

void DeathScreen::drawCenteredText(
    const FramebufferMapping& mapping,
    std::string_view text,
    int centerX,
    int topY,
    int physicalFontSize,
    unsigned int colour)
{
    ImFont* font = ImGui::GetFont();
    const float logicalFontSize =
        static_cast<float>(physicalFontSize) /
        mapping.logicalToPhysicalY;
    const ImVec2 textSize = font->CalcTextSizeA(
        logicalFontSize,
        10000.0f,
        0.0f,
        text.data(),
        text.data() + text.size()
    );

    ImGui::GetForegroundDrawList()->AddText(
        font,
        logicalFontSize,
        ImVec2(
            mapping.logicalX(centerX) - textSize.x * 0.5f,
            mapping.logicalY(topY)
        ),
        colour,
        text.data(),
        text.data() + text.size()
    );
}
