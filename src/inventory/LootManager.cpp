#include "LootManager.h"

LootTable LootManager::GetEnemyLoot(EnemyType type, int nightLevel) {
    LootTable table;

    switch (type) {
        case EnemyType::SKELETON_ARCHER:
            // 100% Flechas de Caza (x4 - x10), 35% Madera, 18% Arco Recurvo
            table.AddGuaranteedByString("hunting_arrow", 4, 10);
            table.AddIndependentDropByString("wood_log", 0.35f, 1, 2);
            table.AddIndependentDropByString("hunting_bow", 0.18f, 1, 1);
            break;

        case EnemyType::DEATH_KNIGHT: {
            // Materiales garantizados
            table.AddGuaranteedByString("stone_rock", 2, 4);
            table.AddIndependentDropByString("blood_vial", 0.50f, 1, 2);

            // 45% de probabilidad de soltar MÁXIMO UNA pieza del set o arma del Caballero de la Muerte
            float equipRoll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            if (equipRoll < 0.45f) {
                const char* dkPool[] = {
                    "deathknight_greatsword",
                    "deathknight_helm",
                    "deathknight_armor",
                    "deathknight_greaves",
                    "deathknight_boots",
                    "deathknight_gauntlets"
                };
                int pick = rand() % 6;
                table.AddGuaranteedByString(dkPool[pick], 1, 1);
            }
            break;
        }

        case EnemyType::VAMPIRE:
            // 100% Vial de Sangre (x2 - x3), 40% Medicina, 20% Anillo de Sifón Vampírico
            table.AddGuaranteedByString("blood_vial", 2, 3);
            table.AddIndependentDropByString("potion_health", 0.40f, 1, 2);
            table.AddIndependentDropByString("vampiric_ring", 0.20f, 1, 1);
            break;

        case EnemyType::SHADOW_ASSASSIN: {
            // Materiales garantizados
            table.AddGuaranteedByString("beast_pelt", 1, 2);
            table.AddIndependentDropByString("shadow_dagger", 0.35f, 1, 1); // Daga de las Sombras
            table.AddIndependentDropByString("potion_health", 0.30f, 1, 1);
            table.AddIndependentDropByString("shadow_ring", 0.25f, 1, 1);

            // 40% de probabilidad de soltar MÁXIMO UNA pieza del set de sombras o dagas
            float equipRoll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            if (equipRoll < 0.40f) {
                const char* shadowPool[] = {
                    "shadow_dagger",
                    "shadow_hood",
                    "shadow_garb",
                    "shadow_pants",
                    "shadow_boots",
                    "shadow_gloves"
                };
                int pick = rand() % 6;
                table.AddGuaranteedByString(shadowPool[pick], 1, 1);
            }
            break;
        }

        case EnemyType::BERSERKER_WARRIOR: {
            // Materiales garantizados
            table.AddGuaranteedByString("raw_meat", 2, 3);
            table.AddIndependentDropByString("blood_vial", 0.45f, 1, 2);

            // 45% de probabilidad de soltar MÁXIMO UNA pieza del set bárbaro o hachas
            float equipRoll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            if (equipRoll < 0.45f) {
                const char* berserkerPool[] = {
                    "berserker_onehand_axe",
                    "berserker_axe",
                    "executioner_axe",
                    "berserker_helm",
                    "berserker_armor",
                    "berserker_pants",
                    "berserker_boots",
                    "berserker_gauntlets"
                };
                int pick = rand() % 8;
                table.AddGuaranteedByString(berserkerPool[pick], 1, 1);
            }
            break;
        }

        case EnemyType::DARK_MAGE:
            // 100% Poción de maná, 35% Pergamino Arcano, 20% Amuleto Antiguo
            table.AddGuaranteedByString("potion_mana", 1, 2);
            table.AddIndependentDropByString("arcane_scroll", 0.35f, 1, 1);
            table.AddIndependentDropByString("ancient_amulet", 0.20f, 1, 1);
            break;

        case EnemyType::TREANT:
            // 100% Troncos de Madera (x4 - x7), 50% Medicina natural
            table.AddGuaranteedByString("wood_log", 4, 7);
            table.AddIndependentDropByString("potion_health", 0.50f, 1, 2);
            break;

        case EnemyType::NEUTRAL_GIANT: {
            // Materiales de construcción masivos garantizados
            table.AddGuaranteedByString("stone_rock", 5, 8);
            table.AddGuaranteedByString("wood_log", 3, 5);

            // 50% de probabilidad de soltar MÁXIMO UNA pieza pesada
            float equipRoll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            if (equipRoll < 0.50f) {
                const char* giantPool[] = {
                    "iron_greatsword",
                    "executioner_axe",
                    "iron_armor",
                    "iron_shield",
                    "iron_helm"
                };
                int pick = rand() % 5;
                table.AddGuaranteedByString(giantPool[pick], 1, 1);
            }
            break;
        }

        case EnemyType::CORRUPTED_WARRIOR:
        default: {
            // Materiales garantizados
            table.AddGuaranteedByString("raw_meat", 1, 2);

            // 40% de probabilidad de soltar MÁXIMO UNA pieza de equipo
            float equipRoll = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            if (equipRoll < 0.40f) {
                if (nightLevel <= 1) {
                    const char* leatherPool[] = {
                        "iron_hatchet",
                        "steel_shortsword",
                        "armor_leather",
                        "helm_leather",
                        "armor_leather_pants",
                        "armor_leather_boots",
                        "armor_leather_gloves",
                        "iron_shield"
                    };
                    int pick = rand() % 8;
                    table.AddGuaranteedByString(leatherPool[pick], 1, 1);
                } else {
                    const char* ironPool[] = {
                        "iron_hatchet",
                        "steel_shortsword",
                        "paladin_longsword",
                        "iron_armor",
                        "iron_helm",
                        "iron_greaves",
                        "iron_boots",
                        "iron_gauntlets",
                        "iron_shield"
                    };
                    int pick = rand() % 9;
                    table.AddGuaranteedByString(ironPool[pick], 1, 1);
                }
            }
            break;
        }
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

LootTable LootManager::GetDragonLoot() {
    LootTable table;
    // Botín Legendario del Dragón:
    // 100% Corazón de Dragón (x1 - x2) y Escamas de Dragón (x5 - x12)
    table.AddGuaranteedByString("dragon_heart", 1, 2);
    table.AddGuaranteedByString("dragon_scale", 5, 12);
    table.AddGuaranteedByString("potion_health", 3, 5);
    table.AddGuaranteedByString("potion_mana", 2, 4);

    // Piezas de armadura y armas legendarias con alta probabilidad
    table.AddIndependentDropByString("dragon_helm", 0.85f, 1, 1);
    table.AddIndependentDropByString("dragon_chest", 0.75f, 1, 1);
    table.AddIndependentDropByString("dragon_scale_ring", 0.90f, 1, 1);
    table.AddIndependentDropByString("dragon_bone_bow", 0.80f, 1, 1);

    return table;
}
