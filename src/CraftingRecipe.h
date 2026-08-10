#pragma once

#include "Inventory.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <variant>
#include <vector>

namespace mc::content { class ContentCatalog; }

using CraftingGrid = std::array<ItemStack, 9>;

using CraftingResult = ItemStack;

struct Ingredient
{
    std::vector<ItemType> alternatives;

    Ingredient() = default;
    Ingredient(ItemType item);
    explicit Ingredient(std::vector<ItemType> items);
    [[nodiscard]] bool matches(ItemType item) const noexcept;
};

class ShapedRecipe
{
public:
    ShapedRecipe(
        int width,
        int height,
        std::initializer_list<ItemType> pattern,
        CraftingResult result
    );
    ShapedRecipe(
        int width,
        int height,
        std::vector<Ingredient> pattern,
        CraftingResult result
    );

    [[nodiscard]] bool matches(const CraftingGrid& grid) const noexcept;
    [[nodiscard]] const CraftingResult& getResult() const noexcept;

private:
    int width_ = 0;
    int height_ = 0;
    std::vector<Ingredient> pattern_;
    CraftingResult result_;

    [[nodiscard]] bool matchesAt(
        const CraftingGrid& grid,
        int offsetX,
        int offsetY,
        bool mirrored
    ) const noexcept;
};

class ShapelessRecipe
{
public:
    ShapelessRecipe(
        std::initializer_list<ItemType> ingredients,
        CraftingResult result
    );
    ShapelessRecipe(
        std::vector<Ingredient> ingredients,
        CraftingResult result
    );

    [[nodiscard]] bool matches(const CraftingGrid& grid) const noexcept;
    [[nodiscard]] const CraftingResult& getResult() const noexcept;

private:
    std::vector<Ingredient> ingredients_;
    CraftingResult result_;
};

using CraftingRecipe = std::variant<ShapedRecipe, ShapelessRecipe>;

class RecipeRegistry
{
public:
    RecipeRegistry() = default;
    RecipeRegistry(
        const std::filesystem::path& assetRoot,
        const mc::content::ContentCatalog& content
    );

    void add(ShapedRecipe recipe);
    void add(ShapelessRecipe recipe);

    [[nodiscard]] const CraftingRecipe* findMatch(
        const CraftingGrid& grid
    ) const noexcept;

    [[nodiscard]] static const CraftingResult& getResult(
        const CraftingRecipe& recipe
    ) noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::vector<CraftingRecipe> recipes_;
};
