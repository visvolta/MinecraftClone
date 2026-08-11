#pragma once
#include <array>
#include <cstdint>
#include <memory>

#include <glm/glm.hpp>

#include "content/BlockState.h"

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
        mc::content::BlockState state,
        int stage,
        const glm::mat4& view,
        const glm::mat4& projection
    ) const;

private:
    void uploadModel(
        mc::content::BlockState state,
        std::uint64_t positionSeed
    ) const;

    unsigned int vertexArray_ = 0;
    unsigned int vertexBuffer_ = 0;
    mutable mc::content::BlockState uploadedState_{};
    mutable int vertexCount_ = 0;
    mutable std::uint64_t uploadedPositionSeed_ = 0;
    mutable bool hasUploadedState_ = false;
    std::unique_ptr<Shader> shader_;
    std::array<std::unique_ptr<Texture2D>, 10> textures_;
};
