#ifndef MESH_H
#define MESH_H

#include <cstdint>
#include <vector>

#include <glad/gl.h>

#include "ChunkVertex.h"

class Mesh
{
public:
    Mesh(
        const std::vector<ChunkVertex>& vertices,
        const std::vector<std::uint32_t>& indices
    );
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(
        const std::vector<ChunkVertex>& vertices,
        const std::vector<std::uint32_t>& indices
    );
    void draw() const;
    void drawRange(std::uint32_t firstIndex, std::uint32_t indexCount) const;

private:
    GLuint vertexArray_ = 0;
    GLuint vertexBuffer_ = 0;
    GLuint indexBuffer_ = 0;
    GLsizei indexCount_ = 0;
    GLsizeiptr vertexCapacity_ = 0;
    GLsizeiptr indexCapacity_ = 0;

    void release();
};

#endif
