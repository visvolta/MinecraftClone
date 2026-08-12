#include "BlockDamageOverlay.h"

#include "AssetPaths.h"
#include "Shader.h"
#include "Texture2D.h"
#include "TextureAtlas.h"
#include "BlockShape.h"
#include "BlockModelSeed.h"
#include "client/render/ModelBakery.h"

#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <string>
#include <vector>

BlockDamageOverlay::BlockDamageOverlay()
{
    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);

    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        0,
        nullptr,
        GL_DYNAMIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        nullptr
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        2,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<void*>(
            3 * sizeof(float)
        )
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    shader_ = std::make_unique<Shader>(
        AssetPaths::get(
            "shaders/damage_overlay.vert"
        ),
        AssetPaths::get(
            "shaders/damage_overlay.frag"
        )
    );

    shader_->use();
    shader_->setInt("damageTexture", 0);

    for (int i = 0; i < 10; ++i)
    {
        textures_[
            static_cast<std::size_t>(i)
        ] = std::make_unique<Texture2D>(
            AssetPaths::get(
                "gui/destroy_stage_" +
                std::to_string(i) +
                ".png"
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
    mc::content::BlockState state,
    int stage,
    const glm::mat4& view,
    const glm::mat4& projection) const
{
    if (stage < 0 || stage >= 10)
        return;

    uploadModel(
        state,
        blockModelSeed(blockPosition)
    );

    if (vertexCount_ == 0)
        return;

    glm::mat4 model(1.0f);
    model = glm::translate(
        model,
        glm::vec3(blockPosition)
    );

    shader_->use();
    shader_->setMat4("model", model);
    shader_->setMat4("view", view);
    shader_->setMat4(
        "projection",
        projection
    );

    textures_[
        static_cast<std::size_t>(stage)
    ]->bind(0);

    // Restore the complete world-depth state explicitly. The damage texture
    // is offset toward the camera only relative to the selected block; solid
    // terrain in front must still occlude it.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    glEnable(GL_BLEND);
    glBlendFuncSeparate(
        GL_DST_COLOR,
        GL_SRC_COLOR,
        GL_ONE,
        GL_ZERO
    );

    glEnable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(-3.0f, -3.0f);

    glBindVertexArray(vertexArray_);
    glDrawArrays(
        GL_TRIANGLES,
        0,
        vertexCount_
    );
    glBindVertexArray(0);

    glDisable(GL_POLYGON_OFFSET_FILL);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // Keep the normal terrain depth mode as the renderer-wide baseline.
    glDepthFunc(GL_LEQUAL);
}

void BlockDamageOverlay::uploadModel(
    mc::content::BlockState state,
    std::uint64_t positionSeed) const
{
    if (hasUploadedState_ &&
        uploadedState_ == state &&
        uploadedPositionSeed_ == positionSeed)
    {
        return;
    }

    std::vector<float> vertices;

    if (const mc::client::BakedModel* model =
            TextureAtlas::getBakedBlockModel(
                state,
                positionSeed
            ))
    {
        vertices.reserve(
            model->quads.size() * 6U * 5U
        );

        for (const mc::client::BakedQuad& quad :
             model->quads)
        {
            constexpr std::array<std::size_t, 6>
                indices{0, 1, 2, 0, 2, 3};

            for (const std::size_t index :
                 indices)
            {
                const glm::vec3& position =
                    quad.positions[index];

                const glm::vec2 uv =
                    quad.textureCoordinates[index] /
                    16.0f;

                vertices.insert(
                    vertices.end(),
                    {
                        position.x,
                        position.y,
                        position.z,
                        uv.x,
                        uv.y
                    }
                );
            }
        }
    }

    if (vertices.empty())
    {
        const auto appendFace =
            [&vertices](
                const glm::vec3& a,
                const glm::vec3& b,
                const glm::vec3& c,
                const glm::vec3& d)
        {
            const std::array<glm::vec3, 6>
                positions{
                    a, b, c,
                    a, c, d
                };

            constexpr std::array<glm::vec2, 6>
                uvs{{
                    {0, 0},
                    {1, 0},
                    {1, 1},
                    {0, 0},
                    {1, 1},
                    {0, 1}
                }};

            for (std::size_t i = 0;
                 i < positions.size();
                 ++i)
            {
                vertices.insert(
                    vertices.end(),
                    {
                        positions[i].x,
                        positions[i].y,
                        positions[i].z,
                        uvs[i].x,
                        uvs[i].y
                    }
                );
            }
        };

        for (const BlockBox& box :
             getBlockShape(state).selectionBoxes)
        {
            const glm::vec3& lo = box.minimum;
            const glm::vec3& hi = box.maximum;

            appendFace(
                {lo.x, lo.y, hi.z},
                {hi.x, lo.y, hi.z},
                {hi.x, hi.y, hi.z},
                {lo.x, hi.y, hi.z}
            );

            appendFace(
                {hi.x, lo.y, lo.z},
                {lo.x, lo.y, lo.z},
                {lo.x, hi.y, lo.z},
                {hi.x, hi.y, lo.z}
            );

            appendFace(
                {lo.x, lo.y, lo.z},
                {lo.x, lo.y, hi.z},
                {lo.x, hi.y, hi.z},
                {lo.x, hi.y, lo.z}
            );

            appendFace(
                {hi.x, lo.y, hi.z},
                {hi.x, lo.y, lo.z},
                {hi.x, hi.y, lo.z},
                {hi.x, hi.y, hi.z}
            );

            appendFace(
                {lo.x, hi.y, hi.z},
                {hi.x, hi.y, hi.z},
                {hi.x, hi.y, lo.z},
                {lo.x, hi.y, lo.z}
            );

            appendFace(
                {lo.x, lo.y, lo.z},
                {hi.x, lo.y, lo.z},
                {hi.x, lo.y, hi.z},
                {lo.x, lo.y, hi.z}
            );
        }
    }

    glBindBuffer(
        GL_ARRAY_BUFFER,
        vertexBuffer_
    );

    glBufferData(
        GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(
            vertices.size() * sizeof(float)
        ),
        vertices.data(),
        GL_DYNAMIC_DRAW
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );

    uploadedState_ = state;
    uploadedPositionSeed_ =
        positionSeed;
    hasUploadedState_ = true;

    vertexCount_ =
        static_cast<int>(
            vertices.size() / 5U
        );
}
