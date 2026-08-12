#include "client/render/MobEntityRenderer.h"

#include "AssetPaths.h"
#include "Atmosphere.h"
#include "Shader.h"
#include "Texture2D.h"
#include "entity/Mob.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace mc::client
{
namespace
{
struct Vertex
{
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec3 colour;
};

struct Quad
{
    std::array<int, 4> vertices{};
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
};

glm::vec3 modelPosition(const glm::mat4& partMatrix, glm::vec3 pixelPosition)
{
    const glm::vec3 transformed = glm::vec3(
        partMatrix * glm::vec4(pixelPosition, 1.0f)
    );
    return {
        transformed.x / 16.0f,
        (24.0f - transformed.y) / 16.0f,
        transformed.z / 16.0f
    };
}

void appendQuad(
    std::vector<Vertex>& output,
    const std::array<glm::vec3, 8>& points,
    const Quad& quad,
    const glm::mat4& transform,
    int textureWidth,
    int textureHeight,
    bool mirror)
{
    std::array<int, 4> indices = quad.vertices;
    if (mirror)
        std::reverse(indices.begin(), indices.end());
    std::array<glm::vec3, 4> positions{};
    for (std::size_t index = 0; index < positions.size(); ++index)
        positions[index] = modelPosition(transform, points[indices[index]]);
    const glm::vec3 edgeOne = positions[1] - positions[0];
    const glm::vec3 edgeTwo = positions[2] - positions[1];
    glm::vec3 normal = glm::cross(edgeOne, edgeTwo);
    if (glm::dot(normal, normal) > 0.0000001f)
        normal = glm::normalize(normal);
    // Match block face shading (top 1.0, sides 0.8). World light is applied
    // as a uniform from the entity sample location, like vanilla lightmap.
    const float light = std::clamp(
        0.8f + std::max(0.0f, normal.y) * 0.2f,
        0.8f, 1.0f
    );
    const glm::vec3 colour(light);
    const float u0 = quad.u0 / static_cast<float>(textureWidth);
    const float u1 = quad.u1 / static_cast<float>(textureWidth);
    const float v0 = 1.0f - quad.v0 / static_cast<float>(textureHeight);
    const float v1 = 1.0f - quad.v1 / static_cast<float>(textureHeight);
    const std::array<glm::vec2, 4> uv{{
        {u1,v0},{u0,v0},{u0,v1},{u1,v1}
    }};
    constexpr std::array<std::size_t,6> triangles{0,1,2,0,2,3};
    for (const std::size_t index : triangles)
        output.push_back({positions[index],uv[index],colour});
}

void appendCube(
    std::vector<Vertex>& output,
    const MobModelCube& cube,
    const glm::mat4& transform,
    int textureWidth,
    int textureHeight)
{
    glm::vec3 minimum = cube.origin - glm::vec3(cube.inflate);
    glm::vec3 maximum = cube.origin + glm::vec3(cube.size) +
                        glm::vec3(cube.inflate);
    if (cube.mirror)
        std::swap(minimum.x, maximum.x);
    const std::array<glm::vec3,8> points{{
        {minimum.x,minimum.y,minimum.z},
        {maximum.x,minimum.y,minimum.z},
        {maximum.x,maximum.y,minimum.z},
        {minimum.x,maximum.y,minimum.z},
        {minimum.x,minimum.y,maximum.z},
        {maximum.x,minimum.y,maximum.z},
        {maximum.x,maximum.y,maximum.z},
        {minimum.x,maximum.y,maximum.z}
    }};
    const float u = static_cast<float>(cube.textureOffset.x);
    const float v = static_cast<float>(cube.textureOffset.y);
    const float dx = static_cast<float>(cube.size.x);
    const float dy = static_cast<float>(cube.size.y);
    const float dz = static_cast<float>(cube.size.z);
    const std::array<Quad,6> quads{{
        {{{5,1,2,6}},u+dz+dx,v+dz,u+dz+dx+dz,v+dz+dy},
        {{{0,4,7,3}},u,v+dz,u+dz,v+dz+dy},
        {{{5,4,0,1}},u+dz,v,u+dz+dx,v+dz},
        {{{2,3,7,6}},u+dz+dx,v+dz,u+dz+dx+dx,v},
        {{{1,0,3,2}},u+dz,v+dz,u+dz+dx,v+dz+dy},
        {{{4,5,6,7}},u+dz+dx+dz,v+dz,
                       u+dz+dx+dz+dx,v+dz+dy}
    }};
    for (const Quad& quad : quads)
        appendQuad(
            output, points, quad, transform,
            textureWidth, textureHeight, cube.mirror
        );
}

std::vector<Vertex> buildVertices(
    MobModelDefinition model,
    const entity::Mob& entity,
    float partialTick)
{
    const entity::Mob::PoseState animation = entity.poseState(partialTick);
    animateMobModel(
        entity.getModelKind(),
        model,
        {
            animation.age, animation.limbSwing,
            animation.limbSwingAmount, animation.headYaw,
            animation.headPitch, animation.attackProgress,
            animation.jumpProgress, animation.hurtProgress,
            animation.deathProgress,
            animation.onGround, animation.inWater, animation.aggressive,
            animation.child, animation.sitting, animation.begging
        }
    );
    std::vector<glm::mat4> matrices(model.parts.size(), glm::mat4(1.0f));
    std::vector<Vertex> vertices;
    for (std::size_t index = 0; index < model.parts.size(); ++index)
    {
        const MobModelPart& part = model.parts[index];
        glm::mat4 transform = part.parent >= 0
            ? matrices[static_cast<std::size_t>(part.parent)]
            : glm::mat4(1.0f);
        transform = glm::translate(transform, part.pivot);
        transform = glm::rotate(transform, part.rotation.z, {0,0,1});
        transform = glm::rotate(transform, part.rotation.y, {0,1,0});
        transform = glm::rotate(transform, part.rotation.x, {1,0,0});
        matrices[index] = transform;
        for (const MobModelCube& cube : part.cubes)
            appendCube(
                vertices, cube, transform,
                model.textureWidth, model.textureHeight
            );
    }
    return vertices;
}

bool hasOverlay(const core::ResourceLocation& texture)
{
    return texture.path() != "entity/empty";
}

float overlayScale(const core::ResourceLocation& entityType)
{
    if (entityType.path() == "sheep") return 1.08f;
    if (entityType.path() == "stray") return 1.035f;
    return 1.003f;
}
}

MobEntityRenderer::MobEntityRenderer()
    : shader_(std::make_unique<Shader>(
          AssetPaths::get("shaders/entity.vert"),
          AssetPaths::get("shaders/entity.frag")))
{
    shader_->use();
    shader_->setInt("entityTexture",0);
    shader_->setVec3("entityTint",{1,1,1});
    glGenVertexArrays(1,&vertexArray_);
    glGenBuffers(1,&vertexBuffer_);
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER,vertexBuffer_);
    glVertexAttribPointer(
        0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex,position))
    );
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex,uv))
    );
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
        2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),
        reinterpret_cast<void*>(offsetof(Vertex,colour))
    );
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

