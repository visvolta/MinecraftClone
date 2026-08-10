#pragma once

#include "core/ResourceLocation.h"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc::content::resources
{
struct ModelVariant
{
    core::ResourceLocation model;
    int rotationX = 0;
    int rotationY = 0;
    int weight = 1;
    bool uvLock = false;
};

struct StatePredicate
{
    std::unordered_map<std::string, std::string> properties;
};

struct MultipartPart
{
    std::vector<StatePredicate> alternatives;
    std::vector<ModelVariant> apply;
};

struct BlockStateResource
{
    std::unordered_map<std::string, std::vector<ModelVariant>> variants;
    std::vector<MultipartPart> multipart;
};

struct ModelFace
{
    std::string texture;
    std::optional<std::string> cullFace;
    std::optional<std::array<float, 4>> uv;
    int tintIndex = -1;
    int rotation = 0;
};

struct ElementRotation
{
    std::array<float, 3> origin{8.0f, 8.0f, 8.0f};
    std::string axis;
    float angle = 0.0f;
    bool rescale = false;
};

struct ModelElement
{
    std::array<float, 3> from{0.0f, 0.0f, 0.0f};
    std::array<float, 3> to{16.0f, 16.0f, 16.0f};
    std::unordered_map<std::string, ModelFace> faces;
    std::optional<ElementRotation> rotation;
    bool shade = true;
};

struct DisplayTransform
{
    std::array<float, 3> rotation{0.0f, 0.0f, 0.0f};
    std::array<float, 3> translation{0.0f, 0.0f, 0.0f};
    std::array<float, 3> scale{1.0f, 1.0f, 1.0f};
};

struct ModelResource
{
    std::optional<core::ResourceLocation> parent;
    std::unordered_map<std::string, std::string> textures;
    std::vector<ModelElement> elements;
    std::unordered_map<std::string, DisplayTransform> display;
    std::optional<bool> ambientOcclusion;
};

struct ResolvedModel
{
    std::unordered_map<std::string, std::string> textures;
    std::vector<ModelElement> elements;
    std::unordered_map<std::string, DisplayTransform> display;
    bool ambientOcclusion = true;
};

class ResourcePack
{
public:
    explicit ResourcePack(std::filesystem::path assetRoot);

    [[nodiscard]] BlockStateResource loadBlockState(
        const core::ResourceLocation& name
    ) const;
    [[nodiscard]] ModelResource loadModel(
        const core::ResourceLocation& name
    ) const;
    [[nodiscard]] ResolvedModel resolveModel(
        const core::ResourceLocation& name
    ) const;
    [[nodiscard]] std::unordered_set<
        core::ResourceLocation,
        core::ResourceLocationHash
    > textureDependencies(const ResolvedModel& model) const;
    [[nodiscard]] core::ResourceLocation resolveTexture(
        std::string textureReference,
        const ResolvedModel& model
    ) const;

private:
    std::filesystem::path assetRoot_;

    [[nodiscard]] ResolvedModel resolveModel(
        const core::ResourceLocation& name,
        std::unordered_set<
            core::ResourceLocation,
            core::ResourceLocationHash
        >& resolving
    ) const;
};
}
