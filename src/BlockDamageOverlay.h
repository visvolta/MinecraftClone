#pragma once
#include <array>
#include <memory>

#include <glm/glm.hpp>

class Shader;
class Texture2D;

class BlockDamageOverlay
{
public:
    BlockDamageOverlay();
    ~BlockDamageOverlay();

    BlockDamageOverlay(const BlockDamageOverlay&) = delete;
    BlockDamageOverlay& operator=(const BlockDamageOverlay&) = delete;

    void draw(
        const glm::ivec3& blockPosition,
        int stage,
        const glm::mat4& view,
        const glm::mat4& projection
    ) const;

private:
    unsigned int vertexArray_ = 0;
    unsigned int vertexBuffer_ = 0;
    std::unique_ptr<Shader> shader_;
    std::array<std::unique_ptr<Texture2D>, 10> textures_;
};
