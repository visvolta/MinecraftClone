#include "BlockOutline.h"

#include "BlockShape.h"
#include "BlockModelSeed.h"
#include "TextureAtlas.h"
#include "client/render/ModelBakery.h"

#include <glad/gl.h>

#include <array>
#include <vector>

BlockOutline::BlockOutline()
{
    glGenVertexArrays(
        1,
        &vertexArray_
    );

    glGenBuffers(
        1,
        &vertexBuffer_
    );

    glBindVertexArray(
        vertexArray_
    );

    glBindBuffer(
        GL_ARRAY_BUFFER,
        vertexBuffer_
    );

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
        3 * sizeof(float),
        nullptr
    );

    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    glBindBuffer(
        GL_ARRAY_BUFFER,
        0
    );
}

BlockOutline::~BlockOutline()
{
    if (vertexBuffer_ != 0)
        glDeleteBuffers(
            1,
            &vertexBuffer_
        );

    if (vertexArray_ != 0)
        glDeleteVertexArrays(
            1,
            &vertexArray_
        );
}

void BlockOutline::draw(
    mc::content::BlockState state,
    const glm::ivec3& blockPosition) const
{
    const std::uint64_t positionSeed =
        blockModelSeed(blockPosition);

    if (!hasUploadedState_ ||
        uploadedState_ != state ||
        uploadedPositionSeed_ != positionSeed)
    {
        std::vector<glm::vec3> vertices;

        const auto appendQuad =
            [&vertices](
                const std::array<glm::vec3, 4>& quad)
        {
            constexpr std::array<
                std::size_t,
                8
            > edges{
                0,1, 1,2,
                2,3, 3,0
            };

            for (const std::size_t index : edges)
            {
                const glm::vec3 centred =
                    quad[index] -
                    glm::vec3(0.5f);

                vertices.push_back(
                    glm::vec3(0.5f) +
                    centred * 1.004f
                );
            }
        };

        if (const mc::client::BakedModel* model =
                TextureAtlas::getBakedBlockModel(
                    state,
                    positionSeed
                ))
        {
            vertices.reserve(
                model->quads.size() * 8U
            );

            for (const mc::client::BakedQuad& quad :
                 model->quads)
            {
                appendQuad(
                    quad.positions
                );
            }
        }

        if (vertices.empty())
        {
            for (const BlockBox& box :
                 getBlockShape(state).selectionBoxes)
            {
                const glm::vec3 lo =
                    box.minimum -
                    glm::vec3(0.002f);

                const glm::vec3 hi =
                    box.maximum +
                    glm::vec3(0.002f);

                const std::array<
                    glm::vec3,
                    8
                > corners{{
                    {lo.x,lo.y,lo.z},
                    {hi.x,lo.y,lo.z},
                    {hi.x,lo.y,hi.z},
                    {lo.x,lo.y,hi.z},
                    {lo.x,hi.y,lo.z},
                    {hi.x,hi.y,lo.z},
                    {hi.x,hi.y,hi.z},
                    {lo.x,hi.y,hi.z}
                }};

                constexpr std::array<
                    std::size_t,
                    24
                > edges{
                    0,1,1,2,2,3,3,0,
                    4,5,5,6,6,7,7,4,
                    0,4,1,5,2,6,3,7
                };

                for (const std::size_t index :
                     edges)
                {
                    vertices.push_back(
                        corners[index]
                    );
                }
            }
        }

        glBindBuffer(
            GL_ARRAY_BUFFER,
            vertexBuffer_
        );

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                vertices.size() *
                sizeof(glm::vec3)
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
            static_cast<GLsizei>(
                vertices.size()
            );
    }

    // Explicit depth state prevents entity/UI renderers from accidentally
    // turning the selection outline into an x-ray overlay.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glDepthMask(GL_FALSE);

    glBindVertexArray(
        vertexArray_
    );

    glDrawArrays(
        GL_LINES,
        0,
        vertexCount_
    );

    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
}
