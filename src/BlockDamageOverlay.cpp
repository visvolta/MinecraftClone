#include "BlockDamageOverlay.h"

#include "AssetPaths.h"
#include "Shader.h"
#include "Texture2D.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <string>

BlockDamageOverlay::BlockDamageOverlay()
{
    // Keep the overlay on the real block surface. Polygon offset in draw()
    // resolves coplanar depth without expanding faces through neighbours.
    constexpr float lo = -0.5f;
    constexpr float hi =  0.5f;

    constexpr float vertices[] = {
        // position                 // uv
        lo,lo,hi, 0,0,  hi,lo,hi, 1,0,  hi,hi,hi, 1,1,
        lo,lo,hi, 0,0,  hi,hi,hi, 1,1,  lo,hi,hi, 0,1,
        hi,lo,lo, 0,0,  lo,lo,lo, 1,0,  lo,hi,lo, 1,1,
        hi,lo,lo, 0,0,  lo,hi,lo, 1,1,  hi,hi,lo, 0,1,
        lo,lo,lo, 0,0,  lo,lo,hi, 1,0,  lo,hi,hi, 1,1,
        lo,lo,lo, 0,0,  lo,hi,hi, 1,1,  lo,hi,lo, 0,1,
        hi,lo,hi, 0,0,  hi,lo,lo, 1,0,  hi,hi,lo, 1,1,
        hi,lo,hi, 0,0,  hi,hi,lo, 1,1,  hi,hi,hi, 0,1,
        lo,hi,hi, 0,0,  hi,hi,hi, 1,0,  hi,hi,lo, 1,1,
        lo,hi,hi, 0,0,  hi,hi,lo, 1,1,  lo,hi,lo, 0,1,
        lo,lo,lo, 0,0,  hi,lo,lo, 1,0,  hi,lo,hi, 1,1,
        lo,lo,lo, 0,0,  hi,lo,hi, 1,1,  lo,lo,hi, 0,1
    };

    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5*sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, 5*sizeof(float),
        reinterpret_cast<void*>(3*sizeof(float))
    );
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    shader_ = std::make_unique<Shader>(
        AssetPaths::get("shaders/damage_overlay.vert"),
        AssetPaths::get("shaders/damage_overlay.frag")
    );
    shader_->use();
    shader_->setInt("damageTexture", 0);

    for (int i = 0; i < 10; ++i)
    {
        textures_[static_cast<std::size_t>(i)] =
            std::make_unique<Texture2D>(
                AssetPaths::get(
                    "gui/destroy_stage_" + std::to_string(i) + ".png"
                ),
                16,
                16
            );
    }
}

BlockDamageOverlay::~BlockDamageOverlay()
{
    if (vertexBuffer_ != 0)
        glDeleteBuffers(1, &vertexBuffer_);
    if (vertexArray_ != 0)
        glDeleteVertexArrays(1, &vertexArray_);
}

void BlockDamageOverlay::draw(
    const glm::ivec3& blockPosition,
    int stage,
    const glm::mat4& view,
    const glm::mat4& projection) const
{
    if (stage < 0 || stage >= 10)
        return;

    glm::mat4 model(1.0f);
    model = glm::translate(
        model,
        glm::vec3(blockPosition) + glm::vec3(0.5f)
    );

    shader_->use();
    shader_->setMat4("model", model);
    shader_->setMat4("view", view);
    shader_->setMat4("projection", projection);
    textures_[static_cast<std::size_t>(stage)]->bind(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
    glDepthMask(GL_FALSE);
    glEnable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-1.0f, -1.0f);

    glBindVertexArray(vertexArray_);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}
