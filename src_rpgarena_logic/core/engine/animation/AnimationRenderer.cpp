#include "Animation.h"
#include "Config.h"
#include "core/items/Item.h"
#include "core/items/WeaponSprite.h"
#include <algorithm>
#include <cmath>

void Animation::drawSingleNodeAndArmor(const SkeletonNode& node, sf::RenderTarget& target, sf::RenderStates states) const {
    sf::RenderStates nodeStates = states;
    nodeStates.texture = mAtlasTexture;
    
    bool isFoot = (node.name == "foot_l" || node.name == "foot_r");
    
    if (states.shader && mAtlasTexture) {
        sf::Vector2u atlasSize = mAtlasTexture->getSize();
        float minU = 999999.f, minV = 999999.f, maxU = -999999.f, maxV = -999999.f;
        for (int i = 0; i < 6; ++i) {
            float u = node.quad[i].texCoords.x / static_cast<float>(atlasSize.x);
            float v = node.quad[i].texCoords.y / static_cast<float>(atlasSize.y);
            minU = std::min(minU, u);
            minV = std::min(minV, v);
            maxU = std::max(maxU, u);
            maxV = std::max(maxV, v);
        }
        float uvW = std::max(0.0001f, maxU - minU);
        float uvH = std::max(0.0001f, maxV - minV);
        float pixW = std::max(1.0f, std::round(uvW * static_cast<float>(atlasSize.x)));
        float pixH = std::max(1.0f, std::round(uvH * static_cast<float>(atlasSize.y)));

        auto* shader = const_cast<sf::Shader*>(states.shader);
        shader->setUniform("u_SpriteUVBounds", sf::Glsl::Vec4(minU, minV, uvW, uvH));
        shader->setUniform("u_SpritePixelSize", sf::Glsl::Vec2(pixW, pixH));
        shader->setUniform("u_SpriteTexelSize", sf::Glsl::Vec2(1.f / static_cast<float>(atlasSize.x), 1.f / static_cast<float>(atlasSize.y)));

        float footShift = 0.f;
        if (isFoot) {
            if (mLastFacingDir == -1) {
                footShift = (node.name == "foot_l") ? mIK.getFootRDepth() : mIK.getFootLDepth();
            } else {
                footShift = (node.name == "foot_l") ? mIK.getFootLDepth() : mIK.getFootRDepth();
            }
        }
        shader->setUniform("u_FootShift", footShift);
        nodeStates.shader = shader;
    }

    target.draw(node.quad.data(), 6, sf::PrimitiveType::Triangles, nodeStates);

    // --- DIBUJAR ARMADURA SOBRE EL NODO ---
    EquipmentSlot armorSlot = EquipmentSlot::None;
    if (node.name == "head") {
        armorSlot = EquipmentSlot::Head;
    } else if (node.name == "body") {
        armorSlot = EquipmentSlot::Chest;
    } else if (node.name == "hand_l" || node.name == "hand_r") {
        armorSlot = EquipmentSlot::Hands;
    } else if (node.name == "foot_l" || node.name == "foot_r") {
        armorSlot = EquipmentSlot::Feet;
    }

    if (armorSlot != EquipmentSlot::None) {
        int slotIdx = static_cast<int>(armorSlot);
        const auto& armorVis = mArmorVisuals[slotIdx];
        if (armorVis.exists && armorVis.texture) {
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
            
            float finalRot = node.currentRot * mLastFacingDir;
            armorSprite.setRotation(sf::degrees(finalRot));
            
            sf::Vector2f nodeScale = node.currentScale;
            sf::Vector2f finalScale = {
                nodeScale.x * mBaseScale.x * -mLastFacingDir * armorVis.scale,
                nodeScale.y * mBaseScale.y * armorVis.scale
            };
            armorSprite.setScale(finalScale);
            
            float angleRad = finalRot * 3.14159265f / 180.f;
            float cosA = std::cos(angleRad);
            float sinA = std::sin(angleRad);
            sf::Vector2f localOffset = armorVis.offset;
            
            localOffset.x *= -mLastFacingDir;
            
            sf::Vector2f rotatedOffset = {
                localOffset.x * cosA - localOffset.y * sinA,
                localOffset.x * sinA + localOffset.y * cosA
            };
            armorSprite.move(rotatedOffset);
            
            sf::RenderStates armorStates = states;
            armorStates.texture = armorVis.texture;

            if (states.shader) {
                sf::Vector2u texSize = armorVis.texture->getSize();
                float minU = static_cast<float>(armorVis.textureRect.position.x) / static_cast<float>(texSize.x);
                float minV = static_cast<float>(armorVis.textureRect.position.y) / static_cast<float>(texSize.y);
                float uvW = static_cast<float>(armorVis.textureRect.size.x) / static_cast<float>(texSize.x);
                float uvH = static_cast<float>(armorVis.textureRect.size.y) / static_cast<float>(texSize.y);
                float pixW = static_cast<float>(armorVis.textureRect.size.x);
                float pixH = static_cast<float>(armorVis.textureRect.size.y);

                auto* shader = const_cast<sf::Shader*>(states.shader);
                shader->setUniform("u_SpriteUVBounds", sf::Glsl::Vec4(minU, minV, uvW, uvH));
                shader->setUniform("u_SpritePixelSize", sf::Glsl::Vec2(pixW, pixH));
                shader->setUniform("u_SpriteTexelSize", sf::Glsl::Vec2(1.f / static_cast<float>(texSize.x), 1.f / static_cast<float>(texSize.y)));
                armorStates.shader = shader;
            }

            // Aura para armaduras fortificadas >= 6
            if (armorVis.fortificationLevel >= 6) {
                sf::RenderStates glowStates = states;
                glowStates.texture = armorVis.texture;
                glowStates.shader = nullptr;
                ItemAuraRenderer::drawAura(target, armorSprite, armorVis.fortificationLevel, 1.0f, glowStates);
            }

            target.draw(armorSprite, armorStates);
        }
    }
}

