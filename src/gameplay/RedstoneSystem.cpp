#include "gameplay/RedstoneSystem.h"

#include "Block.h"
#include "World.h"
#include "content/ContentCatalog.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mc::gameplay
{
namespace
{
struct Direction
{
    int x;
    int y;
    int z;
    std::string_view name;
};

constexpr std::array<Direction, 6> Directions{{
    {-1, 0, 0, "west"},
    { 1, 0, 0, "east"},
    { 0,-1, 0, "down"},
    { 0, 1, 0, "up"},
    { 0, 0,-1, "north"},
    { 0, 0, 1, "south"}
}};

constexpr std::array<Direction, 4> Horizontal{{
    {-1, 0, 0, "west"},
    { 1, 0, 0, "east"},
    { 0, 0,-1, "north"},
    { 0, 0, 1, "south"}
}};

const mc::content::ContentCatalog* catalog()
{
    return mc::content::ContentCatalog::active();
}

std::string_view blockName(mc::content::BlockState state)
{
    const auto* c = catalog();
    if (c == nullptr)
        return {};
    const auto* name = c->blockName(state);
    return name == nullptr ? std::string_view{} : name->path();
}

std::vector<std::pair<std::string, std::string>> properties(
    mc::content::BlockState state)
{
    const auto* c = catalog();
    return c == nullptr
        ? std::vector<std::pair<std::string, std::string>>{}
        : c->serializeStateProperties(state);
}

std::optional<std::string> property(
    mc::content::BlockState state,
    std::string_view key)
{
    for (const auto& [name, value] : properties(state))
        if (name == key)
            return value;
    return std::nullopt;
}

bool boolProperty(
    mc::content::BlockState state,
    std::string_view key,
    bool fallback = false)
{
    const auto value = property(state, key);
    if (!value)
        return fallback;
    return *value == "true" || *value == "1";
}

int intProperty(
    mc::content::BlockState state,
    std::string_view key,
    int fallback = 0)
{
    const auto value = property(state, key);
    if (!value)
        return fallback;
    int result = fallback;
    const auto parse = std::from_chars(
        value->data(), value->data() + value->size(), result);
    return parse.ec == std::errc{} ? result : fallback;
}

Direction directionFor(
    mc::content::BlockState state,
    Direction fallback = {0, 0, -1, "north"})
{
    const auto facing = property(state, "facing");
    if (!facing)
        return fallback;

    for (const Direction& direction : Directions)
        if (*facing == direction.name)
            return direction;
    return fallback;
}

Direction opposite(Direction direction)
{
    return {-direction.x, -direction.y, -direction.z, {}};
}

bool isName(std::string_view name, std::string_view value)
{
    return name == value;
}

bool isAny(
    std::string_view name,
    std::initializer_list<std::string_view> values)
{
    return std::find(values.begin(), values.end(), name) != values.end();
}

bool isWire(std::string_view name)
{
    return name == "redstone_wire";
}

bool isTorch(std::string_view name)
{
    return isAny(name, {
        "redstone_torch",
        "unlit_redstone_torch"
    });
}

bool isRepeater(std::string_view name)
{
    return isAny(name, {
        "unpowered_repeater",
        "powered_repeater"
    });
}

bool isComparator(std::string_view name)
{
    return isAny(name, {
        "unpowered_comparator",
        "powered_comparator"
    });
}

bool isButton(std::string_view name)
{
    return name == "stone_button" || name == "wooden_button" ||
           name.ends_with("_button");
}

bool isPressurePlate(std::string_view name)
{
    return name.ends_with("_pressure_plate") ||
           name == "light_weighted_pressure_plate" ||
           name == "heavy_weighted_pressure_plate";
}

bool isConstantSource(std::string_view name)
{
    return isAny(name, {
        "redstone_block",
        "daylight_detector",
        "daylight_detector_inverted",
        "detector_rail",
        "observer"
    });
}

bool isPowerableConsumer(std::string_view name)
{
    return name == "redstone_lamp" ||
           name == "lit_redstone_lamp" ||
           name == "tnt" ||
           name == "iron_door" ||
           name == "wooden_door" ||
           name.ends_with("_door") ||
           name == "trapdoor" ||
           name == "iron_trapdoor" ||
           name.ends_with("_trapdoor") ||
           name.ends_with("_fence_gate") ||
           name == "dispenser" ||
           name == "dropper" ||
           name == "hopper" ||
           name == "noteblock" ||
           name == "piston" ||
           name == "sticky_piston";
}

bool setProperties(
    World& world,
    int x, int y, int z,
    mc::content::BlockState current,
    std::initializer_list<std::pair<std::string, std::string>> changes)
{
    const auto* c = catalog();
    if (c == nullptr)
        return false;
    const auto* name = c->blockName(current);
    if (name == nullptr)
        return false;

    auto values = c->serializeStateProperties(current);
    for (const auto& [changeName, changeValue] : changes)
    {
        const auto found = std::find_if(
            values.begin(), values.end(),
            [&](const auto& entry)
            {
                return entry.first == changeName;
            });
        if (found != values.end())
            found->second = changeValue;
        else
            values.emplace_back(changeName, changeValue);
    }

    const auto next = c->state(*name, values);
    if (!next || *next == current)
        return false;
    return world.setBlockState(x, y, z, *next);
}

bool replaceBlockKeepingProperties(
    World& world,
    int x, int y, int z,
    mc::content::BlockState current,
    std::string_view replacement)
{
    const auto* c = catalog();
    if (c == nullptr)
        return false;
    auto values = c->serializeStateProperties(current);
    const auto next = c->state(
        mc::core::ResourceLocation("minecraft", std::string(replacement)),
        values
    );
    if (!next)
        return false;
    return world.setBlockState(x, y, z, *next);
}

bool isSolidAt(const World& world, int x, int y, int z)
{
    return world.isSolidBlock(x, y, z);
}

int clampPower(int value)
{
    return std::clamp(value, 0, 15);
}
}

std::uint64_t RedstoneSystem::key(int x, int y, int z) noexcept
{
    const std::uint64_t px =
        static_cast<std::uint32_t>(x) & 0x3FFFFFFULL;
    const std::uint64_t pz =
        static_cast<std::uint32_t>(z) & 0x3FFFFFFULL;
    return (px << 38U) |
           (pz << 12U) |
           static_cast<std::uint64_t>(y & 0xFFF);
}

void RedstoneSystem::enqueue(int x, int y, int z)
{
    if (y < 0 || y >= Chunk::HEIGHT)
        return;
    if (queued_.insert(key(x, y, z)).second)
        queue_.push_back({x, y, z});
}

void RedstoneSystem::enqueueNeighbours(int x, int y, int z)
{
    enqueue(x, y, z);
    for (const Direction& direction : Directions)
        enqueue(x + direction.x, y + direction.y, z + direction.z);

    // Redstone dust in 1.12 can connect one block upward/downward around
    // neighbouring solid blocks.
    for (const Direction& direction : Horizontal)
    {
        enqueue(x + direction.x, y + 1, z + direction.z);
        enqueue(x + direction.x, y - 1, z + direction.z);
    }
}

void RedstoneSystem::onBlockChanged(int x, int y, int z)
{
    enqueueNeighbours(x, y, z);
}

void RedstoneSystem::schedule(
    int x, int y, int z,
    mc::content::BlockState expected,
    std::uint64_t delay)
{
    scheduled_.push({
        gameTick_ + std::max<std::uint64_t>(1, delay),
        {x, y, z},
        expected
    });
}

int RedstoneSystem::directPower(
    const World& world,
    int x, int y, int z,
    int fromX, int fromY, int fromZ) const
{
    const mc::content::BlockState state =
        world.getBlockState(x, y, z);
    const std::string_view name = blockName(state);

    if (name.empty() || state.isAir())
        return 0;

    if (name == "redstone_block")
        return 15;

    if (name == "lever" && boolProperty(state, "powered",
                                        (state.properties() & 1U) != 0U))
        return 15;

    if (isButton(name) && boolProperty(state, "powered"))
        return 15;

    if (isPressurePlate(name))
    {
        if (const auto powered = property(state, "powered"))
            return *powered == "true" ? 15 : 0;
        return clampPower(intProperty(state, "power", 0));
    }

    if (isTorch(name))
    {
        const bool lit =
            name != "unlit_redstone_torch" &&
            boolProperty(state, "lit", true);
        if (!lit)
            return 0;

        // A wall/floor torch powers everything except the block it is attached
        // to. For resource states, facing points away from the support.
        const Direction facing = directionFor(state, {0, 1, 0, "up"});
        const int supportX = x - facing.x;
        const int supportY = y - facing.y;
        const int supportZ = z - facing.z;
        if (fromX == supportX && fromY == supportY && fromZ == supportZ)
            return 0;
        return 15;
    }

    if (isRepeater(name) || isComparator(name))
    {
        const bool powered =
            name.starts_with("powered_") ||
            boolProperty(state, "powered");
        if (!powered)
            return 0;
        const Direction facing = directionFor(state);
        return fromX == x + facing.x &&
               fromY == y + facing.y &&
               fromZ == z + facing.z
            ? 15 : 0;
    }

    if (name == "observer")
    {
        if (!boolProperty(state, "powered"))
            return 0;
        const Direction facing = directionFor(state);
        const Direction output = opposite(facing);
        return fromX == x + output.x &&
               fromY == y + output.y &&
               fromZ == z + output.z
            ? 15 : 0;
    }

    if (name == "daylight_detector" ||
        name == "daylight_detector_inverted")
        return clampPower(intProperty(state, "power", 0));

    if (name == "detector_rail")
        return boolProperty(state, "powered") ? 15 : 0;

    if (isWire(name))
        return clampPower(intProperty(
            state, "power",
            static_cast<int>(state.properties() & 15U)));

    return 0;
}

int RedstoneSystem::incomingPower(
    const World& world,
    int x, int y, int z) const
{
    int power = 0;
    for (const Direction& direction : Directions)
    {
        power = std::max(
            power,
            directPower(
                world,
                x + direction.x,
                y + direction.y,
                z + direction.z,
                x, y, z
            )
        );
        if (power >= 15)
            return 15;
    }
    return power;
}

int RedstoneSystem::wireNeighbourPower(
    const World& world,
    int x, int y, int z) const
{
    int best = 0;
    const bool spaceAbove =
        !isSolidAt(world, x, y + 1, z);

    for (const Direction& direction : Horizontal)
    {
        const int nx = x + direction.x;
        const int nz = z + direction.z;
        mc::content::BlockState adjacent =
            world.getBlockState(nx, y, nz);
        std::string_view adjacentName = blockName(adjacent);

        if (isWire(adjacentName))
            best = std::max(
                best,
                intProperty(
                    adjacent, "power",
                    static_cast<int>(adjacent.properties() & 15U))
            );

        if (isSolidAt(world, nx, y, nz) && spaceAbove)
        {
            const auto upper =
                world.getBlockState(nx, y + 1, nz);
            if (isWire(blockName(upper)))
                best = std::max(
                    best,
                    intProperty(
                        upper, "power",
                        static_cast<int>(upper.properties() & 15U))
                );
        }
        else if (!isSolidAt(world, nx, y, nz))
        {
            const auto lower =
                world.getBlockState(nx, y - 1, nz);
            if (isWire(blockName(lower)))
                best = std::max(
                    best,
                    intProperty(
                        lower, "power",
                        static_cast<int>(lower.properties() & 15U))
                );
        }
    }
    return best;
}

void RedstoneSystem::updatePosition(
    World& world,
    const Position& position)
{
    const int x = position.x;
    const int y = position.y;
    const int z = position.z;

    const mc::content::BlockState state =
        world.getBlockState(x, y, z);
    if (state.isAir())
        return;

    const std::string_view name = blockName(state);
    if (name.empty())
        return;

    if (isWire(name))
    {
        // Match 1.12 BlockRedstoneWire: direct non-wire power wins; otherwise
        // the strongest neighbouring wire loses one signal level.
        const int direct = incomingPower(world, x, y, z);
        const int neighbour = wireNeighbourPower(world, x, y, z);
        const int power = std::max(
            direct,
            std::max(0, neighbour - 1)
        );

        const int oldPower = intProperty(
            state, "power",
            static_cast<int>(state.properties() & 15U));

        if (power != oldPower)
        {
            if (!setProperties(
                    world, x, y, z, state,
                    {{"power", std::to_string(power)}}))
            {
                // Compatibility with the original legacy wire state.
                world.setBlockAndMetadata(
                    x, y, z,
                    BlockType::RedstoneWire,
                    static_cast<std::uint8_t>(power)
                );
            }
            enqueueNeighbours(x, y, z);
        }
        return;
    }

    if (isTorch(name))
    {
        const Direction facing =
            directionFor(state, {0, 1, 0, "up"});
        const int supportX = x - facing.x;
        const int supportY = y - facing.y;
        const int supportZ = z - facing.z;

        const bool shouldLight =
            incomingPower(
                world, supportX, supportY, supportZ) == 0;
        const bool lit =
            name != "unlit_redstone_torch" &&
            boolProperty(state, "lit", true);

        if (lit != shouldLight)
            schedule(x, y, z, state, 2);
        return;
    }

    if (isRepeater(name))
    {
        const Direction facing = directionFor(state);
        const Direction back = opposite(facing);

        const int rearPower = directPower(
            world,
            x + back.x,
            y + back.y,
            z + back.z,
            x, y, z
        );

        const Direction left{-facing.z, 0, facing.x, {}};
        const Direction right{facing.z, 0, -facing.x, {}};

        const auto sidePower = [&](Direction side)
        {
            const auto sideState = world.getBlockState(
                x + side.x, y, z + side.z);
            const std::string_view sideName =
                blockName(sideState);
            if (!isRepeater(sideName) &&
                !isComparator(sideName))
                return 0;
            return directPower(
                world,
                x + side.x, y, z + side.z,
                x, y, z
            );
        };

        const bool locked =
            std::max(sidePower(left), sidePower(right)) > 0;

        setProperties(
            world, x, y, z, state,
            {{"locked", locked ? "true" : "false"}}
        );

        if (locked)
            return;

        const bool powered =
            name == "powered_repeater" ||
            boolProperty(state, "powered");
        const bool shouldPower = rearPower > 0;

        if (powered != shouldPower)
        {
            const int delaySetting =
                std::clamp(intProperty(state, "delay", 1), 1, 4);
            schedule(
                x, y, z, state,
                static_cast<std::uint64_t>(delaySetting * 2)
            );
        }
        return;
    }

    if (isComparator(name))
    {
        const Direction facing = directionFor(state);
        const Direction back = opposite(facing);
        const Direction left{-facing.z, 0, facing.x, {}};
        const Direction right{facing.z, 0, -facing.x, {}};

        const int rear = directPower(
            world,
            x + back.x, y + back.y, z + back.z,
            x, y, z);
        const int side = std::max(
            directPower(
                world, x + left.x, y, z + left.z,
                x, y, z),
            directPower(
                world, x + right.x, y, z + right.z,
                x, y, z)
        );

        const bool subtract =
            property(state, "mode").value_or("compare") == "subtract";
        const int output = subtract
            ? std::max(0, rear - side)
            : (rear >= side ? rear : 0);

        const bool powered =
            name == "powered_comparator" ||
            boolProperty(state, "powered");

        if (powered != (output > 0) ||
            intProperty(state, "output", output) != output)
        {
            schedule(x, y, z, state, 2);
        }
        return;
    }

    if (isPowerableConsumer(name))
    {
        const bool powered =
            incomingPower(world, x, y, z) > 0;

        if (name == "redstone_lamp" && powered)
        {
            replaceBlockKeepingProperties(
                world, x, y, z, state, "lit_redstone_lamp");
            return;
        }
        if (name == "lit_redstone_lamp" && !powered)
        {
            // Vanilla lamps turn off two game ticks later.
            schedule(x, y, z, state, 2);
            return;
        }

        // Doors, trapdoors, fence gates, hoppers, dispensers/droppers and
        // pistons expose powered/triggered state in their 1.12 blockstate
        // definitions. Their mechanical action is handled when the scheduled
        // update is consumed below.
        setProperties(
            world, x, y, z, state,
            {{"powered", powered ? "true" : "false"}}
        );

        if (name.ends_with("_door") ||
            name.ends_with("_trapdoor") ||
            name.ends_with("_fence_gate"))
        {
            setProperties(
                world, x, y, z,
                world.getBlockState(x, y, z),
                {{"open", powered ? "true" : "false"}}
            );
        }

        if (name == "dispenser" || name == "dropper")
            setProperties(
                world, x, y, z,
                world.getBlockState(x, y, z),
                {{"triggered", powered ? "true" : "false"}}
            );

        return;
    }
}

void RedstoneSystem::tick(
    World& world,
    std::size_t updateBudget)
{
    ++gameTick_;

    while (!scheduled_.empty() &&
           scheduled_.top().dueTick <= gameTick_)
    {
        const ScheduledUpdate update = scheduled_.top();
        scheduled_.pop();

        const auto current = world.getBlockState(
            update.position.x,
            update.position.y,
            update.position.z
        );

        // A scheduled tick belongs to the block family at this position; if a
        // player replaced the block before the tick fired, discard it.
        if (blockName(current) != blockName(update.expected))
            continue;

        const std::string_view name = blockName(current);

        if (isTorch(name))
        {
            const Direction facing =
                directionFor(current, {0, 1, 0, "up"});
            const bool shouldLight =
                incomingPower(
                    world,
                    update.position.x - facing.x,
                    update.position.y - facing.y,
                    update.position.z - facing.z) == 0;

            if (shouldLight && name == "unlit_redstone_torch")
                replaceBlockKeepingProperties(
                    world,
                    update.position.x,
                    update.position.y,
                    update.position.z,
                    current,
                    "redstone_torch");
            else if (!shouldLight && name == "redstone_torch")
                replaceBlockKeepingProperties(
                    world,
                    update.position.x,
                    update.position.y,
                    update.position.z,
                    current,
                    "unlit_redstone_torch");

            enqueueNeighbours(
                update.position.x,
                update.position.y,
                update.position.z);
            continue;
        }

        if (isRepeater(name))
        {
            const Direction facing = directionFor(current);
            const Direction back = opposite(facing);
            const bool shouldPower =
                directPower(
                    world,
                    update.position.x + back.x,
                    update.position.y + back.y,
                    update.position.z + back.z,
                    update.position.x,
                    update.position.y,
                    update.position.z) > 0;

            const std::string_view target =
                shouldPower
                    ? "powered_repeater"
                    : "unpowered_repeater";

            if (!replaceBlockKeepingProperties(
                    world,
                    update.position.x,
                    update.position.y,
                    update.position.z,
                    current,
                    target))
            {
                setProperties(
                    world,
                    update.position.x,
                    update.position.y,
                    update.position.z,
                    current,
                    {{"powered", shouldPower ? "true" : "false"}}
                );
            }

            enqueueNeighbours(
                update.position.x,
                update.position.y,
                update.position.z);
            continue;
        }

        if (isComparator(name))
        {
            const Direction facing = directionFor(current);
            const Direction back = opposite(facing);
            const Direction left{-facing.z, 0, facing.x, {}};
            const Direction right{facing.z, 0, -facing.x, {}};

            const int rear = directPower(
                world,
                update.position.x + back.x,
                update.position.y + back.y,
                update.position.z + back.z,
                update.position.x,
                update.position.y,
                update.position.z);
            const int side = std::max(
                directPower(
                    world,
                    update.position.x + left.x,
                    update.position.y,
                    update.position.z + left.z,
                    update.position.x,
                    update.position.y,
                    update.position.z),
                directPower(
                    world,
                    update.position.x + right.x,
                    update.position.y,
                    update.position.z + right.z,
                    update.position.x,
                    update.position.y,
                    update.position.z)
            );

            const bool subtract =
                property(current, "mode").value_or("compare") ==
                "subtract";
            const int output = subtract
                ? std::max(0, rear - side)
                : (rear >= side ? rear : 0);
            const bool powered = output > 0;

            const std::string_view target =
                powered
                    ? "powered_comparator"
                    : "unpowered_comparator";
            replaceBlockKeepingProperties(
                world,
                update.position.x,
                update.position.y,
                update.position.z,
                current,
                target
            );

            setProperties(
                world,
                update.position.x,
                update.position.y,
                update.position.z,
                world.getBlockState(
                    update.position.x,
                    update.position.y,
                    update.position.z),
                {
                    {"powered", powered ? "true" : "false"},
                    {"output", std::to_string(output)}
                }
            );

            enqueueNeighbours(
                update.position.x,
                update.position.y,
                update.position.z);
            continue;
        }

        if (name == "lit_redstone_lamp")
        {
            if (incomingPower(
                    world,
                    update.position.x,
                    update.position.y,
                    update.position.z) == 0)
            {
                replaceBlockKeepingProperties(
                    world,
                    update.position.x,
                    update.position.y,
                    update.position.z,
                    current,
                    "redstone_lamp");
            }
            continue;
        }

        enqueue(update.position.x, update.position.y, update.position.z);
    }

    std::size_t processed = 0;
    while (!queue_.empty() && processed < updateBudget)
    {
        const Position position = queue_.front();
        queue_.pop_front();
        queued_.erase(key(position.x, position.y, position.z));
        ++processed;
        updatePosition(world, position);
    }
}

std::size_t RedstoneSystem::pendingUpdates() const noexcept
{
    return queue_.size() + scheduled_.size();
}

std::uint64_t RedstoneSystem::gameTick() const noexcept
{
    return gameTick_;
}
}
