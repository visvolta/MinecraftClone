#include "content/resources/ResourcePack.h"

#include <fstream>
#include <cmath>
#include <stdexcept>
#include <string_view>

#include <nlohmann/json.hpp>

namespace mc::content::resources
{
namespace
{
using Json = nlohmann::json;

Json readJson(const std::filesystem::path& path)
{
    std::ifstream stream(path);
    if (!stream)
        throw std::runtime_error("Missing resource JSON: " + path.string());
    try
    {
        return Json::parse(stream);
    }
    catch (const Json::exception& error)
    {
        throw std::runtime_error(
            "Invalid resource JSON " + path.string() + ": " + error.what()
        );
    }
}

core::ResourceLocation reference(
    std::string_view text,
    std::string_view defaultFolder)
{
    core::ResourceLocation parsed(text);
    if (parsed.path().find('/') != std::string::npos || defaultFolder.empty())
        return parsed;
    return core::ResourceLocation(
        parsed.nameSpace(),
        std::string(defaultFolder) + "/" + parsed.path()
    );
}

std::filesystem::path namespaceRoot(
    const std::filesystem::path& root,
    const core::ResourceLocation& name)
{
    return root / name.nameSpace();
}

ModelVariant parseVariant(const Json& json)
{
    if (!json.is_object() || !json.contains("model"))
        throw std::runtime_error("Model variant is missing its model");
    ModelVariant result;
    result.model = reference(json.at("model").get<std::string>(), "block");
    result.rotationX = json.value("x", 0);
    result.rotationY = json.value("y", 0);
    result.weight = json.value("weight", 1);
    result.uvLock = json.value("uvlock", false);
    if (result.weight <= 0 || result.rotationX % 90 != 0 ||
        result.rotationY % 90 != 0)
        throw std::runtime_error("Model variant has invalid rotation or weight");
    return result;
}

std::vector<ModelVariant> parseVariants(const Json& json)
{
    std::vector<ModelVariant> result;
    if (json.is_array())
    {
        result.reserve(json.size());
        for (const Json& value : json)
            result.push_back(parseVariant(value));
    }
    else
    {
        result.push_back(parseVariant(json));
    }
    return result;
}

StatePredicate parsePredicate(const Json& json)
{
    if (!json.is_object())
        throw std::runtime_error("Multipart condition must be an object");
    StatePredicate result;
    for (auto entry = json.begin(); entry != json.end(); ++entry)
    {
        if (!entry.value().is_string())
            throw std::runtime_error("Multipart property must be a string");
        result.properties.emplace(entry.key(), entry.value().get<std::string>());
    }
    return result;
}

std::vector<StatePredicate> parseCondition(const Json& json)
{
    if (json.empty())
        return {};
    if (json.contains("OR"))
    {
        std::vector<StatePredicate> result;
        for (const Json& alternative : json.at("OR"))
            result.push_back(parsePredicate(alternative));
        return result;
    }
    if (json.contains("AND"))
    {
        StatePredicate combined;
        for (const Json& condition : json.at("AND"))
        {
            const StatePredicate part = parsePredicate(condition);
            combined.properties.insert(
                part.properties.begin(), part.properties.end()
            );
        }
        return {std::move(combined)};
    }
    return {parsePredicate(json)};
}

template<std::size_t Size>
std::array<float, Size> floatArray(const Json& json, const char* field)
{
    if (!json.contains(field) || !json.at(field).is_array() ||
        json.at(field).size() != Size)
        throw std::runtime_error(std::string("Model element has invalid ") + field);
    std::array<float, Size> result{};
    for (std::size_t index = 0; index < Size; ++index)
        result[index] = json.at(field).at(index).get<float>();
    return result;
}

std::string resolveTextureValue(
    std::string value,
    const std::unordered_map<std::string, std::string>& textures)
{
    std::unordered_set<std::string> visited;
    while (!value.empty() && value.front() == '#')
    {
        const std::string variable = value.substr(1);
        if (!visited.insert(variable).second)
            throw std::runtime_error("Cyclic model texture reference");
        const auto found = textures.find(variable);
        if (found == textures.end())
            throw std::runtime_error("Unknown model texture variable: " + variable);
        value = found->second;
    }
    return value;
}

DisplayTransform parseDisplayTransform(const Json& json)
{
    if (!json.is_object())
        throw std::runtime_error("Model display transform must be an object");
    DisplayTransform result;
    if (json.contains("rotation"))
        result.rotation = floatArray<3>(json, "rotation");
    if (json.contains("translation"))
        result.translation = floatArray<3>(json, "translation");
    if (json.contains("scale"))
        result.scale = floatArray<3>(json, "scale");
    return result;
}

ElementRotation parseElementRotation(const Json& json)
{
    if (!json.is_object())
        throw std::runtime_error("Model element rotation must be an object");
    ElementRotation result;
    result.origin = floatArray<3>(json, "origin");
    result.axis = json.at("axis").get<std::string>();
    result.angle = json.at("angle").get<float>();
    result.rescale = json.value("rescale", false);
    if ((result.axis != "x" && result.axis != "y" && result.axis != "z") ||
        !std::isfinite(result.angle))
    {
        throw std::runtime_error("Model element has an invalid rotation");
    }
    return result;
}
}

ResourcePack::ResourcePack(std::filesystem::path assetRoot)
    : assetRoot_(std::move(assetRoot))
{
}

BlockStateResource ResourcePack::loadBlockState(
    const core::ResourceLocation& name) const
{
    const Json json = readJson(
        namespaceRoot(assetRoot_, name) / "blockstates" /
        (name.path() + ".json")
    );
    BlockStateResource result;
    if (json.contains("variants"))
    {
        for (auto entry = json.at("variants").begin();
             entry != json.at("variants").end(); ++entry)
            result.variants.emplace(entry.key(), parseVariants(entry.value()));
    }
    if (json.contains("multipart"))
    {
        for (const Json& part : json.at("multipart"))
        {
            MultipartPart parsed;
            if (part.contains("when"))
                parsed.alternatives = parseCondition(part.at("when"));
            parsed.apply = parseVariants(part.at("apply"));
            result.multipart.push_back(std::move(parsed));
        }
    }
    if (result.variants.empty() && result.multipart.empty())
        throw std::runtime_error("Blockstate has no variants or multipart rules");
    return result;
}

ModelResource ResourcePack::loadModel(
    const core::ResourceLocation& name) const
{
    const std::string& path = name.path();
    const Json json = readJson(
        namespaceRoot(assetRoot_, name) / "models" / (path + ".json")
    );
    ModelResource result;
    if (json.contains("parent"))
        result.parent = reference(json.at("parent").get<std::string>(), "block");
    if (json.contains("ambientocclusion"))
        result.ambientOcclusion = json.at("ambientocclusion").get<bool>();
    if (json.contains("textures"))
    {
        for (auto entry = json.at("textures").begin();
             entry != json.at("textures").end(); ++entry)
            result.textures.emplace(entry.key(), entry.value().get<std::string>());
    }
    if (json.contains("elements"))
    {
        for (const Json& elementJson : json.at("elements"))
        {
            ModelElement element;
            element.from = floatArray<3>(elementJson, "from");
            element.to = floatArray<3>(elementJson, "to");
            element.shade = elementJson.value("shade", true);
            if (elementJson.contains("rotation"))
                element.rotation = parseElementRotation(elementJson.at("rotation"));
            for (auto face = elementJson.at("faces").begin();
                 face != elementJson.at("faces").end(); ++face)
            {
                ModelFace parsed;
                parsed.texture = face.value().at("texture").get<std::string>();
                if (face.value().contains("cullface"))
                    parsed.cullFace = face.value().at("cullface").get<std::string>();
                if (face.value().contains("uv"))
                    parsed.uv = floatArray<4>(face.value(), "uv");
                parsed.tintIndex = face.value().value("tintindex", -1);
                parsed.rotation = face.value().value("rotation", 0);
                if (parsed.rotation != 0 && parsed.rotation != 90 &&
                    parsed.rotation != 180 && parsed.rotation != 270)
                {
                    throw std::runtime_error("Model face has an invalid rotation");
                }
                element.faces.emplace(face.key(), std::move(parsed));
            }
            result.elements.push_back(std::move(element));
        }
    }
    if (json.contains("display"))
    {
        for (auto entry = json.at("display").begin();
             entry != json.at("display").end(); ++entry)
        {
            result.display.emplace(
                entry.key(),
                parseDisplayTransform(entry.value())
            );
        }
    }
    return result;
}

ResolvedModel ResourcePack::resolveModel(
    const core::ResourceLocation& name) const
{
    std::unordered_set<core::ResourceLocation, core::ResourceLocationHash> resolving;
    return resolveModel(name, resolving);
}

ResolvedModel ResourcePack::resolveModel(
    const core::ResourceLocation& name,
    std::unordered_set<core::ResourceLocation, core::ResourceLocationHash>& resolving) const
{
    if (!resolving.insert(name).second)
        throw std::runtime_error("Cyclic model parent: " + name.toString());
    const ModelResource model = loadModel(name);
    ResolvedModel result;
    if (model.parent)
        result = resolveModel(*model.parent, resolving);
    for (const auto& [key, value] : model.textures)
        result.textures[key] = value;
    if (!model.elements.empty())
        result.elements = model.elements;
    for (const auto& [key, value] : model.display)
        result.display[key] = value;
    if (model.ambientOcclusion)
        result.ambientOcclusion = *model.ambientOcclusion;
    resolving.erase(name);
    return result;
}

std::unordered_set<core::ResourceLocation, core::ResourceLocationHash>
ResourcePack::textureDependencies(const ResolvedModel& model) const
{
    std::unordered_set<core::ResourceLocation, core::ResourceLocationHash> result;
    for (const auto& [name, value] : model.textures)
    {
        static_cast<void>(name);
        result.emplace(resolveTextureValue(value, model.textures));
    }
    for (const ModelElement& element : model.elements)
    {
        for (const auto& [faceName, face] : element.faces)
        {
            static_cast<void>(faceName);
            const std::string texture = resolveTextureValue(face.texture, model.textures);
            result.emplace(texture);
        }
    }
    return result;
}

core::ResourceLocation ResourcePack::resolveTexture(
    std::string textureReference,
    const ResolvedModel& model) const
{
    return core::ResourceLocation(
        resolveTextureValue(std::move(textureReference), model.textures)
    );
}
}
