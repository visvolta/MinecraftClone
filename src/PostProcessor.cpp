#include "PostProcessor.h"

#include "AssetPaths.h"
#include "Shader.h"

#include <algorithm>
#include <stdexcept>

#include <glm/gtc/matrix_inverse.hpp>

namespace
{
void configureColourTexture(GLuint texture, int width, int height)
{
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA8,
        width,
        height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void requireCompleteFramebuffer(const char* label)
{
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error(std::string(label) + " is incomplete");
}
}

const char* antiAliasingModeName(AntiAliasingMode mode) noexcept
{
    switch (mode)
    {
        case AntiAliasingMode::Off: return "Off";
        case AntiAliasingMode::Fxaa: return "FXAA";
        case AntiAliasingMode::Smaa: return "SMAA";
        case AntiAliasingMode::Taa: return "TAA / TXAA-style";
    }
    return "Off";
}

PostProcessor::PostProcessor()
    : copyShader_(std::make_unique<Shader>(
          AssetPaths::get("shaders/post_process.vert"),
          AssetPaths::get("shaders/post_copy.frag"))),
      fxaaShader_(std::make_unique<Shader>(
          AssetPaths::get("shaders/post_process.vert"),
          AssetPaths::get("shaders/fxaa.frag"))),
      smaaShader_(std::make_unique<Shader>(
          AssetPaths::get("shaders/post_process.vert"),
          AssetPaths::get("shaders/smaa.frag"))),
      taaShader_(std::make_unique<Shader>(
          AssetPaths::get("shaders/post_process.vert"),
          AssetPaths::get("shaders/taa.frag")))
{
    glGenVertexArrays(1, &fullscreenVertexArray_);

    copyShader_->use();
    copyShader_->setInt("sourceTexture", 0);
    fxaaShader_->use();
    fxaaShader_->setInt("sourceTexture", 0);
    smaaShader_->use();
    smaaShader_->setInt("sourceTexture", 0);
    taaShader_->use();
    taaShader_->setInt("currentColour", 0);
    taaShader_->setInt("depthTexture", 1);
    taaShader_->setInt("historyTexture", 2);
}

PostProcessor::~PostProcessor()
{
    releaseTargets();
    if (fullscreenVertexArray_ != 0)
        glDeleteVertexArrays(1, &fullscreenVertexArray_);
}

void PostProcessor::beginFrame(
    int width,
    int height,
    AntiAliasingMode mode)
{
    resize(width, height);
    if (mode != previousMode_)
    {
        invalidateHistory();
        previousMode_ = mode;
    }

    if (mode == AntiAliasingMode::Taa)
    {
        const std::uint64_t sample = frameIndex_ % 8U + 1U;
        jitterPixels_ = {
            halton(sample, 2U) - 0.5f,
            halton(sample, 3U) - 0.5f
        };
        ++frameIndex_;
    }
    else
    {
        jitterPixels_ = glm::vec2(0.0f);
    }

    glBindFramebuffer(
        GL_FRAMEBUFFER,
        mode == AntiAliasingMode::Off ? 0 : sceneFramebuffer_
    );
    glViewport(0, 0, width_, height_);
}

glm::mat4 PostProcessor::jitterProjection(
    const glm::mat4& projection,
    AntiAliasingMode mode) const
{
    if (mode != AntiAliasingMode::Taa || width_ <= 0 || height_ <= 0)
        return projection;

    glm::mat4 jittered = projection;
    jittered[2][0] += 2.0f * jitterPixels_.x /
        static_cast<float>(width_);
    jittered[2][1] += 2.0f * jitterPixels_.y /
        static_cast<float>(height_);
    return jittered;
}

void PostProcessor::resolve(
    AntiAliasingMode mode,
    const glm::mat4& currentViewProjection)
{
    if (mode == AntiAliasingMode::Off)
        return;

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glDepthMask(GL_FALSE);

    if (mode == AntiAliasingMode::Fxaa)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        fxaaShader_->use();
        fxaaShader_->setVec2(
            "inverseResolution",
            {1.0f / static_cast<float>(width_),
             1.0f / static_cast<float>(height_)}
        );
        drawSinglePass(*fxaaShader_, sceneColour_);
        invalidateHistory();
    }
    else if (mode == AntiAliasingMode::Smaa)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        smaaShader_->use();
        smaaShader_->setVec2(
            "inverseResolution",
            {1.0f / static_cast<float>(width_),
             1.0f / static_cast<float>(height_)}
        );
        drawSinglePass(*smaaShader_, sceneColour_);
        invalidateHistory();
    }
    else
    {
        const int writeIndex = 1 - historyReadIndex_;
        glBindFramebuffer(GL_FRAMEBUFFER, historyFramebuffer_);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            historyColour_[writeIndex],
            0
        );
        requireCompleteFramebuffer("TAA history framebuffer");
        glViewport(0, 0, width_, height_);

        taaShader_->use();
        taaShader_->setVec2(
            "inverseResolution",
            {1.0f / static_cast<float>(width_),
             1.0f / static_cast<float>(height_)}
        );
        taaShader_->setMat4(
            "inverseCurrentViewProjection",
            glm::inverse(currentViewProjection)
        );
        taaShader_->setMat4(
            "previousViewProjection",
            previousViewProjection_
        );
        taaShader_->setBool("historyValid", historyValid_);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sceneColour_);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, sceneDepth_);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, historyColour_[historyReadIndex_]);
        drawFullscreen();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, width_, height_);
        drawSinglePass(*copyShader_, historyColour_[writeIndex]);

        historyReadIndex_ = writeIndex;
        historyValid_ = true;
        previousViewProjection_ = currentViewProjection;
    }

    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
}

