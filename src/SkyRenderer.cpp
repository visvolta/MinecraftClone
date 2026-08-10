#include "SkyRenderer.h"

#include "AssetPaths.h"
#include "Shader.h"
#include "Texture2D.h"
#include "worldgen/JavaRandom.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x3.hpp>

namespace
{
constexpr float PI = std::numbers::pi_v<float>;

template<typename Vertex>
void uploadStaticVertices(
    GLuint& vertexArray,
    GLuint& vertexBuffer,
    const std::vector<Vertex>& vertices,
    std::size_t scalarOffset)
{
    glGenVertexArrays(1, &vertexArray);
    glGenBuffers(1, &vertexBuffer);
    glBindVertexArray(vertexArray);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
        vertices.data(),
        GL_STATIC_DRAW
    );
    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex, position))
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(scalarOffset)
    );
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}
}

SkyRenderer::SkyRenderer()
    : gradientShader_(std::make_unique<Shader>(
          AssetPaths::get("shaders/sky_gradient.vert"),
          AssetPaths::get("shaders/sky_gradient.frag"))),
      colourShader_(std::make_unique<Shader>(
          AssetPaths::get("shaders/sky_colour.vert"),
          AssetPaths::get("shaders/sky_colour.frag"))),
      celestialShader_(std::make_unique<Shader>(
          AssetPaths::get("shaders/celestial.vert"),
          AssetPaths::get("shaders/celestial.frag"))),
      sunTexture_(std::make_unique<Texture2D>(
          AssetPaths::get("textures/sun.png"), 32, 32)),
      moonTexture_(std::make_unique<Texture2D>(
          AssetPaths::get("textures/moon.png"), 32, 32))
{
    buildDome();
    buildStars();
    createDynamicMeshes();

    celestialShader_->use();
    celestialShader_->setInt("celestialTexture", 0);
}

SkyRenderer::~SkyRenderer()
{
    if (celestialVertexBuffer_ != 0)
        glDeleteBuffers(1, &celestialVertexBuffer_);
    if (celestialVertexArray_ != 0)
        glDeleteVertexArrays(1, &celestialVertexArray_);
    if (sunriseVertexBuffer_ != 0)
        glDeleteBuffers(1, &sunriseVertexBuffer_);
    if (sunriseVertexArray_ != 0)
        glDeleteVertexArrays(1, &sunriseVertexArray_);
    if (starVertexBuffer_ != 0)
        glDeleteBuffers(1, &starVertexBuffer_);
    if (starVertexArray_ != 0)
        glDeleteVertexArrays(1, &starVertexArray_);
    if (domeVertexBuffer_ != 0)
        glDeleteBuffers(1, &domeVertexBuffer_);
    if (domeVertexArray_ != 0)
        glDeleteVertexArrays(1, &domeVertexArray_);
}

