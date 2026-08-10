#pragma once

#include <cstdint>

class JavaRandom
{
public:
    explicit JavaRandom(std::int64_t seed = 0);

    void setSeed(std::int64_t seed);

    [[nodiscard]] std::int32_t nextInt();
    [[nodiscard]] std::int32_t nextInt(std::int32_t bound);
    [[nodiscard]] std::int64_t nextLong();
    [[nodiscard]] float nextFloat();
    [[nodiscard]] double nextDouble();
    [[nodiscard]] bool nextBoolean();

private:
    std::uint64_t state_ = 0;

    [[nodiscard]] std::int32_t next(int bits);
};
