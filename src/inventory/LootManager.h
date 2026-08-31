#pragma once

#include "LootTable.h"
#include "entities/EnemyMob.h"
#include "PassiveMob.h"

/**
 * @brief LootManager: Repositorio central de tablas de botín temáticas para todas las criaturas del juego.
 */
class LootManager {
public:
    static LootTable GetEnemyLoot(EnemyType type, int nightLevel = 1);
    static LootTable GetDeerLoot(DeerSize size);
    static LootTable GetChestLoot(int chestTier = 1);
    static LootTable GetDragonLoot();
};
