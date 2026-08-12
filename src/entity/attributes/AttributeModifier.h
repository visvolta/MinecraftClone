#pragma once

#include "entity/EntityUuid.h"

#include <string>

namespace mc::entity
{
class AttributeModifier
{
public:
    enum class Operation
    {
        Add = 0,
        MultiplyBase = 1,
        MultiplyTotal = 2
    };

    AttributeModifier(
        EntityUuid id,
        std::string name,
        double amount,
        Operation operation,
        bool saved = true)
        : id_(id),
          name_(std::move(name)),
          amount_(amount),
          operation_(operation),
          saved_(saved)
    {
    }

    [[nodiscard]] const EntityUuid& getId() const noexcept { return id_; }
    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    [[nodiscard]] double getAmount() const noexcept { return amount_; }
    [[nodiscard]] Operation getOperation() const noexcept { return operation_; }
    [[nodiscard]] bool isSaved() const noexcept { return saved_; }

private:
    EntityUuid id_{};
    std::string name_;
    double amount_ = 0.0;
    Operation operation_ = Operation::Add;
    bool saved_ = true;
};
}
