#pragma once

#include "core/ResourceLocation.h"
#include "client/render/MobModel.h"
#include "gameplay/GameplayRegistries.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <map>
#include <memory>
#include <span>

class Shader;
class Texture2D;
struct AtmosphereState;
namespace mc::entity { class MobEntity; }

namespace mc::client
{
class MobEntityRenderer
{
public:
    MobEntityRenderer();
    ~MobEntityRenderer();

    MobEntityRenderer(const MobEntityRenderer&) = delete;
    MobEntityRenderer& operator=(const MobEntityRenderer&) = delete;

    void draw(
        std::span<const entity::MobEntity> entities,
        float partialTick,
        const glm::mat4& view,
        const glm::mat4& projection,
        const AtmosphereState& atmosphere
    );

private:
    std::unique_ptr<Shader> shader_;
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    std::map<gameplay::MobModelKind, MobModelDefinition> models_;
    std::map<core::ResourceLocation, std::unique_ptr<Texture2D>> textures_;

    [[nodiscard]] const Texture2D& textureFor(
        const core::ResourceLocation& texture
    );
    [[nodiscard]] const MobModelDefinition& modelFor(
        gameplay::MobModelKind model
    );
};
}
