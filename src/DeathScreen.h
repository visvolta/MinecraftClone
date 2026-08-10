#pragma once

#include <memory>
#include <string_view>

class Texture2D;

enum class DeathScreenAction
{
    None,
    Respawn,
    TitleMenu
};

class DeathScreen
{
public:
    DeathScreen();

    [[nodiscard]] DeathScreenAction draw(
        int framebufferWidth,
        int framebufferHeight
    ) const;

private:
    static constexpr int UI_PIXEL_SCALE = 3;

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

    std::unique_ptr<Texture2D> widgets_;

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

    [[nodiscard]] bool drawButton(
        const FramebufferMapping& mapping,
        int x,
        int y,
        std::string_view label
    ) const;

    static void drawCenteredText(
        const FramebufferMapping& mapping,
        std::string_view text,
        int centerX,
        int topY,
        int physicalFontSize,
        unsigned int colour
    );
};
