#include "CraftingRecipe.h"

#include "content/ContentCatalog.h"

#include <algorithm>
#include <fstream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace
{
using Json = nlohmann::json;

ItemType resolveItem(
    const Json& ingredient,
    const mc::content::ContentCatalog& content)
{
    if (!ingredient.is_object() || !ingredient.contains("item"))
        throw std::runtime_error("Recipe ingredient must name an item");
    const auto item = content.legacyItem(mc::core::ResourceLocation(
        ingredient.at("item").get<std::string>()
    ));
    if (!item)
        throw std::runtime_error("Recipe references an unknown item");
    return *item;
}

Ingredient parseIngredient(
    const Json& json,
    const mc::content::ContentCatalog& content)
{
    std::vector<ItemType> alternatives;
    if (json.is_array())
    {
        alternatives.reserve(json.size());
        for (const Json& value : json)
            alternatives.push_back(resolveItem(value, content));
    }
    else
    {
        alternatives.push_back(resolveItem(json, content));
    }
    return Ingredient(std::move(alternatives));
}

ItemStack parseResult(
    const Json& json,
    const mc::content::ContentCatalog& content)
{
    const ItemType item = resolveItem(json, content);
    const int count = json.value("count", 1);
    if (count < 1 || count > 255)
        throw std::runtime_error("Recipe result has an invalid count");
    ItemStack result{item, static_cast<std::uint8_t>(count)};
    if (result.count > result.maximumStackSize())
        throw std::runtime_error("Recipe result exceeds its stack limit");
    return result;
}

Json readJson(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input)
        throw std::runtime_error("Could not open recipe: " + path.string());
    try
    {
        return Json::parse(input);
    }
    catch (const Json::exception& error)
    {
        throw std::runtime_error(
            "Invalid recipe " + path.string() + ": " + error.what()
        );
    }
}

std::vector<Ingredient> convertIngredients(
    std::initializer_list<ItemType> items)
{
    std::vector<Ingredient> converted;
    converted.reserve(items.size());
    for (const ItemType item : items)
        converted.emplace_back(item);
    return converted;
}
}

Ingredient::Ingredient(ItemType item)
{
    if (item != ItemType::Empty)
        alternatives.push_back(item);
}

Ingredient::Ingredient(std::vector<ItemType> items)
    : alternatives(std::move(items))
{
    std::erase(alternatives, ItemType::Empty);
    std::ranges::sort(alternatives);
    const auto duplicate = std::ranges::unique(alternatives);
    alternatives.erase(duplicate.begin(), duplicate.end());
    if (alternatives.empty())
        throw std::invalid_argument("Ingredient cannot be empty");
}

bool Ingredient::matches(ItemType item) const noexcept
{
    return item == ItemType::Empty
        ? alternatives.empty()
        : std::ranges::find(alternatives, item) != alternatives.end();
}

ShapedRecipe::ShapedRecipe(
    int width,
    int height,
    std::initializer_list<ItemType> pattern,
    CraftingResult result)
    : ShapedRecipe(
          width,
          height,
          convertIngredients(pattern),
          result)
{
}

ShapedRecipe::ShapedRecipe(
    int width,
    int height,
    std::vector<Ingredient> pattern,
    CraftingResult result)
    : width_(width),
      height_(height),
      pattern_(std::move(pattern)),
      result_(result)
{
    if (width_ < 1 || width_ > 3 ||
        height_ < 1 || height_ > 3 ||
        pattern_.size() != static_cast<std::size_t>(width_ * height_) ||
        result_.empty() ||
        result_.count > result_.maximumStackSize())
    {
        throw std::invalid_argument("Invalid shaped crafting recipe");
    }
}

bool ShapedRecipe::matches(const CraftingGrid& grid) const noexcept
{
    for (int y = 0; y <= 3 - height_; ++y)
    {
        for (int x = 0; x <= 3 - width_; ++x)
        {
            if (matchesAt(grid, x, y, false) ||
                matchesAt(grid, x, y, true))
            {
                return true;
            }
        }
    }
    return false;
}

const CraftingResult& ShapedRecipe::getResult() const noexcept
{
    return result_;
}

bool ShapedRecipe::matchesAt(
    const CraftingGrid& grid,
    int offsetX,
    int offsetY,
    bool mirrored) const noexcept
{
    for (int gridY = 0; gridY < 3; ++gridY)
    {
        for (int gridX = 0; gridX < 3; ++gridX)
        {
            const Ingredient* expected = nullptr;
            const int recipeX = gridX - offsetX;
            const int recipeY = gridY - offsetY;
            if (recipeX >= 0 && recipeX < width_ &&
                recipeY >= 0 && recipeY < height_)
            {
                const int patternX = mirrored
                    ? width_ - 1 - recipeX
                    : recipeX;
                expected = &pattern_[static_cast<std::size_t>(
                    recipeY * width_ + patternX
                )];
            }
            const ItemStack& stack = grid[static_cast<std::size_t>(
                gridY * 3 + gridX
            )];
            const ItemType actual = stack.empty()
                ? ItemType::Empty
                : stack.item;
            if (expected == nullptr
                    ? actual != ItemType::Empty
                    : !expected->matches(actual))
            {
                return false;
            }
        }
    }
    return true;
}

