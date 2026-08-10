#pragma once

#include <glad/gl.h>

class BlockOutline
{
public:
    BlockOutline();
    ~BlockOutline();

    BlockOutline(const BlockOutline&) = delete;
    BlockOutline& operator=(const BlockOutline&) = delete;

    BlockOutline(BlockOutline&&) = delete;
    BlockOutline& operator=(BlockOutline&&) = delete;

    void draw() const;

private:
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
};
