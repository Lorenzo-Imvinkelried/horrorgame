#include "Animation.h"
#include "core/items/Item.h"
#include "core/items/WeaponSprite.h"
#include "core/systems/gore/GoreSystem.h"

void Animation::setWeaponVisuals(const sf::Texture* baseTex, const sf::Texture* layoutTex, 
                                 const sf::IntRect& baseRect, const sf::IntRect& overlayRect, ItemQuality quality, 
                                 sf::Vector2f offset, bool isTwoHanded, int fortificationLevel, bool isShield, bool shieldOverHand) 
{
    mWeaponIsTwoHanded = isTwoHanded;
    mWeaponIsShield = isShield;
    mWeaponShieldOverHand = isShield && shieldOverHand;
    if (baseTex && layoutTex) {
        mWeapon = std::make_unique<WeaponSprite>();
        mWeapon->setTextures(*baseTex, *layoutTex);
        mWeapon->setVisuals(baseRect, overlayRect, getQualityColor(quality));
        mWeapon->setFortificationLevel(fortificationLevel);
        if (isShield) {
            mWeapon->setOrigin({(float)baseRect.size.x * 0.5f, (float)baseRect.size.y * 0.5f});
        } else {
            mWeapon->setOrigin({0.f, (float)baseRect.size.y}); // Handle del arma
        }
        mWeaponOffset = offset;
    } else {
        mWeapon.reset();
    }
}

void Animation::setSecondaryWeaponVisuals(const sf::Texture* baseTex, const sf::Texture* layoutTex, 
                                          const sf::IntRect& baseRect, const sf::IntRect& overlayRect, ItemQuality quality, 
                                          sf::Vector2f offset, int fortificationLevel, bool isShield, bool shieldOverHand)
{
    mSecondaryIsShield = isShield;
    mSecondaryShieldOverHand = isShield && shieldOverHand;
    if (baseTex && layoutTex) {
        mWeaponSecondary = std::make_unique<WeaponSprite>();
        mWeaponSecondary->setTextures(*baseTex, *layoutTex);
        mWeaponSecondary->setVisuals(baseRect, overlayRect, getQualityColor(quality));
        mWeaponSecondary->setFortificationLevel(fortificationLevel);
        if (isShield) {
            mWeaponSecondary->setOrigin({(float)baseRect.size.x * 0.5f, (float)baseRect.size.y * 0.5f}); 
        } else {
            mWeaponSecondary->setOrigin({0.f, (float)baseRect.size.y}); // Handle del arma secundaria
        }
        mSecondaryWeaponOffset = offset;
    } else {
        mWeaponSecondary.reset();
    }
}

Animation::WeaponSpawnInfo Animation::getWeaponSpawnInfo(int slotIndex) const {
    WeaponSpawnInfo info;
    if (slotIndex == 0 && mWeapon) {
        info.position = mWeapon->getPosition();
        info.rotation = 0.f;
        info.scale = mWeapon->getScale();
        info.origin = mWeapon->getOrigin();
        info.exists = true;
    } else if (slotIndex == 1 && mWeaponSecondary) {
        info.position = mWeaponSecondary->getPosition();
        info.rotation = 0.f;
        info.scale = mWeaponSecondary->getScale();
        info.origin = mWeaponSecondary->getOrigin();
        info.exists = true;
    }
    return info;
}

void Animation::setArmorVisuals(EquipmentSlot slot, const sf::Texture* tex, const sf::IntRect& rect, sf::Vector2f offset, float scale, int fortificationLevel) {
    int idx = static_cast<int>(slot);
    if (idx >= 0 && idx < 12) {
        mArmorVisuals[idx].texture = tex;
        mArmorVisuals[idx].textureRect = rect;
        mArmorVisuals[idx].offset = offset;
        mArmorVisuals[idx].scale = scale;
        mArmorVisuals[idx].exists = (tex != nullptr);
        mArmorVisuals[idx].fortificationLevel = fortificationLevel;
    }
}

void Animation::emitWeaponGibs(GoreSystem& gore, float floorY, sf::Vector2f sourcePos, float forceMultiplier) const {
    auto emitSingleWeapon = [&](const std::unique_ptr<WeaponSprite>& weapon) {
        if (!weapon) return;
        
        // Base Sprite
        if (weapon->m_baseSprite) {
            sf::Sprite tempSprite = *weapon->m_baseSprite;
            tempSprite.setPosition(weapon->getPosition());
            tempSprite.setRotation(sf::degrees(0.f));
            tempSprite.setScale(weapon->getScale());
            tempSprite.setOrigin(weapon->getOrigin());
            gore.emitGibs(tempSprite, floorY, sourcePos, forceMultiplier);
        }
        
        // Overlay Sprite
        if (weapon->m_overlaySprite) {
            sf::Sprite tempSprite = *weapon->m_overlaySprite;
            tempSprite.setPosition(weapon->getPosition());
            tempSprite.setRotation(sf::degrees(0.f));
            tempSprite.setScale(weapon->getScale());
            tempSprite.setOrigin(weapon->getOrigin());
            gore.emitGibs(tempSprite, floorY, sourcePos, forceMultiplier);
        }
    };
    
    emitSingleWeapon(mWeapon);
    emitSingleWeapon(mWeaponSecondary);
}
