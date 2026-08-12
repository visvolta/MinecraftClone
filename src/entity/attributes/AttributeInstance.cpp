#include "entity/attributes/AttributeInstance.h"

#include <algorithm>

namespace mc::entity
{
AttributeInstance::AttributeInstance(const IAttribute* attribute)
    : attribute_(attribute),
      baseValue_(attribute->getDefaultValue()),
      cachedValue_(attribute->getDefaultValue())
{
}

void AttributeInstance::setBaseValue(double value)
{
    baseValue_ = std::clamp(
        value, attribute_->getMinimum(), attribute_->getMaximum());
    dirty_ = true;
}

double AttributeInstance::getAttributeValue()
{
    if (dirty_)
        compute();
    return cachedValue_;
}

void AttributeInstance::applyModifier(const AttributeModifier& modifier)
{
    modifiers_.insert_or_assign(key(modifier.getId()), modifier);
    dirty_ = true;
}

void AttributeInstance::removeModifier(const EntityUuid& id)
{
    modifiers_.erase(key(id));
    dirty_ = true;
}

bool AttributeInstance::hasModifier(const EntityUuid& id) const
{
    return modifiers_.contains(key(id));
}

void AttributeInstance::compute()
{
    double value = baseValue_;
    for (const auto& [_, modifier] : modifiers_)
        if (modifier.getOperation() == AttributeModifier::Operation::Add)
            value += modifier.getAmount();

    double multiplied = value;
    for (const auto& [_, modifier] : modifiers_)
        if (modifier.getOperation() == AttributeModifier::Operation::MultiplyBase)
            multiplied += value * modifier.getAmount();

    for (const auto& [_, modifier] : modifiers_)
        if (modifier.getOperation() == AttributeModifier::Operation::MultiplyTotal)
            multiplied *= 1.0 + modifier.getAmount();

    cachedValue_ = std::clamp(
        multiplied, attribute_->getMinimum(), attribute_->getMaximum());
    dirty_ = false;
}

std::uint64_t AttributeInstance::key(const EntityUuid& id) noexcept
{
    return id.most ^ id.least;
}
}
