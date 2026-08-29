#include "Entity.h"
#include "core/engine/animation/Animation.h"
#include <array>
#include "Config.h" // [HD MODE] For virtual resolution check

void Entity::triggerHitEffect(sf::Vector2f sourcePos) {
    mFlashTimer = HIT_FLASH_DURATION;
    mShakeTimer = HIT_SHAKE_DURATION;
    
    // Calculate knockback direction (away from source)
    sf::Vector2f diff = mPos - sourcePos;
    float lenSq = diff.x*diff.x + diff.y*diff.y;
    float kbDist = cfg::Mob::HIT_KNOCKBACK_DIST;

    if (lenSq > 0.001f) {
        float len = std::sqrt(lenSq);
        mShakeDir = (diff / len) * kbDist;
    } else {
        mShakeDir = {kbDist, 0.f}; // Default if on top
    }

    Animation* anim = getAnimation();
    if (anim) {
        anim->applyHitRecoil(mShakeDir, 1.0f);
        anim->triggerHitStop(0.04f);
    }
}

void Entity::getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const {
    if (!mSprite) {
        texture = nullptr;
        return;
    }

    // SFML 3: getTexture returns reference.
    texture = &mSprite->getTexture();

    // SFML 3: Rect and Transform changes
    sf::Transform trans = mSprite->getTransform();
    
    // [SUB-PIXEL SNAP] Only snap to pixel grid when using virtual resolution (pixel-art mode).
    // At native HD, the GPU's bilinear filtering handles sub-pixel positioning smoothly.
    // Rounding at HD causes visible vibration since 1 world unit = multiple screen pixels.
    bool usePixelSnap = (cfg::Window::INTERNAL_WIDTH > 0 && cfg::Window::INTERNAL_HEIGHT > 0);
    sf::Transform snappedTrans = trans;
    if (usePixelSnap) {
        sf::Vector2f pos = mSprite->getPosition();
        sf::Vector2f diff(std::round(pos.x) - pos.x, std::round(pos.y) - pos.y);
        snappedTrans = sf::Transform().translate(diff) * trans;
    }

    const auto& seedRect = mSprite->getTextureRect();
    sf::FloatRect rect(sf::Vector2f(seedRect.position), sf::Vector2f(seedRect.size));
    
    sf::Color col = mSprite->getColor();

    float left = 0.f;
    float top = 0.f;
    float right = rect.size.x;
    float bottom = rect.size.y;

    // Calcular Posiciones Mundiales (Vector2f args)
    sf::Vector2f p1 = snappedTrans.transformPoint({left, top});
    sf::Vector2f p2 = snappedTrans.transformPoint({right, top});
    sf::Vector2f p3 = snappedTrans.transformPoint({right, bottom});
    sf::Vector2f p4 = snappedTrans.transformPoint({left, bottom});

    // Calcular UVs
    float u1 = rect.position.x;
    float v1 = rect.position.y;
    float u2 = rect.position.x + rect.size.x;
    float v2 = rect.position.y + rect.size.y;

    // T1: P1, P2, P4
    vertices.emplace_back(sf::Vertex{p1, col, sf::Vector2f(u1, v1)});
    vertices.emplace_back(sf::Vertex{p2, col, sf::Vector2f(u2, v1)});
    vertices.emplace_back(sf::Vertex{p4, col, sf::Vector2f(u1, v2)});

    // T2: P2, P3, P4
    vertices.emplace_back(sf::Vertex{p2, col, sf::Vector2f(u2, v1)});
    vertices.emplace_back(sf::Vertex{p3, col, sf::Vector2f(u2, v2)});
    vertices.emplace_back(sf::Vertex{p4, col, sf::Vector2f(u1, v2)});
}

void Entity::getShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const {
    texture = nullptr;
}

std::vector<Entity::ActiveStatusEffect> Entity::getActiveStatusEffects() const {
    std::vector<ActiveStatusEffect> list;

    // 1. Stun
    if (isStunned()) {
        list.push_back({"stun", mStunState.duration});
    }

    // 2. Bleed
    if (mBleedState.isBleeding()) {
        if (mBleedState.durationFlat > 0.f) {
            list.push_back({"bleed_flat", mBleedState.durationFlat});
        }
        if (mBleedState.durationPercent > 0.f) {
            list.push_back({"bleed_percent", mBleedState.durationPercent});
        }
    }

    // 3. Slow Move
    if (mDebuffState.isSlowedMove()) {
        list.push_back({"slow_move", mDebuffState.slowMoveTimer});
    }

    // 4. Slow Attack
    if (mDebuffState.isSlowedAttack()) {
        list.push_back({"slow_attack", mDebuffState.slowAttackTimer});
    }

    // 5. Active crowd control effects from mActiveEffects map
    for (const auto& [effect, duration] : mActiveEffects) {
        if (duration > 0.f) {
            std::string id = "";
            switch (effect) {
                case StatusEffect::Silence:   id = "silence"; break;
                case StatusEffect::Root:      id = "root"; break;
                case StatusEffect::Fear:      id = "fear"; break;
                case StatusEffect::Polymorph: id = "polymorph"; break;
                case StatusEffect::Banish:    id = "banish"; break;
                default: break;
            }
            if (!id.empty()) {
                list.push_back({id, duration});
            }
        }
    }

    // 6. Active Buffs & Debuffs - dynamically populated based on statusEffectId
    struct TempEffectRecord {
        std::string id;
        float maxDuration;
        std::vector<float> castDurations;
    };
    std::vector<TempEffectRecord> tempRecords;

    for (const auto& buff : mActiveBuffs) {
        if (!buff.statusEffectId.empty() && buff.duration > 0.f) {
            TempEffectRecord* record = nullptr;
            for (auto& r : tempRecords) {
                if (r.id == buff.statusEffectId) {
                    record = &r;
                    break;
                }
            }
            if (!record) {
                tempRecords.push_back({buff.statusEffectId, buff.duration, {buff.duration}});
            } else {
                if (buff.duration > record->maxDuration) {
                    record->maxDuration = buff.duration;
                }
                bool isNewCast = true;
                for (float d : record->castDurations) {
                    if (std::abs(d - buff.duration) < 0.001f) {
                        isNewCast = false;
                        break;
                    }
                }
                if (isNewCast) {
                    record->castDurations.push_back(buff.duration);
                }
            }
        }
    }

    for (const auto& record : tempRecords) {
        bool alreadyAdded = false;
        for (auto& active : list) {
            if (active.id == record.id) {
                alreadyAdded = true;
                active.stacks += record.castDurations.size() - 1;
                if (record.maxDuration > active.remainingDuration) {
                    active.remainingDuration = record.maxDuration;
                }
                break;
            }
        }
        if (!alreadyAdded) {
            list.push_back({record.id, record.maxDuration, static_cast<int>(record.castDurations.size())});
        }
    }

    return list;
}
