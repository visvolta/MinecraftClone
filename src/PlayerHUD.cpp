#include "PlayerHUD.h"

#include "AssetPaths.h"
#include "Camera.h"
#include "Player.h"
#include "Texture2D.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <string>

#include <glad/gl.h>
#include <imgui.h>

namespace
{
ImTextureID textureId(const Texture2D& texture)
{
    return static_cast<ImTextureID>(texture.getId());
}

void beginInvertedCrosshairBlend(const ImDrawList*, const ImDrawCmd*)
{
    // GuiIngame uses GL_ONE_MINUS_DST_COLOR / GL_ONE_MINUS_SRC_COLOR so the
    // crosshair remains visible against both bright sky and dark caves.
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
}
}

PlayerHUD::PlayerHUD()
    : icons_(std::make_unique<Texture2D>(
          AssetPaths::get("gui/icons.png"),
          256,
          256)),
      waterOverlay_(std::make_unique<Texture2D>(
          AssetPaths::get("textures/water_overlay.png"),
          16,
          16))
{
    waterOverlay_->setRepeatWrapping(true);
}

float PlayerHUD::FramebufferMapping::logicalX(
    int physicalX) const noexcept
{
    return std::round(
        static_cast<float>(physicalX) / logicalToPhysicalX
    );
}

float PlayerHUD::FramebufferMapping::logicalY(
    int physicalY) const noexcept
{
    return std::round(
        static_cast<float>(physicalY) / logicalToPhysicalY
    );
}

