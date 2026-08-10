#ifndef MESH_H
#define MESH_H

#include <cstddef>
#include <cstdint>
#include <vector>

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
    std::uint32_t vertexArray_ = 0;
    std::uint32_t vertexBuffer_ = 0;
    std::uint32_t indexBuffer_ = 0;
    std::int32_t indexCount_ = 0;
    std::ptrdiff_t vertexCapacity_ = 0;
    std::ptrdiff_t indexCapacity_ = 0;

    void release();
};

#endif