void SkyRenderer::draw(
    const glm::mat4& view,
    const glm::mat4& projection,
    const AtmosphereState& atmosphere)
{
    const glm::mat4 skyView = glm::mat4(glm::mat3(view));

    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    gradientShader_->use();
    gradientShader_->setMat4("view", skyView);
    gradientShader_->setMat4("projection", projection);
    gradientShader_->setVec3("skyColour", atmosphere.skyColour);
    gradientShader_->setVec3("horizonColour", atmosphere.horizonColour);
    gradientShader_->setVec3("lowerSkyColour", atmosphere.lowerSkyColour);
    glBindVertexArray(domeVertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, domeVertexCount_);

    drawSunriseSunset(skyView, projection, atmosphere);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glm::mat4 celestialModel(1.0f);
    celestialModel = glm::rotate(
        celestialModel,
        glm::radians(atmosphere.celestialAngle * 360.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );

    celestialShader_->use();
    celestialShader_->setMat4("model", celestialModel);
    celestialShader_->setMat4("view", skyView);
    celestialShader_->setMat4("projection", projection);
    celestialShader_->setFloat("alpha", 1.0f);
    drawCelestialBody(*sunTexture_, 30.0f, 100.0f, false);
    drawCelestialBody(*moonTexture_, 20.0f, -100.0f, true);

    if (atmosphere.starBrightness > 0.0f)
    {
        colourShader_->use();
        colourShader_->setMat4("model", celestialModel);
        colourShader_->setMat4("view", skyView);
        colourShader_->setMat4("projection", projection);
        colourShader_->setVec4(
            "colour",
            glm::vec4(
                atmosphere.starBrightness,
                atmosphere.starBrightness,
                atmosphere.starBrightness,
                atmosphere.starBrightness
            )
        );
        glBindVertexArray(starVertexArray_);
        glDrawArrays(GL_TRIANGLES, 0, starVertexCount_);
    }

    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
}

void SkyRenderer::buildDome()
{
    constexpr int segments = 32;
    constexpr int rings = 16;
    constexpr float radius = 200.0f;
    std::vector<GradientVertex> vertices;
    vertices.reserve(segments * rings * 6);

    const auto vertex = [radius](float latitude, float longitude)
    {
        const float cosine = std::cos(latitude);
        const float altitude = std::sin(latitude);
        return GradientVertex{
            glm::vec3(
                cosine * std::cos(longitude),
                altitude,
                cosine * std::sin(longitude)
            ) * radius,
            altitude
        };
    };

    for (int ring = 0; ring < rings; ++ring)
    {
        const float latitude0 =
            -PI * 0.5f + PI * static_cast<float>(ring) / rings;
        const float latitude1 =
            -PI * 0.5f + PI * static_cast<float>(ring + 1) / rings;

        for (int segment = 0; segment < segments; ++segment)
        {
            const float longitude0 =
                2.0f * PI * static_cast<float>(segment) / segments;
            const float longitude1 =
                2.0f * PI * static_cast<float>(segment + 1) / segments;
            const GradientVertex a = vertex(latitude0, longitude0);
            const GradientVertex b = vertex(latitude0, longitude1);
            const GradientVertex c = vertex(latitude1, longitude1);
            const GradientVertex d = vertex(latitude1, longitude0);
            vertices.insert(vertices.end(), {a, b, c, a, c, d});
        }
    }

    uploadStaticVertices(
        domeVertexArray_,
        domeVertexBuffer_,
        vertices,
        offsetof(GradientVertex, altitude)
    );
    domeVertexCount_ = static_cast<GLsizei>(vertices.size());
}

void SkyRenderer::buildStars()
{
    JavaRandom random(10842);
    std::vector<ColourVertex> vertices;
    vertices.reserve(1500 * 6);
    constexpr std::array<int, 6> triangleOrder = {0, 1, 2, 0, 2, 3};

    for (int star = 0; star < 1500; ++star)
    {
        double x = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
        double y = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
        double z = static_cast<double>(random.nextFloat() * 2.0f - 1.0f);
        const double size =
            static_cast<double>(0.25f + random.nextFloat() * 0.25f);
        double lengthSquared = x * x + y * y + z * z;
        if (lengthSquared >= 1.0 || lengthSquared <= 0.01)
            continue;

        const double inverseLength = 1.0 / std::sqrt(lengthSquared);
        x *= inverseLength;
        y *= inverseLength;
        z *= inverseLength;
        const double centerX = x * 100.0;
        const double centerY = y * 100.0;
        const double centerZ = z * 100.0;
        const double longitude = std::atan2(x, z);
        const double sinLongitude = std::sin(longitude);
        const double cosLongitude = std::cos(longitude);
        const double latitude = std::atan2(std::sqrt(x * x + z * z), y);
        const double sinLatitude = std::sin(latitude);
        const double cosLatitude = std::cos(latitude);
        const double rotation = random.nextDouble() *
            std::numbers::pi_v<double> * 2.0;
        const double sinRotation = std::sin(rotation);
        const double cosRotation = std::cos(rotation);

        std::array<glm::vec3, 4> corners{};
        for (int corner = 0; corner < 4; ++corner)
        {
            const double localX = ((corner & 2) - 1) * size;
            const double localY = (((corner + 1) & 2) - 1) * size;
            const double rotatedX =
                localX * cosRotation - localY * sinRotation;
            const double rotatedY =
                localY * cosRotation + localX * sinRotation;
            const double latitudeX = rotatedX * sinLatitude;
            const double latitudeZ = -rotatedX * cosLatitude;
            const double worldX =
                latitudeZ * sinLongitude - rotatedY * cosLongitude;
            const double worldZ =
                rotatedY * sinLongitude + latitudeZ * cosLongitude;

            corners[static_cast<std::size_t>(corner)] = glm::vec3(
                static_cast<float>(centerX + worldX),
                static_cast<float>(centerY + latitudeX),
                static_cast<float>(centerZ + worldZ)
            );
        }

        for (int corner : triangleOrder)
        {
            vertices.push_back({
                corners[static_cast<std::size_t>(corner)],
                1.0f
            });
        }
    }

    uploadStaticVertices(
        starVertexArray_,
        starVertexBuffer_,
        vertices,
        offsetof(ColourVertex, opacity)
    );
    starVertexCount_ = static_cast<GLsizei>(vertices.size());
}

void SkyRenderer::createDynamicMeshes()
{
    glGenVertexArrays(1, &sunriseVertexArray_);
    glGenBuffers(1, &sunriseVertexBuffer_);
    glBindVertexArray(sunriseVertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, sunriseVertexBuffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(18 * sizeof(ColourVertex)),
        nullptr,
        GL_DYNAMIC_DRAW
    );
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(ColourVertex),
        reinterpret_cast<void*>(offsetof(ColourVertex, position))
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 1, GL_FLOAT, GL_FALSE, sizeof(ColourVertex),
        reinterpret_cast<void*>(offsetof(ColourVertex, opacity))
    );
    glEnableVertexAttribArray(1);

    glGenVertexArrays(1, &celestialVertexArray_);
    glGenBuffers(1, &celestialVertexBuffer_);
    glBindVertexArray(celestialVertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, celestialVertexBuffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(6 * sizeof(CelestialVertex)),
        nullptr,
        GL_DYNAMIC_DRAW
    );
    glVertexAttribPointer(
        0, 3, GL_FLOAT, GL_FALSE, sizeof(CelestialVertex),
        reinterpret_cast<void*>(offsetof(CelestialVertex, position))
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, sizeof(CelestialVertex),
        reinterpret_cast<void*>(offsetof(CelestialVertex, uv))
    );
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}

