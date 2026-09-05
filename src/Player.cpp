#include "Player.h"
#include <cmath>
#include <iostream>
#include "core/PlatformInput.h"
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include "Config.h"
#include "Monster.h"
#include "PassiveMob.h"
#include "ParticleSystem.h"
#include "world/StructureSystem.h"

Player::Player(glm::vec3 startPos) 
    : Position(startPos), Front(glm::vec3(0.0f, 0.0f, -1.0f)), WorldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
      Yaw(-90.0f), Pitch(0.0f), Velocity(glm::vec3(0.0f)), IsGrounded(false),
      HeadBobTimer(0.0f), BreathTimer(0.0f), WeaponSwayPos(glm::vec3(0.0f)),
      ModelYaw(180.0f), WalkAnimTimer(0.0f)
{
    WalkSpeed = Config::Gameplay::PlayerSpeed;
    updateCameraVectors();
    initModel();
}

Player::~Player() {
    if (m_playerVAO) {
        glDeleteVertexArrays(1, &m_playerVAO);
        m_playerVAO = 0;
    }
    if (m_playerVBO) {
        glDeleteBuffers(1, &m_playerVBO);
        m_playerVBO = 0;
    }
    if (m_fpVAO) {
        glDeleteVertexArrays(1, &m_fpVAO);
        m_fpVAO = 0;
    }
    if (m_fpVBO) {
        glDeleteBuffers(1, &m_fpVBO);
        m_fpVBO = 0;
    }
}

#include "UIRenderer.h"

bool Player::TryAttack() {
    if (m_isDead || StunTimer > 0.0f) return false;
    if (m_attackTimer <= 0.0f && m_attackCooldownTimer <= 0.0f && !m_isBlocking) {
        float agiFactor = Stats.GetAttackSpeedMultiplier();
        m_attackDuration = 0.52f / agiFactor;
        m_attackCooldown = 0.28f / agiFactor;
        m_attackTimer = m_attackDuration;
        m_attackCooldownTimer = m_attackDuration + m_attackCooldown;
        m_attackHitDone = false;
        m_attackCombo = (m_attackCombo + 1) % 2;

        // Empuñadura dual: alternar mano derecha e izquierda (el escudo no pega)
        bool isDualWieldingWeapons = (!m_equippedMainHandId.empty() && 
                                      !m_equippedOffHandId.empty() && 
                                      m_equippedOffHandId != "iron_shield");
        if (isDualWieldingWeapons) {
            m_activeAttackHand = (m_activeAttackHand == 0) ? 1 : 0;
        } else {
            m_activeAttackHand = 0; // Siempre mano derecha si es arma sola o escudo
        }

        return true;
    }
    return false;
}

bool Player::AreBothHandsOccupied() const {
    bool is2H = (m_equippedMainHandId.find("greatsword") != std::string::npos ||
                 m_equippedMainHandId.find("executioner") != std::string::npos ||
                 m_equippedMainHandId.find("dragonslayer") != std::string::npos ||
                 m_equippedMainHandId == "hunting_bow");
    return is2H || (!m_equippedMainHandId.empty() && !m_equippedOffHandId.empty());
}

void Player::TakeDamage(int dmg, DamageNumberSystem& damageNumbers, FatalErrorPopup* fatalError, bool shadowAegis) {
    if (shadowAegis) {
        dmg = std::max(1, (int)(dmg * 0.30f)); // 70% damage reduction with Shadow Aegis
    }
    bool heavyHit = (dmg >= 28);
    if (m_isBlocking) {
        if (heavyHit) {
            // Posture Break on massive blows!
            StunTimer = 1.2f;
            m_isBlocking = false;
            dmg = std::max(1, dmg / 2); // 50% damage through broken block
        } else {
            dmg = std::max(1, dmg / 4); // Shield blocks 75%
        }
    } else {
        float defRatio = (float)Stats.Defense / (Stats.Defense + 45.0f);
        dmg = std::max(2, (int)(dmg * (1.0f - defRatio)));
    }
    Stats.CurrentHP -= dmg;
    if (Stats.CurrentHP <= 0) {
        Stats.CurrentHP = 0;
        if (!m_isDead) {
            m_isDead = true;
            DeathTimer = 0.0f;
            m_isBlocking = false;
            m_attackTimer = 0.0f;
            std::cout << "[Player] Has sucumbido. 0 HP alcanzado. Estado: MUERTO." << std::endl;
        }
    }
    damageNumbers.SpawnPlayerDamage(Position, dmg, Front);

    // Fatal Error Popup Trigger on massive damage hits
    if (dmg >= 22 && fatalError != nullptr && !m_isDead) {
        fatalError->active = true;
        fatalError->damageValue = dmg;
        fatalError->timer = 4.0f;
        fatalError->message = "EXCEPCION EN 0x000000FF";
    }
}

void Player::Respawn(glm::vec3 spawnPos) {
    Position = spawnPos;
    Velocity = glm::vec3(0.0f);
    Stats.CurrentHP = Stats.MaxHP;
    Stats.CurrentMP = Stats.MaxMP;
    m_isDead = false;
    DeathTimer = 0.0f;
    StunTimer = 0.0f;
    m_isBlocking = false;
    m_attackTimer = 0.0f;
    updateModelMesh();
    std::cout << "[Player] Renacido con exito en el campamento." << std::endl;
}

#include "CombatCalculator.h"
#include "DamageNumberSystem.h"

#include "EnemyMob.h"
#include "WaterMonster.h"
#include "entities/Dragon.h"
#include "mobs/BaseMob.h"

