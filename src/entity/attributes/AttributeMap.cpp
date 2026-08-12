#include "entity/attributes/AttributeMap.h"

#include <stdexcept>

namespace mc::entity
{
AttributeInstance& AttributeMap::registerAttribute(const IAttribute& attribute)
{
    auto& slot = instances_[&attribute];
    if (!slot)
        slot = std::make_unique<AttributeInstance>(&attribute);
    return *slot;
}

AttributeInstance* AttributeMap::getAttributeInstance(const IAttribute& attribute)
{
    const auto found = instances_.find(&attribute);
    return found == instances_.end() ? nullptr : found->second.get();
}

const AttributeInstance* AttributeMap::getAttributeInstance(
    const IAttribute& attribute) const
{
    const auto found = instances_.find(&attribute);
    return found == instances_.end() ? nullptr : found->second.get();
}

double AttributeMap::getAttributeValue(const IAttribute& attribute)
{
    if (AttributeInstance* instance = getAttributeInstance(attribute))
        return instance->getAttributeValue();
    return attribute.getDefaultValue();
}
}
