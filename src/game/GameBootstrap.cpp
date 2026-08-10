#include "game/GameBootstrap.h"

#include "content/MinecraftContent.h"

#include <memory>
#include <stdexcept>
#include <utility>

namespace mc::game
{
GameBootstrap::GameBootstrap()
{
    gameplay::registerVanillaGameplay(gameplay_);
    addContentModule(std::make_unique<content::MinecraftContentModule>());
}

void GameBootstrap::addContentModule(
    std::unique_ptr<content::ContentModule> module)
{
    if (modulesLoaded_ || content_.frozen())
        throw std::logic_error("Content modules must be added before loading");
    if (!module)
        throw std::invalid_argument("Content module cannot be null");

    const core::ResourceLocation moduleId = module->id();
    if (!moduleIds_.insert(moduleId).second)
    {
        throw std::invalid_argument(
            "Duplicate content module: " + moduleId.toString()
        );
    }
    modules_.push_back(std::move(module));
}

void GameBootstrap::loadContentModules()
{
    if (modulesLoaded_)
        throw std::logic_error("Content modules were already loaded");

    for (const auto& module : modules_)
        module->registerContent(content_);
    modulesLoaded_ = true;
}

void GameBootstrap::freezeRegistries()
{
    if (!modulesLoaded_)
        throw std::logic_error("Content modules must load before registry freeze");
    content_.freeze();
    content_.activate();
    gameplay_.freeze();
}

content::ContentCatalog& GameBootstrap::content() noexcept
{
    return content_;
}

const content::ContentCatalog& GameBootstrap::content() const noexcept
{
    return content_;
}

gameplay::GameplayRegistries& GameBootstrap::gameplay() noexcept
{
    return gameplay_;
}

const gameplay::GameplayRegistries& GameBootstrap::gameplay() const noexcept
{
    return gameplay_;
}
}
