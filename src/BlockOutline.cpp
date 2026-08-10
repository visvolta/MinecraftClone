#include "BlockOutline.h"

BlockOutline::BlockOutline()
{
    // A slightly enlarged unit cube centred on the origin. The small expansion
    // keeps the black lines from fighting with the block surface depth values.
    constexpr float minimum = -0.502f;
    constexpr float maximum =  0.502f;

    constexpr float vertices[] =
    {
        // Bottom square
        minimum, minimum, minimum,  maximum, minimum, minimum,
        maximum, minimum, minimum,  maximum, minimum, maximum,
        maximum, minimum, maximum,  minimum, minimum, maximum,
        minimum, minimum, maximum,  minimum, minimum, minimum,

        // Top square
        minimum, maximum, minimum,  maximum, maximum, minimum,
        maximum, maximum, minimum,  maximum, maximum, maximum,
        maximum, maximum, maximum,  minimum, maximum, maximum,
        minimum, maximum, maximum,  minimum, maximum, minimum,

        // Vertical edges
        minimum, minimum, minimum,  minimum, maximum, minimum,
        maximum, minimum, minimum,  maximum, maximum, minimum,
        maximum, minimum, maximum,  maximum, maximum, maximum,
        minimum, minimum, maximum,  minimum, maximum, maximum
    };

    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);

    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

BlockOutline::~BlockOutline()
{
    if (vertexBuffer_ != 0)
        glDeleteBuffers(1, &vertexBuffer_);
    if (vertexArray_ != 0)
        glDeleteVertexArrays(1, &vertexArray_);
}

void BlockOutline::draw() const
{
    glBindVertexArray(vertexArray_);
    glDrawArrays(GL_LINES, 0, 24);
    glBindVertexArray(0);
}
