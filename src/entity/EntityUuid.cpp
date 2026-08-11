#include "entity/EntityUuid.h"

#include <random>

namespace mc::entity
{
EntityUuid EntityUuid::random()
{
    thread_local std::mt19937_64 random([]
    {
        std::random_device source;
        return (static_cast<std::uint64_t>(source()) << 32U) ^ source();
    }());
    EntityUuid result{random(), random()};
    result.most = (result.most & 0xffffffffffff0fffULL) |
                  0x0000000000004000ULL;
    result.least = (result.least & 0x3fffffffffffffffULL) |
                   0x8000000000000000ULL;
    return result;
}
}