void SkyRenderer::drawSunriseSunset(
    const glm::mat4& skyView,
    const glm::mat4& projection,
    const AtmosphereState& atmosphere)
{
    if (atmosphere.sunriseSunsetColour.a <= 0.0f)
        return;

    std::array<ColourVertex, 18> vertices{};
    vertices[0] = {{0.0f, 100.0f, 0.0f}, 1.0f};
    for (int point = 0; point <= 16; ++point)
    {
        const float angle = static_cast<float>(point) * 2.0f * PI / 16.0f;
        const float sine = std::sin(angle);
        const float cosine = std::cos(angle);
        vertices[static_cast<std::size_t>(point + 1)] = {
            {
                sine * 120.0f,
                cosine * 120.0f,
                -cosine * 40.0f * atmosphere.sunriseSunsetColour.a
            },
            0.0f
        };
    }

    glBindBuffer(GL_ARRAY_BUFFER, sunriseVertexBuffer_);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices.data()
    );

    glm::mat4 model(1.0f);
    model = glm::rotate(
        model,
        glm::radians(90.0f),
        glm::vec3(1.0f, 0.0f, 0.0f)
    );
    if (atmosphere.celestialAngle > 0.5f)
    {
        model = glm::rotate(
            model,
            glm::radians(180.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    colourShader_->use();
    colourShader_->setMat4("model", model);
    colourShader_->setMat4("view", skyView);
    colourShader_->setMat4("projection", projection);
    colourShader_->setVec4("colour", atmosphere.sunriseSunsetColour);
    glBindVertexArray(sunriseVertexArray_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 18);
    glDisable(GL_BLEND);
}

void SkyRenderer::drawCelestialBody(
    const Texture2D& texture,
    float size,
    float y,
    bool moon)
{
    const float leftU = moon ? 1.0f : 0.0f;
    const float rightU = moon ? 0.0f : 1.0f;
    const std::array<CelestialVertex, 4> corners = {{
        {{-size, y, -size}, {leftU, 1.0f}},
        {{ size, y, -size}, {rightU, 1.0f}},
        {{ size, y,  size}, {rightU, 0.0f}},
        {{-size, y,  size}, {leftU, 0.0f}}
    }};
    constexpr std::array<int, 6> order = {0, 1, 2, 0, 2, 3};
    std::array<CelestialVertex, 6> vertices{};
    for (std::size_t index = 0; index < order.size(); ++index)
        vertices[index] = corners[static_cast<std::size_t>(order[index])];

    glBindBuffer(GL_ARRAY_BUFFER, celestialVertexBuffer_);
    glBufferSubData(
        GL_ARRAY_BUFFER,
        0,
        sizeof(vertices),
        vertices.data()
    );
    texture.bind(0);
    glBindVertexArray(celestialVertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
