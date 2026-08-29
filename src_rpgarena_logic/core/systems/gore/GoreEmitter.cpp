#include "GoreSystem.h"
#include "core/engine/animation/Animation.h"
#include "core/items/Item.h"
#include "utils/Random.h"
#include "Config.h"
#include <cmath>
#include <iostream>
#include <algorithm>

void GoreSystem::spawnGib(const sf::Texture& texture, const sf::IntRect& rect, sf::Vector2f pos, float rotationDeg, sf::Vector2f scale, sf::Color color, float floorY, sf::Vector2f sourcePos, float forceMultiplier, sf::Vector2f initialVelocity, float deathSortY) {
    Gib* g = spawnGibSlot();
    if (!g) return;

    g->texture = &texture;

    sf::Transform t;
    t.translate(pos);
    t.rotate(sf::degrees(rotationDeg));
    t.scale(scale);

    float hw = static_cast<float>(rect.size.x) * 0.5f;
    float hh = static_cast<float>(rect.size.y) * 0.5f;

    sf::Vector2f p1 = t.transformPoint({-hw, -hh});
    sf::Vector2f p2 = t.transformPoint({hw, -hh});
    sf::Vector2f p3 = t.transformPoint({hw, hh});
    sf::Vector2f p4 = t.transformPoint({-hw, hh});

    float u1 = static_cast<float>(rect.position.x);
    float v1 = static_cast<float>(rect.position.y);
    float u2 = static_cast<float>(rect.position.x + rect.size.x);
    float v2 = static_cast<float>(rect.position.y + rect.size.y);

    g->vertices[0] = {p1, color, {u1, v1}};
    g->vertices[1] = {p2, color, {u2, v1}};
    g->vertices[2] = {p4, color, {u1, v2}};
    g->vertices[3] = {p2, color, {u2, v1}};
    g->vertices[4] = {p3, color, {u2, v2}};
    g->vertices[5] = {p4, color, {u1, v2}};

    g->groundY = floorY + Random::Float(cfg::Gore::GROUND_SPREAD_MIN, cfg::Gore::GROUND_SPREAD_MAX); 
    g->mobBaseX = pos.x;
    g->facingDir = 1.f;
    g->deathSortY = (deathSortY != 0.f) ? deathSortY : floorY;
    g->onGround = false;

    g->rotation = rotationDeg;
    g->restitution = cfg::Gore::RESTITUTION * Random::Float(0.8f, 1.2f);
    g->friction = cfg::Gore::FRICTION * Random::Float(0.9f, 1.1f);
    g->parentIndex = -1;
    g->restOffset = {0.f, 0.f};

    float heightAboveGround = std::max(0.f, floorY - pos.y);
    float upwardBoost = cfg::Gore::UPWARD_BOOST_BASE - heightAboveGround * cfg::Gore::HEIGHT_MULTIPLIER;
    upwardBoost = std::min(upwardBoost, -15.f);

    float angleRad = Random::Float(0.f, 6.28f);
    if (sourcePos != sf::Vector2f{0.f, 0.f}) {
        sf::Vector2f diff = pos - sourcePos;
        float baseAngle = std::atan2(diff.y, diff.x);
        angleRad = baseAngle + Random::Float(-0.52f, 0.52f);
    }
    float hSpeed = Random::Float(cfg::Gore::H_SPEED_MIN, cfg::Gore::H_SPEED_MAX) * forceMultiplier;
    
    float verticalForceMult = 1.0f + (forceMultiplier - 1.0f) * 0.3f;
    g->velocity = initialVelocity + sf::Vector2f{std::cos(angleRad) * hSpeed, upwardBoost * verticalForceMult};

    sf::Vector2f gibCenter = g->getCenter();
    float dy = std::max(1.f, g->groundY - gibCenter.y);
    float vy = g->velocity.y;
    float gravity = cfg::Gore::GRAVITY;
    float discr = vy * vy + 2.f * gravity * dy;
    float tAir = (discr > 0.f) ? (-vy + std::sqrt(discr)) / gravity : 0.3f;
    tAir = std::max(0.15f, tAir);

    float targetAngle = (g->velocity.x >= 0.f) ? 90.f : -90.f;
    float targetTime = tAir * 0.6f;
    g->angularVelocity = targetAngle / targetTime;
    g->allowRotation = true;
    g->rotationLocked = false;
    g->lifetime = Random::Float(cfg::Gore::LIFETIME_MIN, cfg::Gore::LIFETIME_MAX);
    g->maxLifetime = g->lifetime;
}

