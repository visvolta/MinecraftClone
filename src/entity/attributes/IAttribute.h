#pragma once

#include <string>

namespace mc::entity
{
class IAttribute
{
public:
    IAttribute(
        const char* name,
        double defaultValue,
        double minimum,
        double maximum,
        bool shouldWatch = false) noexcept
        : name_(name),
          defaultValue_(defaultValue),
          minimum_(minimum),
          maximum_(maximum),
          shouldWatch_(shouldWatch)
    {
    }

    [[nodiscard]] const char* getName() const noexcept { return name_; }
    [[nodiscard]] double getDefaultValue() const noexcept { return defaultValue_; }
    [[nodiscard]] double getMinimum() const noexcept { return minimum_; }
    [[nodiscard]] double getMaximum() const noexcept { return maximum_; }
    [[nodiscard]] bool getShouldWatch() const noexcept { return shouldWatch_; }

private:
    const char* name_ = "";
    double defaultValue_ = 0.0;
    double minimum_ = 0.0;
    double maximum_ = 0.0;
    bool shouldWatch_ = false;
};
}