void Animation::draw(sf::RenderTarget& target, sf::RenderStates states) {
    if (!mIsLoaded || !mAtlasTexture) return;

    // Unified 2.5D layered drawing (Layer 1: Back, Layer 2: Torso/Head, Layer 3: Front)
    drawLayer(target, 1, states);
    drawLayer(target, 2, states);
    drawLayer(target, 3, states);
}

void Animation::drawLayer(sf::RenderTarget& target, int layer, sf::RenderStates states) {
    if (!mIsLoaded || !mAtlasTexture) return;

    bool is2HFacingLeft = mWeaponIsTwoHanded && (mLastFacingDir == -1);

    std::vector<const SkeletonNode*> sortedNodes;
    sortedNodes.reserve(mNodes.size());
    for (const auto& node : mNodes) sortedNodes.push_back(&node);

    bool hasCustomLayerOrder = (mCurrentClip && !mCurrentClip->layerOrder.empty());
    if (hasCustomLayerOrder) {
        auto getOrderIdx = [&](const std::string& name) -> int {
            auto it = std::find(mCurrentClip->layerOrder.begin(), mCurrentClip->layerOrder.end(), name);
            if (it != mCurrentClip->layerOrder.end()) {
                return static_cast<int>(std::distance(mCurrentClip->layerOrder.begin(), it));
            }
            return 999;
        };
        std::sort(sortedNodes.begin(), sortedNodes.end(), [&](const SkeletonNode* a, const SkeletonNode* b) {
            return getOrderIdx(a->name) < getOrderIdx(b->name);
        });
    }

    for (const auto* pNode : sortedNodes) {
        const auto& node = *pNode;

        // Determinamos a qué capa pertenece este nodo
        int nodeLayer = 2; // Por defecto capa media (cuerpo, cabeza)
        
        bool isDepthBone = (node.name == "hand_r" || node.name == "hand_l" ||
                            node.name == "foot_r" || node.name == "foot_l");

        if (isDepthBone) {
            if (mLastFacingDir == 1) {
                if (node.name == "hand_r" || node.name == "foot_r") {
                    nodeLayer = 3; // Partes derechas al frente (Capa 3)
                } else {
                    nodeLayer = 1; // Partes izquierdas al fondo (Capa 1)
                }
            } else {
                if (node.name == "hand_l" || node.name == "foot_l") {
                    nodeLayer = 3; // Partes izquierdas al frente (Capa 3)
                } else {
                    nodeLayer = 1; // Partes derechas al fondo (Capa 1)
                }
            }
        } else {
            nodeLayer = 2; // Cuerpo y cabeza siempre en capa media
        }

        // Si hay guardia de escudo activa, la mano del escudo siempre va al frente (capa 3)
        if (mShieldGuardClip != nullptr && node.name == mShieldedBoneName) {
            nodeLayer = 3;
        }

        // Dibujar solo si coincide con la capa solicitada
        if (nodeLayer == layer) {
            if (is2HFacingLeft && node.name == "hand_r") {
                // Orden: mano trasera -> arma de dos manos (capa media/delantera) -> mano delantera (capa delantera)
                drawSingleNodeAndArmor(node, target, states);
                continue;
            }

            auto drawWeaponWithShader = [&](const std::unique_ptr<WeaponSprite>& wp) {
                if (!wp) return;
                sf::RenderStates weaponStates = states;
                weaponStates.texture = nullptr;
                if (states.shader && wp->m_baseSprite && wp->m_baseTexture) {
                    sf::Vector2u wTexSize = wp->m_baseTexture->getSize();
                    sf::IntRect wRect = wp->m_baseSprite->getTextureRect();
                    float minU = static_cast<float>(wRect.position.x) / static_cast<float>(wTexSize.x);
                    float minV = static_cast<float>(wRect.position.y) / static_cast<float>(wTexSize.y);
                    float uvW = static_cast<float>(wRect.size.x) / static_cast<float>(wTexSize.x);
                    float uvH = static_cast<float>(wRect.size.y) / static_cast<float>(wTexSize.y);
                    float pixW = static_cast<float>(wRect.size.x);
                    float pixH = static_cast<float>(wRect.size.y);

                    auto* shader = const_cast<sf::Shader*>(states.shader);
                    shader->setUniform("u_SpriteUVBounds", sf::Glsl::Vec4(minU, minV, uvW, uvH));
                    shader->setUniform("u_SpritePixelSize", sf::Glsl::Vec2(pixW, pixH));
                    shader->setUniform("u_SpriteTexelSize", sf::Glsl::Vec2(1.f / static_cast<float>(wTexSize.x), 1.f / static_cast<float>(wTexSize.y)));
                    weaponStates.shader = shader;
                }
                target.draw(*wp, weaponStates);
            };

            if (is2HFacingLeft && node.name == "hand_l") {
                // En capa 3, dibujamos el arma primero y luego la mano delantera
                if (mWeapon) {
                    drawWeaponWithShader(mWeapon);
                }
                drawSingleNodeAndArmor(node, target, states);
                continue;
            }

            // Determinamos si esta mano es la delantera (primer plano en pantalla)
            bool isFrontHand = (mLastFacingDir == 1 && node.name == "hand_r") ||
                               (mLastFacingDir == -1 && node.name == "hand_l");
            bool isGuardOnThisHand = (mShieldGuardClip != nullptr && node.name == mShieldedBoneName);

            // Regla de dibujado respecto a la mano:
            // - Armas normales: siempre detrás de la mano (la mano agarra el mango).
            // - Escudo delantero o en guardia activa: SOBRE la mano (tapa la mano desde el frente).
            // - Escudo trasero (sin guardia): DETRÁS de la mano (mano primero, escudo detrás).
            bool drawOverHand = false;
            if (node.name == "hand_r" && mWeapon && mWeaponIsShield) {
                drawOverHand = isFrontHand || isGuardOnThisHand;
            } else if (node.name == "hand_l" && mWeaponSecondary && mSecondaryIsShield) {
                drawOverHand = isFrontHand || isGuardOnThisHand;
            }

            // Dibujado detrás de la mano (armas normales y escudo trasero)
            if (!is2HFacingLeft && !drawOverHand) {
                if (node.name == "hand_l" && mWeaponSecondary) {
                    drawWeaponWithShader(mWeaponSecondary);
                }
                if (node.name == "hand_r" && mWeapon) {
                    drawWeaponWithShader(mWeapon);
                }
            }

            drawSingleNodeAndArmor(node, target, states);

            // Dibujado sobre la mano (escudo delantero y escudo en guardia)
            if (!is2HFacingLeft && drawOverHand) {
                if (node.name == "hand_l" && mWeaponSecondary) {
                    drawWeaponWithShader(mWeaponSecondary);
                }
                if (node.name == "hand_r" && mWeapon) {
                    drawWeaponWithShader(mWeapon);
                }
            }
        }
    }
}

