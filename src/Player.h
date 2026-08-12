#pragma once

#include "entity/PlayerEntity.h"

using PlayerPersistentState = mc::entity::PlayerEntity::PersistentState;

class Player : public mc::entity::PlayerEntity
{
public:
    using PlayerEntity::PlayerEntity;
};