void GoreSystem::emitGibs(const sf::Sprite& mobSprite, sf::Vector2f sourcePos, float forceMultiplier, sf::Vector2f initialVelocity, float deathSortY) {
    sf::Vector2f pos = mobSprite.getPosition();
    sf::Vector2f scale = mobSprite.getScale();
    sf::IntRect fullRect = mobSprite.getTextureRect();
    float visualBottomY = pos.y + (fullRect.size.y - mobSprite.getOrigin().y) * std::abs(scale.y);
    emitGibs(mobSprite, visualBottomY, sourcePos, forceMultiplier, initialVelocity, deathSortY);
}

void GoreSystem::emitGibs(const sf::Sprite& partSprite, float floorY, sf::Vector2f sourcePos, float forceMultiplier, sf::Vector2f initialVelocity, float deathSortY) {
    auto& tex = partSprite.getTexture();
    
    sf::IntRect fullRect = partSprite.getTextureRect();
    sf::Vector2f pos = partSprite.getPosition();
    float rotation = partSprite.getRotation().asDegrees(); 
    sf::Vector2f scale = partSprite.getScale();
    sf::Color color = partSprite.getColor();

    int cols = 1;
    int rows = 1;
    
    int w = fullRect.size.x / cols;
    int h = fullRect.size.y / rows;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            sf::IntRect subRect(
                {fullRect.position.x + x * w, fullRect.position.y + y * h},
                {w, h}
            );
            
            float lx = (x * w) + (w * 0.5f);
            float ly = (y * h) + (h * 0.5f);
            
            lx -= partSprite.getOrigin().x;
            ly -= partSprite.getOrigin().y;
            
            sf::Vector2f spawnPos = pos + sf::Vector2f(lx * scale.x, ly * scale.y);

            spawnGib(tex, subRect, spawnPos, rotation, scale, color, floorY, sourcePos, forceMultiplier, initialVelocity, deathSortY);
        }
    }
}