ShapelessRecipe::ShapelessRecipe(
    std::initializer_list<ItemType> ingredients,
    CraftingResult result)
    : ShapelessRecipe(convertIngredients(ingredients), result)
{
}

ShapelessRecipe::ShapelessRecipe(
    std::vector<Ingredient> ingredients,
    CraftingResult result)
    : ingredients_(std::move(ingredients)),
      result_(result)
{
    if (ingredients_.empty() || ingredients_.size() > 9 ||
        std::ranges::any_of(
            ingredients_,
            [](const Ingredient& value)
            {
                return value.alternatives.empty();
            }
        ) ||
        result_.empty() ||
        result_.count > result_.maximumStackSize())
    {
        throw std::invalid_argument("Invalid shapeless crafting recipe");
    }
}

bool ShapelessRecipe::matches(const CraftingGrid& grid) const noexcept
{
    std::vector<Ingredient> remaining = ingredients_;
    for (const ItemStack& stack : grid)
    {
        if (stack.empty())
            continue;
        const auto ingredient = std::ranges::find_if(
            remaining,
            [&stack](const Ingredient& value)
            {
                return value.matches(stack.item);
            }
        );
        if (ingredient == remaining.end())
            return false;
        remaining.erase(ingredient);
    }
    return remaining.empty();
}

const CraftingResult& ShapelessRecipe::getResult() const noexcept
{
    return result_;
}

RecipeRegistry::RecipeRegistry(
    const std::filesystem::path& assetRoot,
    const mc::content::ContentCatalog& content)
{
    const std::filesystem::path root = assetRoot / "minecraft" / "recipes";
    if (!std::filesystem::is_directory(root))
        throw std::runtime_error("Recipe directory is missing: " + root.string());

    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".json")
            files.push_back(entry.path());
    }
    std::ranges::sort(files);
    for (const std::filesystem::path& file : files)
    {
        const Json json = readJson(file);
        const std::string type = json.at("type").get<std::string>();
        if (type == "minecraft:crafting_shapeless")
        {
            std::vector<Ingredient> ingredients;
            for (const Json& value : json.at("ingredients"))
                ingredients.push_back(parseIngredient(value, content));
            add(ShapelessRecipe(
                std::move(ingredients),
                parseResult(json.at("result"), content)
            ));
        }
        else if (type == "minecraft:crafting_shaped")
        {
            const std::vector<std::string> pattern =
                json.at("pattern").get<std::vector<std::string>>();
            if (pattern.empty() || pattern.size() > 3 ||
                pattern.front().empty() || pattern.front().size() > 3)
            {
                throw std::runtime_error("Shaped recipe has an invalid pattern");
            }
            const std::size_t width = pattern.front().size();
            std::unordered_map<char, Ingredient> key;
            for (auto entry = json.at("key").begin();
                 entry != json.at("key").end(); ++entry)
            {
                if (entry.key().size() != 1 || entry.key().front() == ' ')
                    throw std::runtime_error("Shaped recipe has an invalid key");
                key.emplace(
                    entry.key().front(),
                    parseIngredient(entry.value(), content)
                );
            }
            std::vector<Ingredient> ingredients;
            ingredients.reserve(width * pattern.size());
            for (const std::string& row : pattern)
            {
                if (row.size() != width)
                    throw std::runtime_error("Shaped recipe rows differ in width");
                for (const char symbol : row)
                {
                    if (symbol == ' ')
                        ingredients.emplace_back();
                    else if (const auto value = key.find(symbol); value != key.end())
                        ingredients.push_back(value->second);
                    else
                        throw std::runtime_error("Shaped recipe uses an unknown key");
                }
            }
            add(ShapedRecipe(
                static_cast<int>(width),
                static_cast<int>(pattern.size()),
                std::move(ingredients),
                parseResult(json.at("result"), content)
            ));
        }
        else
        {
            throw std::runtime_error("Unsupported recipe type: " + type);
        }
    }
}

void RecipeRegistry::add(ShapedRecipe recipe)
{
    recipes_.emplace_back(std::move(recipe));
}

void RecipeRegistry::add(ShapelessRecipe recipe)
{
    recipes_.emplace_back(std::move(recipe));
}

const CraftingRecipe* RecipeRegistry::findMatch(
    const CraftingGrid& grid) const noexcept
{
    for (const CraftingRecipe& recipe : recipes_)
    {
        const bool matches = std::visit(
            [&grid](const auto& value)
            {
                return value.matches(grid);
            },
            recipe
        );
        if (matches)
            return &recipe;
    }
    return nullptr;
}

const CraftingResult& RecipeRegistry::getResult(
    const CraftingRecipe& recipe) noexcept
{
    return std::visit(
        [](const auto& value) -> const CraftingResult&
        {
            return value.getResult();
        },
        recipe
    );
}

std::size_t RecipeRegistry::size() const noexcept
{
    return recipes_.size();
}
