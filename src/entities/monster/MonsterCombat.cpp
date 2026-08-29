#include "Monster.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <algorithm>
#include <cmath>
#include "Config.h"
#include "WorldGenerator.h"

// Helper for AABB overlap
static bool RayAABBLocal(glm::vec3 origin, glm::vec3 dir, glm::vec3 minB, glm::vec3 maxB, float& tScale) {
    float t1 = (minB.x - origin.x)/dir.x;
    float t2 = (maxB.x - origin.x)/dir.x;
    float t3 = (minB.y - origin.y)/dir.y;
    float t4 = (maxB.y - origin.y)/dir.y;
    float t5 = (minB.z - origin.z)/dir.z;
    float t6 = (maxB.z - origin.z)/dir.z;

    float tNear = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
    float tFar = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

    if (tFar < 0 || tNear > tFar) return false;
    
    tScale = tNear;
    return true;
}

void Monster::HearSound(glm::vec3 sourcePos, float volume) {
    float dist = glm::distance(m_pos, sourcePos);
    if (dist < volume) {
        m_memPlayerPos = sourcePos;
        m_memTimeSinceHeard = 0.0f;
        std::cout << "[AI] Heard noise! Memory updated." << std::endl;

        // If volume is Config::Gameplay::GunshotSoundRange, it's a shotgun blast! Incur bullet count estimation
        if (std::abs(volume - Config::Gameplay::GunshotSoundRange) < 0.1f) {
            m_estimatedPlayerAmmo = std::max(0, m_estimatedPlayerAmmo - 1);
            std::cout << "[AI] Heard shotgun blast! Estimated player ammo: " << m_estimatedPlayerAmmo << std::endl;
        } else if (std::abs(volume - 120.0f) < 0.1f) {
            m_estimatedPlayerAmmo = std::max(0, m_estimatedPlayerAmmo - 1);
            std::cout << "[AI] Heard normal gunshot! Estimated ammo: " << m_estimatedPlayerAmmo << std::endl;
        }
    }
}

void Monster::TakeDamage(float amount, bool isHeadshot) {
    if (m_isDead) return;

    if (isHeadshot) {
        m_health = 0.0f; // Insta-kill
    } else {
        m_health -= amount;
        m_decisionLockTimer = 0.0f; // Break decision lock on taking damage
        
        // --- CLIMBING INTERRUPT ---
        // If monster is currently in the tree, force it to fall/drop to the ground immediately
        bool wasOnTree = (m_treeClimbHeight > 0.0f || m_action == MonsterAction::CLIMB_TREE);
        if (wasOnTree) {
            m_treeClimbHeight = 0.0f;
            m_isClimbing = false;
            m_bestTreeIndex = -1;
            m_pos.y = WorldGenerator::GetHeight(m_pos.x, m_pos.z);
            m_visualPos = m_pos;
            m_climbCooldownTimer = 10.0f; // Prevent re-climbing immediately
        }

        // --- CLIMBING ATTACK vs FLEE DECISION ---
        if (glm::length(m_knownPlayerTreePos) > 0.1f || wasOnTree) {
            // Decrement the estimated ammo immediately as they just fired the shot that hit us!
            m_estimatedPlayerAmmo = std::max(0, m_estimatedPlayerAmmo - 1);

            bool shouldFlee = false;
            if (m_estimatedPlayerAmmo == 0) {
                // Player has no ammo left, attack violently!
                shouldFlee = false;
            } else {
                // Roll probability based on health
                float fleeChance = 0.5f; // 50% base
                if (m_health >= 3.0f) fleeChance = 0.3f; // 30% if high health
                else if (m_health < 2.0f) fleeChance = 0.8f; // 80% if low health
                
                if ((rand() % 100) < (fleeChance * 100.0f)) {
                    shouldFlee = true;
                }
            }
            
            if (shouldFlee) {
                std::cout << "[AI-Damage] Shot in tree! Decided to FLEE. Retreating..." << std::endl;
                m_climbCooldownTimer = 20.0f; // 20 seconds cooldown on tree climbing to prevent immediate re-climbing
                m_knownPlayerTreePos = glm::vec3(0.0f);
                m_memTimeSinceSeen = 999.0f;
                m_memTimeSinceHeard = 999.0f;
                m_memTimeSinceSmelled = 999.0f;
                
                m_action = MonsterAction::RETREAT;
                m_stateTimer = 0.0f;
                m_stress = 1.0f;
                m_confidence = 0.0f;
            } else {
                std::cout << "[AI-Damage] Shot in tree! Decided to ATTACK VIOLENTLY. Enraging..." << std::endl;
                m_isEnraged = true;
                m_rageTimer = 0.0f;
                m_confidence = 1.0f;
                m_stress = 0.0f;
                m_shouldScream = true; // Trigger visual scream burst in Update()
                
                m_action = MonsterAction::CHASE; // Charge from the ground
                m_stateTimer = 0.0f;
            }
        }
    }

    if (m_health <= 0.0f) {
        m_health = 0.0f;
        m_isDead = true;
        m_pos = glm::vec3(0, -1000, 0); // Move away
        m_visualPos = m_pos;
    }
}

bool Monster::IntersectRay(glm::vec3 origin, glm::vec3 dir, float& dist, bool& isHeadshot) {
    if (m_isDead) return false;

    // Transform Ray to Local Space
    // 1. Translate
    glm::vec3 localOrigin = origin - m_pos;
    
    // 2. Rotate (Inverse Yaw)
    float angle = -glm::radians(m_yaw); 
    float c = cos(angle);
    float s = sin(angle);
    
    auto rotateY = [&](glm::vec3 v) {
        return glm::vec3(v.x * c - v.z * s, v.y, v.x * s + v.z * c);
    };
    
    localOrigin = rotateY(localOrigin);
    glm::vec3 localDir = rotateY(dir);

    // Use Pre-Calculated Dynamic AABBs
    glm::vec3 headMin = m_headMin;
    glm::vec3 headMax = m_headMax;
    glm::vec3 bodyMin = m_bodyMin;
    glm::vec3 bodyMax = m_bodyMax;

    // Crouch Hitbox Adjustments (Visual matching)
    if (m_crouchFactor > 0.01f) {
        headMin.y -= 0.7f * m_crouchFactor;
        headMax.y -= 0.7f * m_crouchFactor;
        bodyMax.y -= 0.6f * m_crouchFactor;
    }

    // Lean Hitbox Compensation: during chase, body tilts forward (bodyLean = 0.4)
    if (m_action == MonsterAction::CHASE) {
        headMin.z += 0.3f;
        headMax.z += 0.5f;
        headMin.y -= 0.25f; // Lower head hitbox to match visually leaned run
        headMax.y -= 0.25f;
        bodyMin.z += 0.1f;
        bodyMax.z += 0.4f;
    }

    // HEAD (Local)
    float tHead = 10000.0f;
    bool hitHead = RayAABBLocal(localOrigin, localDir, headMin, headMax, tHead);

    // BODY (Local)
    float tBody = 10000.0f;
    bool hitBody = RayAABBLocal(localOrigin, localDir, bodyMin, bodyMax, tBody);

    if (hitHead && hitBody) {
        // Prioritize Head if depths are similar or if head is reasonably close
        if ((tHead - 0.5f) < tBody) { isHeadshot = true; dist = tHead; return true; }
        else { isHeadshot = false; dist = tBody; return true; }
    } else if (hitHead) {
        isHeadshot = true; dist = tHead; return true;
    } else if (hitBody) {
        isHeadshot = false; dist = tBody; return true;
    }

    return false;
}
