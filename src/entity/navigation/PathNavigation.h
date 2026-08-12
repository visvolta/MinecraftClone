#pragma once

#include "BlockShape.h"

#include <glm/glm.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

class World;

namespace mc::entity::navigation
{
enum class PathNodeType : std::uint8_t
{
    Blocked,
    Open,
    Walkable,
    Trapdoor,
    Fence,
    Lava,
    Water,
    Rail,
    DangerFire,
    DamageFire,
    DangerCactus,
    DamageCactus,
    DangerOther,
    DamageOther,
    DoorOpen,
    DoorWoodClosed,
    DoorIronClosed,
    Count
};

[[nodiscard]] float defaultPriority(PathNodeType type) noexcept;

enum class NavigationKind : std::uint8_t
{
    Ground,
    Flying,
    Swimming,
    Climbing
};

struct NavigationSettings
{
    NavigationKind kind = NavigationKind::Ground;
    float width = 0.6f;
    float height = 1.8f;
    float stepHeight = 0.6f;
    int maximumFallHeight = 3;
    bool canOpenDoors = false;
    bool canEnterDoors = true;
    bool canSwim = false;
    std::array<float, static_cast<std::size_t>(PathNodeType::Count)>
        priorities{};

    NavigationSettings();
    [[nodiscard]] float priority(PathNodeType type) const noexcept;
    void setPriority(PathNodeType type, float value) noexcept;
};

class NavigationBlockAccess
{
public:
    virtual ~NavigationBlockAccess() = default;
    [[nodiscard]] virtual content::BlockState blockState(
        int x,
        int y,
        int z
    ) const = 0;
    [[nodiscard]] virtual bool loaded(int x, int y, int z) const = 0;
};

class WorldNavigationBlockAccess final : public NavigationBlockAccess
{
public:
    explicit WorldNavigationBlockAccess(const World& world) noexcept;
    [[nodiscard]] content::BlockState blockState(
        int x,
        int y,
        int z
    ) const override;
    [[nodiscard]] bool loaded(int x, int y, int z) const override;

private:
    const World* world_ = nullptr;
};

struct PathPoint
{
    glm::ivec3 position{};
    float totalPathDistance = 0.0f;
    float distanceToNext = 0.0f;
    float distanceToTarget = 0.0f;
    float distanceFromOrigin = 0.0f;
    float cost = 0.0f;
    float costMalus = 0.0f;
    PathNodeType nodeType = PathNodeType::Blocked;
};

class Path
{
public:
    Path() = default;
    explicit Path(std::vector<PathPoint> points);

    void advance() noexcept;
    void clear() noexcept;
    [[nodiscard]] bool finished() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t currentIndex() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] const PathPoint* current() const noexcept;
    [[nodiscard]] const PathPoint* finalPoint() const noexcept;
    [[nodiscard]] const PathPoint& at(std::size_t index) const;
    [[nodiscard]] glm::vec3 positionFor(
        std::size_t index,
        float entityWidth
    ) const;

private:
    std::vector<PathPoint> points_;
    std::size_t currentIndex_ = 0;
};

class PathFinder
{
public:
    [[nodiscard]] std::optional<Path> findPath(
        const NavigationBlockAccess& world,
        const NavigationSettings& settings,
        const glm::vec3& start,
        const glm::vec3& target,
        float maximumDistance
    );

    [[nodiscard]] static PathNodeType classify(
        const NavigationBlockAccess& world,
        int x,
        int y,
        int z
    );

    [[nodiscard]] static bool volumeClear(
        const NavigationBlockAccess& world,
        const NavigationSettings& settings,
        const glm::ivec3& position
    );

private:
    struct SearchPoint;
    struct PositionHash
    {
        [[nodiscard]] std::size_t operator()(
            const glm::ivec3& value
        ) const noexcept;
    };

    using PointMap = std::unordered_map<glm::ivec3, SearchPoint, PositionHash>;

    [[nodiscard]] static PathNodeType classifyVolume(
        const NavigationBlockAccess& world,
        const NavigationSettings& settings,
        const glm::ivec3& position
    );
};

class PathNavigation
{
public:
    explicit PathNavigation(NavigationSettings settings = {});

    bool tryMoveTo(
        const NavigationBlockAccess& world,
        const glm::vec3& currentPosition,
        const glm::vec3& target,
        double speed,
        float maximumDistance
    );
    void setPath(Path path, double speed);
    void clear() noexcept;
    void tick(
        const NavigationBlockAccess& world,
        const glm::vec3& currentPosition,
        bool onGround,
        bool inLiquid
    );

    [[nodiscard]] bool noPath() const noexcept;
    [[nodiscard]] const Path* path() const noexcept;
    [[nodiscard]] std::optional<glm::vec3> currentMoveTarget() const noexcept;
    [[nodiscard]] glm::vec3 desiredDirection() const noexcept;
    [[nodiscard]] double speed() const noexcept;
    void setSpeed(double speed) noexcept;
    [[nodiscard]] const NavigationSettings& settings() const noexcept;
    [[nodiscard]] NavigationSettings& settings() noexcept;

private:
    NavigationSettings settings_;
    PathFinder pathFinder_;
    std::optional<Path> path_;
    glm::vec3 desiredDirection_{};
    glm::vec3 lastStuckCheckPosition_{};
    glm::vec3 climbingTarget_{};
    double speed_ = 0.0;
    int totalTicks_ = 0;
    int ticksAtLastStuckCheck_ = 0;
    bool hasClimbingTarget_ = false;

    [[nodiscard]] bool canNavigate(bool onGround, bool inLiquid) const noexcept;
    [[nodiscard]] bool directPath(
        const NavigationBlockAccess& world,
        const glm::vec3& from,
        const glm::vec3& to
    ) const;
};
}
