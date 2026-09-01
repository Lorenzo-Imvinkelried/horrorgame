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
}

#include "UIRenderer.h"

bool Player::TryAttack() {
    if (StunTimer > 0.0f) return false;
    if (m_attackTimer <= 0.0f && m_attackCooldownTimer <= 0.0f && !m_isBlocking) {
        float agiFactor = Stats.GetAttackSpeedMultiplier();
        m_attackDuration = 0.52f / agiFactor;
        m_attackCooldown = 0.28f / agiFactor;
        m_attackTimer = m_attackDuration;
        m_attackCooldownTimer = m_attackDuration + m_attackCooldown;
        m_attackHitDone = false;
        m_attackCombo = (m_attackCombo + 1) % 2;
        return true;
    }
    return false;
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
        dmg = std::max(1, dmg - Stats.Defense / 2);
    }
    Stats.CurrentHP -= dmg;
    if (Stats.CurrentHP < 0) Stats.CurrentHP = 0;
    damageNumbers.SpawnDamage(Position + glm::vec3(0.0f, 1.2f, 0.0f), dmg, false);

    // Fatal Error Popup Trigger on massive damage hits
    if (dmg >= 22 && fatalError != nullptr) {
        fatalError->active = true;
        fatalError->damageValue = dmg;
        fatalError->timer = 4.0f;
        fatalError->message = "EXCEPCION EN 0x000000FF";
    }
}

#include "CombatCalculator.h"
#include "DamageNumberSystem.h"

#include "EnemyMob.h"
#include "WaterMonster.h"
#include "entities/Dragon.h"

