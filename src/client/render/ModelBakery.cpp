#include "client/render/ModelBakery.h"

#include "content/ContentCatalog.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string_view>
#include <unordered_map>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

namespace mc::client
{
namespace
{
using content::resources::BlockStateResource;
using content::resources::ModelElement;
using content::resources::ModelFace;
using content::resources::ModelVariant;
using content::resources::MultipartPart;
using content::resources::ResolvedModel;
using content::resources::StatePredicate;

std::unordered_map<std::string, std::string> propertyMap(
    const content::ContentCatalog& catalog,
    content::BlockState state)
{
    std::unordered_map<std::string, std::string> result;
    for (auto& [name, value] : catalog.serializeStateProperties(state))
        result.emplace(std::move(name), std::move(value));
    return result;
}

bool valueMatches(std::string_view expected, std::string_view actual)
{
    std::size_t begin = 0;
    do
    {
        const std::size_t end = expected.find('|', begin);
        const std::string_view candidate = expected.substr(
            begin,
            end == std::string_view::npos ? expected.size() - begin : end - begin
        );
        if (candidate == actual)
            return true;
        if (end == std::string_view::npos)
            break;
        begin = end + 1;
    } while (begin <= expected.size());
    return false;
}

bool predicateMatches(
    const StatePredicate& predicate,
    const std::unordered_map<std::string, std::string>& properties)
{
    for (const auto& [name, expected] : predicate.properties)
    {
        const auto found = properties.find(name);
        if (found == properties.end() || !valueMatches(expected, found->second))
            return false;
    }
    return true;
}

bool variantMatches(
    std::string_view key,
    const std::unordered_map<std::string, std::string>& properties)
{
    if (key.empty() || key == "normal")
        return true;
    std::size_t begin = 0;
    while (begin < key.size())
    {
        const std::size_t end = key.find(',', begin);
        const std::string_view part = key.substr(
            begin,
            end == std::string_view::npos ? key.size() - begin : end - begin
        );
        const std::size_t equals = part.find('=');
        if (equals == std::string_view::npos)
            return false;
        const auto found = properties.find(std::string(part.substr(0, equals)));
        if (found == properties.end() ||
            !valueMatches(part.substr(equals + 1), found->second))
            return false;
        if (end == std::string_view::npos)
            break;
        begin = end + 1;
    }
    return true;
}

const ModelVariant& chooseWeighted(
    const std::vector<ModelVariant>& variants,
    std::uint64_t seed)
{
    int totalWeight = 0;
    for (const ModelVariant& variant : variants)
        totalWeight += variant.weight;
    int choice = static_cast<int>(seed % static_cast<std::uint64_t>(totalWeight));
    for (const ModelVariant& variant : variants)
    {
        choice -= variant.weight;
        if (choice < 0)
            return variant;
    }
    return variants.back();
}

BlockFace faceFromName(std::string_view name)
{
    if (name == "north") return BlockFace::Back;
    if (name == "south") return BlockFace::Front;
    if (name == "west") return BlockFace::Left;
    if (name == "east") return BlockFace::Right;
    if (name == "down") return BlockFace::Bottom;
    return BlockFace::Top;
}

glm::vec3 normalFor(BlockFace face)
{
    switch (face)
    {
        case BlockFace::Back: return {0.0f, 0.0f, -1.0f};
        case BlockFace::Front: return {0.0f, 0.0f, 1.0f};
        case BlockFace::Left: return {-1.0f, 0.0f, 0.0f};
        case BlockFace::Right: return {1.0f, 0.0f, 0.0f};
        case BlockFace::Bottom: return {0.0f, -1.0f, 0.0f};
        case BlockFace::Top: return {0.0f, 1.0f, 0.0f};
    }
    return {0.0f, 1.0f, 0.0f};
}

BlockFace closestFace(const glm::vec3& normal)
{
    const glm::vec3 absolute = glm::abs(normal);
    if (absolute.x >= absolute.y && absolute.x >= absolute.z)
        return normal.x < 0.0f ? BlockFace::Left : BlockFace::Right;
    if (absolute.y >= absolute.z)
        return normal.y < 0.0f ? BlockFace::Bottom : BlockFace::Top;
    return normal.z < 0.0f ? BlockFace::Back : BlockFace::Front;
}

std::array<glm::vec3, 4> faceVertices(
    const ModelElement& element,
    BlockFace face)
{
    const glm::vec3 a(
        element.from[0] / 16.0f,
        element.from[1] / 16.0f,
        element.from[2] / 16.0f
    );
    const glm::vec3 b(
        element.to[0] / 16.0f,
        element.to[1] / 16.0f,
        element.to[2] / 16.0f
    );
    switch (face)
    {
        case BlockFace::Back: return {{{b.x,a.y,a.z},{a.x,a.y,a.z},{a.x,b.y,a.z},{b.x,b.y,a.z}}};
        case BlockFace::Front: return {{{a.x,a.y,b.z},{b.x,a.y,b.z},{b.x,b.y,b.z},{a.x,b.y,b.z}}};
        case BlockFace::Left: return {{{a.x,a.y,a.z},{a.x,a.y,b.z},{a.x,b.y,b.z},{a.x,b.y,a.z}}};
        case BlockFace::Right: return {{{b.x,a.y,b.z},{b.x,a.y,a.z},{b.x,b.y,a.z},{b.x,b.y,b.z}}};
        case BlockFace::Bottom: return {{{a.x,a.y,a.z},{b.x,a.y,a.z},{b.x,a.y,b.z},{a.x,a.y,b.z}}};
        case BlockFace::Top: return {{{a.x,b.y,b.z},{b.x,b.y,b.z},{b.x,b.y,a.z},{a.x,b.y,a.z}}};
    }
    return {};
}

std::array<float, 4> defaultUv(const ModelElement& element, BlockFace face)
{
    const auto& from = element.from;
    const auto& to = element.to;
    switch (face)
    {
        case BlockFace::Bottom: return {from[0], 16.0f-to[2], to[0], 16.0f-from[2]};
        case BlockFace::Top: return {from[0], from[2], to[0], to[2]};
        case BlockFace::Back: return {16.0f-to[0], 16.0f-to[1], 16.0f-from[0], 16.0f-from[1]};
        case BlockFace::Front: return {from[0], 16.0f-to[1], to[0], 16.0f-from[1]};
        case BlockFace::Left: return {from[2], 16.0f-to[1], to[2], 16.0f-from[1]};
        case BlockFace::Right: return {16.0f-to[2], 16.0f-to[1], 16.0f-from[2], 16.0f-from[1]};
    }
    return {0.0f, 0.0f, 16.0f, 16.0f};
}

std::array<glm::vec2, 4> faceUv(
    const ModelFace& face,
    const ModelElement& element,
    BlockFace direction,
    bool uvLock,
    const std::array<glm::vec3, 4>& transformedPositions,
    BlockFace transformedFace)
{
    const std::array<float, 4> uv = face.uv.value_or(defaultUv(element, direction));
    std::array<glm::vec2, 4> result{{
        {uv[0], 16.0f - uv[3]}, {uv[2], 16.0f - uv[3]},
        {uv[2], 16.0f - uv[1]}, {uv[0], 16.0f - uv[1]}
    }};
    int turns = (face.rotation / 90) % 4;
    if (uvLock)
    {
        // UV lock is face-local in 1.12. A single inverse of x+y rotation is
        // incorrect for vertical faces and for variants rotated on both axes.
        // Pick the quarter turn whose texture gradients align with the
        // canonical world-space axes of the transformed face. This is the
        // geometric equivalent of FaceBakery.applyUVLock and also works for
        // modded element sizes and element rotations.
        ModelElement unit;
        unit.from = {0.0f, 0.0f, 0.0f};
        unit.to = {16.0f, 16.0f, 16.0f};
        const auto canonical = faceVertices(unit, transformedFace);
        const glm::vec3 canonicalU = glm::normalize(canonical[1] - canonical[0]);
        const glm::vec3 canonicalV = glm::normalize(canonical[3] - canonical[0]);
        constexpr std::array<glm::vec2, 4> basis{{
            {0, 0}, {1, 0}, {1, 1}, {0, 1}
        }};
        float bestScore = -std::numeric_limits<float>::max();
        int correction = 0;
        for (int candidate = 0; candidate < 4; ++candidate)
        {
            glm::vec3 gradientU(0.0f);
            glm::vec3 gradientV(0.0f);
            for (std::size_t vertex = 0; vertex < 4; ++vertex)
            {
                const glm::vec2 coordinate =
                    basis[(vertex + static_cast<std::size_t>(candidate)) % 4U];
                gradientU += transformedPositions[vertex] * (coordinate.x - 0.5f);
                gradientV += transformedPositions[vertex] * (coordinate.y - 0.5f);
            }
            if (glm::length(gradientU) < 0.00001f ||
                glm::length(gradientV) < 0.00001f)
                continue;
            const float score =
                glm::dot(glm::normalize(gradientU), canonicalU) +
                glm::dot(glm::normalize(gradientV), canonicalV);
            if (score > bestScore)
            {
                bestScore = score;
                correction = candidate;
            }
        }
        turns = (turns + correction) % 4;
    }
    std::rotate(result.begin(), result.begin() + turns, result.end());
    return result;
}

glm::mat4 elementTransform(const ModelElement& element)
{
    glm::mat4 transform(1.0f);
    if (!element.rotation)
        return transform;
    const glm::vec3 origin(
        element.rotation->origin[0] / 16.0f,
        element.rotation->origin[1] / 16.0f,
        element.rotation->origin[2] / 16.0f
    );
    glm::vec3 axis(0.0f);
    if (element.rotation->axis == "x") axis.x = 1.0f;
    else if (element.rotation->axis == "y") axis.y = 1.0f;
    else axis.z = 1.0f;
    transform = glm::translate(transform, origin);
    transform = glm::rotate(transform, glm::radians(element.rotation->angle), axis);
    if (element.rotation->rescale)
    {
        const float scale = 1.0f / std::cos(glm::radians(element.rotation->angle));
        glm::vec3 factors(scale);
        if (axis.x != 0.0f) factors.x = 1.0f;
        if (axis.y != 0.0f) factors.y = 1.0f;
        if (axis.z != 0.0f) factors.z = 1.0f;
        transform = glm::scale(transform, factors);
    }
    return glm::translate(transform, -origin);
}

glm::mat4 variantTransform(const ModelVariant& variant)
{
    glm::mat4 transform(1.0f);
    transform = glm::translate(transform, glm::vec3(0.5f));
    transform = glm::rotate(transform, glm::radians(static_cast<float>(variant.rotationY)), {0,1,0});
    transform = glm::rotate(transform, glm::radians(static_cast<float>(variant.rotationX)), {1,0,0});
    return glm::translate(transform, glm::vec3(-0.5f));
}

void appendVariant(
    BakedModel& result,
    const ModelVariant& variant,
    const content::resources::ResourcePack& resources)
{
    const ResolvedModel model = resources.resolveModel(variant.model);
    result.ambientOcclusion = result.ambientOcclusion && model.ambientOcclusion;
    result.display.insert(model.display.begin(), model.display.end());
    const glm::mat4 variantMatrix = variantTransform(variant);
    for (const ModelElement& element : model.elements)
    {
        const glm::mat4 transform = variantMatrix * elementTransform(element);
        const glm::vec3 from(
            element.from[0] / 16.0f,
            element.from[1] / 16.0f,
            element.from[2] / 16.0f
        );
        const glm::vec3 to(
            element.to[0] / 16.0f,
            element.to[1] / 16.0f,
            element.to[2] / 16.0f
        );
        glm::vec3 minimum(std::numeric_limits<float>::max());
        glm::vec3 maximum(std::numeric_limits<float>::lowest());
        for (int corner = 0; corner < 8; ++corner)
        {
            const glm::vec3 local(
                (corner & 1) != 0 ? to.x : from.x,
                (corner & 2) != 0 ? to.y : from.y,
                (corner & 4) != 0 ? to.z : from.z
            );
            const glm::vec3 transformed = glm::vec3(
                transform * glm::vec4(local, 1.0f)
            );
            minimum = glm::min(minimum, transformed);
            maximum = glm::max(maximum, transformed);
        }
        constexpr float boxEpsilon = 0.00001f;
        if (maximum.x - minimum.x > boxEpsilon &&
            maximum.y - minimum.y > boxEpsilon &&
            maximum.z - minimum.z > boxEpsilon)
        {
            result.elementBoxes.push_back({minimum, maximum});
        }
        for (const auto& [faceName, modelFace] : element.faces)
        {
            const BlockFace originalFace = faceFromName(faceName);
            BakedQuad quad;
            quad.positions = faceVertices(element, originalFace);
            for (glm::vec3& position : quad.positions)
                position = glm::vec3(transform * glm::vec4(position, 1.0f));
            const glm::vec3 transformedNormal = glm::normalize(
                glm::mat3(transform) * normalFor(originalFace)
            );
            quad.face = closestFace(transformedNormal);
            if (modelFace.cullFace)
            {
                const glm::vec3 cullNormal = glm::normalize(
                    glm::mat3(variantMatrix) * normalFor(
                        faceFromName(*modelFace.cullFace)
                    )
                );
                quad.cullFace = closestFace(cullNormal);
            }
            quad.textureCoordinates = faceUv(
                modelFace, element, originalFace, variant.uvLock,
                quad.positions, quad.face
            );
            quad.texture = resources.resolveTexture(modelFace.texture, model);
            quad.tintIndex = modelFace.tintIndex;
            quad.shade = element.shade;
            result.quads.push_back(std::move(quad));
        }
    }
}
}

ModelBakery::ModelBakery(
    const content::resources::ResourcePack& resources,
    const content::ContentCatalog& content)
    : resources_(resources), content_(content)
{
}

BakedModel ModelBakery::bakeModel(
    const core::ResourceLocation& modelName) const
{
    BakedModel result;
    ModelVariant variant;
    variant.model = modelName;
    appendVariant(result, variant, resources_);
    return result;
}

BakedModel ModelBakery::bake(
    content::BlockState state,
    std::uint64_t positionSeed) const
{
    const core::ResourceLocation* name = content_.blockName(state);
    if (name == nullptr)
        return {};
    const BlockStateResource blockState = resources_.loadBlockState(*name);
    const auto properties = propertyMap(content_, state);
    BakedModel result;

    for (const auto& [key, variants] : blockState.variants)
    {
        if (variantMatches(key, properties))
            appendVariant(result, chooseWeighted(variants, positionSeed), resources_);
    }
    for (const MultipartPart& part : blockState.multipart)
    {
        bool matches = part.alternatives.empty();
        for (const StatePredicate& alternative : part.alternatives)
            matches = matches || predicateMatches(alternative, properties);
        if (matches)
            appendVariant(result, chooseWeighted(part.apply, positionSeed), resources_);
    }
    return result;
}
}
