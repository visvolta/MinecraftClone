#pragma once

#include <glad/gl.h>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include <cstdint>
#include <memory>

class Shader;

enum class AntiAliasingMode
{
    Off = 0,
    Fxaa,
    Smaa,
    Taa
};

[[nodiscard]] const char* antiAliasingModeName(
    AntiAliasingMode mode
) noexcept;

class PostProcessor
{
public:
    PostProcessor();
    ~PostProcessor();

    PostProcessor(const PostProcessor&) = delete;
    PostProcessor& operator=(const PostProcessor&) = delete;

    void beginFrame(
        int width,
        int height,
        AntiAliasingMode mode
    );
    [[nodiscard]] glm::mat4 jitterProjection(
        const glm::mat4& projection,
        AntiAliasingMode mode
    ) const;
    void resolve(
        AntiAliasingMode mode,
        const glm::mat4& currentViewProjection
    );
    void invalidateHistory() noexcept;

private:
    int width_ = 0;
    int height_ = 0;
    GLuint fullscreenVertexArray_ = 0;
    GLuint sceneFramebuffer_ = 0;
    GLuint sceneColour_ = 0;
    GLuint sceneDepth_ = 0;
    GLuint historyFramebuffer_ = 0;
    GLuint historyColour_[2]{0, 0};
    int historyReadIndex_ = 0;
    bool historyValid_ = false;
    std::uint64_t frameIndex_ = 0;
    glm::vec2 jitterPixels_{0.0f};
    glm::mat4 previousViewProjection_{1.0f};
    AntiAliasingMode previousMode_ = AntiAliasingMode::Off;

    std::unique_ptr<Shader> copyShader_;
    std::unique_ptr<Shader> fxaaShader_;
    std::unique_ptr<Shader> smaaShader_;
    std::unique_ptr<Shader> taaShader_;

    void resize(int width, int height);
    void releaseTargets();
    void drawFullscreen() const;
    void drawSinglePass(Shader& shader, GLuint sourceTexture);
    [[nodiscard]] static float halton(
        std::uint64_t index,
        std::uint64_t base
    ) noexcept;
};
