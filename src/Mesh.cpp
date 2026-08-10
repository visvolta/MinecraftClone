#include "Mesh.h"

#include <glad/gl.h>

#include <algorithm>
#include <cstddef>
#include <utility>

namespace
{
GLsizeiptr uploadCapacity(GLsizeiptr required, GLsizeiptr current)
{
    if (required <= current)
        return current;
    if (current == 0)
        return std::max<GLsizeiptr>(required, 4096);
    return std::max(required, current + current / 2);
}
}

Mesh::Mesh(
    const std::vector<ChunkVertex>& vertices,
    const std::vector<std::uint32_t>& indices)
{
    glGenVertexArrays(1, &vertexArray_);
    glGenBuffers(1, &vertexBuffer_);
    glGenBuffers(1, &indexBuffer_);

    glBindVertexArray(vertexArray_);

    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);

    constexpr GLsizei stride = static_cast<GLsizei>(sizeof(ChunkVertex));

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        stride,
        reinterpret_cast<void*>(offsetof(ChunkVertex, positionX))
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1,
        2,
        GL_UNSIGNED_SHORT,
        GL_TRUE,
        stride,
        reinterpret_cast<void*>(offsetof(ChunkVertex, textureU))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        3,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        stride,
        reinterpret_cast<void*>(offsetof(ChunkVertex, tintRed))
    );
    glEnableVertexAttribArray(2);


    glVertexAttribPointer(
        3,
        2,
        GL_UNSIGNED_SHORT,
        GL_TRUE,
        stride,
        reinterpret_cast<void*>(offsetof(ChunkVertex, overlayU))
    );
    glEnableVertexAttribArray(3);

    glVertexAttribPointer(
        4,
        3,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        stride,
        reinterpret_cast<void*>(offsetof(ChunkVertex, overlayTintRed))
    );
    glEnableVertexAttribArray(4);

    glVertexAttribPointer(
        5,
        1,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        stride,
        reinterpret_cast<void*>(offsetof(ChunkVertex, hasOverlay))
    );
    glEnableVertexAttribArray(5);

    glVertexAttribPointer(
        6,
        1,
        GL_UNSIGNED_BYTE,
        GL_TRUE,
        stride,
        reinterpret_cast<void*>(offsetof(ChunkVertex, isLeaf))
    );
    glEnableVertexAttribArray(6);


    glVertexAttribPointer(
        7, 1, GL_UNSIGNED_BYTE, GL_FALSE, stride,
        reinterpret_cast<void*>(offsetof(ChunkVertex, materialTexture))
    );
    glEnableVertexAttribArray(7);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    upload(vertices, indices);
}

Mesh::~Mesh()
{
    release();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vertexArray_(std::exchange(other.vertexArray_, 0)),
      vertexBuffer_(std::exchange(other.vertexBuffer_, 0)),
      indexBuffer_(std::exchange(other.indexBuffer_, 0)),
      indexCount_(std::exchange(other.indexCount_, 0)),
      vertexCapacity_(std::exchange(other.vertexCapacity_, 0)),
      indexCapacity_(std::exchange(other.indexCapacity_, 0))
{
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this != &other)
    {
        release();
        vertexArray_ = std::exchange(other.vertexArray_, 0);
        vertexBuffer_ = std::exchange(other.vertexBuffer_, 0);
        indexBuffer_ = std::exchange(other.indexBuffer_, 0);
        indexCount_ = std::exchange(other.indexCount_, 0);
        vertexCapacity_ = std::exchange(other.vertexCapacity_, 0);
        indexCapacity_ = std::exchange(other.indexCapacity_, 0);
    }

    return *this;
}

void Mesh::upload(
    const std::vector<ChunkVertex>& vertices,
    const std::vector<std::uint32_t>& indices)
{
    indexCount_ = static_cast<GLsizei>(indices.size());
    const GLsizeiptr vertexBytes = static_cast<GLsizeiptr>(
        vertices.size() * sizeof(ChunkVertex)
    );
    const GLsizeiptr indexBytes = static_cast<GLsizeiptr>(
        indices.size() * sizeof(std::uint32_t)
    );
    vertexCapacity_ = uploadCapacity(vertexBytes, vertexCapacity_);
    indexCapacity_ = uploadCapacity(indexBytes, indexCapacity_);

    glBindVertexArray(vertexArray_);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertexCapacity_,
        nullptr,
        GL_DYNAMIC_DRAW
    );
    if (vertexBytes > 0)
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexBytes, vertices.data());
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        indexCapacity_,
        nullptr,
        GL_DYNAMIC_DRAW
    );
    if (indexBytes > 0)
        glBufferSubData(
            GL_ELEMENT_ARRAY_BUFFER, 0, indexBytes, indices.data()
        );
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Mesh::draw() const
{
    glBindVertexArray(vertexArray_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
}

void Mesh::drawRange(
    std::uint32_t firstIndex,
    std::uint32_t indexCount) const
{
    if (indexCount == 0)
        return;
    glBindVertexArray(vertexArray_);
    glDrawElements(
        GL_TRIANGLES,
        static_cast<GLsizei>(indexCount),
        GL_UNSIGNED_INT,
        reinterpret_cast<const void*>(
            static_cast<std::uintptr_t>(firstIndex) * sizeof(std::uint32_t)
        )
    );
}

void Mesh::release()
{
    if (indexBuffer_ != 0)
        glDeleteBuffers(1, &indexBuffer_);
    if (vertexBuffer_ != 0)
        glDeleteBuffers(1, &vertexBuffer_);
    if (vertexArray_ != 0)
        glDeleteVertexArrays(1, &vertexArray_);

    vertexArray_ = 0;
    vertexBuffer_ = 0;
    indexBuffer_ = 0;
    indexCount_ = 0;
    vertexCapacity_ = 0;
    indexCapacity_ = 0;
}
