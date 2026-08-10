#pragma once

#include <filesystem>
#include <cstdint>
#include <span>

#include <glad/gl.h>

class Texture2D
{
public:
    explicit Texture2D(const std::filesystem::path& path);
    Texture2D(
        const std::filesystem::path& path,
        int expectedWidth,
        int expectedHeight
    );
    Texture2D(
        int width,
        int height,
        std::span<const std::uint8_t> rgbaPixels
    );
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    Texture2D(Texture2D&& other) noexcept;
    Texture2D& operator=(Texture2D&& other) noexcept;

    void bind(unsigned int textureUnit = 0) const;
    void setRepeatWrapping(bool enabled) const;

    [[nodiscard]] int getWidth() const noexcept;
    [[nodiscard]] int getHeight() const noexcept;
    [[nodiscard]] int getChannelCount() const noexcept;
    [[nodiscard]] GLuint getId() const noexcept;

private:
    void load(
        const std::filesystem::path& path,
        int expectedWidth,
        int expectedHeight
    );
    void uploadRgba(
        int width,
        int height,
        std::span<const std::uint8_t> rgbaPixels
    );

    GLuint id_ = 0;
    int width_ = 0;
    int height_ = 0;
    int channelCount_ = 0;
};
