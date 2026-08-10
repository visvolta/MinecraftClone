#include "Texture2D.h"

#include <stdexcept>
#include <string>
#include <utility>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Texture2D::Texture2D(const std::filesystem::path& path)
{
    load(path, 0, 0);
}

Texture2D::Texture2D(
    const std::filesystem::path& path,
    int expectedWidth,
    int expectedHeight)
{
    load(path, expectedWidth, expectedHeight);
}

Texture2D::Texture2D(
    int width,
    int height,
    std::span<const std::uint8_t> rgbaPixels)
{
    uploadRgba(width, height, rgbaPixels);
}

void Texture2D::uploadRgba(
    int width,
    int height,
    std::span<const std::uint8_t> rgbaPixels)
{
    if (width <= 0 || height <= 0 ||
        rgbaPixels.size() != static_cast<std::size_t>(width * height * 4))
    {
        throw std::invalid_argument("Invalid RGBA texture dimensions or pixel count");
    }

    width_ = width;
    height_ = height;
    channelCount_ = 4;
    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0,
        GL_RGBA, GL_UNSIGNED_BYTE, rgbaPixels.data()
    );
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture2D::load(
    const std::filesystem::path& path,
    int expectedWidth,
    int expectedHeight)
{
    // Keep the source PNG visually upright in OpenGL. TextureAtlas converts
    // source rows to the correct bottom-origin OpenGL UV rows.
    stbi_set_flip_vertically_on_load(true);

    const std::string pathString = path.string();
    unsigned char* pixels = stbi_load(
        pathString.c_str(),
        &width_,
        &height_,
        &channelCount_,
        0
    );

    if (pixels == nullptr)
    {
        const char* reason = stbi_failure_reason();
        throw std::runtime_error(
            "Failed to load texture: " + pathString +
            (reason != nullptr
                ? "\nstb_image reason: " + std::string(reason)
                : std::string{})
        );
    }

    if ((expectedWidth > 0 && width_ != expectedWidth) ||
        (expectedHeight > 0 && height_ != expectedHeight))
    {
        stbi_image_free(pixels);

        throw std::runtime_error(
            "Texture has the wrong dimensions: " + pathString +
            "\nExpected: " + std::to_string(expectedWidth) + "x" +
            std::to_string(expectedHeight) +
            "\nLoaded: " + std::to_string(width_) + "x" +
            std::to_string(height_) +
            "\nDelete the build directory so the current assets are recopied."
        );
    }

    GLenum format = GL_RGB;
    GLenum internalFormat = GL_RGB8;

    if (channelCount_ == 1)
    {
        format = GL_RED;
        internalFormat = GL_R8;
    }
    else if (channelCount_ == 4)
    {
        format = GL_RGBA;
        internalFormat = GL_RGBA8;
    }

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D, id_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // All game and GUI artwork is pixel art. Keep exact source texels and
    // never allow linear filtering or implicit mip-level sampling.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        static_cast<GLint>(internalFormat),
        width_,
        height_,
        0,
        format,
        GL_UNSIGNED_BYTE,
        pixels
    );
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    stbi_image_free(pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D::~Texture2D()
{
    if (id_ != 0)
        glDeleteTextures(1, &id_);
}

Texture2D::Texture2D(Texture2D&& other) noexcept
    : id_(std::exchange(other.id_, 0)),
      width_(std::exchange(other.width_, 0)),
      height_(std::exchange(other.height_, 0)),
      channelCount_(std::exchange(other.channelCount_, 0))
{
}

Texture2D& Texture2D::operator=(Texture2D&& other) noexcept
{
    if (this != &other)
    {
        if (id_ != 0)
            glDeleteTextures(1, &id_);

        id_ = std::exchange(other.id_, 0);
        width_ = std::exchange(other.width_, 0);
        height_ = std::exchange(other.height_, 0);
        channelCount_ = std::exchange(other.channelCount_, 0);
    }

    return *this;
}

void Texture2D::bind(unsigned int textureUnit) const
{
    glActiveTexture(GL_TEXTURE0 + textureUnit);
    glBindTexture(GL_TEXTURE_2D, id_);
}

void Texture2D::setRepeatWrapping(bool enabled) const
{
    glBindTexture(GL_TEXTURE_2D, id_);
    const GLint mode = enabled ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode);
    glBindTexture(GL_TEXTURE_2D, 0);
}

int Texture2D::getWidth() const noexcept
{
    return width_;
}

int Texture2D::getHeight() const noexcept
{
    return height_;
}

int Texture2D::getChannelCount() const noexcept
{
    return channelCount_;
}

GLuint Texture2D::getId() const noexcept
{
    return id_;
}
