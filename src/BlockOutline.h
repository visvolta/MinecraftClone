#pragma once

#include <glad/gl.h>
#include <cstdint>
#include <glm/vec3.hpp>

#include "content/BlockState.h"

class BlockOutline
{
public:
    BlockOutline();
    ~BlockOutline();

    BlockOutline(const BlockOutline&) = delete;
    BlockOutline& operator=(const BlockOutline&) = delete;

    BlockOutline(BlockOutline&&) = delete;
    BlockOutline& operator=(BlockOutline&&) = delete;

    void draw(
        mc::content::BlockState state,
        const glm::ivec3& blockPosition
    ) const;

private:
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    mutable mc::content::BlockState uploadedState_{};
    mutable GLsizei vertexCount_ = 0;
    mutable std::uint64_t uploadedPositionSeed_ = 0;
    mutable bool hasUploadedState_ = false;
};