void Player::UpdateCombat(float deltaTime, std::vector<std::unique_ptr<Monster>>& monsters, std::vector<std::unique_ptr<PassiveMob>>& passiveMobs, std::vector<std::unique_ptr<EnemyMob>>& enemyMobs, std::vector<std::unique_ptr<WaterMonster>>& waterMonsters, ParticleSystem& particles, DamageNumberSystem& damageNumbers, Dragon* dragon) {
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
        
        // Midpoint of sword swing (at the climax of the downward chop): deliver hit
        if (progress >= 0.48f && !m_attackHitDone) {
            m_attackHitDone = true;
            
            // In 3rd person use character facing, in 1st person use camera Front
            glm::vec3 forwardDir = IsThirdPerson ? glm::vec3(sin(glm::radians(ModelYaw)), 0.0f, cos(glm::radians(ModelYaw))) : Front;

            // 0. Check Dragon
            if (dragon != nullptr && dragon->IsAlive() && !dragon->IsDying()) {
                glm::vec3 dPos = dragon->GetPosition();
                float dist = glm::distance(Position, dPos);
                if (dist < (4.8f + dragon->GetRadius())) {
                    AttackDamageResult dmgResult = CombatCalculator::CalculatePlayerAttack(Stats.Attack, Stats.CritChance, Stats.CritMultiplier, 12, 5);
                    if (dmgResult.IsHit) {
                        bool killed = dragon->TakeDamage(dmgResult.Damage, Position, particles, damageNumbers, this);
                        damageNumbers.SpawnDamage(dPos + glm::vec3(0, 2.0f, 0), dmgResult.Damage, dmgResult.IsCrit);

                        if (killed) {
                            bool leveledUp = false;
                            int expGain = dragon->GetExpReward();
                            Stats.AddExp(expGain, leveledUp);
                            damageNumbers.SpawnExp(dPos + glm::vec3(0, 2.5f, 0), expGain);
                            if (leveledUp) damageNumbers.SpawnLevelUp(Position);
                        }
                    }
                }
            }
            
            // 1. Check aggressive shadow monsters
            for (auto& mPtr : monsters) {
                if (mPtr->IsDead()) continue;
                glm::vec3 mPos = mPtr->GetPosition();
                float dist = glm::distance(Position, mPos);
                if (dist < 3.8f) {
                    glm::vec3 toEnemy = glm::normalize(mPos - Position);
                    float dot = glm::dot(forwardDir, toEnemy);
                    if (dot > 0.0f || dist < 1.8f) { // Wide front arc & close combat guarantee
                        AttackDamageResult dmgResult = CombatCalculator::CalculatePlayerAttack(Stats.Attack, Stats.CritChance, Stats.CritMultiplier, 10, 5);
                        if (dmgResult.IsHit) {
                            mPtr->TakeDamage((float)dmgResult.Damage, false);
                            damageNumbers.SpawnDamage(mPos, dmgResult.Damage, dmgResult.IsCrit);

                            // Spawn impact blood / spark particles
                            for (int i = 0; i < 24; ++i) {
                                glm::vec3 pVel((rand()%100/50.0f - 1.0f)*3.5f, (rand()%100/50.0f + 0.3f)*4.0f, (rand()%100/50.0f - 1.0f)*3.5f);
                                particles.SpawnParticle(mPos + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.85f, 0.05f, 0.05f, 1.0f), 0.14f, 0.85f, -9.8f);
                            }

                            if (mPtr->IsDead()) {
                                bool leveledUp = false;
                                Stats.AddExp(85, leveledUp);
                                damageNumbers.SpawnExp(mPos, 85);
                                if (leveledUp) damageNumbers.SpawnLevelUp(Position);
                            }
                        }
                    }
                }
            }

            // 2. Check lake water monsters (Water Lurkers)
            for (auto& wm : waterMonsters) {
                if (!wm->IsAlive()) continue;
                glm::vec3 wPos = wm->GetPosition();
                float dist = glm::distance(Position, wPos);
                if (dist < (3.8f + wm->GetRadius()) || wm->IsDragging()) {
                    glm::vec3 toWm = (dist > 0.001f) ? glm::normalize(wPos - Position) : forwardDir;
                    float dot = glm::dot(forwardDir, toWm);
                    if (dot > 0.0f || dist < 2.2f || wm->IsDragging()) {
                        AttackDamageResult dmgResult = CombatCalculator::CalculatePlayerAttack(Stats.Attack, Stats.CritChance, Stats.CritMultiplier, 4, 10);
                        if (dmgResult.IsHit) {
                            bool killed = wm->TakeDamage(dmgResult.Damage, Position, particles, this, damageNumbers);
                            damageNumbers.SpawnDamage(wPos, dmgResult.Damage, dmgResult.IsCrit);

                            if (killed) {
                                bool leveledUp = false;
                                int expGain = wm->GetExpReward();
                                Stats.AddExp(expGain, leveledUp);
                                damageNumbers.SpawnExp(wPos, expGain);
                                if (leveledUp) damageNumbers.SpawnLevelUp(Position);
                            }
                        }
                    }
                }
            }

            // 3. Check enemy mobs (Corrupted Warriors, Neutral Giants, Dark Mages)
            for (auto& enemyPtr : enemyMobs) {
                if (!enemyPtr->IsAlive()) continue;
                glm::vec3 ePos = enemyPtr->GetPosition();
                float dist = glm::distance(Position, ePos);
                if (dist < (3.8f + enemyPtr->GetRadius())) {
                    glm::vec3 toEnemy = glm::normalize(ePos - Position);
                    float dot = glm::dot(forwardDir, toEnemy);
                    if (dot > 0.0f || dist < (1.8f + enemyPtr->GetRadius() * 0.5f)) {
                        AttackDamageResult dmgResult = CombatCalculator::CalculatePlayerAttack(Stats.Attack, Stats.CritChance, Stats.CritMultiplier, 6, 8);
                        if (dmgResult.IsHit) {
                            bool killed = enemyPtr->TakeDamage(dmgResult.Damage, Position, particles, this, damageNumbers);
                            damageNumbers.SpawnDamage(ePos, dmgResult.Damage, dmgResult.IsCrit);

                            if (killed) {
                                bool leveledUp = false;
                                int expGain = enemyPtr->GetExpReward();
                                Stats.AddExp(expGain, leveledUp);
                                damageNumbers.SpawnExp(ePos, expGain);
                                if (leveledUp) damageNumbers.SpawnLevelUp(Position);
                            }
                        }
                    }
                }
            }

            // 4. Check passive & hostile mobs (Forest Deer & Demonic Deer)
            for (auto& mobPtr : passiveMobs) {
                if (!mobPtr->IsAlive()) continue;
                glm::vec3 mobPos = mobPtr->GetPosition();
                float dist = glm::distance(Position, mobPos);
                if (dist < (3.8f + mobPtr->GetRadius())) {
                    glm::vec3 toMob = glm::normalize(mobPos - Position);
                    float dot = glm::dot(forwardDir, toMob);
                    if (dot > 0.0f || dist < 1.8f) { // Close quarters guarantee
                        AttackDamageResult dmgResult = CombatCalculator::CalculatePlayerAttack(Stats.Attack, Stats.CritChance, Stats.CritMultiplier, 4, 12);
                        if (dmgResult.IsHit) {
                            bool killed = mobPtr->TakeDamage(dmgResult.Damage, Position, particles, this, damageNumbers);
                            damageNumbers.SpawnDamage(mobPos, dmgResult.Damage, dmgResult.IsCrit);

                            // EXP is ONLY granted when the mob is killed!
                            if (killed) {
                                bool leveledUp = false;
                                int expGain = mobPtr->GetExpReward();
                                Stats.AddExp(expGain, leveledUp);
                                damageNumbers.SpawnExp(mobPos, expGain);
                                if (leveledUp) damageNumbers.SpawnLevelUp(Position);
                            }
                        }
                    }
                }
            }
        }
    }
}