void Animation::getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const {
    if (!mIsLoaded || !mAtlasTexture) {
        texture = nullptr;
        return;
    }
    texture = mAtlasTexture;

    // Cuerpo real
    for (const auto& node : mNodes) {
        vertices.insert(vertices.end(), node.quad.begin(), node.quad.end());
    }
}

void Animation::getShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const {
    if (!mIsLoaded || !mAtlasTexture) {
        texture = nullptr;
        return;
    }
    texture = mAtlasTexture;

    float shadowScaleY = cfg::Shadow::SCALE_Y;
    float shadowScaleX = cfg::Shadow::SCALE_X;
    float shadowSkewX = cfg::Shadow::SKEW_X;
    float shOffsetX = cfg::Shadow::OFFSET_X * mLastFacingDir;
    float shOffsetY = cfg::Shadow::OFFSET_Y;

    for (const auto& node : mNodes) {
        float nodeGroundY = mLastBasePos.y;
        if (node.name == "foot_l") {
            float restY = (mLastFacingDir == -1 && mNodeMap.count("foot_r")) ? mNodes[mNodeMap.at("foot_r")].customRestPos.y : node.customRestPos.y;
            float depth = (mLastFacingDir == -1) ? mIK.getFootRDepth() : mIK.getFootLDepth();
            nodeGroundY += restY + depth;
        } else if (node.name == "foot_r") {
            float restY = (mLastFacingDir == -1 && mNodeMap.count("foot_l")) ? mNodes[mNodeMap.at("foot_l")].customRestPos.y : node.customRestPos.y;
            float depth = (mLastFacingDir == -1) ? mIK.getFootLDepth() : mIK.getFootRDepth();
            nodeGroundY += restY + depth;
        } else {
            nodeGroundY += (mGroundOffsetY != 0.f ? mGroundOffsetY : 30.f) + mIK.getBodyDepth();
        }

        for (int i = 0; i < 6; ++i) {
            sf::Vertex v = node.quad[i];
            
            float relX = v.position.x - mLastBasePos.x;
            float height = nodeGroundY - v.position.y;
            
            v.position.x = mLastBasePos.x + relX * shadowScaleX + shOffsetX + height * shadowSkewX;
            v.position.y = nodeGroundY + (v.position.y - nodeGroundY) * shadowScaleY + shOffsetY;
            
            // Conservar el alpha original de los vertices del nodo (para fade out)
            v.color = sf::Color(46, 34, 47, v.color.a);
            
            vertices.push_back(v);
        }
    }
}

