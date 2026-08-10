#pragma once

#include "Atmosphere.h"

#include <glad/gl.h>
#include <glm/mat4x4.hpp>

#include <memory>
#include <vector>

class Shader;
class Texture2D;

class SkyRenderer
{
public:
    SkyRenderer();
    ~SkyRenderer();

    SkyRenderer(const SkyRenderer&) = delete;
    SkyRenderer& operator=(const SkyRenderer&) = delete;

    void draw(
        const glm::mat4& view,
        const glm::mat4& projection,
        const AtmosphereState& atmosphere
    );

private:
    struct GradientVertex
    {
        glm::vec3 position;
        float altitude = 0.0f;
    };

    struct ColourVertex
    {
        glm::vec3 position;
        float opacity = 1.0f;
    };

    struct CelestialVertex
    {
        glm::vec3 position;
        glm::vec2 uv;
    };

    GLuint domeVertexArray_ = 0;
    GLuint domeVertexBuffer_ = 0;
    GLsizei domeVertexCount_ = 0;

    GLuint starVertexArray_ = 0;
    GLuint starVertexBuffer_ = 0;
    GLsizei starVertexCount_ = 0;

    GLuint sunriseVertexArray_ = 0;
    GLuint sunriseVertexBuffer_ = 0;

    GLuint celestialVertexArray_ = 0;
    GLuint celestialVertexBuffer_ = 0;

    std::unique_ptr<Shader> gradientShader_;
    std::unique_ptr<Shader> colourShader_;
    std::unique_ptr<Shader> celestialShader_;
    std::unique_ptr<Texture2D> sunTexture_;
    std::unique_ptr<Texture2D> moonTexture_;

    void buildDome();
    void buildStars();
    void createDynamicMeshes();

    void drawSunriseSunset(
        const glm::mat4& skyView,
        const glm::mat4& projection,
        const AtmosphereState& atmosphere
    );
    void drawCelestialBody(
        const Texture2D& texture,
        float size,
        float y,
        bool moon
    );
};
