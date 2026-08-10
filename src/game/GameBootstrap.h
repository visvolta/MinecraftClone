#pragma once

#include "content/ContentCatalog.h"
#include "content/ContentModule.h"
#include "gameplay/GameplayRegistries.h"

#include <memory>
#include <unordered_set>
#include <vector>

namespace mc::game
{
class GameBootstrap
{
public:
    GameBootstrap();

    void addContentModule(std::unique_ptr<content::ContentModule> module);
    void loadContentModules();
    void freezeRegistries();

    [[nodiscard]] content::ContentCatalog& content() noexcept;
    [[nodiscard]] const content::ContentCatalog& content() const noexcept;
    [[nodiscard]] gameplay::GameplayRegistries& gameplay() noexcept;
    [[nodiscard]] const gameplay::GameplayRegistries& gameplay() const noexcept;

private:
    content::ContentCatalog content_;
    gameplay::GameplayRegistries gameplay_;
    std::vector<std::unique_ptr<content::ContentModule>> modules_;
    std::unordered_set<
        core::ResourceLocation,
        core::ResourceLocationHash
    > moduleIds_;
    bool modulesLoaded_ = false;
};
}
