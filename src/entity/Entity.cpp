#include "entity/Entity.h"

#include "BlockShape.h"
#include "World.h"
#include "entity/LivingEntity.h"
#include "entity/Math.h"

#include <algorithm>
#include <cmath>

namespace mc::entity
{
Entity::Entity(World& world)
    : world_(&world)
{
    setPosition(0.0, 0.0, 0.0);
}

void Entity::onUpdate()
{
    if (!isDead_)
        onEntityUpdate();
}

void Entity::onEntityUpdate()
{
    prevPosX = posX;
    prevPosY = posY;
    prevPosZ = posZ;
    prevRotationPitch = rotationPitch;
    prevRotationYaw = rotationYaw;
    lastTickPosX = posX;
    lastTickPosY = posY;
    lastTickPosZ = posZ;

    if (rideCooldown_ > 0)
        --rideCooldown_;
    if (isRiding() && ridingEntity_->isDead())
        dismountRidingEntity();

    handleWaterMovement();

    if (isInLava())
        setOnFireFromLava();

    if (posY < -64.0)
        attackEntityFrom(DamageSource::OUT_OF_WORLD, 4.0f);

    if (fire_ > 0)
    {
        if (immuneToFire_)
        {
            fire_ -= 4;
            if (fire_ < 0)
                fire_ = 0;
        }
        else
        {
            if (fire_ % 20 == 0)
                attackEntityFrom(DamageSource::ON_FIRE, 1.0f);
            --fire_;
        }
    }

    if (hurtResistantTime > 0)
        --hurtResistantTime;

    ++ticksExisted_;
    firstUpdate_ = false;
}

bool Entity::attackEntityFrom(const DamageSource&, float)
{
    return false;
}

void Entity::applyEntityCollision(Entity& other)
{
    if (noClip || other.noClip)
        return;
    double dx = other.posX - posX;
    double dz = other.posZ - posZ;
    double absMax = std::max(std::abs(dx), std::abs(dz));
    if (absMax < 0.009999999776482582)
        return;
    absMax = std::sqrt(dx * dx + dz * dz);
    if (absMax < 1.0e-6)
        return;
    dx /= absMax;
    dz /= absMax;
    double scale = 1.0 / absMax;
    if (scale > 1.0)
        scale = 1.0;
    dx *= scale * 0.05000000074505806 * (1.0 - entityCollisionReduction);
    dz *= scale * 0.05000000074505806 * (1.0 - entityCollisionReduction);
    if (!isBeingRidden())
        addVelocity(-dx, 0.0, -dz);
    if (!other.isBeingRidden())
        other.addVelocity(dx, 0.0, dz);
}

void Entity::fall(float, float)
{
}

float Entity::getEyeHeight() const
{
    return height_ * 0.85f;
}

bool Entity::canBeCollidedWith() const
{
    return !isDead_;
}

bool Entity::canBePushed() const
{
    return false;
}

core::ResourceLocation Entity::getType() const
{
    return core::ResourceLocation("minecraft:entity");
}

bool Entity::isInRangeToRender3d(double x, double y, double z) const
{
    const double dx = posX - x;
    const double dy = posY - y;
    const double dz = posZ - z;
    return dx * dx + dy * dy + dz * dz < 128.0 * 128.0;
}

bool Entity::isAlive() const
{
    return !isDead_;
}

void Entity::setSize(float width, float height)
{
    if (width == width_ && height == height_)
        return;
    const float previousWidth = width_;
    width_ = width;
    height_ = height;
    if (width < previousWidth)
    {
        const double half = static_cast<double>(width) * 0.5;
        setEntityBoundingBox({
            posX - half, posY, posZ - half,
            posX + half, posY + static_cast<double>(height_), posZ + half
        });
        return;
    }
    const AxisAlignedBB box = boundingBox_;
    setEntityBoundingBox({
        box.minX, box.minY, box.minZ,
        box.minX + static_cast<double>(width_),
        box.minY + static_cast<double>(height_),
        box.minZ + static_cast<double>(width_)
    });
}

void Entity::setPosition(double x, double y, double z)
{
    posX = x;
    posY = y;
    posZ = z;
    const double half = static_cast<double>(width_) * 0.5;
    setEntityBoundingBox({
        x - half, y, z - half,
        x + half, y + static_cast<double>(height_), z + half
    });
}

void Entity::setLocationAndAngles(
    double x, double y, double z, float yaw, float pitch)
{
    prevPosX = x;
    prevPosY = y;
    prevPosZ = z;
    lastTickPosX = x;
    lastTickPosY = y;
    lastTickPosZ = z;
    rotationYaw = yaw;
    rotationPitch = pitch;
    prevRotationYaw = yaw;
    prevRotationPitch = pitch;
    setPosition(x, y, z);
}

void Entity::setPositionAndUpdate(double x, double y, double z)
{
    setPosition(x, y, z);
}

void Entity::collectCollisionBoxes(
    const AxisAlignedBB& area,
    std::vector<AxisAlignedBB>& out) const
{
    const int minX = floorInt(area.minX) - 1;
    const int maxX = floorInt(area.maxX) + 1;
    const int minY = std::max(0, floorInt(area.minY) - 1);
    const int maxY = std::min(255, floorInt(area.maxY) + 1);
    const int minZ = floorInt(area.minZ) - 1;
    const int maxZ = floorInt(area.maxZ) + 1;
    for (int y = minY; y <= maxY; ++y)
    for (int z = minZ; z <= maxZ; ++z)
    for (int x = minX; x <= maxX; ++x)
    {
        if (!world_->isBlockLoaded(x, y, z))
            continue;
        const auto state = world_->getActualBlockState(x, y, z);
        for (const BlockBox& box : getBlockShape(state).collisionBoxes)
        {
            out.push_back({
                static_cast<double>(x) + box.minimum.x,
                static_cast<double>(y) + box.minimum.y,
                static_cast<double>(z) + box.minimum.z,
                static_cast<double>(x) + box.maximum.x,
                static_cast<double>(y) + box.maximum.y,
                static_cast<double>(z) + box.maximum.z
            });
        }
    }
}

void Entity::move(MoverType type, double x, double y, double z)
{
    if (noClip)
    {
        setEntityBoundingBox(boundingBox_.offset(x, y, z));
        resetPositionToBB();
        return;
    }

    if (inWeb_)
    {
        inWeb_ = false;
        x *= 0.25;
        y *= 0.05000000074505806;
        z *= 0.25;
        motionX = 0.0;
        motionY = 0.0;
        motionZ = 0.0;
    }

    double originalX = x;
    const double originalY = y;
    double originalZ = z;

    if ((type == MoverType::Self || type == MoverType::Player) &&
        onGround && isSneaking() && entityKind() == EntityKind::Player)
    {
        constexpr double step = 0.05;
        while (x != 0.0 &&
               world_->getCollisionBoxes(
                   this,
                   boundingBox_.offset(x, -static_cast<double>(stepHeight), 0.0)
               ).empty())
        {
            if (x < step && x >= -step) x = 0.0;
            else if (x > 0.0) x -= step;
            else x += step;
            originalX = x;
        }
        while (z != 0.0 &&
               world_->getCollisionBoxes(
                   this,
                   boundingBox_.offset(0.0, -static_cast<double>(stepHeight), z)
               ).empty())
        {
            if (z < step && z >= -step) z = 0.0;
            else if (z > 0.0) z -= step;
            else z += step;
            originalZ = z;
        }
        while (x != 0.0 && z != 0.0 &&
               world_->getCollisionBoxes(
                   this,
                   boundingBox_.offset(x, -static_cast<double>(stepHeight), z)
               ).empty())
        {
            if (x < step && x >= -step) x = 0.0;
            else if (x > 0.0) x -= step;
            else x += step;
            if (z < step && z >= -step) z = 0.0;
            else if (z > 0.0) z -= step;
            else z += step;
            originalX = x;
            originalZ = z;
        }
    }

    std::vector<AxisAlignedBB> boxes;
    boxes.reserve(64);
    collectCollisionBoxes(boundingBox_.expand(x, y, z), boxes);

    AxisAlignedBB current = boundingBox_;
    for (const AxisAlignedBB& box : boxes)
        y = box.calculateYOffset(current, y);
    current = current.offset(0.0, y, 0.0);

    const bool steppedOnGround = onGround || (originalY != y && originalY < 0.0);

    for (const AxisAlignedBB& box : boxes)
        x = box.calculateXOffset(current, x);
    current = current.offset(x, 0.0, 0.0);

    for (const AxisAlignedBB& box : boxes)
        z = box.calculateZOffset(current, z);
    current = current.offset(0.0, 0.0, z);

    if (stepHeight > 0.0f && steppedOnGround && (originalX != x || originalZ != z))
    {
        const double firstX = x;
        const double firstY = y;
        const double firstZ = z;
        const AxisAlignedBB firstBox = current;

        y = static_cast<double>(stepHeight);
        std::vector<AxisAlignedBB> stepBoxes;
        collectCollisionBoxes(
            boundingBox_.expand(originalX, y, originalZ), stepBoxes);

        AxisAlignedBB stepA = boundingBox_;
        AxisAlignedBB stepB = stepA.expand(originalX, 0.0, originalZ);
        double stepY = y;
        for (const AxisAlignedBB& box : stepBoxes)
            stepY = box.calculateYOffset(stepB, stepY);
        stepA = stepA.offset(0.0, stepY, 0.0);
        double stepX = originalX;
        for (const AxisAlignedBB& box : stepBoxes)
            stepX = box.calculateXOffset(stepA, stepX);
        stepA = stepA.offset(stepX, 0.0, 0.0);
        double stepZ = originalZ;
        for (const AxisAlignedBB& box : stepBoxes)
            stepZ = box.calculateZOffset(stepA, stepZ);
        stepA = stepA.offset(0.0, 0.0, stepZ);

        AxisAlignedBB stepC = boundingBox_;
        double stepY2 = y;
        for (const AxisAlignedBB& box : stepBoxes)
            stepY2 = box.calculateYOffset(stepC, stepY2);
        stepC = stepC.offset(0.0, stepY2, 0.0);
        double stepX2 = originalX;
        for (const AxisAlignedBB& box : stepBoxes)
            stepX2 = box.calculateXOffset(stepC, stepX2);
        stepC = stepC.offset(stepX2, 0.0, 0.0);
        double stepZ2 = originalZ;
        for (const AxisAlignedBB& box : stepBoxes)
            stepZ2 = box.calculateZOffset(stepC, stepZ2);
        stepC = stepC.offset(0.0, 0.0, stepZ2);

        const double horizA = stepX * stepX + stepZ * stepZ;
        const double horizC = stepX2 * stepX2 + stepZ2 * stepZ2;
        if (horizA > horizC)
        {
            x = stepX;
            z = stepZ;
            y = -stepY;
            current = stepA;
        }
        else
        {
            x = stepX2;
            z = stepZ2;
            y = -stepY2;
            current = stepC;
        }

        for (const AxisAlignedBB& box : stepBoxes)
            y = box.calculateYOffset(current, y);
        current = current.offset(0.0, y, 0.0);

        if (firstX * firstX + firstZ * firstZ >= x * x + z * z)
        {
            x = firstX;
            y = firstY;
            z = firstZ;
            current = firstBox;
        }
    }

    setEntityBoundingBox(current);
    resetPositionToBB();

    collidedHorizontally = originalX != x || originalZ != z;
    collidedVertically = originalY != y;
    onGround = collidedVertically && originalY < 0.0;
    collided = collidedHorizontally || collidedVertically;

    const int blockX = floorInt(posX);
    const int blockY = floorInt(posY - 0.20000000298023224);
    const int blockZ = floorInt(posZ);
    updateFallState(y, onGround, blockX, blockY, blockZ);

    if (originalX != x)
        motionX = 0.0;
    if (originalZ != z)
        motionZ = 0.0;
    if (originalY != y)
    {
        if (originalY < 0.0)
            motionY = 0.0;
        else
            motionY = 0.0;
    }

    doBlockCollisions();
}

void Entity::resetPositionToBB()
{
    posX = (boundingBox_.minX + boundingBox_.maxX) * 0.5;
    posY = boundingBox_.minY;
    posZ = (boundingBox_.minZ + boundingBox_.maxZ) * 0.5;
}

void Entity::moveRelative(float strafe, float up, float forward, float friction)
{
    float length = strafe * strafe + up * up + forward * forward;
    if (length < 1.0e-4f)
        return;
    length = std::sqrt(length);
    if (length < 1.0f)
        length = 1.0f;
    length = friction / length;
    strafe *= length;
    up *= length;
    forward *= length;
    const float sinYaw = std::sin(toRadians(rotationYaw));
    const float cosYaw = std::cos(toRadians(rotationYaw));
    motionX += static_cast<double>(strafe * cosYaw - forward * sinYaw);
    motionY += static_cast<double>(up);
    motionZ += static_cast<double>(forward * cosYaw + strafe * sinYaw);
}

void Entity::addVelocity(double x, double y, double z)
{
    motionX += x;
    motionY += y;
    motionZ += z;
    isAirBorne = true;
}

void Entity::setDead()
{
    isDead_ = true;
}

bool Entity::isInLava() const noexcept
{
    const AxisAlignedBB box = boundingBox_.grow(-0.1, -0.4, -0.1);
    const int minX = floorInt(box.minX);
    const int maxX = floorInt(box.maxX);
    const int minY = floorInt(box.minY);
    const int maxY = floorInt(box.maxY);
    const int minZ = floorInt(box.minZ);
    const int maxZ = floorInt(box.maxZ);
    for (int y = minY; y <= maxY; ++y)
    for (int z = minZ; z <= maxZ; ++z)
    for (int x = minX; x <= maxX; ++x)
        if (world_->getBlock(x, y, z) == BlockType::Lava)
            return true;
    return false;
}

bool Entity::handleWaterMovement()
{
    const AxisAlignedBB box = boundingBox_.grow(0.0, -0.4000000059604645, 0.0)
                                  .contract(0.001);
    bool water = false;
    const int minX = floorInt(box.minX);
    const int maxX = floorInt(box.maxX);
    const int minY = floorInt(box.minY);
    const int maxY = floorInt(box.maxY);
    const int minZ = floorInt(box.minZ);
    const int maxZ = floorInt(box.maxZ);
    glm::dvec3 push(0.0);
    int samples = 0;
    for (int y = minY; y <= maxY; ++y)
    for (int z = minZ; z <= maxZ; ++z)
    for (int x = minX; x <= maxX; ++x)
    {
        if (world_->getBlock(x, y, z) != BlockType::Water)
            continue;
        water = true;
        if (isPushedByWater())
        {
            const glm::vec3 flow = world_->getFluidFlowVector(
                x, y, z, BlockType::Water);
            push += glm::dvec3(flow);
            ++samples;
        }
    }
    if (water)
    {
        fallDistance = 0.0f;
        inWater_ = true;
        extinguish();
        if (samples > 0 && isPushedByWater())
        {
            push /= static_cast<double>(samples);
            const double len = std::sqrt(push.x * push.x + push.y * push.y + push.z * push.z);
            if (len > 0.0)
            {
                push /= len;
                constexpr double accel = 0.014;
                motionX += push.x * accel;
                motionY += push.y * accel;
                motionZ += push.z * accel;
            }
        }
    }
    else
    {
        inWater_ = false;
    }
    return inWater_;
}

void Entity::setFire(int seconds)
{
    int ticks = seconds * 20;
    if (fire_ < ticks)
        fire_ = ticks;
}

void Entity::dealFireDamage(int amount)
{
    if (!immuneToFire_)
        attackEntityFrom(DamageSource::IN_FIRE, static_cast<float>(amount));
}

void Entity::setOnFireFromLava()
{
    if (!immuneToFire_)
    {
        attackEntityFrom(DamageSource::LAVA, 4.0f);
        setFire(15);
    }
}

Entity* Entity::getControllingPassenger() const noexcept
{
    return riddenBy_.empty() ? nullptr : riddenBy_.front();
}

void Entity::startRiding(Entity& vehicle)
{
    if (ridingEntity_ == &vehicle)
        return;
    dismountRidingEntity();
    ridingEntity_ = &vehicle;
    vehicle.riddenBy_.push_back(this);
}

void Entity::dismountRidingEntity()
{
    if (ridingEntity_ == nullptr)
        return;
    auto& list = ridingEntity_->riddenBy_;
    std::erase(list, this);
    ridingEntity_ = nullptr;
    rideCooldown_ = 60;
}

void Entity::removePassengers()
{
    while (!riddenBy_.empty())
        riddenBy_.back()->dismountRidingEntity();
}

void Entity::updatePassenger(Entity& passenger)
{
    if (std::find(riddenBy_.begin(), riddenBy_.end(), &passenger) == riddenBy_.end())
        return;
    passenger.setPosition(posX, posY + static_cast<double>(height_) * 0.75, posZ);
}

glm::vec3 Entity::getPositionVec() const noexcept
{
    return {
        static_cast<float>(posX),
        static_cast<float>(posY),
        static_cast<float>(posZ)
    };
}

glm::vec3 Entity::getInterpolatedPosition(float partialTick) const
{
    const double px = prevPosX + (posX - prevPosX) * static_cast<double>(partialTick);
    const double py = prevPosY + (posY - prevPosY) * static_cast<double>(partialTick);
    const double pz = prevPosZ + (posZ - prevPosZ) * static_cast<double>(partialTick);
    return {
        static_cast<float>(px),
        static_cast<float>(py),
        static_cast<float>(pz)
    };
}

glm::vec3 Entity::getLookVec() const
{
    const float pitch = toRadians(rotationPitch);
    const float yaw = toRadians(rotationYaw);
    return {
        -std::sin(yaw) * std::cos(pitch),
        -std::sin(pitch),
        std::cos(yaw) * std::cos(pitch)
    };
}

double Entity::getDistanceSq(const Entity& other) const
{
    return getDistanceSq(other.posX, other.posY, other.posZ);
}

double Entity::getDistanceSq(double x, double y, double z) const
{
    const double dx = posX - x;
    const double dy = posY - y;
    const double dz = posZ - z;
    return dx * dx + dy * dy + dz * dz;
}

float Entity::getDistance(const Entity& other) const
{
    return static_cast<float>(std::sqrt(getDistanceSq(other)));
}

bool Entity::canEntityBeSeen(const Entity& other) const
{
    const double x0 = posX;
    const double y0 = posY + static_cast<double>(getEyeHeight());
    const double z0 = posZ;
    const double x1 = other.posX;
    const double y1 = other.posY + static_cast<double>(other.getEyeHeight());
    const double z1 = other.posZ;
    const double dx = x1 - x0;
    const double dy = y1 - y0;
    const double dz = z1 - z0;
    const int steps = std::max(1, static_cast<int>(std::ceil(
        std::sqrt(dx * dx + dy * dy + dz * dz) * 2.0)));
    for (int i = 1; i < steps; ++i)
    {
        const double t = static_cast<double>(i) / static_cast<double>(steps);
        const int bx = floorInt(x0 + dx * t);
        const int by = floorInt(y0 + dy * t);
        const int bz = floorInt(z0 + dz * t);
        if (world_->isSolidBlock(bx, by, bz))
            return false;
    }
    return true;
}

void Entity::doBlockCollisions()
{
    const int minX = floorInt(boundingBox_.minX + 0.001);
    const int minY = floorInt(boundingBox_.minY + 0.001);
    const int minZ = floorInt(boundingBox_.minZ + 0.001);
    const int maxX = floorInt(boundingBox_.maxX - 0.001);
    const int maxY = floorInt(boundingBox_.maxY - 0.001);
    const int maxZ = floorInt(boundingBox_.maxZ - 0.001);
    for (int y = minY; y <= maxY; ++y)
    for (int z = minZ; z <= maxZ; ++z)
    for (int x = minX; x <= maxX; ++x)
    {
        const BlockType block = world_->getBlock(x, y, z);
        if (block == BlockType::Cobweb)
        {
            inWeb_ = true;
            fallDistance = 0.0f;
        }
        else if (block == BlockType::Cactus)
        {
            attackEntityFrom(DamageSource::CACTUS, 1.0f);
        }
    }
}

void Entity::updateFallState(
    double y,
    bool onGroundIn,
    int,
    int,
    int)
{
    if (onGroundIn)
    {
        if (fallDistance > 0.0f)
        {
            fall(fallDistance, 1.0f);
            fallDistance = 0.0f;
        }
    }
    else if (y < 0.0)
    {
        fallDistance -= static_cast<float>(y);
    }
}

bool Entity::isOffsetPositionInLiquid(double x, double y, double z) const
{
    const AxisAlignedBB box = boundingBox_.offset(x, y, z);
    return world_->getCollisionBoxes(this, box).empty();
}
}