MobEntityRenderer::~MobEntityRenderer()
{
    if(vertexBuffer_!=0) glDeleteBuffers(1,&vertexBuffer_);
    if(vertexArray_!=0) glDeleteVertexArrays(1,&vertexArray_);
}

void MobEntityRenderer::draw(
        std::span<entity::Mob* const> entities,
    float partialTick,
    const glm::mat4& view,
    const glm::mat4& projection,
    const AtmosphereState& atmosphere)
{
    shader_->use();
    shader_->setMat4("view",view);
    shader_->setMat4("projection",projection);
    shader_->setFloat("daylightBrightness",atmosphere.daylightBrightness);
    shader_->setFloat("entityLight",1.0f);
    shader_->setInt("fogMode",static_cast<int>(atmosphere.fogMode));
    shader_->setVec3("fogColour",atmosphere.fogColour);
    shader_->setFloat("fogStart",atmosphere.fogStart);
    shader_->setFloat("fogEnd",atmosphere.fogEnd);
    shader_->setFloat("fogDensity",atmosphere.fogDensity);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glBindVertexArray(vertexArray_);
    for(entity::Mob* pointer:entities)
    {
        if(!pointer) continue;
        const entity::Mob& entity=*pointer;
        const MobModelDefinition& model=modelFor(entity.getModelKind());
        const std::vector<Vertex> vertices=buildVertices(
            model,entity,partialTick
        );
        if(vertices.empty()) continue;
        glBindBuffer(GL_ARRAY_BUFFER,vertexBuffer_);
        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(vertices.size()*sizeof(Vertex)),
            vertices.data(),GL_STREAM_DRAW
        );
        glm::mat4 transform(1.0f);
        const entity::Mob::PoseState animation =
            entity.poseState(partialTick);
        transform=glm::translate(
            transform,entity.getInterpolatedPosition(partialTick)
        );
        transform=glm::rotate(
            transform,entity.interpolatedYaw(partialTick),{0,1,0}
        );
        transform=glm::rotate(
            transform,
            animation.deathProgress*1.57079632679f,
            {0,0,1}
        );
        transform=glm::scale(
            transform,glm::vec3(entity.getRenderScale())
        );
        if (entity.getType().path() == "creeper")
        {
            float swell = std::clamp(animation.attackProgress, 0.0f, 1.0f);
            const float pulse = 1.0f + std::sin(swell * 100.0f) * swell * 0.01f;
            swell *= swell;
            swell *= swell;
            transform = glm::scale(transform, glm::vec3(
                (1.0f + swell * 0.4f) * pulse,
                (1.0f + swell * 0.1f) / pulse,
                (1.0f + swell * 0.4f) * pulse
            ));
        }
        shader_->setMat4("model",transform);
        shader_->setFloat("entityLight", entity.getRenderBrightness());
        const glm::vec3 hurtTint = glm::mix(
            glm::vec3(1.0f), glm::vec3(1.0f,0.35f,0.35f),
            animation.hurtProgress
        );
        const bool creeperFlash = entity.getType().path() == "creeper" &&
            static_cast<int>(animation.attackProgress * 10.0f) % 2 != 0;
        shader_->setVec3(
            "entityTint",
            creeperFlash
                ? glm::mix(hurtTint, glm::vec3(1.0f),
                           animation.attackProgress * 0.2f)
                : hurtTint
        );
        textureFor(entity.getTexture()).bind(0);
        glDrawArrays(
            GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size())
        );
        if(hasOverlay(entity.getOverlayTexture()) && !entity.isSheared())
        {
            const float scale=overlayScale(entity.getType());
            glm::mat4 overlay=glm::scale(
                transform,glm::vec3(scale)
            );
            shader_->setMat4("model",overlay);
            shader_->setVec3(
                "entityTint",entity.getOverlayColour()*hurtTint
            );
            textureFor(entity.getOverlayTexture()).bind(0);
            glDepthMask(GL_FALSE);
            glDrawArrays(
                GL_TRIANGLES,0,static_cast<GLsizei>(vertices.size())
            );
            glDepthMask(GL_TRUE);
        }
    }
    glBindVertexArray(0);
    glDisable(GL_BLEND);
}

const Texture2D& MobEntityRenderer::textureFor(
    const core::ResourceLocation& texture)
{
    if(const auto found=textures_.find(texture);found!=textures_.end())
        return *found->second;
    const auto path=AssetPaths::root()/"minecraft"/"textures"/
        (texture.path()+".png");
    auto loaded=std::make_unique<Texture2D>(path);
    loaded->setRepeatWrapping(true);
    const Texture2D& result=*loaded;
    textures_.emplace(texture,std::move(loaded));
    return result;
}

const MobModelDefinition& MobEntityRenderer::modelFor(
    gameplay::MobModelKind model)
{
    if(const auto found=models_.find(model);found!=models_.end())
        return found->second;
    return models_.emplace(model,createMobModel(model)).first->second;
}
}