void GoreSystem::emitGibs(const std::vector<sf::Vertex>& vertices, const sf::Texture* texture, float floorY, sf::Vector2f sourcePos, float forceMultiplier, const std::vector<std::string>& nodeNames, sf::Vector2f initialVelocity, const class Animation* anim, const std::vector<std::shared_ptr<class Item>>& armorItems, const std::string& mobType, float deathSortY) {
    if (!texture || vertices.empty() || vertices.size() % 6 != 0) {
        for (const auto& item : armorItems) {
            if (item) {
                mLandedDrops.push_back({item, sourcePos, false, nullptr, {}});
            }
        }
        return;
    }
    
    std::vector<int> spawnedIndices;
    std::vector<std::string> spawnedNames;
    std::vector<bool> armorAttached(armorItems.size(), false);

    for (size_t i = 0; i < vertices.size(); i += 6) {
        std::shared_ptr<Item> armorItem = nullptr;
        EquipmentSlot armorSlot = EquipmentSlot::None;
        std::string name = "";
        if (i / 6 < nodeNames.size()) {
            name = nodeNames[i / 6];
        }

        if (i / 6 < nodeNames.size()) {
            if (name == "head") armorSlot = EquipmentSlot::Head;
            else if (name == "body") armorSlot = EquipmentSlot::Chest;
            else if (name == "hand_l" || name == "hand_r") armorSlot = EquipmentSlot::Hands;
            else if (name == "foot_l" || name == "foot_r") armorSlot = EquipmentSlot::Feet;
            
            if (anim && armorSlot != EquipmentSlot::None) {
                if (static_cast<size_t>(armorSlot) < armorItems.size()) {
                    if (name != "hand_r" && name != "foot_r") {
                        armorItem = armorItems[static_cast<size_t>(armorSlot)];
                    }
                }
            }
        }

        if (vertices[i].color.r == 0 && vertices[i].color.g == 0 && vertices[i].color.b == 0) {
            if (armorItem) {
                sf::Vector2f center(0.f, 0.f);
                for (int j = 0; j < 6; ++j) center += vertices[i + j].position;
                center /= 6.f;
                std::cout << "  [DEBUG_GORE] Shadow quad matched armor item " << armorItem->name 
                          << " [ID: " << armorItem->id << "]. Dropping directly." << std::endl;
                armorAttached[static_cast<size_t>(armorSlot)] = true;
                mLandedDrops.push_back({armorItem, center, false, nullptr, {}});
            }
            continue;
        }
        
        Gib* g = spawnGibSlot();
        if (!g) {
            if (armorItem) {
                sf::Vector2f center(0.f, 0.f);
                for (int j = 0; j < 6; ++j) center += vertices[i + j].position;
                center /= 6.f;
                std::cout << "  [DEBUG_GORE] spawnGibSlot failed for armor item " << armorItem->name 
                          << " [ID: " << armorItem->id << "]. Dropping directly." << std::endl;
                armorAttached[static_cast<size_t>(armorSlot)] = true;
                mLandedDrops.push_back({armorItem, center, false, nullptr, {}});
            }
            continue;
        }
        
        g->texture = texture;
        g->armorTexture = nullptr;
        g->item = armorItem;
        if (armorItem) {
            std::cout << "  [DEBUG_GORE] Attaching armor item " << armorItem->name 
                      << " [ID: " << armorItem->id << "] to gib for node " << name << std::endl;
            armorAttached[static_cast<size_t>(armorSlot)] = true;
        }
        g->hasSpawnedItem = false;
        g->parentIndex = -1;
        g->restOffset = {0.f, 0.f};
        g->decayTimer = 0.f;
        for (int j = 0; j < 6; ++j) {
            g->vertices[j] = vertices[i + j];
        }

        g->boneTexture = getBoneTexture(mobType, name);
        if (g->boneTexture) {
            g->boneVertices = g->vertices;
        }

        if (anim && armorSlot != EquipmentSlot::None) {
            if (cfg::Gore::LOOT_ARMOR_ATTACHED) {
                const auto& armorVis = anim->getArmorVisual(armorSlot);
                if (armorVis.exists && armorVis.texture) {
                    g->armorTexture = armorVis.texture;
                    
                    auto it = anim->getNodeMap().find(name);
                    if (it != anim->getNodeMap().end()) {
                        const auto& node = anim->getNodes()[it->second];
                        
                        sf::Sprite armorSprite(*armorVis.texture);
                        armorSprite.setTextureRect(armorVis.textureRect);
                        
                        sf::FloatRect localB = armorSprite.getLocalBounds();
                        armorSprite.setOrigin({ localB.size.x * 0.5f, localB.size.y * 0.5f });
                        
                        sf::Vector2f nodeCenter = {
                            node.localBounds.position.x + node.localBounds.size.x * 0.5f,
                            node.localBounds.position.y + node.localBounds.size.y * 0.5f
                        };
                        
                        sf::Vector2f worldCenter = node.currentTransform.transformPoint(nodeCenter);
                        armorSprite.setPosition(worldCenter);
                        
                        float finalRot = node.currentRot * anim->getLastFacingDir();
                        armorSprite.setRotation(sf::degrees(finalRot));
                        
                        sf::Vector2f nodeScale = node.currentScale;
                        sf::Vector2f finalScale = {
                            nodeScale.x * anim->getBaseScale().x * -anim->getLastFacingDir() * armorVis.scale,
                            nodeScale.y * anim->getBaseScale().y * armorVis.scale
                        };
                        armorSprite.setScale(finalScale);
                        
                        float angleRad = finalRot * 3.14159265f / 180.f;
                        float cosA = std::cos(angleRad);
                        float sinA = std::sin(angleRad);
                        sf::Vector2f localOffset = armorVis.offset;
                        
                        localOffset.x *= -anim->getLastFacingDir();
                        
                        sf::Vector2f rotatedOffset = {
                            localOffset.x * cosA - localOffset.y * sinA,
                            localOffset.x * sinA + localOffset.y * cosA
                        };
                        armorSprite.move(rotatedOffset);
                        
                        sf::Transform spriteT = armorSprite.getTransform();
                        sf::Vector2f p1 = spriteT.transformPoint({0.f, 0.f});
                        sf::Vector2f p2 = spriteT.transformPoint({localB.size.x, 0.f});
                        sf::Vector2f p3 = spriteT.transformPoint({localB.size.x, localB.size.y});
                        sf::Vector2f p4 = spriteT.transformPoint({0.f, localB.size.y});
                        
                        g->armorVertices[0].position = p1;
                        g->armorVertices[1].position = p2;
                        g->armorVertices[2].position = p4;
                        g->armorVertices[3].position = p2;
                        g->armorVertices[4].position = p3;
                        g->armorVertices[5].position = p4;
                        
                        float tx = armorVis.textureRect.position.x;
                        float ty = armorVis.textureRect.position.y;
                        float tw = armorVis.textureRect.size.x;
                        float th = armorVis.textureRect.size.y;
                        
                        g->armorVertices[0].texCoords = {tx, ty};
                        g->armorVertices[1].texCoords = {tx + tw, ty};
                        g->armorVertices[2].texCoords = {tx, ty + th};
                        g->armorVertices[3].texCoords = {tx + tw, ty};
                        g->armorVertices[4].texCoords = {tx + tw, ty + th};
                        g->armorVertices[5].texCoords = {tx, ty + th};
                        
                        for (int j = 0; j < 6; ++j) {
                            g->armorVertices[j].color = sf::Color::White;
                        }
                    }
                }
            }
        }
        
        sf::Vector2f center(0.f, 0.f);
        for (int j = 0; j < 6; ++j) center += vertices[i + j].position;
        center /= 6.f;
        
        g->groundY = floorY + Random::Float(cfg::Gore::GROUND_SPREAD_MIN, cfg::Gore::GROUND_SPREAD_MAX);
        g->mobBaseX = sourcePos.x;
        g->facingDir = anim ? anim->getLastFacingDir() : 1.f;
        g->deathSortY = (deathSortY != 0.f) ? deathSortY : floorY;
        g->onGround = false;
        
        g->rotation = 0.f;
        g->restitution = cfg::Gore::RESTITUTION * Random::Float(0.8f, 1.2f);
        g->friction = cfg::Gore::FRICTION * Random::Float(0.9f, 1.1f);
        
        float heightAboveGround = std::max(0.f, floorY - center.y);
        float upwardBoost = cfg::Gore::UPWARD_BOOST_BASE - heightAboveGround * cfg::Gore::HEIGHT_MULTIPLIER;
        upwardBoost = std::min(upwardBoost, -15.f);
        
        float angleRad = Random::Float(0.f, 6.28f);
        if (sourcePos != sf::Vector2f{0.f, 0.f}) {
            sf::Vector2f diff = center - sourcePos;
            float baseAngle = std::atan2(diff.y, diff.x);
            angleRad = baseAngle + Random::Float(-0.52f, 0.52f);
        }
        float hSpeed = Random::Float(cfg::Gore::H_SPEED_MIN, cfg::Gore::H_SPEED_MAX) * forceMultiplier;
        
        float verticalForceMult = 1.0f + (forceMultiplier - 1.0f) * 0.3f;
        g->velocity = initialVelocity + sf::Vector2f{std::cos(angleRad) * hSpeed, upwardBoost * verticalForceMult};

        if (name.empty() && i / 6 < nodeNames.size()) {
            name = nodeNames[i / 6];
        }

        bool isLimb = (name == "hand_l" || name == "hand_r" || name == "foot_l" || name == "foot_r");
        if (name == "body") g->layerPriority = 0;
        else if (name == "head") g->layerPriority = 1;
        else if (name == "foot_l" || name == "foot_r") g->layerPriority = 2;
        else if (name == "hand_l" || name == "hand_r") g->layerPriority = 3;
        else g->layerPriority = 1;

        if (isLimb) {
            g->allowRotation = false;
            g->rotationLocked = true;
            g->angularVelocity = 0.f;
        } else {
            g->allowRotation = true;
            g->rotationLocked = false;

            float dy = std::max(1.f, g->groundY - center.y);
            float vy = g->velocity.y;
            float gravity = cfg::Gore::GRAVITY;
            float discr = vy * vy + 2.f * gravity * dy;
            float tAir = (discr > 0.f) ? (-vy + std::sqrt(discr)) / gravity : 0.3f;
            tAir = std::max(0.15f, tAir);

            float targetAngle = (g->velocity.x >= 0.f) ? 90.f : -90.f;
            float targetTime = tAir * 0.6f;
            g->angularVelocity = targetAngle / targetTime;
        }

        g->lifetime = Random::Float(cfg::Gore::LIFETIME_MIN, cfg::Gore::LIFETIME_MAX);
        g->maxLifetime = g->lifetime;

        int currentGibIndex = mActiveCount - 1;
        spawnedIndices.push_back(currentGibIndex);
        spawnedNames.push_back(name);
    }

    if (spawnedIndices.size() > 1) {
        int torsoIndex = -1;

        for (size_t k = 0; k < spawnedIndices.size(); ++k) {
            if (spawnedNames[k] == "body") {
                torsoIndex = spawnedIndices[k];
                break;
            }
        }

        if (torsoIndex == -1) {
            float maxArea = 0.f;
            for (int idx : spawnedIndices) {
                const Gib& g = mGibs[idx];
                float minX = 999999.f, maxX = -999999.f;
                float minY = 999999.f, maxY = -999999.f;
                for (int j = 0; j < 6; ++j) {
                    sf::Vector2f p = g.vertices[j].position;
                    if (p.x < minX) minX = p.x;
                    if (p.x > maxX) maxX = p.x;
                    if (p.y < minY) minY = p.y;
                    if (p.y > maxY) maxY = p.y;
                }
                float area = (maxX - minX) * (maxY - minY);
                if (area > maxArea) {
                    maxArea = area;
                    torsoIndex = idx;
                }
            }
        }

        if (torsoIndex != -1) {
            sf::Vector2f torsoCenter = mGibs[torsoIndex].getCenter();
            for (int idx : spawnedIndices) {
                if (idx != torsoIndex) {
                    Gib& limb = mGibs[idx];
                    limb.parentIndex = torsoIndex;
                    sf::Vector2f limbCenter = limb.getCenter();
                    limb.restOffset = limbCenter - torsoCenter;
                    
                    if (!limb.allowRotation) {
                        limb.angularVelocity = 0.f;
                    }
                }
            }
        }
    }

    for (size_t idx = 0; idx < armorItems.size(); ++idx) {
        if (armorItems[idx] && !armorAttached[idx]) {
            std::cout << "  [DEBUG_GORE] WARNING: Armor item " << armorItems[idx]->name 
                      << " [ID: " << armorItems[idx]->id << "] in slot " << idx 
                      << " was NOT attached to any gib. Dropping directly." << std::endl;
            mLandedDrops.push_back({armorItems[idx], sourcePos, false, nullptr, {}});
        }
    }
}
