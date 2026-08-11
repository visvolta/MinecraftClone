#include "content/resources/ResourcePack.h"

#include <fstream>
#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
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
        if (entry.value().is_string())
            result.properties.emplace(entry.key(), entry.value().get<std::string>());
        else if (entry.value().is_boolean())
            result.properties.emplace(
                entry.key(), entry.value().get<bool>() ? "true" : "false"
            );
        else
            throw std::runtime_error("Multipart property must be scalar");
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
    if (const auto cached = blockStateCache_.find(name);
        cached != blockStateCache_.end())
        return cached->second;
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
    blockStateCache_.emplace(name, result);
    return result;
}

ModelResource ResourcePack::loadModel(
    const core::ResourceLocation& name) const
{
    if (const auto cached = modelCache_.find(name);
        cached != modelCache_.end())
        return cached->second;
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
    modelCache_.emplace(name, result);
    return result;
}

ResolvedModel ResourcePack::resolveModel(
    const core::ResourceLocation& name) const
{
    if (const auto cached = resolvedModelCache_.find(name);
        cached != resolvedModelCache_.end())
        return cached->second;
    std::unordered_set<core::ResourceLocation, core::ResourceLocationHash> resolving;
    ResolvedModel result = resolveModel(name, resolving);
    resolvedModelCache_.emplace(name, result);
    return result;
}

