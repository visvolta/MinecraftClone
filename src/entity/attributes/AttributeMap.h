#pragma once

#include "entity/attributes/AttributeInstance.h"

#include <memory>
#include <unordered_map>

namespace mc::entity
{
class AttributeMap
{
public:
    AttributeInstance& registerAttribute(const IAttribute& attribute);
    [[nodiscard]] AttributeInstance* getAttributeInstance(const IAttribute& attribute);
    [[nodiscard]] const AttributeInstance* getAttributeInstance(
        const IAttribute& attribute) const;
    [[nodiscard]] double getAttributeValue(const IAttribute& attribute);

private:
    std::unordered_map<const IAttribute*, std::unique_ptr<AttributeInstance>>
        instances_;
};
}
