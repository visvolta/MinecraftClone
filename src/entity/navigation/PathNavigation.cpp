#include "entity/navigation/PathNavigation.h"

#include "Block.h"
#include "World.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>

namespace mc::entity::navigation
{
namespace
{
constexpr std::array<float, static_cast<std::size_t>(PathNodeType::Count)>
    DefaultPriorities{{
        -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, -1.0f, 8.0f, 0.0f,
        8.0f, 16.0f, 8.0f, -1.0f, 8.0f, -1.0f, 0.0f, -1.0f,
        -1.0f
    }};

float manhattan(const glm::ivec3& first, const glm::ivec3& second)
{
    const glm::ivec3 delta = glm::abs(second - first);
    return static_cast<float>(delta.x + delta.y + delta.z);
}

float distance(const glm::ivec3& first, const glm::ivec3& second)
{
    const glm::vec3 delta = glm::vec3(second - first);
    return std::sqrt(glm::dot(delta, delta));
}

bool hasCollision(const content::BlockState state)
{
    return !getBlockShape(state).collisionBoxes.empty();
}

bool tallCollision(const content::BlockState state)
{
    for (const BlockBox& box : getBlockShape(state).collisionBoxes)
        if (box.maximum.y > 1.0001f)
            return true;
    return false;
}

glm::ivec3 floorPosition(const glm::vec3& value)
{
    return {
        static_cast<int>(std::floor(value.x)),
        static_cast<int>(std::floor(value.y)),
        static_cast<int>(std::floor(value.z))
    };
}
}

struct PathFinder::SearchPoint
{
    glm::ivec3 position{};
    float totalPathDistance = std::numeric_limits<float>::infinity();
    float distanceToNext = 0.0f;
    float distanceToTarget = std::numeric_limits<float>::infinity();
    float distanceFromOrigin = 0.0f;
    float cost = 0.0f;
    float costMalus = 0.0f;
    PathNodeType nodeType = PathNodeType::Blocked;
    SearchPoint* previous = nullptr;
    bool visited = false;
};

float defaultPriority(PathNodeType type) noexcept
{
    return DefaultPriorities[static_cast<std::size_t>(type)];
}

NavigationSettings::NavigationSettings() : priorities(DefaultPriorities) {}

float NavigationSettings::priority(PathNodeType type) const noexcept
{
    return priorities[static_cast<std::size_t>(type)];
}

void NavigationSettings::setPriority(
    PathNodeType type,
    float value) noexcept
{
    priorities[static_cast<std::size_t>(type)] = value;
}

Path::Path(std::vector<PathPoint> points) : points_(std::move(points)) {}
void Path::advance() noexcept { ++currentIndex_; }
void Path::clear() noexcept { currentIndex_ = points_.size(); }
bool Path::finished() const noexcept { return currentIndex_ >= points_.size(); }
bool Path::empty() const noexcept { return points_.empty(); }
std::size_t Path::currentIndex() const noexcept { return currentIndex_; }
std::size_t Path::size() const noexcept { return points_.size(); }

const PathPoint* Path::current() const noexcept
{
    return finished() ? nullptr : &points_[currentIndex_];
}

const PathPoint* Path::finalPoint() const noexcept
{
    return points_.empty() ? nullptr : &points_.back();
}

const PathPoint& Path::at(std::size_t index) const
{
    return points_.at(index);
}

glm::vec3 Path::positionFor(std::size_t index, float entityWidth) const
{
    const PathPoint& point = points_.at(index);
    const float centre = static_cast<float>(
        static_cast<int>(entityWidth + 1.0f)
    ) * 0.5f;
    return glm::vec3(point.position) + glm::vec3(centre, 0.0f, centre);
}

std::size_t PathFinder::PositionHash::operator()(
    const glm::ivec3& value) const noexcept
{
    std::size_t seed = static_cast<std::size_t>(
        static_cast<std::uint32_t>(value.x)
    );
    seed ^= static_cast<std::size_t>(
        static_cast<std::uint32_t>(value.y)
    ) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    seed ^= static_cast<std::size_t>(
        static_cast<std::uint32_t>(value.z)
    ) + 0x9e3779b9U + (seed << 6U) + (seed >> 2U);
    return seed;
}

PathNodeType PathFinder::classify(
    const NavigationBlockAccess& world,
    int x,
    int y,
    int z)
{
    if (!world.loaded(x, y, z))
        return PathNodeType::Blocked;
    const content::BlockState state = world.blockState(x, y, z);
    const BlockType block = state.block();
    if (block == BlockType::Air || isPlant(block) || isLadder(block) ||
        block == BlockType::RedstoneWire || block == BlockType::RedstoneTorch ||
        block == BlockType::Lever || block == BlockType::Repeater)
        return PathNodeType::Open;
    if (block == BlockType::Water)
        return PathNodeType::Water;
    if (block == BlockType::Lava)
        return PathNodeType::Lava;
    if (block == BlockType::Cactus)
        return PathNodeType::DamageCactus;
    if (tallCollision(state))
        return PathNodeType::Fence;
    return hasCollision(state) ? PathNodeType::Blocked : PathNodeType::Open;
}

PathNodeType PathFinder::classifyVolume(
    const NavigationBlockAccess& world,
    const NavigationSettings& settings,
    const glm::ivec3& position)
{
    const int sizeX = static_cast<int>(std::floor(settings.width + 1.0f));
    const int sizeY = static_cast<int>(std::floor(settings.height + 1.0f));
    const int sizeZ = sizeX;
    PathNodeType first = PathNodeType::Blocked;
    PathNodeType selected = PathNodeType::Blocked;
    float selectedPriority = settings.priority(selected);
    bool containsFence = false;

    for (int x = 0; x < sizeX; ++x)
    for (int y = 0; y < sizeY; ++y)
    for (int z = 0; z < sizeZ; ++z)
    {
        PathNodeType type = classify(
            world, position.x + x, position.y + y, position.z + z
        );
        if (x == 0 && y == 0 && z == 0)
            first = type;
        if (type == PathNodeType::Fence)
            containsFence = true;
        if (settings.priority(type) < 0.0f)
            return type;
        if (settings.priority(type) >= selectedPriority)
        {
            selected = type;
            selectedPriority = settings.priority(type);
        }
    }
    if (containsFence)
        return PathNodeType::Fence;
    if (first == PathNodeType::Open && selectedPriority == 0.0f)
    {
        PathNodeType result = PathNodeType::Open;
        const PathNodeType below = classify(
            world, position.x, position.y - 1, position.z
        );
        if (below == PathNodeType::DamageCactus)
            result = PathNodeType::DamageCactus;
        else if (below == PathNodeType::Lava)
            result = PathNodeType::DamageFire;
        else if (below != PathNodeType::Open &&
                 below != PathNodeType::Water)
            result = PathNodeType::Walkable;

        if (result == PathNodeType::Walkable)
        {
            for (int x = -1; x <= sizeX; ++x)
            for (int z = -1; z <= sizeZ; ++z)
            {
                const PathNodeType neighbour = classify(
                    world, position.x + x, position.y,
                    position.z + z
                );
                if (neighbour == PathNodeType::DamageCactus)
                    return PathNodeType::DangerCactus;
                if (neighbour == PathNodeType::Lava ||
                    neighbour == PathNodeType::DamageFire)
                    return PathNodeType::DangerFire;
            }
        }
        return result;
    }
    return selected;
}

bool PathFinder::volumeClear(
    const NavigationBlockAccess& world,
    const NavigationSettings& settings,
    const glm::ivec3& position)
{
    const int sizeX = static_cast<int>(std::floor(settings.width + 1.0f));
    const int sizeY = static_cast<int>(std::floor(settings.height + 1.0f));
    const int sizeZ = sizeX;
    for (int x = 0; x < sizeX; ++x)
    for (int y = 0; y < sizeY; ++y)
    for (int z = 0; z < sizeZ; ++z)
    {
        if (!world.loaded(position.x + x, position.y + y, position.z + z))
            return false;
        if (hasCollision(world.blockState(
                position.x + x, position.y + y, position.z + z)))
            return false;
    }
    return true;
}

std::optional<Path> PathFinder::findPath(
    const NavigationBlockAccess& world,
    const NavigationSettings& settings,
    const glm::vec3& start,
    const glm::vec3& target,
    float maximumDistance)
{
    if (maximumDistance <= 0.0f)
        return std::nullopt;

    glm::ivec3 startPosition = floorPosition(start);
    glm::ivec3 targetPosition = floorPosition(target);
    if (settings.kind == NavigationKind::Ground ||
        settings.kind == NavigationKind::Climbing)
    {
        while (startPosition.y > 0 &&
               classify(world, startPosition.x, startPosition.y,
                        startPosition.z) == PathNodeType::Open &&
               classify(world, startPosition.x, startPosition.y - 1,
                        startPosition.z) == PathNodeType::Open)
            --startPosition.y;
    }

    // SearchPoint pointers are stored in the open set. An unordered_map of
    // values rehashes and invalidates those pointers once it grows past its
    // reserved bucket count — that was a render-distance-scaled crash.
    std::deque<SearchPoint> storage;
    std::unordered_map<glm::ivec3, SearchPoint*, PositionHash> points;
    points.reserve(1024);
    auto point = [&storage, &points](const glm::ivec3& position) -> SearchPoint&
    {
        if (const auto found = points.find(position); found != points.end())
            return *found->second;
        storage.push_back({});
        storage.back().position = position;
        points.emplace(position, &storage.back());
        return storage.back();
    };

    SearchPoint& startPoint = point(startPosition);
    startPoint.totalPathDistance = 0.0f;
    startPoint.distanceToNext = manhattan(startPosition, targetPosition);
    startPoint.distanceToTarget = startPoint.distanceToNext;
    startPoint.nodeType = classifyVolume(world, settings, startPosition);

    struct OpenEntry
    {
        float distance = 0.0f;
        SearchPoint* point = nullptr;
        bool operator>(const OpenEntry& other) const noexcept
        {
            return distance > other.distance;
        }
    };
    std::priority_queue<
        OpenEntry, std::vector<OpenEntry>, std::greater<OpenEntry>
    > open;
    open.push({startPoint.distanceToTarget, &startPoint});
    SearchPoint* closest = &startPoint;

    const auto resolveGround = [&](const glm::ivec3& candidate,
                                   int stepHeight,
                                   auto&& self) -> SearchPoint*
    {
        glm::ivec3 resultPosition = candidate;
        PathNodeType type = classifyVolume(world, settings, resultPosition);
        float priority = settings.priority(type);
        if (priority < 0.0f && stepHeight > 0 &&
            type != PathNodeType::Fence && type != PathNodeType::Trapdoor)
            return self(candidate + glm::ivec3(0, 1, 0),
                        stepHeight - 1, self);
        if (priority < 0.0f)
            return nullptr;

        if (type == PathNodeType::Open)
        {
            int fall = 0;
            while (resultPosition.y > 0 && type == PathNodeType::Open)
            {
                if (fall++ >= settings.maximumFallHeight)
                    return nullptr;
                --resultPosition.y;
                type = classifyVolume(world, settings, resultPosition);
                priority = settings.priority(type);
            }
            if (type == PathNodeType::Blocked || type == PathNodeType::Fence)
            {
                ++resultPosition.y;
                type = PathNodeType::Walkable;
                priority = settings.priority(type);
            }
        }
        if (!volumeClear(world, settings, resultPosition) &&
            type != PathNodeType::Water)
            return nullptr;
        SearchPoint& result = point(resultPosition);
        result.nodeType = type;
        result.costMalus = std::max(result.costMalus, priority);
        return &result;
    };

    int iterations = 0;
    while (!open.empty())
    {
        if (++iterations >= 200)
            break;
        SearchPoint* current = open.top().point;
        open.pop();
        if (current->visited)
            continue;
        if (current->position == targetPosition)
        {
            closest = current;
            break;
        }
        if (manhattan(current->position, targetPosition) <
            manhattan(closest->position, targetPosition))
            closest = current;
        current->visited = true;

        std::array<SearchPoint*, 32> options{};
        std::size_t optionCount = 0;
        if (settings.kind == NavigationKind::Flying)
        {
            for (int x = -1; x <= 1; ++x)
            for (int y = -1; y <= 1; ++y)
            for (int z = -1; z <= 1; ++z)
            {
                if (x == 0 && y == 0 && z == 0)
                    continue;
                const glm::ivec3 candidate = current->position +
                    glm::ivec3(x, y, z);
                const PathNodeType type = classifyVolume(
                    world, settings, candidate
                );
                const float priority = settings.priority(type);
                if (priority < 0.0f || !volumeClear(world, settings, candidate))
                    continue;
                SearchPoint& next = point(candidate);
                next.nodeType = type;
                next.costMalus = std::max(next.costMalus, priority);
                options[optionCount++] = &next;
            }
        }
        else if (settings.kind == NavigationKind::Swimming)
        {
            constexpr std::array<glm::ivec3, 6> directions{{
                {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
            }};
            for (const glm::ivec3 direction : directions)
            {
                const glm::ivec3 candidate = current->position + direction;
                if (classifyVolume(world, settings, candidate) !=
                    PathNodeType::Water)
                    continue;
                SearchPoint& next = point(candidate);
                next.nodeType = PathNodeType::Water;
                next.costMalus = settings.priority(PathNodeType::Water);
                options[optionCount++] = &next;
            }
        }
        else
        {
            const int stepHeight = static_cast<int>(std::floor(
                std::max(1.0f, settings.stepHeight)
            ));
            constexpr std::array<glm::ivec3, 4> cardinal{{
                {0,0,1},{-1,0,0},{1,0,0},{0,0,-1}
            }};
            std::array<SearchPoint*, 4> cardinals{};
            for (std::size_t index = 0; index < cardinal.size(); ++index)
            {
                cardinals[index] = resolveGround(
                    current->position + cardinal[index], stepHeight,
                    resolveGround
                );
                if (cardinals[index] != nullptr)
                    options[optionCount++] = cardinals[index];
            }
            constexpr std::array<glm::ivec3, 4> diagonal{{
                {-1,0,-1},{1,0,-1},{-1,0,1},{1,0,1}
            }};
            constexpr std::array<std::pair<int,int>, 4> adjacent{{
                {3,1},{3,2},{0,1},{0,2}
            }};
            for (std::size_t index = 0; index < diagonal.size(); ++index)
            {
                SearchPoint* first = cardinals[adjacent[index].first];
                SearchPoint* second = cardinals[adjacent[index].second];
                const bool firstAllows = first == nullptr ||
                    first->nodeType == PathNodeType::Open ||
                    first->costMalus != 0.0f;
                const bool secondAllows = second == nullptr ||
                    second->nodeType == PathNodeType::Open ||
                    second->costMalus != 0.0f;
                if (!firstAllows || !secondAllows)
                    continue;
                if (SearchPoint* next = resolveGround(
                        current->position + diagonal[index], stepHeight,
                        resolveGround))
                    options[optionCount++] = next;
            }
        }

        for (std::size_t index = 0; index < optionCount; ++index)
        {
            SearchPoint* next = options[index];
            if (next == nullptr || next->visited ||
                distance(next->position, targetPosition) >= maximumDistance)
                continue;
            const float segment = manhattan(
                current->position, next->position
            );
            next->distanceFromOrigin = current->distanceFromOrigin + segment;
            next->cost = segment + next->costMalus;
            const float candidateDistance =
                current->totalPathDistance + next->cost;
            if (next->distanceFromOrigin < maximumDistance &&
                candidateDistance < next->totalPathDistance)
            {
                next->previous = current;
                next->totalPathDistance = candidateDistance;
                next->distanceToNext =
                    manhattan(next->position, targetPosition) + next->costMalus;
                next->distanceToTarget =
                    next->totalPathDistance + next->distanceToNext;
                open.push({next->distanceToTarget, next});
            }
        }
    }

    if (closest == &startPoint)
        return std::nullopt;
    std::vector<SearchPoint*> reverse;
    for (SearchPoint* current = closest;
         current != nullptr;
         current = current->previous)
        reverse.push_back(current);
    std::reverse(reverse.begin(), reverse.end());
    std::vector<PathPoint> result;
    result.reserve(reverse.size());
    for (const SearchPoint* current : reverse)
    {
        result.push_back({
            current->position,
            current->totalPathDistance,
            current->distanceToNext,
            current->distanceToTarget,
            current->distanceFromOrigin,
            current->cost,
            current->costMalus,
            current->nodeType
        });
    }
    return Path(std::move(result));
}

PathNavigation::PathNavigation(NavigationSettings settings)
    : settings_(std::move(settings)) {}

bool PathNavigation::tryMoveTo(
    const NavigationBlockAccess& world,
    const glm::vec3& currentPosition,
    const glm::vec3& target,
    double speed,
    float maximumDistance)
{
    std::optional<Path> path = pathFinder_.findPath(
        world, settings_, currentPosition, target, maximumDistance
    );
    if (!path)
    {
        if (settings_.kind == NavigationKind::Climbing)
        {
            climbingTarget_ = target;
            hasClimbingTarget_ = true;
        }
        return false;
    }
    setPath(std::move(*path), speed);
    return true;
}

void PathNavigation::setPath(Path path, double speed)
{
    path_ = std::move(path);
    speed_ = speed;
    ticksAtLastStuckCheck_ = totalTicks_;
    hasClimbingTarget_ = false;
}

void PathNavigation::clear() noexcept
{
    if (path_)
        path_->clear();
    path_.reset();
    desiredDirection_ = {};
    hasClimbingTarget_ = false;
}

void PathNavigation::tick(
    const NavigationBlockAccess& world,
    const glm::vec3& currentPosition,
    bool onGround,
    bool inLiquid)
{
    ++totalTicks_;
    desiredDirection_ = {};
    if (path_ && !path_->finished() && canNavigate(onGround, inLiquid))
    {
        const float reach = settings_.width > 0.75f
            ? settings_.width * 0.5f
            : 0.75f - settings_.width * 0.5f;
        glm::vec3 node = path_->positionFor(
            path_->currentIndex(), settings_.width
        );
        if (std::abs(currentPosition.x - node.x) < reach &&
            std::abs(currentPosition.z - node.z) < reach &&
            std::abs(currentPosition.y - node.y) < 1.0f)
        {
            path_->advance();
        }

        if (!path_->finished())
        {
            const std::size_t maximum = std::min(
                path_->size(), path_->currentIndex() + 6U
            );
            for (std::size_t index = maximum;
                 index > path_->currentIndex() + 1U;
                 --index)
            {
                const glm::vec3 shortcut = path_->positionFor(
                    index - 1U, settings_.width
                );
                if (directPath(world, currentPosition, shortcut))
                {
                    while (path_->currentIndex() < index - 1U)
                        path_->advance();
                    break;
                }
            }
        }

        if (totalTicks_ - ticksAtLastStuckCheck_ > 100)
        {
            const glm::vec3 delta = currentPosition - lastStuckCheckPosition_;
            if (glm::dot(delta, delta) < 2.25f)
                clear();
            ticksAtLastStuckCheck_ = totalTicks_;
            lastStuckCheckPosition_ = currentPosition;
        }
    }

    glm::vec3 target{};
    bool hasTarget = false;
    if (path_ && !path_->finished())
    {
        target = path_->positionFor(path_->currentIndex(), settings_.width);
        hasTarget = true;
    }
    else if (hasClimbingTarget_)
    {
        target = climbingTarget_;
        hasTarget = true;
        const glm::vec3 delta = target - currentPosition;
        if (delta.x * delta.x + delta.z * delta.z <
                settings_.width * settings_.width &&
            delta.y * delta.y < 1.0f)
            hasClimbingTarget_ = false;
    }
    if (hasTarget)
    {
        glm::vec3 delta = target - currentPosition;
        if (settings_.kind == NavigationKind::Ground ||
            settings_.kind == NavigationKind::Climbing)
            delta.y = 0.0f;
        const float lengthSquared = glm::dot(delta, delta);
        if (lengthSquared > 0.000001f)
            desiredDirection_ = delta / std::sqrt(lengthSquared);
    }
}

bool PathNavigation::noPath() const noexcept
{
    return (!path_ || path_->finished()) && !hasClimbingTarget_;
}

std::optional<glm::vec3> PathNavigation::currentMoveTarget() const noexcept
{
    if (path_ && !path_->finished())
        return path_->positionFor(path_->currentIndex(), settings_.width);
    if (hasClimbingTarget_)
        return climbingTarget_;
    return std::nullopt;
}

const Path* PathNavigation::path() const noexcept
{
    return path_ ? &*path_ : nullptr;
}

glm::vec3 PathNavigation::desiredDirection() const noexcept
{
    return desiredDirection_;
}

double PathNavigation::speed() const noexcept { return speed_; }
void PathNavigation::setSpeed(double speed) noexcept { speed_ = speed; }
const NavigationSettings& PathNavigation::settings() const noexcept
{
    return settings_;
}
NavigationSettings& PathNavigation::settings() noexcept { return settings_; }

bool PathNavigation::canNavigate(bool onGround, bool inLiquid) const noexcept
{
    if (settings_.kind == NavigationKind::Flying ||
        settings_.kind == NavigationKind::Swimming)
        return true;
    return onGround || (settings_.canSwim && inLiquid);
}

bool PathNavigation::directPath(
    const NavigationBlockAccess& world,
    const glm::vec3& from,
    const glm::vec3& to) const
{
    const glm::vec3 delta = to - from;
    const float distanceValue = std::sqrt(glm::dot(delta, delta));
    if (distanceValue < 0.0001f)
        return true;
    const int samples = std::max(1, static_cast<int>(std::ceil(
        distanceValue * 2.0f
    )));
    for (int sample = 1; sample <= samples; ++sample)
    {
        const glm::vec3 position = from + delta *
            (static_cast<float>(sample) / static_cast<float>(samples));
        if (!PathFinder::volumeClear(
                world, settings_, floorPosition(position)))
            return false;
    }
    return true;
}
}