void Player::initModel() {
    m_baseBoxes = ModelLoader::Load("assets/models/player.txt");
    if (m_baseBoxes.empty()) {
        std::cerr << "[Player] Warning: player.txt model is empty or not found!" << std::endl;
    }

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

    updateModelMesh();
}

void Player::updateModelMesh() {
    if (m_baseBoxes.empty() || m_playerVAO == 0) return;

    float moveSpeed = glm::length(glm::vec2(Velocity.x, Velocity.z));
    
    // PS1 style procedural limb swing angles
    float legSwing = (moveSpeed > 0.1f && IsGrounded) ? sin(WalkAnimTimer * 9.0f) * 0.45f : 0.0f;
    float armSwing = (moveSpeed > 0.1f && IsGrounded) ? sin(WalkAnimTimer * 9.0f) * 0.38f : 0.0f;
    float capeFlutter = (moveSpeed > 0.1f) ? sin(WalkAnimTimer * 18.0f) * 0.14f : sin(BreathTimer * 2.5f) * 0.04f;
    float breathingTorso = sin(BreathTimer * 2.5f) * 0.015f;

    bool isAttacking = (m_attackTimer > 0.0f);
    float attackProgress = isAttacking ? (1.0f - (m_attackTimer / m_attackDuration)) : 0.0f;
    
    // Attack rotation for the entire right arm + sword chain (Vertical Overhead Chop from Top to Bottom)
    float rightArmRotX = 0.0f;
    float rightArmRotY = 0.0f;
    float rightArmRotZ = 0.0f;
    
    if (isAttacking) {
        if (attackProgress < 0.32f) {
            // Phase 1: SUBE - Windup upwards above head
            float w = attackProgress / 0.32f;
            float smoothW = w * w * (3.0f - 2.0f * w);
            rightArmRotX = glm::mix(0.0f, 1.65f, smoothW);
            rightArmRotY = glm::mix(0.0f, 0.10f, smoothW);
            rightArmRotZ = glm::mix(0.0f, -0.12f, smoothW);
        } else if (attackProgress < 0.72f) {
            // Phase 2: BAJA - Powerful vertical cleave top to bottom
            float s = (attackProgress - 0.32f) / 0.40f;
            float smoothS = s * s * (3.0f - 2.0f * s);
            rightArmRotX = glm::mix(1.65f, -1.35f, smoothS);
            rightArmRotY = glm::mix(0.10f, -0.04f, smoothS);
            rightArmRotZ = glm::mix(-0.12f, 0.04f, smoothS);
        } else {
            // Phase 3: RECOVERY - Return smoothly to resting stance
            float r = (attackProgress - 0.72f) / 0.28f;
            float smoothR = r * r * (3.0f - 2.0f * r);
            rightArmRotX = glm::mix(-1.35f, 0.0f, smoothR);
            rightArmRotY = glm::mix(-0.04f, 0.0f, smoothR);
            rightArmRotZ = glm::mix(0.04f, 0.0f, smoothR);
        }
    } else if (m_isBlocking) {
        // Defensive Parry Stance
        rightArmRotX = 0.40f;
        rightArmRotY = -0.60f;
        rightArmRotZ = -0.65f;
    } else {
        // Normal walking swing
        rightArmRotX = armSwing * 0.65f;
    }

    // Joint Pivots for rigid hierarchical rotation
    glm::vec3 shoulderPivotR(0.28f, 1.30f, 0.0f);
    glm::vec3 shoulderPivotL(-0.28f, 1.30f, 0.0f);
    glm::vec3 hipPivotL(-0.13f, 0.76f, 0.0f);
    glm::vec3 hipPivotR(0.13f, 0.76f, 0.0f);

    // Build rotation matrix for right arm + sword chain
    glm::mat4 R_Arm = glm::mat4(1.0f);
    R_Arm = glm::rotate(R_Arm, rightArmRotZ, glm::vec3(0, 0, 1));
    R_Arm = glm::rotate(R_Arm, rightArmRotY, glm::vec3(0, 1, 0));
    R_Arm = glm::rotate(R_Arm, rightArmRotX, glm::vec3(1, 0, 0));

    // Build rotation matrix for left arm (Holds torch upright when active)
    float leftArmRotX = HasTorchActive ? 0.72f : (m_isBlocking ? -0.30f : -armSwing);
    float leftArmRotZ = HasTorchActive ? -0.15f : (m_isBlocking ? 0.40f : 0.0f);
    glm::mat4 L_Arm = glm::mat4(1.0f);
    L_Arm = glm::rotate(L_Arm, leftArmRotZ, glm::vec3(0, 0, 1));
    L_Arm = glm::rotate(L_Arm, leftArmRotX, glm::vec3(1, 0, 0));

    std::vector<TransformedBox> transformedBoxes;
    transformedBoxes.reserve(m_baseBoxes.size() + 4);

    for (const auto& box : m_baseBoxes) {
        // Base Box Model Matrix
        glm::mat4 M = glm::mat4(1.0f);
        M = glm::translate(M, box.Pos);
        M = glm::rotate(M, box.Rot.z, glm::vec3(0,0,1));
        M = glm::rotate(M, box.Rot.y, glm::vec3(0,1,0));
        M = glm::rotate(M, box.Rot.x, glm::vec3(1,0,0));
        M = glm::scale(M, box.Scale);

        // Left Leg
        if (box.Name == "THIGH_L" || box.Name == "BOOT_L" || box.Name == "FOOT_L") {
            glm::mat4 finalM = glm::translate(glm::mat4(1.0f), hipPivotL) * glm::rotate(glm::mat4(1.0f), legSwing, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -hipPivotL) * M;
            transformedBoxes.push_back({finalM, box.Color});
        }
        // Right Leg
        else if (box.Name == "THIGH_R" || box.Name == "BOOT_R" || box.Name == "FOOT_R") {
            glm::mat4 finalM = glm::translate(glm::mat4(1.0f), hipPivotR) * glm::rotate(glm::mat4(1.0f), -legSwing, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -hipPivotR) * M;
            transformedBoxes.push_back({finalM, box.Color});
        }
        // Left Arm
        else if (box.Name == "SHOULDER_L" || box.Name == "ARM_UPPER_L" || box.Name == "FOREARM_L" || box.Name == "HAND_L") {
            glm::mat4 finalM = glm::translate(glm::mat4(1.0f), shoulderPivotL) * L_Arm * glm::translate(glm::mat4(1.0f), -shoulderPivotL) * M;
            transformedBoxes.push_back({finalM, box.Color});
        }
        // Right Arm & Complete Sword (100% Rigid Body Transform - Never breaks or separates)
        else if (box.Name == "SHOULDER_R" || box.Name == "ARM_UPPER_R" || box.Name == "FOREARM_R" || box.Name == "HAND_R" || box.Name.find("SWORD") != std::string::npos) {
            glm::mat4 finalM = glm::translate(glm::mat4(1.0f), shoulderPivotR) * R_Arm * glm::translate(glm::mat4(1.0f), -shoulderPivotR) * M;
            transformedBoxes.push_back({finalM, box.Color});
        }
        // Cape & Coat
        else if (box.Name == "CAPE_BACK" || box.Name == "CAPE_LOWER" || box.Name == "SKIRT") {
            glm::mat4 finalM = glm::translate(glm::mat4(1.0f), box.Pos) * glm::rotate(glm::mat4(1.0f), capeFlutter, glm::vec3(1,0,0)) * glm::translate(glm::mat4(1.0f), -box.Pos) * M;
            transformedBoxes.push_back({finalM, box.Color});
        }
        // Torso Breathing
        else if (box.Name == "CHEST_PLATE" || box.Name == "NECK_COLLAR") {
            glm::mat4 finalM = glm::scale(M, glm::vec3(1.0f, 1.0f + breathingTorso, 1.0f + breathingTorso));
            transformedBoxes.push_back({finalM, box.Color});
        }
        // Head / Torso Default
        else {
            transformedBoxes.push_back({M, box.Color});
        }
    }

    // Attach 3D Torch Model to Left Hand in 3rd Person
    if (HasTorchActive) {
        glm::mat4 torchBase = glm::translate(glm::mat4(1.0f), shoulderPivotL) * L_Arm * glm::translate(glm::mat4(1.0f), -shoulderPivotL);
        // Handle
        glm::mat4 handleM = torchBase * glm::translate(glm::mat4(1.0f), glm::vec3(-0.28f, 0.95f, 0.28f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.08f, 0.55f, 0.08f));
        transformedBoxes.push_back({handleM, glm::vec3(0.28f, 0.18f, 0.10f)});
        // Ring
        glm::mat4 ringM = torchBase * glm::translate(glm::mat4(1.0f), glm::vec3(-0.28f, 1.20f, 0.28f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.14f, 0.08f, 0.14f));
        transformedBoxes.push_back({ringM, glm::vec3(0.22f, 0.22f, 0.24f)});
        // Flame
        glm::mat4 flameM = torchBase * glm::translate(glm::mat4(1.0f), glm::vec3(-0.28f, 1.30f, 0.28f)) * glm::scale(glm::mat4(1.0f), glm::vec3(0.12f, 0.16f, 0.12f));
        transformedBoxes.push_back({flameM, glm::vec3(0.98f, 0.70f, 0.15f)});
    }

    std::vector<float> rawVertices;
    ModelLoader::GenerateMeshTransformed(transformedBoxes, rawVertices);
    m_playerVertexCount = rawVertices.size() / 11;

    glBindBuffer(GL_ARRAY_BUFFER, m_playerVBO);
    glBufferData(GL_ARRAY_BUFFER, rawVertices.size() * sizeof(float), rawVertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Player::RenderFirstPersonSword(GLuint shaderProgram) {
    if (m_baseBoxes.empty()) return;

    // Filter only right hand and sword boxes for 1st person viewmodel
    std::vector<BoxDef> fpBoxes;
    for (const auto& box : m_baseBoxes) {
        if (box.Name.find("SWORD") != std::string::npos || box.Name == "HAND_R" || box.Name == "FOREARM_R") {
            BoxDef b = box;
            // Center relative to right hand
            b.Pos.x -= 0.28f;
            b.Pos.y -= 0.72f;
            b.Pos.z -= 0.08f;
            fpBoxes.push_back(b);
        }
    }

    static GLuint fpVAO = 0, fpVBO = 0;
    static size_t fpVertexCount = 0;
    if (fpVAO == 0) {
        std::vector<float> rawVertices;
        ModelLoader::GenerateMesh(fpBoxes, rawVertices);
        fpVertexCount = rawVertices.size() / 11;

        glGenVertexArrays(1, &fpVAO);
        glGenBuffers(1, &fpVBO);
        glBindVertexArray(fpVAO);
        glBindBuffer(GL_ARRAY_BUFFER, fpVBO);
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

    // Position sword in lower-right of screen
    glm::vec3 viewPos(0.24f, -0.28f, 0.46f);
    viewPos += WeaponSwayPos;
    if (IsGrounded) {
        if (HeadBobTimer > 0.001f) viewPos.y += sin(HeadBobTimer) * 0.02f;
        else viewPos.y += sin(BreathTimer) * 0.01f;
    }
    model = glm::translate(model, viewPos);

    if (m_attackTimer > 0.0f) {
        float progress = 1.0f - (m_attackTimer / m_attackDuration);
        if (progress < 0.32f) {
            // Phase 1: SUBE - Windup upwards high above screen
            float w = progress / 0.32f;
            float smoothW = w * w * (3.0f - 2.0f * w);
            model = glm::translate(model, glm::vec3(0.02f, 0.32f, -0.12f) * smoothW);
            model = glm::rotate(model, glm::radians(52.0f * smoothW), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(-18.0f * smoothW), glm::vec3(0, 0, 1));
        } else if (progress < 0.72f) {
            // Phase 2: BAJA - Heavy cleave chopping from top to bottom
            float s = (progress - 0.32f) / 0.40f;
            float smoothS = s * s * (3.0f - 2.0f * s);
            model = glm::translate(model, glm::mix(glm::vec3(0.02f, 0.32f, -0.12f), glm::vec3(-0.04f, -0.42f, 0.14f), smoothS));
            model = glm::rotate(model, glm::radians(glm::mix(52.0f, -75.0f, smoothS)), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(glm::mix(-18.0f, -5.0f, smoothS)), glm::vec3(0, 0, 1));
        } else {
            // Phase 3: RECOVERY - Reset smoothly to idle
            float r = (progress - 0.72f) / 0.28f;
            float smoothR = r * r * (3.0f - 2.0f * r);
            model = glm::translate(model, glm::mix(glm::vec3(-0.04f, -0.42f, 0.14f), glm::vec3(0.0f), smoothR));
            model = glm::rotate(model, glm::radians(glm::mix(-75.0f, 0.0f, smoothR)), glm::vec3(1, 0, 0));
            model = glm::rotate(model, glm::radians(glm::mix(-5.0f, 0.0f, smoothR)), glm::vec3(0, 0, 1));
        }
    } else if (m_isBlocking) {
        // Defensive block stance
        model = glm::translate(model, glm::vec3(-0.12f, 0.10f, -0.05f));
        model = glm::rotate(model, glm::radians(-35.0f), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0, 0, 1));
    } else {
        // Idle tilt
        model = glm::rotate(model, glm::radians(10.0f), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(1, 0, 0));
    }

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(glGetUniformLocation(shaderProgram, "u_IsInstanced"), 0);
    glUniform1i(glGetUniformLocation(shaderProgram, "u_ConformToTerrain"), 0);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_WindStrength"), 0.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_Alpha"), 1.0f);

    glBindVertexArray(fpVAO);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)fpVertexCount);
    glBindVertexArray(0);
}