PlayerHUD::FramebufferMapping PlayerHUD::makeMapping(
    int framebufferWidth,
    int framebufferHeight) noexcept
{
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    FramebufferMapping mapping;
    mapping.widthPixels = std::max(framebufferWidth, 1);
    mapping.heightPixels = std::max(framebufferHeight, 1);
    UI_PIXEL_SCALE = 1;
    while (UI_PIXEL_SCALE < 1000 &&
           mapping.widthPixels / (UI_PIXEL_SCALE + 1) >= 320 &&
           mapping.heightPixels / (UI_PIXEL_SCALE + 1) >= 240)
    {
        ++UI_PIXEL_SCALE;
    }
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

void PlayerHUD::draw(
    const Player& player,
    const Camera& camera,
    int framebufferWidth,
    int framebufferHeight,
    bool showCrosshair) const
{
    const FramebufferMapping mapping = makeMapping(
        framebufferWidth,
        framebufferHeight
    );

    ImGui::GetBackgroundDrawList()->AddCallback(
        [](const ImDrawList*, const ImDrawCmd*) { glBindSampler(0, 0); },
        nullptr
    );

    if (player.isHeadUnderwater())
        drawUnderwaterOverlay(camera, mapping);

    drawHealth(player, mapping);
    drawFood(player, mapping);
    drawArmor(player, mapping);
    drawExperience(player, mapping);

    if (player.isHeadUnderwater())
        drawAir(player, mapping);

    if (showCrosshair)
        drawCrosshair(mapping);
}

void PlayerHUD::drawFood(
    const Player& player,
    const FramebufferMapping& mapping) const
{
    const int food = player.survival().foodLevel();
    const int originX = mapping.widthPixels / 2 + 91 * UI_PIXEL_SCALE;
    const int baseY = mapping.heightPixels - 39 * UI_PIXEL_SCALE;
    std::minstd_rand random(
        static_cast<std::uint32_t>(player.getTicksExisted()) * 312871U + 7U
    );

    for (int icon = 0; icon < 10; ++icon)
    {
        const int x = originX - (icon + 1) * 8 * UI_PIXEL_SCALE - UI_PIXEL_SCALE;
        int y = baseY;
        if (player.survival().saturation() <= 0.0f &&
            player.getTicksExisted() % (food * 3 + 1) == 0)
        {
            y += (static_cast<int>(random() % 3U) - 1) * UI_PIXEL_SCALE;
        }

        drawRegion(mapping, x, y, 16, 27, 9, 9);
        if (icon * 2 + 1 < food)
            drawRegion(mapping, x, y, 52, 27, 9, 9);
        else if (icon * 2 + 1 == food)
            drawRegion(mapping, x, y, 61, 27, 9, 9);
    }
}

void PlayerHUD::drawArmor(
    const Player& player,
    const FramebufferMapping& mapping) const
{
    const int armor = player.survival().armorPoints();
    if (armor <= 0)
        return;

    const int originX = mapping.widthPixels / 2 - 91 * UI_PIXEL_SCALE;
    const int y = mapping.heightPixels - 49 * UI_PIXEL_SCALE;
    for (int icon = 0; icon < 10; ++icon)
    {
        const int x = originX + icon * 8 * UI_PIXEL_SCALE;
        drawRegion(mapping, x, y, 16, 9, 9, 9);
        if (icon * 2 + 1 < armor)
            drawRegion(mapping, x, y, 34, 9, 9, 9);
        else if (icon * 2 + 1 == armor)
            drawRegion(mapping, x, y, 25, 9, 9, 9);
    }
}

void PlayerHUD::drawExperience(
    const Player& player,
    const FramebufferMapping& mapping) const
{
    const float progress = player.survival().experienceProgress();
    const int level = player.survival().experienceLevel();
    const int width = 182;
    const int x = mapping.widthPixels / 2 - 91 * UI_PIXEL_SCALE;
    const int y = mapping.heightPixels - 29 * UI_PIXEL_SCALE;
    drawRegion(mapping, x, y, 0, 64, width, 5);
    const int filled = std::clamp(
        static_cast<int>(std::floor(progress * static_cast<float>(width))),
        0,
        width
    );
    if (filled > 0)
        drawRegion(mapping, x, y, 0, 69, filled, 5);

    if (level > 0)
    {
        const std::string text = std::to_string(level);
        const ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
        ImGui::GetBackgroundDrawList()->AddText(
            ImVec2(
                mapping.logicalX(mapping.widthPixels / 2) - textSize.x * 0.5f,
                mapping.logicalY(y - 8 * UI_PIXEL_SCALE)
            ),
            IM_COL32(128, 255, 32, 255),
            text.c_str()
        );
    }
}

void PlayerHUD::drawUnderwaterOverlay(
    const Camera& camera,
    const FramebufferMapping& mapping) const
{
    // ItemRenderer.renderWarpedTextureOverlay repeats misc/water.png four
    // times and offsets it by view yaw/pitch so it moves with the camera.
    const float yawOffset = -camera.getYaw() / 64.0f;
    const float pitchOffset = camera.getPitch() / 64.0f;

    ImGui::GetBackgroundDrawList()->AddImage(
        textureId(*waterOverlay_),
        ImVec2(mapping.logicalX(0), mapping.logicalY(0)),
        ImVec2(
            mapping.logicalX(mapping.widthPixels),
            mapping.logicalY(mapping.heightPixels)
        ),
        ImVec2(4.0f + yawOffset, pitchOffset),
        ImVec2(yawOffset, 4.0f + pitchOffset),
        IM_COL32(255, 255, 255, 128)
    );
}

void PlayerHUD::drawRegion(
    const FramebufferMapping& mapping,
    int destinationX,
    int destinationY,
    int sourceX,
    int sourceY,
    int sourceWidth,
    int sourceHeight) const
{
    const int destinationWidth = sourceWidth * UI_PIXEL_SCALE;
    const int destinationHeight = sourceHeight * UI_PIXEL_SCALE;

    const float textureWidth = static_cast<float>(icons_->getWidth());
    const float textureHeight = static_cast<float>(icons_->getHeight());
    const float minU = static_cast<float>(sourceX) / textureWidth;
    const float maxU = static_cast<float>(sourceX + sourceWidth) / textureWidth;
    const float maxV = 1.0f - static_cast<float>(sourceY) / textureHeight;
    const float minV =
        1.0f - static_cast<float>(sourceY + sourceHeight) / textureHeight;

    ImGui::GetBackgroundDrawList()->AddImage(
        textureId(*icons_),
        ImVec2(
            mapping.logicalX(destinationX),
            mapping.logicalY(destinationY)
        ),
        ImVec2(
            mapping.logicalX(destinationX + destinationWidth),
            mapping.logicalY(destinationY + destinationHeight)
        ),
        ImVec2(minU, maxV),
        ImVec2(maxU, minV)
    );
}

void PlayerHUD::drawHealth(
    const Player& player,
    const FramebufferMapping& mapping) const
{
    const int health = player.getHealth();
    const int previousHealth = player.getPreviousHealth();
    const int flashTicks = player.getHeartFlashTicks();
    bool flashing = (flashTicks / 3) % 2 == 1;
    if (flashTicks < 10)
        flashing = false;

    const int originX =
        mapping.widthPixels / 2 - 91 * UI_PIXEL_SCALE;
    const int baseY =
        mapping.heightPixels - 39 * UI_PIXEL_SCALE;

    std::minstd_rand random(
        static_cast<std::uint32_t>(player.getTicksExisted()) * 312871U
    );

    for (int heart = 0; heart < 10; ++heart)
    {
        const int x = originX + heart * 8 * UI_PIXEL_SCALE;
        int y = baseY;
        if (health <= 4)
            y += static_cast<int>(random() & 1U) * UI_PIXEL_SCALE;

        const int emptySourceX = flashing ? 25 : 16;
        drawRegion(mapping, x, y, emptySourceX, 0, 9, 9);

        if (flashing)
        {
            if (heart * 2 + 1 < previousHealth)
                drawRegion(mapping, x, y, 70, 0, 9, 9);
            else if (heart * 2 + 1 == previousHealth)
                drawRegion(mapping, x, y, 79, 0, 9, 9);
        }

        if (heart * 2 + 1 < health)
            drawRegion(mapping, x, y, 52, 0, 9, 9);
        else if (heart * 2 + 1 == health)
            drawRegion(mapping, x, y, 61, 0, 9, 9);
    }
}

void PlayerHUD::drawAir(
    const Player& player,
    const FramebufferMapping& mapping) const
{
    const double maximumAir =
        static_cast<double>(player.getMaximumAir());
    int fullBubbles = static_cast<int>(std::ceil(
        static_cast<double>(player.getAir() - 2) * 10.0 / maximumAir
    ));
    const int visibleBubbles = static_cast<int>(std::ceil(
        static_cast<double>(player.getAir()) * 10.0 / maximumAir
    ));

    fullBubbles = std::clamp(fullBubbles, 0, 10);
    const int bubbleCount = std::clamp(visibleBubbles, 0, 10);

    const int originX =
        mapping.widthPixels / 2 + 91 * UI_PIXEL_SCALE;
    const int y =
        mapping.heightPixels - 49 * UI_PIXEL_SCALE;

    for (int bubble = 0; bubble < bubbleCount; ++bubble)
    {
        const int sourceX = bubble < fullBubbles ? 16 : 25;
        drawRegion(
            mapping,
            originX - (bubble + 1) * 8 * UI_PIXEL_SCALE - UI_PIXEL_SCALE,
            y,
            sourceX,
            18,
            9,
            9
        );
    }
}

void PlayerHUD::drawCrosshair(
    const FramebufferMapping& mapping) const
{
    ImDrawList* draw = ImGui::GetBackgroundDrawList();
    draw->AddCallback(beginInvertedCrosshairBlend, nullptr);

    drawRegion(
        mapping,
        mapping.widthPixels / 2 - 7 * UI_PIXEL_SCALE,
        mapping.heightPixels / 2 - 7 * UI_PIXEL_SCALE,
        0,
        0,
        16,
        16
    );

    draw->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
}