void Player::UpdateCombat(float deltaTime, std::vector<std::unique_ptr<Monster>>& monsters, std::vector<std::unique_ptr<PassiveMob>>& passiveMobs, std::vector<std::unique_ptr<EnemyMob>>& enemyMobs, std::vector<std::unique_ptr<WaterMonster>>& waterMonsters, ParticleSystem& particles, DamageNumberSystem& damageNumbers, Dragon* dragon, std::vector<std::unique_ptr<BaseMob>>* baseMobs) {
    if (StunTimer > 0.0f) {
        StunTimer -= deltaTime;
        m_isBlocking = false;
    }

    if (m_attackCooldownTimer > 0.0f) {
        m_attackCooldownTimer -= deltaTime;
    }

    if (m_attackTimer > 0.0f) {
        m_attackTimer -= deltaTime;
        float progress = 1.0f - (m_attackTimer / m_attackDuration);
        
        // Clímax del tajo descendente (de arriba a abajo): mete el daño exactamente al descargar el golpe abajo
        if (progress >= 0.50f && !m_attackHitDone) {
            m_attackHitDone = true;
            
            // In 3rd person use character facing, in 1st person use camera Front
            glm::vec3 forwardDir = IsThirdPerson ? glm::vec3(sin(glm::radians(ModelYaw)), 0.0f, cos(glm::radians(ModelYaw))) : Front;
            glm::vec2 fwd2D = glm::length(glm::vec2(forwardDir.x, forwardDir.z)) > 0.001f ? glm::normalize(glm::vec2(forwardDir.x, forwardDir.z)) : glm::vec2(0.0f, 1.0f);

            bool isTwoHanded = (m_equippedMainHandId.find("greatsword") != std::string::npos ||
                                m_equippedMainHandId.find("executioner") != std::string::npos ||
                                m_equippedMainHandId.find("dragonslayer") != std::string::npos);
            float baseReach = isTwoHanded ? 3.6f : 2.8f;
            int maxHits = isTwoHanded ? 2 : 1; // 1-handed weapons hit strictly 1 target; 2-handed weapons can cleave at most 2 directly in blade sweep

            struct HitCandidate {
                enum class TargetType { DRAGON, MONSTER, WATER_MONSTER, ENEMY_MOB, PASSIVE_MOB, BASE_MOB } type;
                void* ptr = nullptr;
                glm::vec3 pos;
                float radius;
                float score;
                int defense;
                int evasion;
            };
            std::vector<HitCandidate> candidates;

            auto evaluateCandidate = [&](void* ptr, HitCandidate::TargetType type, const glm::vec3& targetPos, float radius, int def, int eva) {
                float dx = targetPos.x - Position.x;
                float dz = targetPos.z - Position.z;
                float dist2D = sqrt(dx * dx + dz * dz);
                float playerFeetY = Position.y - 1.6f;
                float dy = std::abs(targetPos.y - playerFeetY);

                // Vertical tolerance: must be within striking reach (up to 2.8m ground difference on slopes)
                if (dy > 2.8f) return;

                // Max physical reach
                if (dist2D > (baseReach + radius)) return;

                // Angle check: target MUST be in front of the player (±50 degrees frontal cone)
                glm::vec2 toTarget2D = (dist2D > 0.001f) ? glm::vec2(dx / dist2D, dz / dist2D) : glm::vec2(0.0f);
                float dot = glm::dot(fwd2D, toTarget2D);

                // Tolerancia a quemarropa: si el mob está físicamente encima o rozando el cuerpo del jugador,
                // omitimos la restricción de ángulo para poder golpearlo siempre
                bool isPointBlank = (dist2D <= (radius + 0.95f));
                if (!isPointBlank && dot < 0.50f) return;

                // Crosshair aim alignment: how directly centered is the target in player's view?
                glm::vec3 toTarget3D = (dist2D > 0.001f) ? glm::normalize((targetPos + glm::vec3(0, 1.0f, 0)) - (Position + glm::vec3(0, 1.6f, 0))) : Front;
                float aimDot = (dist2D > 0.001f) ? glm::dot(Front, toTarget3D) : 1.0f;

                // Priority score: direct crosshair alignment + proximity, with top priority for point-blank touching enemies
                float score = (aimDot * 6.0f) + (dot * 3.0f) - (dist2D * 1.5f);
                if (isPointBlank) {
                    score += 30.0f; // Prioridad absoluta a mobs pegados o adentro del cuerpo
                }

                candidates.push_back({ type, ptr, targetPos, radius, score, def, eva });
            };

            // Evaluate all nearby potential targets
            if (dragon != nullptr && dragon->IsAlive() && !dragon->IsDying()) {
                evaluateCandidate(dragon, HitCandidate::TargetType::DRAGON, dragon->GetPosition(), dragon->GetRadius() + 1.0f, 12, 5);
            }
            for (auto& mPtr : monsters) {
                if (!mPtr->IsDead()) {
                    evaluateCandidate(mPtr.get(), HitCandidate::TargetType::MONSTER, mPtr->GetPosition(), 1.0f, 10, 5);
                }
            }
            for (auto& wm : waterMonsters) {
                if (wm->IsAlive()) {
                    evaluateCandidate(wm.get(), HitCandidate::TargetType::WATER_MONSTER, wm->GetPosition(), wm->GetRadius(), 4, 10);
                }
            }
            for (auto& enemyPtr : enemyMobs) {
                if (enemyPtr->IsAlive()) {
                    evaluateCandidate(enemyPtr.get(), HitCandidate::TargetType::ENEMY_MOB, enemyPtr->GetPosition(), enemyPtr->GetRadius(), enemyPtr->GetDefense(), enemyPtr->GetEvasion());
                }
            }
            for (auto& mobPtr : passiveMobs) {
                if (mobPtr->IsAlive()) {
                    evaluateCandidate(mobPtr.get(), HitCandidate::TargetType::PASSIVE_MOB, mobPtr->GetPosition(), mobPtr->GetRadius(), 4, 12);
                }
            }
            if (baseMobs != nullptr) {
                for (auto& mobPtr : *baseMobs) {
                    if (mobPtr->IsAlive()) {
                        evaluateCandidate(mobPtr.get(), HitCandidate::TargetType::BASE_MOB, mobPtr->GetPosition(), mobPtr->GetRadius(), mobPtr->GetDefense(), mobPtr->GetEvasion());
                    }
                }
            }

            // Sort descending by score: the single target most directly aligned with your strike is first!
            std::sort(candidates.begin(), candidates.end(), [](const HitCandidate& a, const HitCandidate& b) {
                return a.score > b.score;
            });

            // Deliver hit only to top candidates (maxHits: 1 for 1-handed, 2 for colossal 2-handed)
            int hitsDelivered = 0;
            for (const auto& cand : candidates) {
                if (hitsDelivered >= maxHits) break;

                AttackDamageResult dmgResult = CombatCalculator::CalculatePlayerAttack(Stats.Attack, Stats.CritChance, Stats.CritMultiplier, cand.defense, cand.evasion);
                if (dmgResult.IsHit) {
                    if (cand.type == HitCandidate::TargetType::DRAGON) {
                        Dragon* d = static_cast<Dragon*>(cand.ptr);
                        bool killed = d->TakeDamage(dmgResult.Damage, Position, particles, damageNumbers, this);
                        damageNumbers.SpawnDamage(cand.pos + glm::vec3(0, 2.0f, 0), dmgResult.Damage, dmgResult.IsCrit);
                        if (killed) {
                            bool leveledUp = false;
                            int expGain = d->GetExpReward();
                            Stats.AddExp(expGain, leveledUp);
                            damageNumbers.SpawnExp(cand.pos + glm::vec3(0, 2.5f, 0), expGain);
                            if (leveledUp) damageNumbers.SpawnLevelUp(Position);
                        }
                    } else if (cand.type == HitCandidate::TargetType::MONSTER) {
                        Monster* m = static_cast<Monster*>(cand.ptr);
                        m->TakeDamage((float)dmgResult.Damage, false);
                        damageNumbers.SpawnDamage(cand.pos, dmgResult.Damage, dmgResult.IsCrit);
                        for (int i = 0; i < 20; ++i) {
                            glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.5f, (rand()%100/50.0f + 0.3f)*4.0f, (rand()%100/50.0f - 1.0f)*3.5f);
                            particles.SpawnParticle(cand.pos + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.85f, 0.05f, 0.05f, 1.0f), 0.14f, 0.85f, -9.8f);
                        }
                        if (m->IsDead()) {
                            bool leveledUp = false;
                            Stats.AddExp(85, leveledUp);
                            damageNumbers.SpawnExp(cand.pos, 85);
                            if (leveledUp) damageNumbers.SpawnLevelUp(Position);
                        }
                    } else if (cand.type == HitCandidate::TargetType::WATER_MONSTER) {
                        WaterMonster* wm = static_cast<WaterMonster*>(cand.ptr);
                        bool killed = wm->TakeDamage(dmgResult.Damage, Position, particles, this, damageNumbers);
                        damageNumbers.SpawnDamage(cand.pos, dmgResult.Damage, dmgResult.IsCrit);
                        if (killed) {
                            bool leveledUp = false;
                            int expGain = wm->GetExpReward();
                            Stats.AddExp(expGain, leveledUp);
                            damageNumbers.SpawnExp(cand.pos, expGain);
                            if (leveledUp) damageNumbers.SpawnLevelUp(Position);
                        }
                    } else if (cand.type == HitCandidate::TargetType::ENEMY_MOB) {
                        EnemyMob* em = static_cast<EnemyMob*>(cand.ptr);
                        bool killed = em->TakeDamage(dmgResult.Damage, Position, particles, this, damageNumbers);
                        damageNumbers.SpawnDamage(cand.pos, dmgResult.Damage, dmgResult.IsCrit);
                        if (killed) {
                            bool leveledUp = false;
                            int expGain = em->GetExpReward();
                            Stats.AddExp(expGain, leveledUp);
                            damageNumbers.SpawnExp(cand.pos, expGain);
                            if (leveledUp) damageNumbers.SpawnLevelUp(Position);
                        }
                    } else if (cand.type == HitCandidate::TargetType::PASSIVE_MOB) {
                        PassiveMob* pm = static_cast<PassiveMob*>(cand.ptr);
                        bool killed = pm->TakeDamage(dmgResult.Damage, Position, particles, this, damageNumbers);
                        damageNumbers.SpawnDamage(cand.pos, dmgResult.Damage, dmgResult.IsCrit);
                        if (killed) {
                            bool leveledUp = false;
                            int expGain = pm->GetExpReward();
                            Stats.AddExp(expGain, leveledUp);
                            damageNumbers.SpawnExp(cand.pos, expGain);
                            if (leveledUp) damageNumbers.SpawnLevelUp(Position);
                        }
                    } else if (cand.type == HitCandidate::TargetType::BASE_MOB) {
                        BaseMob* mob = static_cast<BaseMob*>(cand.ptr);
                        bool killed = mob->TakeDamage(dmgResult.Damage, Position, particles, this, damageNumbers);
                        damageNumbers.SpawnDamage(cand.pos, dmgResult.Damage, dmgResult.IsCrit);
                        if (killed) {
                            bool leveledUp = false;
                            int expGain = mob->GetExpReward();
                            Stats.AddExp(expGain, leveledUp);
                            damageNumbers.SpawnExp(cand.pos, expGain);
                            if (leveledUp) damageNumbers.SpawnLevelUp(Position);
                        }
                    }
                } else {
                    // Evaded: spawn evasion sparkle
                    for (int i = 0; i < 6; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*1.5f, (rand()%100/50.0f + 0.5f)*2.0f, (rand()%100/50.0f - 1.0f)*1.5f);
                        particles.SpawnParticle(cand.pos + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.7f, 0.7f, 0.9f, 0.8f), 0.08f, 0.4f, 0.0f);
                    }
                }

                hitsDelivered++;
            }
        }
    }
}

void Player::initModel() {
    glGenVertexArrays(1, &m_playerVAO);
    glGenBuffers(1, &m_playerVBO);

    glBindVertexArray(m_playerVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_playerVBO);

    // Layout: Pos (0), Color (1), UV (2), Normal (3)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    UpdateEquipmentVisuals("", "", "", "");
}