ResolvedModel ResourcePack::resolveModel(
    const core::ResourceLocation& name,
    std::unordered_set<core::ResourceLocation, core::ResourceLocationHash>& resolving) const
{
    if (!resolving.insert(name).second)
        throw std::runtime_error("Cyclic model parent: " + name.toString());
    if (const auto cached = resolvedModelCache_.find(name);
        cached != resolvedModelCache_.end())
    {
        resolving.erase(name);
        return cached->second;
    }
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
    resolvedModelCache_.emplace(name, result);
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

std::vector<core::ResourceLocation> ResourcePack::discoverJsonResources(
    const std::filesystem::path& relativeDirectory) const
{
    std::vector<core::ResourceLocation> result;
    std::error_code rootError;
    for (const auto& namespaceEntry :
         std::filesystem::directory_iterator(assetRoot_, rootError))
    {
        if (rootError || !namespaceEntry.is_directory())
            continue;
        const std::filesystem::path root =
            namespaceEntry.path() / relativeDirectory;
        std::error_code error;
        if (!std::filesystem::is_directory(root, error))
            continue;
        for (const auto& entry :
             std::filesystem::recursive_directory_iterator(root, error))
        {
            if (error)
                break;
            if (!entry.is_regular_file() ||
                entry.path().extension() != ".json")
                continue;
            std::filesystem::path relative =
                std::filesystem::relative(entry.path(), root, error);
            if (error)
                continue;
            relative.replace_extension();
            result.emplace_back(
                namespaceEntry.path().filename().string(),
                relative.generic_string()
            );
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<core::ResourceLocation> ResourcePack::blockStateNames() const
{
    return discoverJsonResources("blockstates");
}

std::vector<core::ResourceLocation> ResourcePack::itemModelNames() const
{
    return discoverJsonResources(std::filesystem::path("models") / "item");
}

std::vector<std::vector<std::pair<std::string, std::string>>>
ResourcePack::blockStateCombinations(const core::ResourceLocation& name) const
{
    const BlockStateResource resource = loadBlockState(name);
    std::map<std::string, std::vector<std::string>> values;
    const auto addValue = [&values](std::string_view property, std::string_view text)
    {
        std::size_t begin = 0;
        do
        {
            const std::size_t end = text.find('|', begin);
            const std::string value(text.substr(
                begin,
                end == std::string_view::npos ? text.size() - begin : end - begin
            ));
            auto& candidates = values[std::string(property)];
            if (std::find(candidates.begin(), candidates.end(), value) ==
                candidates.end())
                candidates.push_back(value);
            if (end == std::string_view::npos)
                break;
            begin = end + 1U;
        } while (begin <= text.size());
    };
    const auto addKey = [&addValue](std::string_view key)
    {
        if (key.empty() || key == "normal")
            return;
        std::size_t begin = 0;
        while (begin < key.size())
        {
            const std::size_t end = key.find(',', begin);
            const std::string_view part = key.substr(
                begin,
                end == std::string_view::npos ? key.size() - begin : end - begin
            );
            const std::size_t equals = part.find('=');
            if (equals != std::string_view::npos)
                addValue(part.substr(0, equals), part.substr(equals + 1U));
            if (end == std::string_view::npos)
                break;
            begin = end + 1U;
        }
    };
    for (const auto& [key, variants] : resource.variants)
    {
        static_cast<void>(variants);
        addKey(key);
    }
    for (const MultipartPart& part : resource.multipart)
    {
        for (const StatePredicate& predicate : part.alternatives)
            for (const auto& [property, value] : predicate.properties)
                addValue(property, value);
    }
    for (auto& [property, candidates] : values)
    {
        static_cast<void>(property);
        // Multipart JSON commonly lists only the rendered "true" branch
        // (fences and walls are the canonical 1.12 examples). PropertyBool
        // still has both values even when the false branch emits no model.
        const bool booleanProperty = std::any_of(
            candidates.begin(), candidates.end(),
            [](const std::string& value)
            {
                return value == "true" || value == "false";
            }
        );
        if (booleanProperty)
        {
            if (std::find(candidates.begin(), candidates.end(), "false") ==
                candidates.end())
                candidates.emplace_back("false");
            if (std::find(candidates.begin(), candidates.end(), "true") ==
                candidates.end())
                candidates.emplace_back("true");
        }
        std::sort(candidates.begin(), candidates.end());
    }

    std::vector<std::vector<std::pair<std::string, std::string>>> states(1);
    for (const auto& [property, candidates] : values)
    {
        std::vector<std::vector<std::pair<std::string, std::string>>> expanded;
        expanded.reserve(states.size() * candidates.size());
        for (const auto& state : states)
        {
            for (const std::string& value : candidates)
            {
                auto next = state;
                next.emplace_back(property, value);
                expanded.push_back(std::move(next));
            }
        }
        states = std::move(expanded);
    }
    if (states.size() > 65536U)
    {
        throw std::runtime_error(
            "Block has too many state combinations: " + name.toString()
        );
    }
    return states;
}

std::vector<core::ResourceLocation> ResourcePack::lootTableNames() const
{
    return discoverJsonResources("loot_tables");
}

std::vector<LootStackResource> ResourcePack::rollLootTable(
    const core::ResourceLocation& name,
    const LootContext& context,
    std::mt19937& random) const
{
    std::vector<LootStackResource> result;
    rollLootTable(name, context, random, result, 0);
    return result;
}

void ResourcePack::rollLootTable(
    const core::ResourceLocation& name,
    const LootContext& context,
    std::mt19937& random,
    std::vector<LootStackResource>& output,
    int recursionDepth) const
{
    if (recursionDepth > 16)
        throw std::runtime_error("Loot table recursion is too deep");
    const Json table = readJson(
        namespaceRoot(assetRoot_, name) / "loot_tables" /
        (name.path() + ".json")
    );
    const auto integerRange = [&random](const Json& value, int fallback)
    {
        if (value.is_number())
            return value.get<int>();
        if (!value.is_object())
            return fallback;
        const int minimum = value.value("min", fallback);
        const int maximum = value.value("max", minimum);
        return std::uniform_int_distribution<int>(minimum, maximum)(random);
    };
    const auto conditionsPass = [&context, &random](const Json& owner)
    {
        if (!owner.contains("conditions"))
            return true;
        for (const Json& condition : owner.at("conditions"))
        {
            const std::string type = condition.value("condition", "");
            if (type == "killed_by_player" && !context.killedByPlayer)
                return false;
            if (type == "random_chance_with_looting")
            {
                const float chance = condition.value("chance", 0.0f) +
                    condition.value("looting_multiplier", 0.0f) *
                    static_cast<float>(context.lootingLevel);
                if (std::uniform_real_distribution<float>(0.0f, 1.0f)(random) >= chance)
                    return false;
            }
            if (type == "entity_properties" &&
                condition.contains("properties") &&
                condition.at("properties").contains("on_fire") &&
                condition.at("properties").at("on_fire").get<bool>() != context.onFire)
                return false;
        }
        return true;
    };
    const auto smelted = [](const core::ResourceLocation& item)
    {
        static const std::unordered_map<std::string, std::string> recipes{
            {"beef", "cooked_beef"}, {"porkchop", "cooked_porkchop"},
            {"chicken", "cooked_chicken"}, {"rabbit", "cooked_rabbit"},
            {"mutton", "cooked_mutton"}, {"fish", "cooked_fish"}
        };
        const auto found = recipes.find(item.path());
        return found == recipes.end()
            ? item
            : core::ResourceLocation(item.nameSpace(), found->second);
    };

    if (!table.contains("pools"))
        return;
    for (const Json& pool : table.at("pools"))
    {
        if (!conditionsPass(pool) || !pool.contains("entries"))
            continue;
        const int rolls = integerRange(pool.value("rolls", Json(1)), 1);
        for (int roll = 0; roll < rolls; ++roll)
        {
            std::vector<const Json*> candidates;
            std::vector<int> weights;
            int totalWeight = 0;
            for (const Json& entry : pool.at("entries"))
            {
                if (!conditionsPass(entry))
                    continue;
                const int weight = std::max(
                    0,
                    entry.value("weight", 1) + static_cast<int>(
                        std::floor(entry.value("quality", 0) * context.luck)
                    )
                );
                if (weight == 0)
                    continue;
                candidates.push_back(&entry);
                weights.push_back(weight);
                totalWeight += weight;
            }
            if (candidates.empty() || totalWeight <= 0)
                continue;
            int choice = std::uniform_int_distribution<int>(0, totalWeight - 1)(random);
            std::size_t selected = 0;
            for (; selected + 1U < weights.size(); ++selected)
            {
                choice -= weights[selected];
                if (choice < 0)
                    break;
            }
            const Json& entry = *candidates[selected];
            const std::string type = entry.value("type", "empty");
            if (type == "empty")
                continue;
            if (type == "loot_table")
            {
                rollLootTable(
                    core::ResourceLocation(entry.at("name").get<std::string>()),
                    context, random, output, recursionDepth + 1
                );
                continue;
            }
            if (type != "item")
                continue;

            LootStackResource stack;
            stack.item = core::ResourceLocation(entry.at("name").get<std::string>());
            stack.count = 1;
            if (entry.contains("functions"))
            {
                for (const Json& function : entry.at("functions"))
                {
                    if (!conditionsPass(function))
                        continue;
                    const std::string functionName = function.value("function", "");
                    if (functionName == "set_count" ||
                        functionName == "minecraft:set_count")
                        stack.count = integerRange(function.at("count"), stack.count);
                    else if (functionName == "looting_enchant" && context.lootingLevel > 0)
                    {
                        const int maximumPerLevel = function.at("count").is_object()
                            ? function.at("count").value("max", 1) : 1;
                        stack.count += std::uniform_int_distribution<int>(
                            0, maximumPerLevel * context.lootingLevel
                        )(random);
                    }
                    else if (functionName == "furnace_smelt")
                        stack.item = smelted(stack.item);
                    else if (functionName == "set_data" ||
                             functionName == "minecraft:set_data")
                        stack.metadata = integerRange(function.at("data"), 0);
                }
            }
            if (stack.count > 0)
                output.push_back(std::move(stack));
        }
    }
}
}
