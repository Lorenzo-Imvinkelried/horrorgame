#include "CharacterPanel.h"
#include "Config.h"
#include "entities/player/Player.h"
#include "core/engine/ResourceManager.h"
#include "core/items/WeaponSprite.h"
#include "core/systems/InteractionSystem.h"
#include "core/systems/CultivoSystem.h"
#include <iostream>
#include <cmath>

void CharacterPanel::draw(sf::RenderTarget& target, ResourceManager& res) {
    if (mEntity) {
        draw(target, mEntity, res);
    }
}

void CharacterPanel::draw(sf::RenderTarget& target, Entity* entity, ResourceManager &res) {
    if (!entity) return; 
    
    if (mEntity != entity) {
        setEntity(entity);
    }
    
    updateTexts();
    
    const float panelX = std::floor(mPosition.x);
    const float panelY = std::floor(mPosition.y);
    float zoom = cfg::Map::ZOOM_FACTOR;
    
    // --- 1. Fondo del Panel ---
    try {
        sf::Texture& bgTex = res.getTexture("assets/ui/char_panel_bg.png");
        if (!mBackgroundSprite) {
            mBackgroundSprite.emplace(bgTex);
        } else {
            mBackgroundSprite->setTexture(bgTex, true);
        }
        mBackgroundSprite->setScale({zoom, zoom});
        mBackgroundSprite->setPosition({panelX, panelY});
        
        mSize = mBackgroundSprite->getGlobalBounds().size;
        mHasValidBackground = true;
        
        target.draw(*mBackgroundSprite);
    } catch (...) {
        mHasValidBackground = false;
        sf::RectangleShape bg(mSize);
        bg.setPosition({panelX, panelY});
        bg.setFillColor(sf::Color(0,0,0,160));
        bg.setOutlineThickness(1.f);
        bg.setOutlineColor(sf::Color(60,60,60));
        target.draw(bg);
    }

    // --- 1.5. Boton de Cierre ---
    try {
        sf::Texture& btnTex = res.getTexture("assets/ui/button_close.png");
        sf::Sprite closeBtn(btnTex);
        closeBtn.setScale({zoom, zoom});
        closeBtn.setPosition({
            panelX + cfg::UI::CharacterPanel::CLOSE_BTN_X * zoom,
            panelY + cfg::UI::CharacterPanel::CLOSE_BTN_Y * zoom
        });
        target.draw(closeBtn);
    } catch (...) {
        sf::Vector2f btnSize(cfg::UI::CharacterPanel::CLOSE_BTN_SIZE * zoom, cfg::UI::CharacterPanel::CLOSE_BTN_SIZE * zoom);
        sf::RectangleShape closeBtn(btnSize);
        sf::Vector2f btnPos(std::floor(panelX + cfg::UI::CharacterPanel::CLOSE_BTN_X * zoom), std::floor(panelY + cfg::UI::CharacterPanel::CLOSE_BTN_Y * zoom));
        closeBtn.setPosition(btnPos);
        closeBtn.setFillColor(sf::Color(180, 0, 0));
        closeBtn.setOutlineColor(sf::Color::White);
        closeBtn.setOutlineThickness(1.f);
        target.draw(closeBtn);
    }

    // --- 2. Texto de Stats (Lado izquierdo) ---
    BitmapText text;
    text.setTexture(mFontTexture);
    text.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
    text.setColor(sf::Color::White);
    
    float textX = std::floor(panelX + cfg::UI::CharacterPanel::TEXT_OFFSET_X * zoom);
    float textY = std::floor(panelY + cfg::UI::CharacterPanel::TEXT_OFFSET_Y * zoom);
    float lineSpacing = std::floor(cfg::UI::CharacterPanel::LINE_SPACING * (zoom / 2.0f));

    // Nivel
    text.setString("Nivel: " + mLvlStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing * 1.5f;

    // Stats Base
    text.setColor(sf::Color::White);
    text.setString("STR: " + mStrStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;
    
    text.setString("AGI: " + mAgiStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;
    
    text.setString("INT: " + mIntStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;
    
    text.setString("VIT: " + mVitStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("HP: " + mHpStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("MANA: " + mMpStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("PESO: " + mWeightStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing * 1.5f;
    
    // Combate Ofensivo
    text.setString("ATQ-F: " + mAtkStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("BONO ATAQUE F: " + mAtkBonusStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("DEF-F: " + mDefStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing * 1.5f; 
    
    text.setString("ATK-SPD: " + mAtkSpdStr);
    text.setColor(mAtkSpdColor);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    text.setColor(sf::Color::White);
    textY += lineSpacing; 

    text.setString("RANGO: " + mRangeStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing * 1.5f; 
    
    text.setString("CRIT: " + mCritStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;
    
    text.setString("D-CRIT: " + mCritDmgStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;
    
    text.setString("PEN DEF-F: " + mPenStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("PEN %: " + mPenPctStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("ACCURACY: " + mAccStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("ROBO VIDA: " + mLifeStealStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color::Green); 
    target.draw(text);
    text.setColor(sf::Color::White);
    textY += lineSpacing;
    
    text.setString("AOE RNG: " + mAoeRngStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("AOE DMG %: " + mAoeDmgStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color(255, 200, 0)); 
    target.draw(text);
    text.setColor(sf::Color::White);
    textY += lineSpacing;

    text.setString("DBL-HIT: " + mDblHitStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("TRI-HIT: " + mTriHitStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("HP-DMG: " + mHpDmgStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;
    
    text.setString("VEL: " + mMoveSpdStr);
    text.setColor(mMoveSpdColor);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    text.setColor(sf::Color::White);
    textY += lineSpacing * 1.5f;

    // Combate Defensivo
    text.setString("BLOCK: " + mBlockStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("THORNS: " + mThornsStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("EVASION: " + mEvasionStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("HP REG: " + mHpRegenStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("MP REG: " + mMpRegenStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    // Ejecución y Daño Verdadero
    text.setString("EXEC. DMG: " + mExecDmgStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color(255, 100, 100)); 
    target.draw(text);
    textY += lineSpacing;

    text.setString("EXEC. THR: " + mExecThrStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    text.setColor(sf::Color::White);
    textY += lineSpacing;

    text.setString("TRUE DMG: " + mTrueDmgStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color::Magenta); 
    target.draw(text);
    text.setColor(sf::Color::White);
    textY += lineSpacing;

    // Sangrado
    text.setString("BLEED FLAT DUR: " + mBleedDurationFlatStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color(255, 100, 100)); 
    target.draw(text);
    textY += lineSpacing;

    text.setString("BLEED FLAT DMG: " + mBleedFlatDmgStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color::White);
    target.draw(text);
    textY += lineSpacing;
    
    text.setString("BLEED % DUR: " + mBleedDurationPctStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color(255, 50, 50)); 
    target.draw(text);
    textY += lineSpacing;

    text.setString("BLEED % DMG: " + mBleedPctDmgStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color::White);
    target.draw(text);
    text.setColor(sf::Color::White);
    textY += lineSpacing;

    // Debuffs
    text.setString("STUN: " + mStunStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color(255, 255, 0));
    target.draw(text);
    textY += lineSpacing;

    text.setString("SLOW MOV: " + mSlowMovStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color(100, 255, 255));
    target.draw(text);
    textY += lineSpacing;

    text.setString("SLOW ATK: " + mSlowAtkStr);
    text.setPosition({textX, textY});
    target.draw(text);
    text.setColor(sf::Color::White);
    textY += lineSpacing;

    // Malicia
    text.setString("MALICIA: " + mMaliceStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color(255, 140, 0));
    target.draw(text);
    text.setColor(sf::Color::White);
    textY += lineSpacing;

    // Stats adicionales
    text.setString("DMG RED: " + mDmgRedStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("TENACITY: " + mTenacityStr);
    text.setPosition({std::floor(textX), std::floor(textY)});
    target.draw(text);
    textY += lineSpacing;

    text.setString("CRIT AVOID: " + mCritAvoidStr);
    text.setPosition({textX, textY});
    target.draw(text);
    textY += lineSpacing;

    text.setString("ANTI-PEN: " + mAntiPenStr);
    text.setPosition({textX, textY});
    target.draw(text);
    textY += lineSpacing;

    text.setString("MANA STEAL: " + mManaStealStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color(0, 150, 255));
    target.draw(text);
    text.setColor(sf::Color::White);
    textY += lineSpacing;

    text.setString("XP BONUS: " + mXpBonusStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color(255, 215, 0));
    target.draw(text);
    text.setColor(sf::Color::White);

    textY += lineSpacing;
    text.setString("CDR: " + mCdrStr);
    text.setPosition({textX, textY});
    text.setColor(sf::Color(0, 200, 255));
    target.draw(text);
    text.setColor(sf::Color::White);

    // --- 3. Slots de Equipamiento ---
    float slotSize = cfg::UI::BASE_SLOT_SIZE * zoom;
    
    BitmapText labelText;
    labelText.setTexture(mFontTexture);
    labelText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
    labelText.setColor(sf::Color::White);
    
    int currentSlotIndex = 0;
    for (const auto& slot : sSlotLayouts) {
        sf::Vector2f slotPos = {
            panelX + slot.offset.x * zoom,
            panelY + slot.offset.y * zoom
        };
        
        // A) Dibujar Slot Fondo
        std::string customPath = "";
        if (slot.name == "Casco") customPath = "assets/ui/char-panel-slots/slot_head_char_panel.png";
        else if (slot.name == "Capa") customPath = "assets/ui/char-panel-slots/slot_capa_char_panel.png";
        else if (slot.name == "Pechera") customPath = "assets/ui/char-panel-slots/slot_chest_char_panel.png";
        else if (slot.name == "Guantes") customPath = "assets/ui/char-panel-slots/slot_hand_char_panel.png";
        else if (slot.name == "Arma 1" || slot.name == "Arma 2") customPath = "assets/ui/char-panel-slots/slot_arma1-2_char_panel.png";
        else if (slot.name == "Anillo 1" || slot.name == "Anillo 2") customPath = "assets/ui/char-panel-slots/slot_anillo1-2_char_panel.png";
        else if (slot.name == "Botas") customPath = "assets/ui/char-panel-slots/slot_foot_char_panel.png";
        else if (slot.name == "subarma1" || slot.name == "subarma2") customPath = "assets/ui/char-panel-slots/slot_subarma1-2_char_panel.png";
        else if (slot.name == "Cultivo") customPath = "assets/ui/char-panel-slots/slot_cultivo.png";

        bool drawnCustom = false;
        if (!customPath.empty()) {
            try {
                sf::Texture& slotTex = res.getTexture(customPath);
                sf::Sprite customSlot(slotTex);
                customSlot.setScale({zoom, zoom});
                customSlot.setPosition(slotPos);
                target.draw(customSlot);
                drawnCustom = true;
            } catch (...) {
            }
        }

        if (!drawnCustom) {
            if (mSlotSprite) {
                mSlotSprite->setScale({zoom, zoom});
                mSlotSprite->setPosition(slotPos);
                target.draw(*mSlotSprite);
            } else {
                drawSlotFallback(target, slotPos);
            }
        }

        // B) Debug layout outline
        if (mDebugLayoutMode) {
            sf::RectangleShape outline({slotSize, slotSize});
            outline.setPosition(slotPos);
            outline.setFillColor(sf::Color(0, 255, 0, 45));
            
            if (currentSlotIndex == mSelectedDebugSlotIndex) {
                outline.setOutlineColor(sf::Color::Yellow);
                outline.setOutlineThickness(2.f);
            } else {
                outline.setOutlineColor(sf::Color::Green);
                outline.setOutlineThickness(1.f);
            }
            target.draw(outline);
            
            labelText.setString(slot.name);
            sf::FloatRect textBounds = labelText.getLocalBounds();
            float textX_centered = std::floor(slotPos.x + (slotSize - textBounds.size.x * cfg::UI::FONT_SCALE) * 0.5f);
            float textY_centered = std::floor(slotPos.y + (slotSize - textBounds.size.y * cfg::UI::FONT_SCALE) * 0.5f);
            labelText.setPosition({ textX_centered, textY_centered });
            labelText.setColor(sf::Color::Green);
            target.draw(labelText);
        }

        // C) Visibilidad de slot
        bool isHidden = (currentSlotIndex == mHiddenSlotIndex);
        if (mHiddenSlotIndex != -1) {
            std::string hiddenName = getSlotName(mHiddenSlotIndex);
            if ((hiddenName == "Arma 1" || hiddenName == "Arma 2") && 
                (slot.name == "Arma 1" || slot.name == "Arma 2")) {
                auto w0 = entity->getWeapon(0);
                auto w1 = entity->getWeapon(1);
                if ((w0 && w0->gripType == GripType::TwoHanded) || (w1 && w1->gripType == GripType::TwoHanded)) {
                    isHidden = true;
                }
            }
        }
        
        // D) Dibujar Item si está equipado
        std::shared_ptr<Item> equippedItem = nullptr;
        bool isTwoHandedGhost = false;

        equippedItem = entity->getEquippedItem(static_cast<EquipmentSlot>(currentSlotIndex));
        if (!equippedItem) {
            if (slot.name == "Arma 1") {
                auto offHandItem = entity->getEquippedItem(EquipmentSlot::OffHand);
                if (offHandItem && offHandItem->gripType == GripType::TwoHanded) {
                    equippedItem = offHandItem;
                    isTwoHandedGhost = true;
                }
            } else if (slot.name == "Arma 2") {
                auto mainHandItem = entity->getEquippedItem(EquipmentSlot::MainHand);
                if (mainHandItem && mainHandItem->gripType == GripType::TwoHanded) {
                    equippedItem = mainHandItem;
                    isTwoHandedGhost = true;
                }
            }
        }

        if (equippedItem && !isHidden) {
            auto cultivated = CultivoSystem::getInstance().getCultivatedItem();
            if (cultivated && cultivated == equippedItem) {
                CultivoSystem::getInstance().drawIndicator(target, res, slotPos, zoom);
            }

            float targetSize = cfg::UI::UNIFIED_SLOT_SIZE * 0.8f;
            float offsetX = 0.f, offsetY = 0.f;

            if (equippedItem->type == ItemType::Weapon) {
                try {
                    const sf::Texture& baseTex = res.getTexture(equippedItem->texturePath);
                    const sf::Texture& layoutTex = res.getTexture("assets/items/weapons/weapons_layout.png");

                    WeaponSprite weaponSprite;
                    weaponSprite.setTextures(baseTex, layoutTex);
                    weaponSprite.setFortificationLevel(equippedItem->fortificationLevel);

                    sf::IntRect overlayRect({0,0}, {0,0});
                    
                    if (equippedItem->quality != ItemQuality::Common && equippedItem->textureRect.size.x == 16) {
                        if (equippedItem->overlayGridCoords.x >= 0 && equippedItem->overlayGridCoords.y >= 0) {
                            overlayRect = sf::IntRect({equippedItem->overlayGridCoords.x * 16, equippedItem->overlayGridCoords.y * 16}, {16, 16});
                        } else {
                            overlayRect = equippedItem->textureRect;
                        }
                    }

                    weaponSprite.setVisuals(equippedItem->textureRect, overlayRect, getQualityColor(equippedItem->quality));
                    
                    float finalW = equippedItem->textureRect.size.x * zoom;
                    float finalH = equippedItem->textureRect.size.y * zoom;
                    
                    float currentScale = zoom;
                    float padding = 4.f * zoom;
                    bool is32x32Item = (equippedItem->textureRect.size.x == 32 || equippedItem->textureRect.size.y == 32);
                    if (!is32x32Item && (finalW > slotSize - padding || finalH > slotSize - padding)) {
                        float maxDim = std::max((float)equippedItem->textureRect.size.x, (float)equippedItem->textureRect.size.y);
                        currentScale = (slotSize - padding) / maxDim;
                        finalW = equippedItem->textureRect.size.x * currentScale;
                        finalH = equippedItem->textureRect.size.y * currentScale;
                    }
                    weaponSprite.setScale(sf::Vector2f(currentScale, currentScale));

                    offsetX = std::floor((slotSize - finalW) * 0.5f);
                    offsetY = std::floor((slotSize - finalH) * 0.5f);

                    weaponSprite.setPosition(sf::Vector2f(slotPos.x + offsetX, slotPos.y + offsetY));
                    target.draw(weaponSprite);
                } 
                catch (const std::exception& e) {
                     std::cerr << "[CharacterPanel] Error drawing weapon sprite: " << e.what() << "\n";
                }
            }
            else {
                try {
                    sf::Texture& tex = res.getTexture(equippedItem->texturePath);
                    sf::Sprite icon(tex);

                    if (equippedItem->textureRect.size.x > 0 && equippedItem->textureRect.size.y > 0) {
                        icon.setTextureRect(equippedItem->textureRect);
                    }

                    sf::FloatRect localB = icon.getLocalBounds();
                    if (localB.size.x > 0 && localB.size.y > 0) {
                         float scaleX = targetSize / localB.size.x;
                         float scaleY = targetSize / localB.size.y;
                         icon.setScale(sf::Vector2f(zoom, zoom));
                         
                         float iconW = localB.size.x * zoom;
                         float iconH = localB.size.y * zoom;
                         offsetX = (slotSize - iconW) / 2.f;
                         offsetY = (slotSize - iconH) / 2.f;
                         
                         icon.setPosition(sf::Vector2f(slotPos.x + offsetX, slotPos.y + offsetY));

                          if (equippedItem->fortificationLevel >= 6) {
                              ItemAuraRenderer::drawAura(target, icon, equippedItem->fortificationLevel, zoom);
                          }

                          target.draw(icon);
                     }
                } catch (...) {
                    sf::RectangleShape err({20.f * zoom, 20.f * zoom});
                    err.setFillColor(sf::Color::Red);
                    err.setPosition(slotPos);
                    target.draw(err);
                }
            }
        }

        if (isTwoHandedGhost && !isHidden) {
            sf::RectangleShape darkOverlay({slotSize, slotSize});
            darkOverlay.setPosition(slotPos);
            darkOverlay.setFillColor(sf::Color(0, 0, 0, 180));
            target.draw(darkOverlay);
        }

        if (InteractionSystem::getInstance().isActive() && !isHidden) {
            if (equippedItem) {
                if (!InteractionSystem::getInstance().isCompatible(equippedItem)) {
                    sf::RectangleShape dimOverlay({slotSize, slotSize});
                    dimOverlay.setPosition(slotPos);
                    dimOverlay.setFillColor(sf::Color(0, 0, 0, 180));
                    target.draw(dimOverlay);
                }
            } else {
                sf::RectangleShape dimOverlay({slotSize, slotSize});
                dimOverlay.setPosition(slotPos);
                dimOverlay.setFillColor(sf::Color(0, 0, 0, 180));
                target.draw(dimOverlay);
            }
        }
        
        currentSlotIndex++;
    }

    if (mDebugLayoutMode) {
        sf::RectangleShape banner({ mSize.x, 30.f * (zoom / 2.0f) });
        banner.setPosition({ panelX, panelY + mSize.y - banner.getSize().y });
        banner.setFillColor(sf::Color(150, 0, 0, 220));
        banner.setOutlineThickness(1.f);
        banner.setOutlineColor(sf::Color::Red);
        target.draw(banner);
        
        BitmapText bannerText;
        bannerText.setTexture(mFontTexture);
        bannerText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
        bannerText.setColor(sf::Color::White);
        bannerText.setString("MODO EDIT LAYOUT (Q Guardar)");
        sf::FloatRect bBounds = bannerText.getLocalBounds();
        bannerText.setPosition({
            std::floor(panelX + (mSize.x - bBounds.size.x * cfg::UI::FONT_SCALE) * 0.5f),
            std::floor(banner.getPosition().y + (banner.getSize().y - bBounds.size.y * cfg::UI::FONT_SCALE) * 0.5f)
        });
        target.draw(bannerText);
    }
}
