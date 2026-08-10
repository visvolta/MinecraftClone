#pragma once

#include "content/ContentModule.h"

namespace mc::content
{
class ContentCatalog;

class MinecraftContentModule final : public ContentModule
{
public:
    [[nodiscard]] core::ResourceLocation id() const override;
    void registerContent(ContentCatalog& catalog) override;
};

void registerMinecraftContent(ContentCatalog& catalog);
}
