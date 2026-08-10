#pragma once

#include "core/ResourceLocation.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace mc::core
{
using RuntimeId = std::uint32_t;
inline constexpr RuntimeId InvalidRuntimeId =
    std::numeric_limits<RuntimeId>::max();

template<typename T>
class Registry
{
public:
    struct Entry
    {
        ResourceLocation name;
        T value;
        RuntimeId runtimeId = InvalidRuntimeId;
    };

    explicit Registry(ResourceLocation registryName)
        : registryName_(std::move(registryName))
    {
    }

    T& registerValue(ResourceLocation name, T value)
    {
        if (frozen_)
            throw std::logic_error("Cannot modify a frozen registry");
        if (indices_.contains(name))
            throw std::invalid_argument("Duplicate registry entry: " + name.toString());

        const std::size_t index = entries_.size();
        entries_.push_back({std::move(name), std::move(value), InvalidRuntimeId});
        indices_.emplace(entries_.back().name, index);
        return entries_.back().value;
    }

    void freeze()
    {
        if (frozen_)
            return;
        if (entries_.size() > static_cast<std::size_t>(InvalidRuntimeId))
            throw std::overflow_error("Registry contains too many entries");
        for (std::size_t index = 0; index < entries_.size(); ++index)
            entries_[index].runtimeId = static_cast<RuntimeId>(index);
        frozen_ = true;
    }

    [[nodiscard]] const T* find(const ResourceLocation& name) const noexcept
    {
        const auto found = indices_.find(name);
        return found == indices_.end() ? nullptr : &entries_[found->second].value;
    }

    [[nodiscard]] T* find(const ResourceLocation& name) noexcept
    {
        const auto found = indices_.find(name);
        return found == indices_.end() ? nullptr : &entries_[found->second].value;
    }

    [[nodiscard]] const Entry* entry(const ResourceLocation& name) const noexcept
    {
        const auto found = indices_.find(name);
        return found == indices_.end() ? nullptr : &entries_[found->second];
    }

    [[nodiscard]] const Entry* entry(RuntimeId runtimeId) const noexcept
    {
        if (!frozen_ || runtimeId >= entries_.size())
            return nullptr;
        return &entries_[runtimeId];
    }

    [[nodiscard]] RuntimeId runtimeId(
        const ResourceLocation& name) const noexcept
    {
        const Entry* found = entry(name);
        return found == nullptr ? InvalidRuntimeId : found->runtimeId;
    }

    [[nodiscard]] const ResourceLocation& name() const noexcept
    {
        return registryName_;
    }

    [[nodiscard]] bool frozen() const noexcept { return frozen_; }
    [[nodiscard]] std::size_t size() const noexcept { return entries_.size(); }
    [[nodiscard]] const std::vector<Entry>& entries() const noexcept
    {
        return entries_;
    }

private:
    ResourceLocation registryName_;
    std::vector<Entry> entries_;
    std::unordered_map<ResourceLocation, std::size_t, ResourceLocationHash> indices_;
    bool frozen_ = false;
};
}
