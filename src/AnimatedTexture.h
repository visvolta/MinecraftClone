#pragma once
#include <filesystem>
#include <glad/gl.h>

class AnimatedTexture
{
public:
    AnimatedTexture(const std::filesystem::path& path, int frameWidth, int frameHeight);
    ~AnimatedTexture();
    AnimatedTexture(const AnimatedTexture&) = delete;
    AnimatedTexture& operator=(const AnimatedTexture&) = delete;
    void bind(unsigned int unit) const;
    [[nodiscard]] int getFrameCount() const noexcept { return frameCount_; }

private:
    GLuint id_ = 0;
    int frameCount_ = 0;
};
