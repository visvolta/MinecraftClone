#include "AnimatedTexture.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
#include <stb_image.h>

AnimatedTexture::AnimatedTexture(
    const std::filesystem::path& path,
    int frameWidth,
    int frameHeight)
{
    stbi_set_flip_vertically_on_load(false);
    int width=0, height=0, channels=0;
    unsigned char* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (!pixels)
        throw std::runtime_error("Failed animated texture: " + path.string());

    if (width != frameWidth || height % frameHeight != 0)
    {
        stbi_image_free(pixels);
        throw std::runtime_error("Animated texture strip dimensions invalid: " + path.string());
    }

    frameCount_ = height / frameHeight;
    std::vector<unsigned char> frames(
        static_cast<std::size_t>(width) * frameHeight * frameCount_ * 4
    );

    for (int frame=0; frame<frameCount_; ++frame)
    {
        for (int y=0; y<frameHeight; ++y)
        {
            const int sourceY = frame * frameHeight + y;
            const int destY = frameHeight - 1 - y;
            const auto sourceOffset = (static_cast<std::size_t>(sourceY) * width) * 4;
            const auto destinationOffset =
                (static_cast<std::size_t>(frame) * frameWidth * frameHeight +
                 static_cast<std::size_t>(destY) * frameWidth) * 4;
            std::copy_n(pixels + sourceOffset, static_cast<std::size_t>(width) * 4,
                        frames.data() + destinationOffset);
        }
    }
    stbi_image_free(pixels);

    glGenTextures(1, &id_);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id_);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0);
    glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
                 frameWidth, frameHeight, frameCount_, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, frames.data());
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

AnimatedTexture::~AnimatedTexture()
{
    if (id_ != 0) glDeleteTextures(1, &id_);
}

void AnimatedTexture::bind(unsigned int unit) const
{
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D_ARRAY, id_);
}