void Animation::getWeaponShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture, int slotIndex) const {
    const auto& weapon = (slotIndex == 0) ? mWeapon : mWeaponSecondary;
    if (!weapon || !weapon->m_baseSprite || !weapon->m_baseTexture) {
        texture = nullptr;
        return;
    }
    texture = weapon->m_baseTexture;

    float shadowScaleY = cfg::Shadow::SCALE_Y;
    float shadowScaleX = cfg::Shadow::SCALE_X;
    float shadowSkewX = cfg::Shadow::SKEW_X;
    float shOffsetX = cfg::Shadow::OFFSET_X * mLastFacingDir;
    float shOffsetY = cfg::Shadow::OFFSET_Y;

    float nodeGroundY = mLastBasePos.y + (mGroundOffsetY != 0.f ? mGroundOffsetY : 30.f) + mIK.getBodyDepth();

    const sf::IntRect& baseRect = weapon->m_baseSprite->getTextureRect();
    sf::Vector2f localSize = sf::Vector2f(baseRect.size);
    sf::Transform t = weapon->getTransform();

    sf::Vector2f p1 = t.transformPoint({0.f, 0.f});
    sf::Vector2f p2 = t.transformPoint({localSize.x, 0.f});
    sf::Vector2f p3 = t.transformPoint({localSize.x, localSize.y});
    sf::Vector2f p4 = t.transformPoint({0.f, localSize.y});

    auto projectPoint = [&](sf::Vector2f p) -> sf::Vector2f {
        float relX = p.x - mLastBasePos.x;
        float height = nodeGroundY - p.y;
        float projX = mLastBasePos.x + relX * shadowScaleX + shOffsetX + height * shadowSkewX;
        float projY = nodeGroundY + (p.y - nodeGroundY) * shadowScaleY + shOffsetY;
        return {projX, projY};
    };

    sf::Vector2f proj1 = projectPoint(p1);
    sf::Vector2f proj2 = projectPoint(p2);
    sf::Vector2f proj3 = projectPoint(p3);
    sf::Vector2f proj4 = projectPoint(p4);

    float u1 = (float)baseRect.position.x;
    float v1 = (float)baseRect.position.y;
    float u2 = (float)(baseRect.position.x + baseRect.size.x);
    float v2 = (float)(baseRect.position.y + baseRect.size.y);

    sf::Color shColor = sf::Color(46, 34, 47, 255);
    if (!mNodes.empty()) {
        shColor.a = mNodes[0].quad[0].color.a;
    }

    // T1: P1, P2, P4
    vertices.push_back(sf::Vertex{proj1, shColor, {u1, v1}});
    vertices.push_back(sf::Vertex{proj2, shColor, {u2, v1}});
    vertices.push_back(sf::Vertex{proj4, shColor, {u1, v2}});

    // T2: P2, P3, P4
    vertices.push_back(sf::Vertex{proj2, shColor, {u2, v1}});
    vertices.push_back(sf::Vertex{proj3, shColor, {u2, v2}});
    vertices.push_back(sf::Vertex{proj4, shColor, {u1, v2}});
}

