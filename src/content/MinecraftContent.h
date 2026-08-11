#pragma once

#include "content/ContentModule.h"

#include <filesystem>

namespace mc::content
{
class ContentCatalog;

class MinecraftContentModule final : public ContentModule
{
public:
    explicit MinecraftContentModule(std::filesystem::path assetRoot = {});
    [[nodiscard]] core::ResourceLocation id() const override;
    void registerContent(ContentCatalog& catalog) override;

private:
    std::filesystem::path assetRoot_;
};

void registerMinecraftContent(
    ContentCatalog& catalog,
    const std::filesystem::path& assetRoot = {}
);
}