void PostProcessor::invalidateHistory() noexcept
{
    historyValid_ = false;
}

void PostProcessor::resize(int width, int height)
{
    width = std::max(width, 1);
    height = std::max(height, 1);
    if (width == width_ && height == height_ && sceneFramebuffer_ != 0)
        return;

    releaseTargets();
    width_ = width;
    height_ = height;
    invalidateHistory();

    glGenFramebuffers(1, &sceneFramebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFramebuffer_);

    glGenTextures(1, &sceneColour_);
    configureColourTexture(sceneColour_, width_, height_);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        sceneColour_,
        0
    );

    glGenTextures(1, &sceneDepth_);
    glBindTexture(GL_TEXTURE_2D, sceneDepth_);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_DEPTH_COMPONENT24,
        width_,
        height_,
        0,
        GL_DEPTH_COMPONENT,
        GL_UNSIGNED_INT,
        nullptr
    );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D,
        sceneDepth_,
        0
    );
    requireCompleteFramebuffer("Scene framebuffer");

    glGenFramebuffers(1, &historyFramebuffer_);
    glGenTextures(2, historyColour_);
    for (GLuint texture : historyColour_)
        configureColourTexture(texture, width_, height_);

    glBindFramebuffer(GL_FRAMEBUFFER, historyFramebuffer_);
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        historyColour_[0],
        0
    );
    requireCompleteFramebuffer("TAA history framebuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void PostProcessor::releaseTargets()
{
    if (historyColour_[0] != 0 || historyColour_[1] != 0)
        glDeleteTextures(2, historyColour_);
    if (sceneDepth_ != 0)
        glDeleteTextures(1, &sceneDepth_);
    if (sceneColour_ != 0)
        glDeleteTextures(1, &sceneColour_);
    if (historyFramebuffer_ != 0)
        glDeleteFramebuffers(1, &historyFramebuffer_);
    if (sceneFramebuffer_ != 0)
        glDeleteFramebuffers(1, &sceneFramebuffer_);

    historyColour_[0] = 0;
    historyColour_[1] = 0;
    sceneDepth_ = 0;
    sceneColour_ = 0;
    historyFramebuffer_ = 0;
    sceneFramebuffer_ = 0;
}

void PostProcessor::drawFullscreen() const
{
    glBindVertexArray(fullscreenVertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void PostProcessor::drawSinglePass(
    Shader& shader,
    GLuint sourceTexture)
{
    shader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, sourceTexture);
    drawFullscreen();
}

float PostProcessor::halton(
    std::uint64_t index,
    std::uint64_t base) noexcept
{
    float result = 0.0f;
    float fraction = 1.0f;
    while (index > 0)
    {
        fraction /= static_cast<float>(base);
        result += fraction * static_cast<float>(index % base);
        index /= base;
    }
    return result;
}
