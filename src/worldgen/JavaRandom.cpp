#include "worldgen/JavaRandom.h"

#include <bit>
#include <stdexcept>

namespace
{
constexpr std::uint64_t multiplier = 0x5DEECE66DULL;
constexpr std::uint64_t addend = 0xBULL;
constexpr std::uint64_t mask = (1ULL << 48U) - 1ULL;
}

JavaRandom::JavaRandom(std::int64_t seed)
{
    setSeed(seed);
}

void JavaRandom::setSeed(std::int64_t seed)
{
    state_ = (static_cast<std::uint64_t>(seed) ^ multiplier) & mask;
}

std::int32_t JavaRandom::next(int bits)
{
    state_ = (state_ * multiplier + addend) & mask;
    return static_cast<std::int32_t>(state_ >> (48 - bits));
}

std::int32_t JavaRandom::nextInt()
{
    return next(32);
}

std::int32_t JavaRandom::nextInt(std::int32_t bound)
{
    if (bound <= 0)
    {
        throw std::invalid_argument("JavaRandom bound must be positive");
    }

    if ((bound & -bound) == bound)
    {
        return static_cast<std::int32_t>(
            (static_cast<std::int64_t>(bound) * next(31)) >> 31
        );
    }

    std::int32_t bits = 0;
    std::int32_t value = 0;
    do
    {
        bits = next(31);
        value = bits % bound;
    }
    while (bits - value + (bound - 1) < 0);

    return value;
}

std::int64_t JavaRandom::nextLong()
{
    const std::int64_t high = static_cast<std::int64_t>(next(32));
    const std::int64_t low = static_cast<std::int64_t>(next(32));

    const std::uint64_t highBits =
        static_cast<std::uint64_t>(static_cast<std::uint32_t>(high)) << 32U;
    const std::uint64_t lowBits = static_cast<std::uint64_t>(low);

    return std::bit_cast<std::int64_t>(highBits + lowBits);
}

float JavaRandom::nextFloat()
{
    return static_cast<float>(next(24)) /
           static_cast<float>(1U << 24U);
}

double JavaRandom::nextDouble()
{
    const std::uint64_t high = static_cast<std::uint64_t>(next(26));
    const std::uint64_t low = static_cast<std::uint64_t>(next(27));
    return static_cast<double>((high << 27U) + low) /
           static_cast<double>(1ULL << 53U);
}

bool JavaRandom::nextBoolean()
{
    return next(1) != 0;
}