void Animation::getArmorShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture, int slotIndex) const {
    if (!mIsLoaded) {
        texture = nullptr;
        return;
    }

    if (slotIndex < 0 || slotIndex >= 12) {
        texture = nullptr;
        return;
    }

    const auto& armorVis = mArmorVisuals[slotIndex];
    if (!armorVis.exists || !armorVis.texture) {
        texture = nullptr;
        return;
    }

    texture = armorVis.texture;

    float shadowScaleY = cfg::Shadow::SCALE_Y;
    float shadowScaleX = cfg::Shadow::SCALE_X;
    float shadowSkewX = cfg::Shadow::SKEW_X;
    float shOffsetX = cfg::Shadow::OFFSET_X * mLastFacingDir;
    float shOffsetY = cfg::Shadow::OFFSET_Y;

    sf::Color shColor = sf::Color(46, 34, 47, 255);
    if (!mNodes.empty()) {
        shColor.a = mNodes[0].quad[0].color.a;
    }

    for (const auto& node : mNodes) {
        bool matches = false;
        if (slotIndex == 0 && node.name == "head") matches = true;
        else if (slotIndex == 2 && node.name == "body") matches = true;
        else if (slotIndex == 3 && (node.name == "hand_l" || node.name == "hand_r")) matches = true;
        else if (slotIndex == 8 && (node.name == "foot_l" || node.name == "foot_r")) matches = true;

        if (!matches) continue;

        float nodeGroundY = mLastBasePos.y;
        if (node.name == "foot_l") {
            float restY = (mLastFacingDir == -1 && mNodeMap.count("foot_r")) ? mNodes[mNodeMap.at("foot_r")].customRestPos.y : node.customRestPos.y;
            float depth = (mLastFacingDir == -1) ? mIK.getFootRDepth() : mIK.getFootLDepth();
            nodeGroundY += restY + depth;
        } else if (node.name == "foot_r") {
            float restY = (mLastFacingDir == -1 && mNodeMap.count("foot_l")) ? mNodes[mNodeMap.at("foot_l")].customRestPos.y : node.customRestPos.y;
            float depth = (mLastFacingDir == -1) ? mIK.getFootLDepth() : mIK.getFootRDepth();
            nodeGroundY += restY + depth;
        } else {
            nodeGroundY += (mGroundOffsetY != 0.f ? mGroundOffsetY : 30.f) + mIK.getBodyDepth();
        }

        sf::Vector2i rectSize = armorVis.textureRect.size;
        if (rectSize.x == 0 && rectSize.y == 0 && armorVis.texture) {
            rectSize = sf::Vector2i(armorVis.texture->getSize());
        }

        sf::FloatRect localB = sf::FloatRect({0.f, 0.f}, sf::Vector2f(rectSize));
        sf::Vector2f origin = { localB.size.x * 0.5f, localB.size.y * 0.5f };

        sf::Vector2f nodeCenter = {
            node.localBounds.position.x + node.localBounds.size.x * 0.5f,
            node.localBounds.position.y + node.localBounds.size.y * 0.5f
        };

        sf::Vector2f worldCenter = node.currentTransform.transformPoint(nodeCenter);

        float finalRot = node.currentRot * mLastFacingDir;
        sf::Vector2f nodeScale = node.currentScale;
        sf::Vector2f finalScale = {
            nodeScale.x * mBaseScale.x * -mLastFacingDir * armorVis.scale,
            nodeScale.y * mBaseScale.y * armorVis.scale
        };

        float angleRad = finalRot * 3.14159265f / 180.f;
        float cosA = std::cos(angleRad);
        float sinA = std::sin(angleRad);
        sf::Vector2f localOffset = armorVis.offset;
        localOffset.x *= -mLastFacingDir;

        sf::Vector2f rotatedOffset = {
            localOffset.x * cosA - localOffset.y * sinA,
            localOffset.x * sinA + localOffset.y * cosA
        };
        sf::Vector2f finalPos = worldCenter + rotatedOffset;

        sf::Transform t;
        t.translate(finalPos);
        t.rotate(sf::degrees(finalRot));
        t.scale(finalScale);
        t.translate(-origin);

        sf::Vector2f localSize = sf::Vector2f(rectSize);

        sf::Vector2f p1 = t.transformPoint({0.f, 0.f});
        sf::Vector2f p2 = t.transformPoint({localSize.x, 0.f});
        sf::Vector2f p3 = t.transformPoint({localSize.x, localSize.y});
        sf::Vector2f p4 = t.transformPoint({0.f, localSize.y});

        auto projectPoint = [&](sf::Vector2f p) -> sf::Vector2f {
            float relX = p.x - mLastBasePos.x;
            float height = nodeGroundY - p.y;
            float projX = mLastBasePos.x + relX * shadowScaleX + shOffsetX + height * shadowSkewX;
            float projY = nodeGroundY + (p.y - nodeGroundY) * shadowScaleY + shOffsetY;
            return {projX, projY};
        };

        sf::Vector2f proj1 = projectPoint(p1);
        sf::Vector2f proj2 = projectPoint(p2);
        sf::Vector2f proj3 = projectPoint(p3);
        sf::Vector2f proj4 = projectPoint(p4);

        float u1 = (float)armorVis.textureRect.position.x;
        float v1 = (float)armorVis.textureRect.position.y;
        float u2 = (float)(armorVis.textureRect.position.x + rectSize.x);
        float v2 = (float)(armorVis.textureRect.position.y + rectSize.y);

        // T1: P1, P2, P4
        vertices.push_back(sf::Vertex{proj1, shColor, {u1, v1}});
        vertices.push_back(sf::Vertex{proj2, shColor, {u2, v1}});
        vertices.push_back(sf::Vertex{proj4, shColor, {u1, v2}});

        // T2: P2, P3, P4
        vertices.push_back(sf::Vertex{proj2, shColor, {u2, v1}});
        vertices.push_back(sf::Vertex{proj3, shColor, {u2, v2}});
        vertices.push_back(sf::Vertex{proj4, shColor, {u1, v2}});
    }
}

float Animation::getLayerSortingY(int layer, float entityBaseY) const {
    if (layer == 1) return entityBaseY - 10.f;
    if (layer == 3) return entityBaseY + 10.f;
    return entityBaseY;
}

void Animation::drawNode(const std::string& name, sf::RenderTarget& target, sf::RenderStates states) const {
    if (!mIsLoaded || !mAtlasTexture) return;
    auto it = mNodeMap.find(name);
    if (it != mNodeMap.end()) {
        states.texture = mAtlasTexture;
        target.draw(mNodes[it->second].quad.data(), 6, sf::PrimitiveType::Triangles, states);
    }
}
