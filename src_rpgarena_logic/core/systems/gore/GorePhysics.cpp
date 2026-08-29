#include "GoreSystem.h"
#include "core/items/Item.h"
#include "Config.h"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace {
    static sf::Vector2f rotateVector(const sf::Vector2f& v, float degrees) {
        float rad = degrees * 3.14159265f / 180.f;
        float cosA = std::cos(rad);
        float sinA = std::sin(rad);
        return { v.x * cosA - v.y * sinA, v.x * sinA + v.y * cosA };
    }
}

void GoreSystem::update(sf::Time dt) {
    float dtSec = dt.asSeconds();

    // PHASE 1: Physics and Rotation Integration
    for (int i = 0; i < mActiveCount; ++i) {
        Gib& g = mGibs[i];

        // 1.1 Rotation Integration
        float stepRot = 0.f;

        if (g.allowRotation && !g.rotationLocked) {
            float targetAngle = (g.angularVelocity >= 0.f || g.velocity.x >= 0.f) ? 90.f : -90.f;

            float effectiveAngVel = g.angularVelocity;
            if (g.onGround || std::abs(effectiveAngVel) < 180.f) {
                float sign = (targetAngle > 0.f) ? 1.f : -1.f;
                effectiveAngVel = sign * std::max(360.f, std::abs(effectiveAngVel));
            }

            float potentialRot = g.rotation + effectiveAngVel * dtSec;

            bool reachedTarget = false;
            if (effectiveAngVel >= 0.f && potentialRot >= targetAngle) {
                reachedTarget = true;
            } else if (effectiveAngVel < 0.f && potentialRot <= targetAngle) {
                reachedTarget = true;
            }

            if (reachedTarget) {
                stepRot = targetAngle - g.rotation;
                g.rotation = targetAngle;
                g.angularVelocity = 0.f;
                g.rotationLocked = true;
            } else {
                stepRot = effectiveAngVel * dtSec;
                g.rotation += stepRot;
            }
        } else {
            g.angularVelocity = 0.f;
        }

        // 1.2 Apply rotation transform to vertices
        if (std::abs(stepRot) > 0.0001f) {
            sf::Vector2f center = g.getCenter();
            sf::Transform tRot;
            tRot.translate(center);
            tRot.rotate(sf::degrees(stepRot));
            tRot.translate(-center);

            for (int j = 0; j < 6; ++j) {
                g.vertices[j].position = tRot.transformPoint(g.vertices[j].position);
            }
            if (g.armorTexture) {
                for (int j = 0; j < 6; ++j) {
                    g.armorVertices[j].position = tRot.transformPoint(g.armorVertices[j].position);
                }
            }
            if (g.boneTexture) {
                for (int j = 0; j < 6; ++j) {
                    g.boneVertices[j].position = tRot.transformPoint(g.boneVertices[j].position);
                }
            }
        }

        // 1.3 Linear Physics Integration (Airborne only)
        if (!g.onGround) {
            g.velocity.y += cfg::Gore::GRAVITY * dtSec;

            sf::Transform tTrans;
            tTrans.translate(g.velocity * dtSec);

            for (int j = 0; j < 6; ++j) {
                g.vertices[j].position = tTrans.transformPoint(g.vertices[j].position);
            }
            if (g.armorTexture) {
                for (int j = 0; j < 6; ++j) {
                    g.armorVertices[j].position = tTrans.transformPoint(g.armorVertices[j].position);
                }
            }
            if (g.boneTexture) {
                for (int j = 0; j < 6; ++j) {
                    g.boneVertices[j].position = tTrans.transformPoint(g.boneVertices[j].position);
                }
            }
        }
    }

    // PHASE 2: Solve constraints (Ragdoll connections)
    for (int iter = 0; iter < 3; ++iter) {
        for (int i = 0; i < mActiveCount; ++i) {
            Gib& g = mGibs[i];
            if (g.parentIndex != -1) {
                Gib& parent = mGibs[g.parentIndex];
                if (parent.active) {
                    sf::Vector2f centerG = g.getCenter();
                    sf::Vector2f centerP = parent.getCenter();
                    
                    sf::Vector2f rotatedOffset = rotateVector(g.restOffset, parent.rotation);
                    sf::Vector2f anchorPos = centerP + rotatedOffset;
                    
                    sf::Vector2f diff = centerG - anchorPos;
                    float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
                    
                    float restLength = std::sqrt(g.restOffset.x * g.restOffset.x + g.restOffset.y * g.restOffset.y);
                    float maxAllowedStretch = std::max(5.f, restLength * (cfg::Gore::CONSTRAINT_MAX_DIST_FACTOR - 1.0f));
                    
                    if (dist > maxAllowedStretch && dist > 0.001f) {
                        sf::Vector2f dir = diff / dist;
                        sf::Vector2f targetPos = anchorPos + dir * maxAllowedStretch;
                        sf::Vector2f correction = targetPos - centerG;
                        
                        for (int j = 0; j < 6; ++j) {
                            g.vertices[j].position += correction;
                        }
                        if (g.armorTexture) {
                            for (int j = 0; j < 6; ++j) {
                                g.armorVertices[j].position += correction;
                            }
                        }
                        if (g.boneTexture) {
                            for (int j = 0; j < 6; ++j) {
                                g.boneVertices[j].position += correction;
                            }
                        }

                        if (g.onGround) {
                            float lowestY = -999999.f;
                            for (int j = 0; j < 6; ++j) {
                                if (g.vertices[j].position.y > lowestY) lowestY = g.vertices[j].position.y;
                            }
                            if (lowestY < g.groundY - 1.f) {
                                g.onGround = false;
                            }
                        }

                        if (iter == 0) {
                            float dot = g.velocity.x * dir.x + g.velocity.y * dir.y;
                            if (dot > 0.f) {
                                g.velocity -= dir * dot;
                            }
                            g.velocity += (parent.velocity - g.velocity) * cfg::Gore::SPRING_PULL_FACTOR;
                        }
                    }
                }
            }
        }
    }

    // PHASE 3: Handle ground collision & snapping
    for (int i = 0; i < mActiveCount; ++i) {
        Gib& g = mGibs[i];
        float lowestY = -999999.f;
        for (int j = 0; j < 6; ++j) {
            if (g.vertices[j].position.y > lowestY) lowestY = g.vertices[j].position.y;
        }

        if (!g.onGround) {
            if (lowestY > g.groundY) {
                float diff = g.groundY - lowestY;
                for (int j = 0; j < 6; ++j) {
                    g.vertices[j].position.y += diff;
                }
                if (g.armorTexture) {
                    for (int j = 0; j < 6; ++j) {
                        g.armorVertices[j].position.y += diff;
                    }
                }
                if (g.boneTexture) {
                    for (int j = 0; j < 6; ++j) {
                        g.boneVertices[j].position.y += diff;
                    }
                }
                lowestY = g.groundY;
            }

            if (g.velocity.y >= 0.f && lowestY >= g.groundY) {
                if (g.velocity.y > cfg::Gore::MIN_BOUNCE_VELOCITY) {
                    g.velocity.y = -g.velocity.y * g.restitution;
                    g.velocity.x = g.velocity.x * g.friction;
                    g.angularVelocity = g.angularVelocity * g.friction;
                    
                    if (std::abs(g.velocity.y) < cfg::Gore::MIN_BOUNCE_VELOCITY) {
                        g.onGround = true;
                        g.velocity = {0.f, 0.f};
                        g.angularVelocity = 0.f;
                        if (cfg::Gore::ENABLE_BONE_DECAY && g.boneTexture) {
                            float totalTime = cfg::Gore::DECAY_DELAY_SEC + cfg::Gore::DECAY_FADE_DURATION + cfg::Gore::BONE_LIFETIME_SEC + cfg::Gore::BONE_FADE_DURATION;
                            g.lifetime = totalTime;
                            g.maxLifetime = totalTime;
                        }
                        if (g.item && !g.hasSpawnedItem) {
                            g.hasSpawnedItem = true;
                            bool isArmor = (g.armorTexture != nullptr);
                            std::cout << "  [DEBUG_GORE] Gib micro-bounced & landed. Spawning drop for item " 
                                      << g.item->name << " [ID: " << g.item->id << "] (isArmor: " << isArmor << ")" << std::endl;
                            mLandedDrops.push_back({g.item, g.getCenter(), isArmor, g.armorTexture, g.armorVertices});
                            if (isArmor) {
                                g.armorTexture = nullptr;
                            }
                        }
                    }
                } else {
                    g.onGround = true;
                    g.velocity = {0.f, 0.f};
                    g.angularVelocity = 0.f;
                    if (cfg::Gore::ENABLE_BONE_DECAY && g.boneTexture) {
                        float totalTime = cfg::Gore::DECAY_DELAY_SEC + cfg::Gore::DECAY_FADE_DURATION + cfg::Gore::BONE_LIFETIME_SEC + cfg::Gore::BONE_FADE_DURATION;
                        g.lifetime = totalTime;
                        g.maxLifetime = totalTime;
                    }
                    if (g.item && !g.hasSpawnedItem) {
                        g.hasSpawnedItem = true;
                        bool isArmor = (g.armorTexture != nullptr);
                        std::cout << "  [DEBUG_GORE] Gib landed. Spawning drop for item " 
                                  << g.item->name << " [ID: " << g.item->id << "] (isArmor: " << isArmor << ")" << std::endl;
                        mLandedDrops.push_back({g.item, g.getCenter(), isArmor, g.armorTexture, g.armorVertices});
                        if (isArmor) {
                            g.armorTexture = nullptr;
                        }
                    }
                }
            }
        } else {
            float diff = g.groundY - lowestY;
            if (std::abs(diff) > 0.001f) {
                for (int j = 0; j < 6; ++j) {
                    g.vertices[j].position.y += diff;
                }
                if (g.armorTexture) {
                    for (int j = 0; j < 6; ++j) {
                        g.armorVertices[j].position.y += diff;
                    }
                }
                if (g.boneTexture) {
                    for (int j = 0; j < 6; ++j) {
                        g.boneVertices[j].position.y += diff;
                    }
                }
            }
        }
    }

    // PHASE 4: Lifetime, Fade & Swap-Removal
    for (int i = 0; i < mActiveCount; ++i) {
        Gib& g = mGibs[i];
        g.lifetime -= dtSec;

        if (g.onGround) {
            g.decayTimer += dtSec;
            if (cfg::Gore::ENABLE_BONE_DECAY && g.boneTexture) {
                float totalTime = cfg::Gore::DECAY_DELAY_SEC + cfg::Gore::DECAY_FADE_DURATION + cfg::Gore::BONE_LIFETIME_SEC + cfg::Gore::BONE_FADE_DURATION;
                if (g.decayTimer >= totalTime) {
                    g.lifetime = 0.f;
                }
            }
        }

        if (g.lifetime <= 0.f) {
            if (g.item && !g.hasSpawnedItem) {
                g.hasSpawnedItem = true;
                bool isArmor = (g.armorTexture != nullptr);
                std::cout << "  [DEBUG_GORE] Gib expired. Spawning drop for item " 
                          << g.item->name << " [ID: " << g.item->id << "] (isArmor: " << isArmor << ")" << std::endl;
                mLandedDrops.push_back({g.item, g.getCenter(), isArmor, g.armorTexture, g.armorVertices});
                if (isArmor) {
                    g.armorTexture = nullptr;
                }
            }
            int swappedIndex = mActiveCount - 1;
            int targetIndex = i;
            
            if (swappedIndex != targetIndex) {
                mGibs[targetIndex] = mGibs[swappedIndex];
                
                for (int k = 0; k < mActiveCount; ++k) {
                    if (mGibs[k].parentIndex == targetIndex) {
                        mGibs[k].parentIndex = -1;
                    } else if (mGibs[k].parentIndex == swappedIndex) {
                        mGibs[k].parentIndex = targetIndex;
                    }
                }
            } else {
                for (int k = 0; k < mActiveCount; ++k) {
                    if (mGibs[k].parentIndex == targetIndex) {
                        mGibs[k].parentIndex = -1;
                    }
                }
            }
            
            mActiveCount--;
            i--;
            continue;
        }
    }
}