void Player::UpdateEquipmentVisuals(const std::string& mainHandId, const std::string& chestId, const std::string& headId, const std::string& offHandId, const std::string& legsId, const std::string& feetId, const std::string& glovesId) {
    if (mainHandId == m_equippedMainHandId && chestId == m_equippedChestId &&
        headId == m_equippedHeadId && offHandId == m_equippedOffHandId &&
        legsId == m_equippedLegsId && feetId == m_equippedFeetId && glovesId == m_equippedGlovesId && !m_baseBoxes.empty()) {
        return;
    }

    m_equippedMainHandId = mainHandId;
    m_equippedChestId = chestId;
    m_equippedHeadId = headId;
    m_equippedOffHandId = offHandId;
    m_equippedLegsId = legsId;
    m_equippedFeetId = feetId;
    m_equippedGlovesId = glovesId;

    m_baseBoxes.clear();

    // 1. Base Player Body (Peasant / Adventurer Frame)
    std::vector<BoxDef> body = ModelLoader::Load("assets/models/equipment/player_body.txt");
    if (body.empty()) {
        body = ModelLoader::Load("assets/models/player.txt");
    }
    m_baseBoxes.insert(m_baseBoxes.end(), body.begin(), body.end());

    // 2. Chest Armor
    if (m_equippedChestId == "leather_armor") {
        auto armor = ModelLoader::Load("assets/models/equipment/armor_leather.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), armor.begin(), armor.end());
    } else if (m_equippedChestId == "iron_armor") {
        auto armor = ModelLoader::Load("assets/models/equipment/armor_iron_plate.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), armor.begin(), armor.end());
    } else if (m_equippedChestId == "deathknight_armor") {
        auto armor = ModelLoader::Load("assets/models/equipment/armor_death_knight.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), armor.begin(), armor.end());
    } else if (m_equippedChestId == "berserker_armor") {
        auto armor = ModelLoader::Load("assets/models/equipment/armor_berserker.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), armor.begin(), armor.end());
    } else if (m_equippedChestId == "shadow_garb") {
        auto armor = ModelLoader::Load("assets/models/equipment/armor_shadow.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), armor.begin(), armor.end());
    } else if (m_equippedChestId == "dragon_armor" || m_equippedChestId == "dragon_chest") {
        auto armor = ModelLoader::Load("assets/models/equipment/armor_dragon.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), armor.begin(), armor.end());
    }

    // 3. Helmets
    if (m_equippedHeadId == "leather_cap") {
        auto helm = ModelLoader::Load("assets/models/equipment/helm_leather.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), helm.begin(), helm.end());
    } else if (m_equippedHeadId == "iron_helm") {
        auto helm = ModelLoader::Load("assets/models/equipment/helm_iron.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), helm.begin(), helm.end());
    } else if (m_equippedHeadId == "deathknight_helm") {
        auto helm = ModelLoader::Load("assets/models/equipment/helm_death_knight.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), helm.begin(), helm.end());
    } else if (m_equippedHeadId == "berserker_helm") {
        auto helm = ModelLoader::Load("assets/models/equipment/helm_berserker.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), helm.begin(), helm.end());
    } else if (m_equippedHeadId == "shadow_hood") {
        auto helm = ModelLoader::Load("assets/models/equipment/helm_shadow.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), helm.begin(), helm.end());
    } else if (m_equippedHeadId == "dragon_helm") {
        auto helm = ModelLoader::Load("assets/models/equipment/helm_dragon.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), helm.begin(), helm.end());
    }

    // 4. Legs / Pants
    if (m_equippedLegsId == "leather_pants") {
        auto pants = ModelLoader::Load("assets/models/equipment/armor_leather_pants.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), pants.begin(), pants.end());
    } else if (m_equippedLegsId == "iron_greaves") {
        auto pants = ModelLoader::Load("assets/models/equipment/armor_iron_greaves.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), pants.begin(), pants.end());
    } else if (m_equippedLegsId == "deathknight_greaves") {
        auto pants = ModelLoader::Load("assets/models/equipment/armor_death_knight_greaves.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), pants.begin(), pants.end());
    } else if (m_equippedLegsId == "berserker_pants") {
        auto pants = ModelLoader::Load("assets/models/equipment/armor_berserker_pants.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), pants.begin(), pants.end());
    } else if (m_equippedLegsId == "shadow_pants") {
        auto pants = ModelLoader::Load("assets/models/equipment/armor_shadow_pants.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), pants.begin(), pants.end());
    } else if (m_equippedLegsId == "dragon_pants") {
        auto pants = ModelLoader::Load("assets/models/equipment/armor_dragon_pants.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), pants.begin(), pants.end());
    }

    // 5. Boots / Feet
    if (m_equippedFeetId == "leather_boots") {
        auto boots = ModelLoader::Load("assets/models/equipment/armor_leather_boots.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), boots.begin(), boots.end());
    } else if (m_equippedFeetId == "iron_boots") {
        auto boots = ModelLoader::Load("assets/models/equipment/armor_iron_boots.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), boots.begin(), boots.end());
    } else if (m_equippedFeetId == "deathknight_boots") {
        auto boots = ModelLoader::Load("assets/models/equipment/armor_death_knight_boots.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), boots.begin(), boots.end());
    } else if (m_equippedFeetId == "berserker_boots") {
        auto boots = ModelLoader::Load("assets/models/equipment/armor_berserker_boots.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), boots.begin(), boots.end());
    } else if (m_equippedFeetId == "shadow_boots") {
        auto boots = ModelLoader::Load("assets/models/equipment/armor_shadow_boots.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), boots.begin(), boots.end());
    } else if (m_equippedFeetId == "dragon_boots") {
        auto boots = ModelLoader::Load("assets/models/equipment/armor_dragon_boots.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), boots.begin(), boots.end());
    }

    // 6. Gloves / Hands
    if (m_equippedGlovesId == "leather_gloves") {
        auto gloves = ModelLoader::Load("assets/models/equipment/armor_leather_gloves.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), gloves.begin(), gloves.end());
    } else if (m_equippedGlovesId == "iron_gauntlets") {
        auto gloves = ModelLoader::Load("assets/models/equipment/armor_iron_gauntlets.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), gloves.begin(), gloves.end());
    } else if (m_equippedGlovesId == "deathknight_gauntlets") {
        auto gloves = ModelLoader::Load("assets/models/equipment/armor_death_knight_gauntlets.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), gloves.begin(), gloves.end());
    } else if (m_equippedGlovesId == "berserker_gauntlets") {
        auto gloves = ModelLoader::Load("assets/models/equipment/armor_berserker_gauntlets.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), gloves.begin(), gloves.end());
    } else if (m_equippedGlovesId == "shadow_gloves") {
        auto gloves = ModelLoader::Load("assets/models/equipment/armor_shadow_gloves.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), gloves.begin(), gloves.end());
    } else if (m_equippedGlovesId == "dragon_gauntlets") {
        auto gloves = ModelLoader::Load("assets/models/equipment/armor_dragon_gauntlets.txt");
        m_baseBoxes.insert(m_baseBoxes.end(), gloves.begin(), gloves.end());
    }

    // 7. Main Hand Weapons (Positioned in right hand at 0.28, 0.72, 0.08)
    std::string weaponFile = "";
    if (m_equippedMainHandId == "steel_shortsword" || m_equippedMainHandId == "cursed_sword") {
        weaponFile = "assets/models/equipment/weapon_shortsword.txt";
    } else if (m_equippedMainHandId == "iron_greatsword" || m_equippedMainHandId == "frost_claymore") {
        weaponFile = "assets/models/equipment/weapon_greatsword.txt";
    } else if (m_equippedMainHandId == "deathknight_greatsword") {
        weaponFile = "assets/models/equipment/weapon_deathknight_greatsword.txt";
    } else if (m_equippedMainHandId == "berserker_axe") {
        weaponFile = "assets/models/equipment/weapon_berserker_axe.txt";
    } else if (m_equippedMainHandId == "iron_hatchet") {
        weaponFile = "assets/models/equipment/weapon_iron_hatchet.txt";
    } else if (m_equippedMainHandId == "berserker_onehand_axe") {
        weaponFile = "assets/models/equipment/weapon_berserker_onehand_axe.txt";
    } else if (m_equippedMainHandId == "executioner_axe") {
        weaponFile = "assets/models/equipment/weapon_executioner_axe.txt";
    } else if (m_equippedMainHandId == "paladin_longsword") {
        weaponFile = "assets/models/equipment/weapon_paladin_longsword.txt";
    } else if (m_equippedMainHandId == "dragonslayer_greatsword") {
        weaponFile = "assets/models/equipment/weapon_dragonslayer_greatsword.txt";
    } else if (m_equippedMainHandId == "shadow_dagger") {
        weaponFile = "assets/models/equipment/weapon_shadow_dagger.txt";
    } else if (m_equippedMainHandId == "hunting_bow") {
        weaponFile = "assets/models/equipment/weapon_hunting_bow.txt";
    }

    if (!weaponFile.empty()) {
        auto weapon = ModelLoader::Load(weaponFile);
        for (auto& b : weapon) {
            if (m_equippedMainHandId == "hunting_bow") {
                // Held upright in left hand ready to shoot (same as skeleton archer)
                b.Pos = b.Pos * 1.05f + glm::vec3(-0.28f, 0.82f, 0.18f);
                b.Name = "WEAPON_BOW_" + b.Name;
            } else {
                b.Pos += glm::vec3(0.28f, 0.72f, 0.08f);
                b.Name = "WEAPON_" + b.Name;
            }
            m_baseBoxes.push_back(b);
        }
    }

    // 8. Off Hand Shield or Dual Wield 1-Handed Weapon
    if (m_equippedOffHandId == "iron_shield") {
        auto shield = ModelLoader::Load("assets/models/equipment/weapon_shield.txt");
        for (auto& b : shield) {
            b.Pos += glm::vec3(-0.35f, 0.88f, 0.12f);
            b.Name = "OFFHAND_SHIELD_" + b.Name;
            m_baseBoxes.push_back(b);
        }
    } else if (!m_equippedOffHandId.empty()) {
        std::string offhandFile = "";
        if (m_equippedOffHandId == "shortsword") offhandFile = "assets/models/equipment/weapon_shortsword.txt";
        else if (m_equippedOffHandId == "iron_hatchet") offhandFile = "assets/models/equipment/weapon_iron_hatchet.txt";
        else if (m_equippedOffHandId == "berserker_onehand_axe") offhandFile = "assets/models/equipment/weapon_berserker_onehand_axe.txt";
        else if (m_equippedOffHandId == "paladin_longsword") offhandFile = "assets/models/equipment/weapon_paladin_longsword.txt";
        else if (m_equippedOffHandId == "shadow_dagger") offhandFile = "assets/models/equipment/weapon_shadow_dagger.txt";

        if (!offhandFile.empty()) {
            auto weapon = ModelLoader::Load(offhandFile);
            for (auto& b : weapon) {
                glm::vec3 p = b.Pos;
                p.x = -p.x; // Mirror X for left hand
                p += glm::vec3(-0.28f, 0.72f, 0.08f);
                b.Pos = p;
                b.Name = "OFFHAND_WEAPON_" + b.Name;
                m_baseBoxes.push_back(b);
            }
        }
    }

    if (AreBothHandsOccupied()) {
        HasTorchActive = false;
    }

    m_fpMeshNeedsRebuild = true;
    updateModelMesh();
}

void Player::updateModelMesh() {
    if (m_baseBoxes.empty() || m_playerVAO == 0) return;

    float moveSpeed = glm::length(glm::vec2(Velocity.x, Velocity.z));
    
    // PS1 style procedural limb swing angles
    float legSwing = (moveSpeed > 0.1f && IsGrounded && !m_isDead) ? sin(WalkAnimTimer * 9.0f) * 0.45f : 0.0f;
    float armSwing = (moveSpeed > 0.1f && IsGrounded && !m_isDead) ? sin(WalkAnimTimer * 9.0f) * 0.38f : 0.0f;
    float capeFlutter = (moveSpeed > 0.1f && !m_isDead) ? sin(WalkAnimTimer * 18.0f) * 0.14f : sin(BreathTimer * 2.5f) * 0.04f;
    float breathingTorso = !m_isDead ? sin(BreathTimer * 2.5f) * 0.015f : 0.0f;

    bool isAttacking = (m_attackTimer > 0.0f && !m_isDead);
    float attackProgress = isAttacking ? (1.0f - (m_attackTimer / m_attackDuration)) : 0.0f;
    
    // Attack rotation for arms
    float rightArmRotX = 0.0f;
    float rightArmRotY = 0.0f;
    float rightArmRotZ = 0.0f;

    float leftArmRotX = 0.0f;
    float leftArmRotY = 0.0f;
    float leftArmRotZ = 0.0f;
    
    if (isAttacking && m_activeAttackHand == 0) {
        // Golpe mano derecha: primero eleva de abajo a arriba (windup), luego tajo con furia de arriba a abajo
        if (attackProgress < 0.28f) {
            float w = attackProgress / 0.28f;
            float smoothW = w * w * (3.0f - 2.0f * w);
            // De abajo a arriba (preparación del golpe sobre el hombro)
            rightArmRotX = glm::mix(0.0f, -1.85f, smoothW);
            rightArmRotY = glm::mix(0.0f, -0.25f, smoothW);
            rightArmRotZ = glm::mix(0.0f, 0.20f, smoothW);
        } else if (attackProgress < 0.65f) {
            float s = (attackProgress - 0.28f) / 0.37f;
            float smoothS = s * s * (3.0f - 2.0f * s);
            // De arriba a abajo (corte descendente donde conecta el daño)
            rightArmRotX = glm::mix(-1.85f, 1.55f, smoothS);
            rightArmRotY = glm::mix(-0.25f, 0.10f, smoothS);
            rightArmRotZ = glm::mix(0.20f, -0.15f, smoothS);
        } else {
            float r = (attackProgress - 0.65f) / 0.35f;
            float smoothR = r * r * (3.0f - 2.0f * r);
            // Recuperación de abajo a posición neutral
            rightArmRotX = glm::mix(1.55f, 0.0f, smoothR);
            rightArmRotY = glm::mix(0.10f, 0.0f, smoothR);
            rightArmRotZ = glm::mix(-0.15f, 0.0f, smoothR);
        }
        leftArmRotX = (HasTorchActive && !m_isDead) ? 0.72f : (m_isBlocking ? -0.30f : -armSwing);
        leftArmRotZ = (HasTorchActive && !m_isDead) ? -0.15f : (m_isBlocking ? 0.40f : 0.0f);
    } else if (isAttacking && m_activeAttackHand == 1) {
        // Golpe mano izquierda (dual wield): de abajo a arriba, luego de arriba a abajo
        rightArmRotX = armSwing * 0.65f;
        if (attackProgress < 0.28f) {
            float w = attackProgress / 0.28f;
            float smoothW = w * w * (3.0f - 2.0f * w);
            // De abajo a arriba
            leftArmRotX = glm::mix(0.0f, -1.85f, smoothW);
            leftArmRotY = glm::mix(0.0f, 0.25f, smoothW);
            leftArmRotZ = glm::mix(0.0f, -0.20f, smoothW);
        } else if (attackProgress < 0.65f) {
            float s = (attackProgress - 0.28f) / 0.37f;
            float smoothS = s * s * (3.0f - 2.0f * s);
            // De arriba a abajo
            leftArmRotX = glm::mix(-1.85f, 1.55f, smoothS);
            leftArmRotY = glm::mix(0.25f, -0.10f, smoothS);
            leftArmRotZ = glm::mix(-0.20f, 0.15f, smoothS);
        } else {
            float r = (attackProgress - 0.65f) / 0.35f;
            float smoothR = r * r * (3.0f - 2.0f * r);
            // Recuperación a posición neutral
            leftArmRotX = glm::mix(1.55f, 0.0f, smoothR);
            leftArmRotY = glm::mix(-0.10f, 0.0f, smoothR);
            leftArmRotZ = glm::mix(0.15f, 0.0f, smoothR);
        }
    } else if (m_isBlocking && !m_isDead) {
        rightArmRotX = 0.40f;
        rightArmRotY = -0.60f;
        rightArmRotZ = -0.65f;
        leftArmRotX = -0.30f;
        leftArmRotZ = 0.40f;
    } else {
        rightArmRotX = armSwing * 0.65f;
        leftArmRotX = (HasTorchActive && !m_isDead && !AreBothHandsOccupied()) ? 0.72f : -armSwing;
        leftArmRotZ = (HasTorchActive && !m_isDead && !AreBothHandsOccupied()) ? -0.15f : 0.0f;
    }

    // Joint Pivots for rigid hierarchical rotation
    glm::vec3 shoulderPivotR(0.28f, 1.30f, 0.0f);
    glm::vec3 shoulderPivotL(-0.28f, 1.30f, 0.0f);
    glm::vec3 hipPivotL(-0.13f, 0.76f, 0.0f);
    glm::vec3 hipPivotR(0.13f, 0.76f, 0.0f);

    glm::mat4 R_Arm = glm::mat4(1.0f);
    R_Arm = glm::rotate(R_Arm, rightArmRotZ, glm::vec3(0, 0, 1));
    R_Arm = glm::rotate(R_Arm, rightArmRotY, glm::vec3(0, 1, 0));
    R_Arm = glm::rotate(R_Arm, rightArmRotX, glm::vec3(1, 0, 0));

    glm::mat4 L_Arm = glm::mat4(1.0f);
    L_Arm = glm::rotate(L_Arm, leftArmRotZ, glm::vec3(0, 0, 1));
    L_Arm = glm::rotate(L_Arm, leftArmRotY, glm::vec3(0, 1, 0));
    L_Arm = glm::rotate(L_Arm, leftArmRotX, glm::vec3(1, 0, 0));

    // Death collapse transform: body tilts back 90 degrees and rests flat on ground
    glm::mat4 rootM = glm::mat4(1.0f);
    if (m_isDead) {
        rootM = glm::translate(rootM, glm::vec3(0.0f, -0.65f, 0.0f));
        rootM = glm::rotate(rootM, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    std::vector<TransformedBox> transformedBoxes;
    transformedBoxes.reserve(m_baseBoxes.size() + 4);

    for (const auto& box : m_baseBoxes) {
        glm::mat4 M = glm::mat4(1.0f);
        M = glm::translate(M, box.Pos);
        M = glm::rotate(M, box.Rot.z, glm::vec3(0,0,1));
        M = glm::rotate(M, box.Rot.y, glm::vec3(0,1,0));
        M = glm::rotate(M, box.Rot.x, glm::vec3(1,0,0));
        M = glm::scale(M, box.Scale);

        // Left Leg
        if (box.Name == "THIGH_L" || box.Name == "BOOT_L" || box.Name == "FOOT_L" ||
            ((box.Name.find("LEGS") != std::string::npos || box.Name.find("BOOTS") != std::string::npos) && box.Name.find("_L") != std::string::npos)) {
            glm::mat4 finalM = glm::translate(glm::mat4(1.0f), hipPivotL) * glm::rotate(glm::mat4(1.0f), legSwing, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -hipPivotL) * M;
            transformedBoxes.push_back({rootM * finalM, box.Color});
        }
        // Right Leg
        else if (box.Name == "THIGH_R" || box.Name == "BOOT_R" || box.Name == "FOOT_R" ||
                 ((box.Name.find("LEGS") != std::string::npos || box.Name.find("BOOTS") != std::string::npos) && box.Name.find("_R") != std::string::npos)) {
            glm::mat4 finalM = glm::translate(glm::mat4(1.0f), hipPivotR) * glm::rotate(glm::mat4(1.0f), -legSwing, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -hipPivotR) * M;
            transformedBoxes.push_back({rootM * finalM, box.Color});
        }
        // Left Arm & Shield & Offhand Weapon & Bow & Gloves
        else if (box.Name == "SHOULDER_L" || box.Name == "ARM_UPPER_L" || box.Name == "FOREARM_L" || box.Name == "HAND_L" ||
                 (box.Name.find("GLOVES") != std::string::npos && box.Name.find("_L") != std::string::npos) ||
                 box.Name.find("SHIELD") != std::string::npos || box.Name.find("OFFHAND") != std::string::npos ||
                 box.Name.find("BOW") != std::string::npos || box.Name.find("PAULDRON_L") != std::string::npos) {
            glm::mat4 finalM = glm::translate(glm::mat4(1.0f), shoulderPivotL) * L_Arm * glm::translate(glm::mat4(1.0f), -shoulderPivotL) * M;
            transformedBoxes.push_back({rootM * finalM, box.Color});
        }
        // Right Arm & Complete Weapon & Gloves
        else if (box.Name == "SHOULDER_R" || box.Name == "ARM_UPPER_R" || box.Name == "FOREARM_R" || box.Name == "HAND_R" ||
                 (box.Name.find("GLOVES") != std::string::npos && box.Name.find("_R") != std::string::npos) ||
                 box.Name.find("SWORD") != std::string::npos || box.Name.find("WEAPON") != std::string::npos ||
                 box.Name.find("BLADE") != std::string::npos || box.Name.find("AXE") != std::string::npos ||
                 box.Name.find("HILT") != std::string::npos || box.Name.find("GUARD") != std::string::npos ||
                 box.Name.find("POMMEL") != std::string::npos || box.Name.find("PAULDRON_R") != std::string::npos) {
            glm::mat4 finalM = glm::translate(glm::mat4(1.0f), shoulderPivotR) * R_Arm * glm::translate(glm::mat4(1.0f), -shoulderPivotR) * M;
            transformedBoxes.push_back({rootM * finalM, box.Color});
        }
        // Cape & Skirt
        else if (box.Name.find("CAPE") != std::string::npos || box.Name == "SKIRT") {
            glm::mat4 finalM = glm::translate(glm::mat4(1.0f), box.Pos) * glm::rotate(glm::mat4(1.0f), capeFlutter, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -box.Pos) * M;
            transformedBoxes.push_back({rootM * finalM, box.Color});
        }
        // Torso Breathing
        else if (box.Name.find("CHEST") != std::string::npos || box.Name.find("TORSO") != std::string::npos ||
                 box.Name.find("GORGET") != std::string::npos || box.Name == "NECK_COLLAR") {
            glm::mat4 finalM = glm::scale(M, glm::vec3(1.0f, 1.0f + breathingTorso, 1.0f + breathingTorso));
            transformedBoxes.push_back({rootM * finalM, box.Color});
        }
        // Head / Default
        else {
            transformedBoxes.push_back({rootM * M, box.Color});
        }
    }

    // Attach 3D Torch Model to Left Hand in 3rd Person
    if (HasTorchActive && !m_isDead) {
        glm::mat4 torchBase = glm::translate(glm::mat4(1.0f), shoulderPivotL) * L_Arm * glm::translate(glm::mat4(1.0f), -shoulderPivotL);
        glm::mat4 handleM = torchBase * glm::translate(glm::mat4(1.0f), glm::vec3(-0.28f, 0.95f, 0.28f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.08f, 0.55f, 0.08f));
        transformedBoxes.push_back({rootM * handleM, glm::vec3(0.28f, 0.18f, 0.10f)});
        glm::mat4 ringM = torchBase * glm::translate(glm::mat4(1.0f), glm::vec3(-0.28f, 1.20f, 0.28f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.14f, 0.08f, 0.14f));
        transformedBoxes.push_back({rootM * ringM, glm::vec3(0.22f, 0.22f, 0.24f)});
        glm::mat4 flameM = torchBase * glm::translate(glm::mat4(1.0f), glm::vec3(-0.28f, 1.30f, 0.28f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.12f, 0.16f, 0.12f));
        transformedBoxes.push_back({rootM * flameM, glm::vec3(0.98f, 0.70f, 0.15f)});
    }

    std::vector<float> rawVertices;
    ModelLoader::GenerateMeshTransformed(transformedBoxes, rawVertices);
    m_playerVertexCount = rawVertices.size() / 11;

    glBindBuffer(GL_ARRAY_BUFFER, m_playerVBO);
    glBufferData(GL_ARRAY_BUFFER, rawVertices.size() * sizeof(float), rawVertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Player::RenderPreview(GLuint shaderProgram, const glm::mat4& modelMatrix) {
    if (m_playerVAO == 0 || m_playerVertexCount == 0) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ParticleMode"), 0);

    glBindVertexArray(m_playerVAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_playerVertexCount);
    glBindVertexArray(0);
}

void Player::RenderFirstPersonSword(GLuint shaderProgram) {
    if (m_fpMeshNeedsRebuild || m_fpVAO == 0) {
        m_fpMeshNeedsRebuild = false;

        std::vector<BoxDef> fpBoxes;

        // Right hand
        BoxDef handR;
        handR.Name = "FP_HAND_R";
        handR.Pos = glm::vec3(0.0f, -0.05f, 0.08f);
        handR.Scale = glm::vec3(0.12f, 0.14f, 0.12f);
        handR.Color = glm::vec3(0.68f, 0.52f, 0.40f);
        fpBoxes.push_back(handR);

        // Forearm sleeve with matching equipped armor tint
        BoxDef armR;
        armR.Name = "FP_ARM_R";
        armR.Pos = glm::vec3(0.02f, -0.32f, 0.05f);
        armR.Scale = glm::vec3(0.14f, 0.42f, 0.14f);
        armR.Color = (m_equippedChestId == "iron_armor") ? glm::vec3(0.72f, 0.75f, 0.82f) :
                     ((m_equippedChestId == "deathknight_armor") ? glm::vec3(0.14f, 0.14f, 0.18f) :
                     ((m_equippedChestId == "berserker_armor") ? glm::vec3(0.45f, 0.28f, 0.16f) :
                     ((m_equippedChestId == "shadow_garb") ? glm::vec3(0.14f, 0.12f, 0.18f) :
                     ((m_equippedChestId == "dragon_armor" || m_equippedChestId == "dragon_chest") ? glm::vec3(0.75f, 0.15f, 0.12f) :
                     ((m_equippedChestId == "leather_armor") ? glm::vec3(0.42f, 0.28f, 0.18f) : glm::vec3(0.42f, 0.38f, 0.32f))))));
        fpBoxes.push_back(armR);

        // Equipped weapon model
        std::string weaponFile = "";
        if (m_equippedMainHandId == "steel_shortsword" || m_equippedMainHandId == "cursed_sword") {
            weaponFile = "assets/models/equipment/weapon_shortsword.txt";
        } else if (m_equippedMainHandId == "iron_greatsword" || m_equippedMainHandId == "frost_claymore") {
            weaponFile = "assets/models/equipment/weapon_greatsword.txt";
        } else if (m_equippedMainHandId == "deathknight_greatsword") {
            weaponFile = "assets/models/equipment/weapon_deathknight_greatsword.txt";
        } else if (m_equippedMainHandId == "berserker_axe") {
            weaponFile = "assets/models/equipment/weapon_berserker_axe.txt";
        } else if (m_equippedMainHandId == "iron_hatchet") {
            weaponFile = "assets/models/equipment/weapon_iron_hatchet.txt";
        } else if (m_equippedMainHandId == "berserker_onehand_axe") {
            weaponFile = "assets/models/equipment/weapon_berserker_onehand_axe.txt";
        } else if (m_equippedMainHandId == "executioner_axe") {
            weaponFile = "assets/models/equipment/weapon_executioner_axe.txt";
        } else if (m_equippedMainHandId == "paladin_longsword") {
            weaponFile = "assets/models/equipment/weapon_paladin_longsword.txt";
        } else if (m_equippedMainHandId == "dragonslayer_greatsword") {
            weaponFile = "assets/models/equipment/weapon_dragonslayer_greatsword.txt";
        } else if (m_equippedMainHandId == "shadow_dagger") {
            weaponFile = "assets/models/equipment/weapon_shadow_dagger.txt";
        } else if (m_equippedMainHandId == "hunting_bow") {
            weaponFile = "assets/models/equipment/weapon_hunting_bow.txt";
        }

        if (!weaponFile.empty()) {
            auto weapon = ModelLoader::Load(weaponFile);
            for (auto& b : weapon) {
                if (m_equippedMainHandId == "hunting_bow") {
                    b.Pos = b.Pos * 0.90f + glm::vec3(-0.24f, -0.06f, 0.35f);
                } else {
                    b.Pos += glm::vec3(0.0f, 0.0f, 0.08f);
                }
                fpBoxes.push_back(b);
            }
        }

        // Dual Wield Off-hand Weapon in First Person
        if (!m_equippedOffHandId.empty() && m_equippedOffHandId != "iron_shield") {
            std::string offFpFile = "";
            if (m_equippedOffHandId == "shortsword") offFpFile = "assets/models/equipment/weapon_shortsword.txt";
            else if (m_equippedOffHandId == "iron_hatchet") offFpFile = "assets/models/equipment/weapon_iron_hatchet.txt";
            else if (m_equippedOffHandId == "berserker_onehand_axe") offFpFile = "assets/models/equipment/weapon_berserker_onehand_axe.txt";
            else if (m_equippedOffHandId == "paladin_longsword") offFpFile = "assets/models/equipment/weapon_paladin_longsword.txt";
            else if (m_equippedOffHandId == "shadow_dagger") offFpFile = "assets/models/equipment/weapon_shadow_dagger.txt";

            if (!offFpFile.empty()) {
                auto offWeapon = ModelLoader::Load(offFpFile);
                for (auto& b : offWeapon) {
                    glm::vec3 p = b.Pos;
                    p.x = -p.x;
                    p += glm::vec3(-0.28f, -0.10f, 0.12f);
                    b.Pos = p;
                    fpBoxes.push_back(b);
                }
            }
        }

        std::vector<float> rawVertices;
        ModelLoader::GenerateMesh(fpBoxes, rawVertices);
        m_fpVertexCount = rawVertices.size() / 11;

        if (m_fpVAO == 0) {
            glGenVertexArrays(1, &m_fpVAO);
            glGenBuffers(1, &m_fpVBO);
        }
        glBindVertexArray(m_fpVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_fpVBO);
        glBufferData(GL_ARRAY_BUFFER, rawVertices.size() * sizeof(float), rawVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glBindVertexArray(0);
    }

    if (m_fpVAO == 0 || m_fpVertexCount == 0) return;

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);

    glm::vec3 viewPos(0.24f, -0.28f, 0.46f);
    viewPos += WeaponSwayPos;
    if (IsGrounded) {
        if (HeadBobTimer > 0.001f) viewPos.y += sin(HeadBobTimer) * 0.02f;
        else viewPos.y += sin(BreathTimer) * 0.01f;
    }
    model = glm::translate(model, viewPos);

    if (m_attackTimer > 0.0f) {
        float progress = 1.0f - (m_attackTimer / m_attackDuration);
        if (progress < 0.28f) {
            float w = progress / 0.28f;
            float smoothW = w * w * (3.0f - 2.0f * w);
            // Eleva el arma hacia arriba y atras (de abajo hacia arriba)
            model = glm::translate(model, glm::vec3(0.03f, 0.28f, -0.15f) * smoothW);
            model = glm::rotate(model, glm::radians(-55.0f * smoothW), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(20.0f * smoothW), glm::vec3(0, 0, 1));
        } else if (progress < 0.65f) {
            float s = (progress - 0.28f) / 0.37f;
            float smoothS = s * s * (3.0f - 2.0f * s);
            // Tajo hacia abajo con descarga de daño (de arriba hacia abajo)
            model = glm::translate(model, glm::mix(glm::vec3(0.03f, 0.28f, -0.15f), glm::vec3(-0.05f, -0.36f, 0.15f), smoothS));
            model = glm::rotate(model, glm::radians(glm::mix(-55.0f, 75.0f, smoothS)), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(glm::mix(20.0f, -18.0f, smoothS)), glm::vec3(0, 0, 1));
        } else {
            float r = (progress - 0.65f) / 0.35f;
            float smoothR = r * r * (3.0f - 2.0f * r);
            // Recuperación a la posición neutral
            model = glm::translate(model, glm::mix(glm::vec3(-0.05f, -0.36f, 0.15f), glm::vec3(0.0f), smoothR));
            model = glm::rotate(model, glm::radians(glm::mix(75.0f, 0.0f, smoothR)), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(glm::mix(-18.0f, 0.0f, smoothR)), glm::vec3(0, 0, 1));
        }
    } else if (m_isBlocking) {
        model = glm::translate(model, glm::vec3(-0.12f, 0.10f, -0.05f));
        model = glm::rotate(model, glm::radians(-35.0f), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0, 0, 1));
    } else {
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(1, 0, 0));
    }

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    glBindVertexArray(m_fpVAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_fpVertexCount);
    glBindVertexArray(0);
}

void Player::RenderFirstPersonTorch(GLuint shaderProgram) {
    if (!HasTorchActive || AreBothHandsOccupied()) return;

    static GLuint torchFpVAO = 0, torchFpVBO = 0;
    static size_t torchFpVertexCount = 0;

    if (torchFpVAO == 0) {
        std::vector<BoxDef> torchBoxes;
        
        // Left arm / sleeve
        BoxDef armL;
        armL.Name = "FP_ARM_L";
        armL.Pos = glm::vec3(-0.02f, -0.32f, 0.05f);
        armL.Scale = glm::vec3(0.14f, 0.42f, 0.14f);
        armL.Color = glm::vec3(0.55f, 0.40f, 0.28f);
        torchBoxes.push_back(armL);

        // Left hand
        BoxDef handL;
        handL.Name = "FP_HAND_L";
        handL.Pos = glm::vec3(0.0f, -0.05f, 0.08f);
        handL.Scale = glm::vec3(0.12f, 0.14f, 0.12f);
        handL.Color = glm::vec3(0.68f, 0.52f, 0.40f);
        torchBoxes.push_back(handL);

        // Torch Handle
        BoxDef torchHandle;
        torchHandle.Name = "FP_TORCH_HANDLE";
        torchHandle.Pos = glm::vec3(0.0f, 0.12f, 0.08f);
        torchHandle.Scale = glm::vec3(0.08f, 0.65f, 0.08f);
        torchHandle.Color = glm::vec3(0.28f, 0.18f, 0.10f);
        torchBoxes.push_back(torchHandle);

        // Iron Ring
        BoxDef ironRing;
        ironRing.Name = "FP_TORCH_RING";
        ironRing.Pos = glm::vec3(0.0f, 0.38f, 0.08f);
        ironRing.Scale = glm::vec3(0.15f, 0.09f, 0.15f);
        ironRing.Color = glm::vec3(0.22f, 0.22f, 0.24f);
        torchBoxes.push_back(ironRing);

        // Flame Core
        BoxDef flameCore;
        flameCore.Name = "FP_TORCH_FLAME";
        flameCore.Pos = glm::vec3(0.0f, 0.48f, 0.08f);
        flameCore.Scale = glm::vec3(0.12f, 0.18f, 0.12f);
        flameCore.Color = glm::vec3(0.98f, 0.70f, 0.15f);
        torchBoxes.push_back(flameCore);

        std::vector<float> rawVertices;
        ModelLoader::GenerateMesh(torchBoxes, rawVertices);
        torchFpVertexCount = rawVertices.size() / 11;

        glGenVertexArrays(1, &torchFpVAO);
        glGenBuffers(1, &torchFpVBO);
        glBindVertexArray(torchFpVAO);
        glBindBuffer(GL_ARRAY_BUFFER, torchFpVBO);
        glBufferData(GL_ARRAY_BUFFER, rawVertices.size() * sizeof(float), rawVertices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(float), (void*)(8 * sizeof(float)));
        glEnableVertexAttribArray(3);
        glBindVertexArray(0);
    }

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);

    // Position torch in lower-left of screen
    glm::vec3 viewPos(-0.26f, -0.26f, 0.44f);
    viewPos.x -= WeaponSwayPos.x * 0.7f;
    viewPos.y += WeaponSwayPos.y * 0.7f;
    if (IsGrounded) {
        if (HeadBobTimer > 0.001f) viewPos.y += sin(HeadBobTimer + 1.57f) * 0.018f;
        else viewPos.y += sin(BreathTimer + 1.57f) * 0.008f;
    }
    model = glm::translate(model, viewPos);
    model = glm::rotate(model, glm::radians(8.0f), glm::vec3(0, 0, 1));
    model = glm::rotate(model, glm::radians(-12.0f), glm::vec3(1, 0, 0));

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    glBindVertexArray(torchFpVAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)torchFpVertexCount);
    glBindVertexArray(0);
}

glm::vec3 Player::GetTorchPosition() const {
    if (!IsThirdPerson) {
        return Position + glm::vec3(0.0f, PlayerHeight * 0.85f, 0.0f) + (-Right * 0.35f) + (Front * 0.45f);
    }
    float rad = glm::radians(-ModelYaw);
    glm::vec3 leftOffset = glm::vec3(-cos(rad) * 0.45f - sin(rad) * 0.25f, 1.35f, -sin(rad) * 0.45f + cos(rad) * 0.25f);
    return Position + leftOffset;
}

void Player::ProcessMouseMovement(float xoffset, float yoffset) {
    float sensitivity = IsThirdPerson ? 0.12f : 0.19f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    Yaw   += xoffset;
    Pitch += yoffset;

    if (Pitch > 85.0f) Pitch = 85.0f;
    if (Pitch < -85.0f) Pitch = -85.0f;

    WeaponSwayPos.x += -xoffset * SwayAmount;
    WeaponSwayPos.y += yoffset * SwayAmount;

    float maxSway = 0.1f;
    WeaponSwayPos.x = glm::clamp(WeaponSwayPos.x, -maxSway, maxSway);
    WeaponSwayPos.y = glm::clamp(WeaponSwayPos.y, -maxSway, maxSway);

    if (IsClimbing) {
        m_cameraNoiseAccumulator += std::abs(xoffset) + std::abs(yoffset);
        if (m_cameraNoiseAccumulator > 15.0f) {
            SoundVolumeEmitted = 8.0f;
            m_cameraNoiseAccumulator = 0.0f;
        }
    }

    updateCameraVectors();
}

void Player::ProcessMouseScroll(float yoffset) {
    if (yoffset == 0.0f) return;
    
    // Zoom in (yoffset > 0) or Zoom out (yoffset < 0)
    if (yoffset > 0.0f) {
        // Zoom in
        if (!IsThirdPerson) {
            // Already in closest 1st person view
            return;
        }
        CameraDistance -= yoffset * 0.65f;
        if (CameraDistance < MinCameraDistance) {
            CameraDistance = MinCameraDistance;
            IsThirdPerson = false; // Smooth switch to 1st person when zooming in all the way!
        }
    } else {
        // Zoom out
        if (!IsThirdPerson) {
            IsThirdPerson = true;
            CameraDistance = MinCameraDistance + 0.5f;
        } else {
            CameraDistance -= yoffset * 0.65f;
            if (CameraDistance > MaxCameraDistance) {
                CameraDistance = MaxCameraDistance;
            }
        }
    }
}

void Player::ProcessKeyboard(int key, float deltaTime, ChunkManager& chunkManager, FootprintSystem& footprints) {
    if (m_isDead) {
        Velocity.x = 0.0f;
        Velocity.z = 0.0f;
        return;
    }

    glm::vec3 moveDir(0.0f);
    glm::vec3 flatFront = glm::normalize(glm::vec3(Front.x, 0.0f, Front.z));
    glm::vec3 flatRight = glm::normalize(glm::cross(flatFront, WorldUp));

    float currentSpeed = m_isBlocking ? (WalkSpeed * 0.45f) : WalkSpeed;

    // Proximity check to nearby trees for climbing
    std::vector<glm::vec4> nearbyTrees;
    chunkManager.GetTreesInRange(Position, 2.5f, nearbyTrees);
    
    glm::vec4 closestTree(0.0f);
    bool treeNear = false;
    float closestDist = 9999.0f;
    
    for (const auto& t : nearbyTrees) {
        glm::vec3 tPos(t.x, t.y, t.z);
        float dist = glm::distance(glm::vec3(Position.x, tPos.y, Position.z), tPos);
        float scaledRadius = 0.6f * t.w;
        if (dist < (scaledRadius + PlayerRadius + 0.35f)) {
            if (dist < closestDist) {
                closestDist = dist;
                closestTree = t;
                treeNear = true;
            }
        }
    }

    if (IsClimbing) {
        glm::vec3 treePos(ClimbingTreePos.x, Position.y, ClimbingTreePos.z);
        glm::vec3 toPlayer = Position - treePos;
        toPlayer.y = 0.0f;
        if (glm::length(toPlayer) > 0.01f) {
            toPlayer = glm::normalize(toPlayer);
        } else {
            toPlayer = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        
        float scaledRadius = 0.6f * ClimbingTreeScale;
        glm::vec3 snapPos = treePos + toPlayer * (scaledRadius + PlayerRadius + 0.05f);
        Position.x = snapPos.x;
        Position.z = snapPos.z;
        
        Velocity.x = 0.0f;
        Velocity.z = 0.0f;
        Velocity.y = 0.0f;
        IsGrounded = false;
        
        float climbSpeed = 5.0f;
        bool isMoving = false;
        if (PlatformInput::IsKeyPressed(PlatformInput::W) || PlatformInput::IsKeyPressed(PlatformInput::Up)) {
            Position.y += climbSpeed * deltaTime;
            isMoving = true;
        }
        if (PlatformInput::IsKeyPressed(PlatformInput::S) || PlatformInput::IsKeyPressed(PlatformInput::Down)) {
            Position.y -= climbSpeed * deltaTime;
            isMoving = true;
        }
        
        if (isMoving) {
            m_climbNoiseTimer += deltaTime;
            if (m_climbNoiseTimer >= 0.35f) {
                SoundVolumeEmitted = 12.0f;
                m_climbNoiseTimer = 0.0f;
            }
        } else {
            m_climbNoiseTimer = 0.35f;
        }
        
        float maxClimb = ClimbingTreePos.y + 16.0f * ClimbingTreeScale;
        if (Position.y > maxClimb) {
            Position.y = maxClimb;
        }
        
        float terrainHeight = WorldGenerator::GetHeight(Position.x, Position.z);
        if (Position.y < terrainHeight + PlayerHeight) {
            Position.y = terrainHeight + PlayerHeight;
            IsClimbing = false;
            IsGrounded = true;
        }
        
        static bool spaceWasPressed = false;
        bool spaceIsPressed = PlatformInput::IsKeyPressed(PlatformInput::Space);
        if (spaceIsPressed && !spaceWasPressed) {
            IsClimbing = false;
            Velocity.y = JumpForce * 0.8f;
            Velocity.x = toPlayer.x * currentSpeed * 0.8f;
            Velocity.z = toPlayer.z * currentSpeed * 0.8f;
        }
        spaceWasPressed = spaceIsPressed;
    } else {
        bool keyW = PlatformInput::IsKeyPressed(PlatformInput::W) || PlatformInput::IsKeyPressed(PlatformInput::Up);
        bool keyS = PlatformInput::IsKeyPressed(PlatformInput::S) || PlatformInput::IsKeyPressed(PlatformInput::Down);
        bool keyA = PlatformInput::IsKeyPressed(PlatformInput::A) || PlatformInput::IsKeyPressed(PlatformInput::Left);
        bool keyD = PlatformInput::IsKeyPressed(PlatformInput::D) || PlatformInput::IsKeyPressed(PlatformInput::Right);

        static glm::vec3 s_latchedBackDir(0.0f);
        static bool s_isBackLatched = false;

        if (keyS && !keyW) {
            if (!s_isBackLatched) {
                glm::vec3 intendedDir = -flatFront;
                if (keyA) intendedDir -= flatRight;
                if (keyD) intendedDir += flatRight;
                s_latchedBackDir = glm::normalize(intendedDir);
                s_isBackLatched = true;
            }
            moveDir = s_latchedBackDir;
        } else {
            s_isBackLatched = false;
            if (keyW) { moveDir += flatFront; }
            if (keyA) { moveDir -= flatRight; }
            if (keyD) { moveDir += flatRight; }
        }

        if (PlatformInput::IsKeyPressed(PlatformInput::Space)) {
            if (treeNear) {
                IsClimbing = true;
                ClimbingTreePos = glm::vec3(closestTree.x, closestTree.y, closestTree.z);
                ClimbingTreeScale = closestTree.w;
                Velocity = glm::vec3(0.0f);
                IsGrounded = false;
            } else if (IsGrounded) {
                Velocity.y = JumpForce;
                IsGrounded = false;
            }
        }

        if (glm::length(moveDir) > 0.0f) {
            moveDir = glm::normalize(moveDir);
            glm::vec3 displacement = moveDir * currentSpeed * deltaTime;
            glm::vec3 nextPos = Position + displacement;
            
            // Smoothly rotate character towards movement direction
            float targetYaw = glm::degrees(atan2(moveDir.x, moveDir.z));
            float diff = targetYaw - ModelYaw;
            while (diff > 180.0f) diff -= 360.0f;
            while (diff < -180.0f) diff += 360.0f;
            ModelYaw += diff * glm::clamp(deltaTime * 14.0f, 0.0f, 1.0f);

            // In 3rd Person: Camera smoothly rotates and tracks behind player movement direction in all directions (W, A, S, D)
            if (IsThirdPerson && !IsFreeOrbiting) {
                float desiredCamYaw = glm::degrees(atan2(moveDir.z, moveDir.x));
                float camDiff = desiredCamYaw - Yaw;
                while (camDiff > 180.0f) camDiff -= 360.0f;
                while (camDiff < -180.0f) camDiff += 360.0f;
                Yaw += camDiff * glm::clamp(deltaTime * 4.5f, 0.0f, 1.0f);
                updateCameraVectors();
            }

            // Footprints
            if (IsGrounded) {
                 static float distAccumulator = 0.0f;
                 distAccumulator += glm::length(displacement);
                 if (distAccumulator > 1.5f) {
                     distAccumulator = 0.0f;
                     footprints.AddFootprint(Position - glm::vec3(0.0f, PlayerHeight, 0.0f), ModelYaw);
                 }
            }

            // Tree Collision
            std::vector<glm::vec4> collideTrees;
            chunkManager.GetTreesInRange(nextPos, 3.0f, collideTrees); 

            for (const auto& treeData : collideTrees) {
                glm::vec3 treePos(treeData.x, treeData.y, treeData.z);
                float treeScale = treeData.w;
                
                float dx = nextPos.x - treePos.x;
                float dz = nextPos.z - treePos.z;
                float dist = sqrt(dx*dx + dz*dz);
                
                float scaledRadius = 0.6f * treeScale; 
                float minDist = PlayerRadius + scaledRadius;

                if (dist < minDist) {
                    if (dist > 0.0001f) {
                        float push = minDist - dist;
                        nextPos.x += (dx / dist) * push;
                        nextPos.z += (dz / dist) * push;
                    }
                }
            }
            
            // World bounds
            float limit = (Config::World::MapRadius - 1) * Config::World::ChunkSize * Config::World::ChunkScale;
            if (nextPos.x > limit) nextPos.x = limit;
            if (nextPos.x < -limit) nextPos.x = -limit;
            if (nextPos.z > limit) nextPos.z = limit;
            if (nextPos.z < -limit) nextPos.z = -limit;

            Position.x = nextPos.x;
            Position.z = nextPos.z;

            // Structure Wall & Parapet Collision
            StructureSystem::CheckCollision(Position, PlayerRadius, PlayerHeight, Velocity);

            Velocity.x = moveDir.x * currentSpeed;
            Velocity.z = moveDir.z * currentSpeed;

            if (IsGrounded) {
                HeadBobTimer += deltaTime * HeadBobSpeed;
            }
        } else {
            HeadBobTimer = 0.0f;
            Velocity.x = 0.0f;
            Velocity.z = 0.0f;
        }
    }
}

void Player::Update(float deltaTime) {
    if (m_isDead) {
        DeathTimer += deltaTime;
        Velocity.x = 0.0f;
        Velocity.z = 0.0f;
        Velocity.y -= Gravity * deltaTime;
        Position.y += Velocity.y * deltaTime;
        float terrainHeight = WorldGenerator::GetHeight(Position.x, Position.z);
        float groundHeight = StructureSystem::GetWalkableHeight(Position.x, Position.z, Position.y - PlayerHeight, terrainHeight);
        if (Position.y < groundHeight + 0.35f) {
            Position.y = groundHeight + 0.35f;
            Velocity.y = 0.0f;
            IsGrounded = true;
        }
        updateModelMesh();
        return;
    }

    if (IsClimbing) {
        BreathTimer += deltaTime * BreathSpeed;
        m_cameraNoiseAccumulator = std::max(0.0f, m_cameraNoiseAccumulator - deltaTime * 10.0f);
        return;
    }

    Velocity.y -= Gravity * deltaTime;
    Position.y += Velocity.y * deltaTime;

    // Breathing & Walk Animation Timers
    BreathTimer += deltaTime * BreathSpeed;

    float moveSpeed = glm::length(glm::vec2(Velocity.x, Velocity.z));
    if (moveSpeed > 0.1f && IsGrounded) {
        WalkAnimTimer += deltaTime;
    } else {
        WalkAnimTimer = glm::mix(WalkAnimTimer, 0.0f, deltaTime * 8.0f);
    }

    float terrainHeight = WorldGenerator::GetHeight(Position.x, Position.z);
    float groundHeight = StructureSystem::GetWalkableHeight(Position.x, Position.z, Position.y - PlayerHeight, terrainHeight);
    
    if (Position.y < groundHeight + PlayerHeight) {
        Position.y = groundHeight + PlayerHeight;
        Velocity.y = 0.0f;
        IsGrounded = true;
    } else {
        // Sticky feet on slope down / stairs
        float distToGround = Position.y - (groundHeight + PlayerHeight);
        if (IsGrounded && distToGround < 0.65f && Velocity.y <= 0.0f) {
             Position.y = groundHeight + PlayerHeight;
             Velocity.y = 0.0f;
             IsGrounded = true;
        } else {
             IsGrounded = false;
        }
    }

    // Colisión sólida continua contra estructuras y almenas
    StructureSystem::CheckCollision(Position, PlayerRadius, PlayerHeight, Velocity);

    WeaponSwayPos = glm::mix(WeaponSwayPos, glm::vec3(0.0f), glm::clamp(deltaTime * SwaySmoothing, 0.0f, 1.0f));
}

glm::vec3 Player::GetCameraPosition() {
    if (IsThirdPerson) {
        // Center focus on upper torso / shoulders
        glm::vec3 focus = Position - glm::vec3(0.0f, 0.4f, 0.0f);
        glm::vec3 camPos = focus - Front * CameraDistance;

        // Anti-clipping terrain & structure check
        float groundY = WorldGenerator::GetHeight(camPos.x, camPos.z) + 0.45f;
        float structY = StructureSystem::GetWalkableHeight(camPos.x, camPos.z, camPos.y, groundY) + 0.45f;
        if (camPos.y < structY) {
            camPos.y = structY;
        }
        return camPos;
    } else {
        glm::vec3 pos = Position;
        if (IsGrounded) {
            if (HeadBobTimer > 0.001f) {
                pos.y += sin(HeadBobTimer) * HeadBobAmount;
            } else {
                pos.y += sin(BreathTimer) * BreathAmount;
            }
        }
        return pos;
    }
}

glm::mat4 Player::GetViewMatrix() {
    if (IsThirdPerson) {
        glm::vec3 focus = Position - glm::vec3(0.0f, 0.4f, 0.0f);
        glm::vec3 camPos = GetCameraPosition();
        return glm::lookAt(camPos, focus, WorldUp);
    } else {
        glm::vec3 pos = GetCameraPosition();
        return glm::lookAt(pos, pos + Front, Up);
    }
}

glm::vec3 Player::GetWeaponOffset() {
    glm::vec3 base = glm::vec3(0.2f, -0.25f, 0.4f); 
    base.x += WeaponSwayPos.x;
    base.y += WeaponSwayPos.y;
    return base;
}

void Player::updateCameraVectors() {
    glm::vec3 front;
    front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front.y = sin(glm::radians(Pitch));
    front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    Front = glm::normalize(front);
    Right = glm::normalize(glm::cross(Front, WorldUp));
    Up    = glm::normalize(glm::cross(Right, Front));
}

float Player::getTerrainHeight(float x, float z) {
    return WorldGenerator::GetHeight(x, z);
}

void Player::Render(GLuint shaderProgram) {
    if (!IsThirdPerson || m_playerVAO == 0 || m_playerVertexCount == 0) return;

    updateModelMesh();

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    // Base feet are at (Position.y - PlayerHeight)
    model = glm::translate(model, Position - glm::vec3(0.0f, PlayerHeight, 0.0f));
    model = glm::rotate(model, glm::radians(ModelYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    glBindVertexArray(m_playerVAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)m_playerVertexCount);
    glBindVertexArray(0);
}

void Player::RenderDebug(GLuint shaderProgram) {
    static GLuint debugVAO = 0, debugVBO = 0;
    if (debugVAO == 0) {
        float b = 0.5f;
        float h = 1.0f;
        float cube[] = {
            -b,h,b, b,h,b, b,-h,b, -b,h,b, b,-h,b, -b,-h,b,
            b,h,-b, -b,h,-b, -b,-h,-b, b,h,-b, -b,-h,-b, b,-h,-b,
            -b,h,-b, -b,h,b, -b,-h,b, -b,h,-b, -b,-h,b, -b,-h,-b,
            b,h,b, b,h,-b, b,-h,-b, b,h,b, b,-h,-b, b,-h,b,
            -b,h,-b, b,h,-b, b,h,b, -b,h,-b, b,h,b, -b,h,b,
            -b,-h,b, b,-h,b, b,-h,-b, -b,-h,b, b,-h,-b, -b,-h,-b 
        };
        glGenVertexArrays(1, &debugVAO);
        glGenBuffers(1, &debugVBO);
        glBindVertexArray(debugVAO);
        glBindBuffer(GL_ARRAY_BUFFER, debugVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
    }

    GLint modelLoc = glGetUniformLocation(shaderProgram, "u_Model");
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, Position);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

    glDisable(GL_DEPTH_TEST);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 2);

    glBindVertexArray(debugVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glEnable(GL_DEPTH_TEST);

    glDisable(GL_DEPTH_TEST);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 2);

    glm::vec3 lineStart = Position;
    glm::vec3 lineEnd = Position + Front * 5.0f;

    std::vector<float> lineData = {
        lineStart.x, lineStart.y, lineStart.z,   0,0,1,  0,0,0,
        lineEnd.x, lineEnd.y, lineEnd.z,         0,0,1,  0,0,0
    };

    GLuint lineVAO, lineVBO;
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, lineData.size() * sizeof(float), lineData.data(), GL_STREAM_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
    glDrawArrays(GL_LINES, 0, 2);

    glDeleteVertexArrays(1, &lineVAO);
    glDeleteBuffers(1, &lineVBO);

    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsDebug"), 0);
    glEnable(GL_DEPTH_TEST);
}
