#pragma once

#include "entity/attributes/AttributeModifier.h"
#include "entity/attributes/IAttribute.h"

#include <unordered_map>
#include <vector>

namespace mc::entity
{
class AttributeInstance
{
public:
    explicit AttributeInstance(const IAttribute* attribute);

    void setBaseValue(double value);
    [[nodiscard]] double getBaseValue() const noexcept { return baseValue_; }
    [[nodiscard]] double getAttributeValue();
    [[nodiscard]] const IAttribute& getAttribute() const noexcept { return *attribute_; }

    void applyModifier(const AttributeModifier& modifier);
    void removeModifier(const EntityUuid& id);
    [[nodiscard]] bool hasModifier(const EntityUuid& id) const;

private:
    const IAttribute* attribute_ = nullptr;
    double baseValue_ = 0.0;
    double cachedValue_ = 0.0;
    bool dirty_ = true;
    std::unordered_map<std::uint64_t, AttributeModifier> modifiers_;

    void compute();
    [[nodiscard]] static std::uint64_t key(const EntityUuid& id) noexcept;
};
}
