#pragma once

#include <memory>

class Player;
class Camera;
class Texture2D;

class PlayerHUD
{
public:
    PlayerHUD();

    void draw(
        const Player& player,
        const Camera& camera,
        int framebufferWidth,
        int framebufferHeight,
        bool showCrosshair
    ) const;

private:
    // Selected per frame with the 1.12 ScaledResolution rules.
    inline static int UI_PIXEL_SCALE = 1;

    struct FramebufferMapping
    {
        int widthPixels = 1;
        int heightPixels = 1;
        float logicalToPhysicalX = 1.0f;
        float logicalToPhysicalY = 1.0f;

        [[nodiscard]] float logicalX(int physicalX) const noexcept;
        [[nodiscard]] float logicalY(int physicalY) const noexcept;
    };

    std::unique_ptr<Texture2D> icons_;
    std::unique_ptr<Texture2D> waterOverlay_;

    [[nodiscard]] static FramebufferMapping makeMapping(
        int framebufferWidth,
        int framebufferHeight
    ) noexcept;

    void drawRegion(
        const FramebufferMapping& mapping,
        int destinationX,
        int destinationY,
        int sourceX,
        int sourceY,
        int sourceWidth,
        int sourceHeight
    ) const;

    void drawHealth(
        const Player& player,
        const FramebufferMapping& mapping
    ) const;

    void drawFood(
        const Player& player,
        const FramebufferMapping& mapping
    ) const;

    void drawArmor(
        const Player& player,
        const FramebufferMapping& mapping
    ) const;

    void drawExperience(
        const Player& player,
        const FramebufferMapping& mapping
    ) const;

    void drawAir(
        const Player& player,
        const FramebufferMapping& mapping
    ) const;

    void drawUnderwaterOverlay(
        const Camera& camera,
        const FramebufferMapping& mapping
    ) const;

    void drawCrosshair(const FramebufferMapping& mapping) const;
};
