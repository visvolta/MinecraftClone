#pragma once

#include "Item.h"

#include <glad/gl.h>
#include <glm/glm.hpp>

#include <map>
#include <memory>
#include <vector>

class ItemEntity;
class ItemAtlas;
struct AtmosphereState;
class Shader;
class Texture2D;

class ItemEntityRenderer
{
public:
    ItemEntityRenderer();
    ~ItemEntityRenderer();

    ItemEntityRenderer(const ItemEntityRenderer&) = delete;
    ItemEntityRenderer& operator=(const ItemEntityRenderer&) = delete;

    void draw(
        const ItemEntity& entity,
        float partialTick,
        const glm::mat4& view,
        const glm::mat4& projection,
        const Texture2D& blockAtlas,
        const ItemAtlas& itemAtlas,
        const AtmosphereState& atmosphere
    );

private:
    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 uv;
        glm::vec3 colour;
    };

    struct GpuMesh
    {
        GLuint vertexArray = 0;
        GLuint vertexBuffer = 0;
        GLsizei vertexCount = 0;
    };

    std::unique_ptr<Shader> shader_;
    std::map<ItemType, GpuMesh> meshes_;

    [[nodiscard]] const GpuMesh& meshFor(
        ItemType item,
        const ItemAtlas& itemAtlas
    );
    [[nodiscard]] static std::vector<Vertex> buildVertices(
        ItemType item,
        const ItemAtlas& itemAtlas
    );
    static void upload(GpuMesh& mesh, const std::vector<Vertex>& vertices);
};
