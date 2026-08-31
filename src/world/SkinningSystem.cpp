#include "SkinningSystem.h"
#include "PassiveMob.h"
#include "inventory/InventorySystem.h"
#include "inventory/LootManager.h"
#include "combat/DamageNumberSystem.h"
#include "ParticleSystem.h"
#include "ScentSystem.h"
#include "Player.h"
#include <iostream>

SkinningSystem::SkinningSystem() {}
SkinningSystem::~SkinningSystem() {}

std::string SkinningSystem::GetPrompt(glm::vec3 playerPos, const std::vector<std::unique_ptr<PassiveMob>>& passiveMobs) {
    for (const auto& deer : passiveMobs) {
        if (!deer->IsAlive() && !deer->IsSkinned()) {
            float dist = glm::distance(playerPos, deer->GetPosition());
            if (dist < 2.8f) {
                return "[G] DESOLLAR ANIMAL (PIEL Y CARNE)";
            }
        }
    }
    return "";
}

bool SkinningSystem::TrySkin(glm::vec3 playerPos, 
                             std::vector<std::unique_ptr<PassiveMob>>& passiveMobs,
                             InventorySystem& inventory,
                             Player& player,
                             DamageNumberSystem& damageNumbers,
                             ParticleSystem& particles,
                             ScentSystem& scentSystem) 
{
    for (auto& deer : passiveMobs) {
        if (!deer->IsAlive() && !deer->IsSkinned()) {
            float dist = glm::distance(playerPos, deer->GetPosition());
            if (dist < 2.8f) {
                deer->SetSkinned(true);

                // Add data-driven loot items to inventory
                LootTable lootTable = LootManager::GetDeerLoot(deer->GetDeerSize());
                std::vector<ItemInstance> drops = lootTable.GenerateLoot(1.0f);
                for (const auto& drop : drops) {
                    inventory.GetInventory().AddInstance(drop);
                }

                // Experience reward
                bool lvlUp = false;
                player.Stats.AddExp(45, lvlUp);
                damageNumbers.SpawnExp(deer->GetPosition() + glm::vec3(0, 1.4f, 0), 45);
                if (lvlUp) damageNumbers.SpawnLevelUp(player.Position);

                // Carnage & Meat Particle Explosion
                for (int i = 0; i < 35; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.4f, (rand()%100/50.0f + 0.4f)*3.0f, (rand()%100/50.0f - 1.0f)*2.4f);
                    glm::vec4 col = (i % 2 == 0) ? glm::vec4(0.75f, 0.10f, 0.10f, 1.0f) : glm::vec4(0.45f, 0.15f, 0.15f, 1.0f);
                    particles.SpawnParticle(deer->GetPosition() + glm::vec3(0, 0.4f, 0), pVel, col, 0.18f, 0.8f, -9.8f);
                }

                // Add heavy blood scent trail
                scentSystem.AddBloodScent(deer->GetPosition(), glm::vec3(0, 0, 1));

                return true;
            }
        }
    }
    return false;
}