void Player::RenderFirstPersonTorch(GLuint shaderProgram) {
    if (!HasTorchActive) return;

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
    float sensitivity = 0.12f;
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
        if (PlatformInput::IsKeyPressed(PlatformInput::W) || PlatformInput::IsKeyPressed(PlatformInput::Up)) { moveDir += flatFront; }
        if (PlatformInput::IsKeyPressed(PlatformInput::S) || PlatformInput::IsKeyPressed(PlatformInput::Down)) { moveDir -= flatFront; }
        if (PlatformInput::IsKeyPressed(PlatformInput::A) || PlatformInput::IsKeyPressed(PlatformInput::Left)) { moveDir -= flatRight; }
        if (PlatformInput::IsKeyPressed(PlatformInput::D) || PlatformInput::IsKeyPressed(PlatformInput::Right)) { moveDir += flatRight; }

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

            // In 3rd Person: Camera smoothly tracks behind player movement when walking forward/steering (W, W+A, W+D)
            // When moving backwards (S), camera stays stable to prevent feedback spin loops and allow kiting/retreating
            if (IsThirdPerson && !IsFreeOrbiting) {
                float forwardAlignment = glm::dot(moveDir, flatFront);
                if (forwardAlignment > 0.25f) {
                    float desiredCamYaw = glm::degrees(atan2(moveDir.z, moveDir.x));
                    float camDiff = desiredCamYaw - Yaw;
                    while (camDiff > 180.0f) camDiff -= 360.0f;
                    while (camDiff < -180.0f) camDiff += 360.0f;
                    Yaw += camDiff * glm::clamp(deltaTime * 4.0f, 0.0f, 1.0f);
                    updateCameraVectors();
                }
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
    
    if (Position.y < terrainHeight + PlayerHeight) {
        float targetY = terrainHeight + PlayerHeight;
        Position.y = glm::mix(Position.y, targetY, glm::clamp(deltaTime * 15.0f, 0.0f, 1.0f));
        Velocity.y = 0.0f;
        IsGrounded = true;
    } else {
        // Sticky feet on slope down
        float distToGround = Position.y - (terrainHeight + PlayerHeight);
        if (IsGrounded && distToGround < 0.5f && Velocity.y <= 0.0f) {
             float targetY = terrainHeight + PlayerHeight;
             Position.y = glm::mix(Position.y, targetY, glm::clamp(deltaTime * 20.0f, 0.0f, 1.0f));
             Velocity.y = 0.0f;
             IsGrounded = true;
        } else {
             IsGrounded = false;
        }
    }

    WeaponSwayPos = glm::mix(WeaponSwayPos, glm::vec3(0.0f), glm::clamp(deltaTime * SwaySmoothing, 0.0f, 1.0f));
}

glm::vec3 Player::GetCameraPosition() {
    if (IsThirdPerson) {
        // Center focus on upper torso / shoulders
        glm::vec3 focus = Position - glm::vec3(0.0f, 0.4f, 0.0f);
        glm::vec3 camPos = focus - Front * CameraDistance;

        // Anti-clipping terrain check
        float groundY = WorldGenerator::GetHeight(camPos.x, camPos.z) + 0.45f;
        if (camPos.y < groundY) {
            camPos.y = groundY;
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
