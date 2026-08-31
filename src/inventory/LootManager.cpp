#include "LootManager.h"

LootTable LootManager::GetEnemyLoot(EnemyType type, int nightLevel) {
    LootTable table;

    switch (type) {
        case EnemyType::SKELETON_ARCHER:
            // 100% Flechas de Caza (x4 - x12), 40% Madera de arco, 15% Arco Recurvo
            table.AddGuaranteedByString("hunting_arrow", 4, 12);
            table.AddIndependentDropByString("wood_log", 0.40f, 1, 2);
            table.AddIndependentDropByString("hunting_bow", 0.15f, 1, 1);
            break;

        case EnemyType::DEATH_KNIGHT:
            // 100% Piedra/Acero (x2 - x4), 60% Vial de Sangre, 20% Mandoble de Escarcha (2H)
            table.AddGuaranteedByString("stone_rock", 2, 4);
            table.AddIndependentDropByString("blood_vial", 0.60f, 1, 2);
            table.AddIndependentDropByString("frost_claymore", 0.20f, 1, 1);
            break;

        case EnemyType::VAMPIRE:
            // 100% Vial de Sangre (x2 - x3), 50% Medicina, 25% Anillo de Sifón Vampírico
            table.AddGuaranteedByString("blood_vial", 2, 3);
            table.AddIndependentDropByString("potion_health", 0.50f, 1, 2);
            table.AddIndependentDropByString("vampiric_ring", 0.25f, 1, 1);
            break;

        case EnemyType::SHADOW_ASSASSIN:
            // 100% Piel de bestia, 45% Anillo de Sombras, 30% Medicina
            table.AddGuaranteedByString("beast_pelt", 1, 2);
            table.AddIndependentDropByString("shadow_ring", 0.45f, 1, 1);
            table.AddIndependentDropByString("potion_health", 0.30f, 1, 1);
            break;

        case EnemyType::BERSERKER_WARRIOR:
            // 100% Carne cruda, 50% Vial de Sangre, 35% Espada Maldita
            table.AddGuaranteedByString("raw_meat", 2, 4);
            table.AddIndependentDropByString("blood_vial", 0.50f, 1, 2);
            table.AddIndependentDropByString("cursed_sword", 0.35f, 1, 1);
            break;

        case EnemyType::DARK_MAGE:
            // 100% Éter Corrupto (x1 - x2), 40% Pergamino Arcano, 25% Amuleto Antiguo
            table.AddGuaranteedByString("potion_mana", 1, 2);
            table.AddIndependentDropByString("arcane_scroll", 0.40f, 1, 1);
            table.AddIndependentDropByString("ancient_amulet", 0.25f, 1, 1);
            break;

        case EnemyType::TREANT:
            // 100% Troncos de Madera (x4 - x8), 60% Medicina natural
            table.AddGuaranteedByString("wood_log", 4, 8);
            table.AddIndependentDropByString("potion_health", 0.60f, 1, 2);
            break;

        case EnemyType::NEUTRAL_GIANT:
            // 100% Piedra (x5 - x10), 100% Madera (x3 - x6), 50% Escudo de Hierro
            table.AddGuaranteedByString("stone_rock", 5, 10);
            table.AddGuaranteedByString("wood_log", 3, 6);
            table.AddIndependentDropByString("iron_shield", 0.50f, 1, 1);
            break;

        case EnemyType::CORRUPTED_WARRIOR:
        default:
            // 100% Carne cruda, 40% Espada, 30% Escudo
            table.AddGuaranteedByString("raw_meat", 1, 2);
            table.AddIndependentDropByString("cursed_sword", 0.40f, 1, 1);
            table.AddIndependentDropByString("iron_shield", 0.30f, 1, 1);
            break;
    }

    return table;
}

LootTable LootManager::GetDeerLoot(DeerSize size) {
    LootTable table;

    switch (size) {
        case DeerSize::DEMONIC:
            table.AddGuaranteedByString("beast_pelt", 3, 5);
            table.AddGuaranteedByString("raw_meat", 4, 7);
            table.AddGuaranteedByString("blood_vial", 2, 4);
            table.AddIndependentDropByString("vampiric_ring", 0.30f, 1, 1);
            break;

        case DeerSize::ALPHA:
            table.AddGuaranteedByString("beast_pelt", 2, 3);
            table.AddGuaranteedByString("raw_meat", 3, 5);
            table.AddIndependentDropByString("blood_vial", 0.60f, 1, 2);
            break;

        case DeerSize::ADULT:
            table.AddGuaranteedByString("beast_pelt", 1, 2);
            table.AddGuaranteedByString("raw_meat", 2, 3);
            table.AddIndependentDropByString("blood_vial", 0.35f, 1, 1);
            break;

        case DeerSize::FAWN:
        default:
            table.AddGuaranteedByString("beast_pelt", 1, 1);
            table.AddGuaranteedByString("raw_meat", 1, 2);
            break;
    }

    return table;
}

LootTable LootManager::GetChestLoot(int chestTier) {
    LootTable table;
    // Tirada garantizada en ruleta ponderada según tier
    table.AddGuaranteedByString("potion_health", 2, 3);
    table.AddGuaranteedByString("potion_mana", 1, 2);

    table.AddWeightedRollByString("cursed_sword", 40, 1, 1);
    table.AddWeightedRollByString("iron_shield", 35, 1, 1);
    table.AddWeightedRollByString("shadow_ring", 25, 1, 1);
    table.AddWeightedRollByString("frost_claymore", 15, 1, 1);
    table.AddWeightedRollByString("ancient_amulet", 10, 1, 1);

    return table;
}
