#pragma once

#include "core/ResourceLocation.h"

namespace mc::content
{
class ContentCatalog;

class ContentModule
{
public:
    virtual ~ContentModule() = default;

    [[nodiscard]] virtual core::ResourceLocation id() const = 0;
    virtual void registerContent(ContentCatalog& catalog) = 0;
};
}
