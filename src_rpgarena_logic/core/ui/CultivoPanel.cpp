#include "CultivoPanel.h"
#include "Config.h"
#include "../engine/ResourceManager.h"
#include "../graphics/BitmapText.h"
#include "../systems/CultivoSystem.h"
#include "../items/WeaponSprite.h"
#include "utils/TinyJson.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

CultivoPanel::CultivoPanel(sf::Texture* fontTexture, std::optional<sf::Sprite>& slotSprite)
    : mFontTexture(fontTexture), mSlotSprite(slotSprite)
{
    mPosition = { 300.f, 150.f };
    mVisible = false;
    loadLayout();
}

void CultivoPanel::loadLayout() {
    std::string path = "assets/data/cultivo-panel.json";
    json::Value root = json::parseFile(path);
    if (root.type == json::Type::Object) {
        const auto& obj = root.asObject();

        auto parseVec2 = [&](const json::Object& parent, const std::string& key, sf::Vector2f& target) {
            auto it = parent.find(key);
            if (it != parent.end() && it->second.type == json::Type::Object) {
                const auto& itemData = it->second.asObject();
                auto itX = itemData.find("x");
                auto itY = itemData.find("y");
                if (itX != itemData.end() && itX->second.type == json::Type::Number &&
                    itY != itemData.end() && itY->second.type == json::Type::Number) {
                    target.x = static_cast<float>(itX->second.asDouble());
                    target.y = static_cast<float>(itY->second.asDouble());
                }
            }
        };

        auto parseStr = [&](const json::Object& parent, const std::string& key, std::string& target) {
            auto it = parent.find(key);
            if (it != parent.end() && it->second.type == json::Type::String) {
                target = it->second.asString();
            }
        };

        // Title section
        auto itTitle = obj.find("title");
        if (itTitle != obj.end() && itTitle->second.type == json::Type::Object) {
            const auto& tObj = itTitle->second.asObject();
            parseVec2(obj, "title", mTitleOffset);
            parseStr(tObj, "text", mTitleText);
        }

        // Item slot section
        parseVec2(obj, "item_slot", mItemSlotOffset);

        // Item info section
        auto itInfo = obj.find("item_info");
        if (itInfo != obj.end() && itInfo->second.type == json::Type::Object) {
            const auto& iObj = itInfo->second.asObject();
            parseVec2(iObj, "name_offset", mItemNameOffset);
            parseVec2(iObj, "level_offset", mItemLevelOffset);
            parseStr(iObj, "level_prefix", mItemLevelPrefix);
            parseVec2(iObj, "placeholder_offset", mPlaceholderOffset);
            parseStr(iObj, "placeholder_text", mPlaceholderText);
        }

        // Exp bar section
        auto itExp = obj.find("exp_bar");
        if (itExp != obj.end() && itExp->second.type == json::Type::Object) {
            const auto& eObj = itExp->second.asObject();
            parseVec2(obj, "exp_bar", mExpBarOffset);
            parseVec2(eObj, "text_offset", mExpTextOffset);
            parseStr(eObj, "text_prefix", mExpTextPrefix);
        }

        // Stats section
        auto itStats = obj.find("stats_section");
        if (itStats != obj.end() && itStats->second.type == json::Type::Object) {
            const auto& sObj = itStats->second.asObject();
            parseVec2(sObj, "header_offset", mStatsHeaderOffset);
            parseStr(sObj, "header_text_unlocked", mStatsHeaderUnlocked);
            parseStr(sObj, "header_text_locked", mStatsHeaderLocked);
            parseVec2(sObj, "sockets_offset", mSocketsOffset);
            parseVec2(sObj, "labels_offset", mSocketLabelsOffset);
            auto itSpace = sObj.find("line_spacing");
            if (itSpace != sObj.end() && itSpace->second.type == json::Type::Number) {
                mStatLineSpacing = static_cast<float>(itSpace->second.asDouble());
            }
        }

        // Cultivar button section
        auto itBtn = obj.find("button_cultivar");
        if (itBtn != obj.end() && itBtn->second.type == json::Type::Object) {
            const auto& bObj = itBtn->second.asObject();
            parseVec2(obj, "button_cultivar", mButtonCultivarOffset);
            parseVec2(bObj, "text_offset", mButtonTextOffset);
            parseStr(bObj, "text_active", mButtonTextActive);
            parseStr(bObj, "text_locked", mButtonTextLocked);
        }
    }
}

void CultivoPanel::setVisible(bool visible) {
    UIPanel::setVisible(visible);
    if (visible) {
        refreshAvailableStats();
    }
}

void CultivoPanel::setPosition(sf::Vector2f pos) {
    mPosition = pos;
}

sf::FloatRect CultivoPanel::getBounds() const {
    float zoom = cfg::Map::ZOOM_FACTOR;
    return sf::FloatRect(mPosition, { mSize.x * zoom, mSize.y * zoom });
}

void CultivoPanel::setItem(std::shared_ptr<Item> item) {
    CultivoSystem::getInstance().setCultivatedItem(item);
    refreshAvailableStats();
}

std::shared_ptr<Item> CultivoPanel::getItem() const {
    return CultivoSystem::getInstance().getCultivatedItem();
}

void CultivoPanel::refreshAvailableStats() {
    auto item = getItem();
    if (!item) {
        mAvailableStats.clear();
        mSelectedFlags.clear();
        return;
    }

    mAvailableStats = CultivoSystem::getAvailableStats(*item);
    mSelectedFlags.assign(mAvailableStats.size(), false);

    // If item is already locked, pre-select its locked stats
    if (item->cultivoLocked) {
        for (size_t i = 0; i < mAvailableStats.size(); ++i) {
            for (const auto& sel : item->cultivoSelectedStats) {
                if (sel == mAvailableStats[i].first) {
                    mSelectedFlags[i] = true;
                    break;
                }
            }
        }
    }
}

bool CultivoPanel::isMouseOverSlot(sf::Vector2f mousePos) const {
    if (!mVisible) return false;
    float zoom = cfg::Map::ZOOM_FACTOR;
    sf::Vector2f slotPos = mPosition + mItemSlotOffset * zoom;
    sf::FloatRect slotBounds(slotPos, { 18.f * zoom, 18.f * zoom });
    return slotBounds.contains(mousePos);
}

void CultivoPanel::drawSlotFallback(sf::RenderTarget& target, sf::Vector2f pos) {
    float zoom = cfg::Map::ZOOM_FACTOR;
    sf::RectangleShape rect({ cfg::UI::UNIFIED_SLOT_SIZE * zoom, cfg::UI::UNIFIED_SLOT_SIZE * zoom });
    rect.setPosition(pos);
    rect.setFillColor(sf::Color(30, 30, 40, 220));
    rect.setOutlineThickness(1.5f * zoom);
    rect.setOutlineColor(sf::Color(100, 100, 140));
    target.draw(rect);
}

void CultivoPanel::draw(sf::RenderTarget& target, ResourceManager& res) {
    if (!mVisible) return;

    float zoom = cfg::Map::ZOOM_FACTOR;
    const float panelX = std::floor(mPosition.x);
    const float panelY = std::floor(mPosition.y);

    // 1. Draw Panel Background (Texture or Fallback)
    bool bgDrawn = false;
    try {
        sf::Texture& bgTex = res.getTexture("assets/ui/cultivo-panel/cultivo_bg.png");
        sf::Sprite bgSprite(bgTex);
        bgSprite.setScale({ zoom, zoom });
        bgSprite.setPosition({ panelX, panelY });
        target.draw(bgSprite);
        mSize = sf::Vector2f(bgTex.getSize().x, bgTex.getSize().y);
        bgDrawn = true;
    } catch (...) {}

    if (!bgDrawn) {
        sf::RectangleShape bg({ mSize.x * zoom, mSize.y * zoom });
        bg.setPosition({ panelX, panelY });
        bg.setFillColor(sf::Color(15, 18, 28, 235));
        bg.setOutlineThickness(2.f * zoom);
        bg.setOutlineColor(sf::Color(180, 140, 50));
        target.draw(bg);
    }

    // 2. Title Bar & Close Button
    BitmapText text;
    text.setTexture(mFontTexture);
    text.setScale({ cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE });
    text.setColor(sf::Color(255, 215, 0));
    text.setString(mTitleText);
    text.setPosition({ panelX + mTitleOffset.x * zoom, panelY + mTitleOffset.y * zoom });
    target.draw(text);

    // Close Button
    bool closeDrawn = false;
    try {
        sf::Texture& closeTex = res.getTexture("assets/ui/cultivo-panel/button_close.png");
        sf::Sprite closeSprite(closeTex);
        closeSprite.setScale({ zoom, zoom });
        closeSprite.setPosition({ panelX + (mSize.x - 22.f) * zoom, panelY + 8.f * zoom });
        target.draw(closeSprite);
        closeDrawn = true;
    } catch (...) {}

    if (!closeDrawn) {
        sf::RectangleShape closeBtn({ 18.f * zoom, 18.f * zoom });
        closeBtn.setPosition({ panelX + (mSize.x - 22.f) * zoom, panelY + 8.f * zoom });
        closeBtn.setFillColor(sf::Color(180, 40, 40));
        closeBtn.setOutlineThickness(1.f);
        closeBtn.setOutlineColor(sf::Color::White);
        target.draw(closeBtn);

        BitmapText xText;
        xText.setTexture(mFontTexture);
        xText.setScale({ cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE });
        xText.setColor(sf::Color::White);
        xText.setString("X");
        xText.setPosition({ panelX + (mSize.x - 19.f) * zoom, panelY + 9.f * zoom });
        target.draw(xText);
    }

    // 3. Item Slot
    sf::Vector2f slotPos = mPosition + mItemSlotOffset * zoom;
    bool slotDrawn = false;
    try {
        sf::Texture& slotTex = res.getTexture("assets/ui/cultivo-panel/item_slot.png");
        sf::Sprite slotSprite(slotTex);
        slotSprite.setScale({ zoom, zoom });
        slotSprite.setPosition(slotPos);
        target.draw(slotSprite);
        slotDrawn = true;
    } catch (...) {}

    if (!slotDrawn) {
        if (mSlotSprite) {
            mSlotSprite->setScale({ zoom, zoom });
            mSlotSprite->setPosition(slotPos);
            target.draw(*mSlotSprite);
        } else {
            drawSlotFallback(target, slotPos);
        }
    }

    // 3.5. Draw Animated Cultivo Indicator (under item)
    CultivoSystem::getInstance().drawIndicator(target, res, slotPos, zoom);

    // 4. Draw Cultivated Item (if present)
    auto item = getItem();
    if (item) {
        float slotSize = cfg::UI::BASE_SLOT_SIZE * zoom;
        float offsetX = 0.f, offsetY = 0.f;

        if (item->type == ItemType::Weapon) {
            try {
                const sf::Texture& baseTex = res.getTexture(item->texturePath);
                const sf::Texture& layoutTex = res.getTexture("assets/items/weapons/weapons_layout.png");

                WeaponSprite weaponSprite;
                weaponSprite.setTextures(baseTex, layoutTex);
                weaponSprite.setFortificationLevel(item->fortificationLevel);

                sf::IntRect overlayRect({0,0}, {0,0});
                if (item->quality != ItemQuality::Common && item->textureRect.size.x == 16) {
                    if (item->overlayGridCoords.x >= 0 && item->overlayGridCoords.y >= 0) {
                        overlayRect = sf::IntRect({item->overlayGridCoords.x * 16, item->overlayGridCoords.y * 16}, {16, 16});
                    } else {
                        overlayRect = item->textureRect;
                    }
                }

                weaponSprite.setVisuals(item->textureRect, overlayRect, getQualityColor(item->quality));

                float finalW = item->textureRect.size.x * zoom;
                float finalH = item->textureRect.size.y * zoom;

                float currentScale = zoom;
                float padding = 4.f * zoom;
                bool is32x32Item = (item->textureRect.size.x == 32 || item->textureRect.size.y == 32);
                if (!is32x32Item && (finalW > slotSize - padding || finalH > slotSize - padding)) {
                    float maxDim = std::max((float)item->textureRect.size.x, (float)item->textureRect.size.y);
                    currentScale = (slotSize - padding) / maxDim;
                    finalW = item->textureRect.size.x * currentScale;
                    finalH = item->textureRect.size.y * currentScale;
                }
                weaponSprite.setScale(sf::Vector2f(currentScale, currentScale));

                offsetX = std::floor((slotSize - finalW) * 0.5f);
                offsetY = std::floor((slotSize - finalH) * 0.5f);

                weaponSprite.setPosition(sf::Vector2f(slotPos.x + offsetX, slotPos.y + offsetY));
                target.draw(weaponSprite);
            } catch (const std::exception& e) {
                std::cerr << "[CultivoPanel] Error drawing weapon sprite: " << e.what() << "\n";
            }
        } else {
            try {
                sf::Texture& tex = res.getTexture(item->texturePath);
                sf::Sprite icon(tex);

                if (item->textureRect.size.x > 0 && item->textureRect.size.y > 0) {
                    icon.setTextureRect(item->textureRect);
                }

                sf::FloatRect localB = icon.getLocalBounds();
                if (localB.size.x > 0 && localB.size.y > 0) {
                    icon.setScale(sf::Vector2f(zoom, zoom));
                    
                    float iconW = localB.size.x * zoom;
                    float iconH = localB.size.y * zoom;
                    offsetX = (slotSize - iconW) / 2.f;
                    offsetY = (slotSize - iconH) / 2.f;
                    
                    icon.setPosition(sf::Vector2f(slotPos.x + offsetX, slotPos.y + offsetY));

                    if (item->fortificationLevel >= 6) {
                        ItemAuraRenderer::drawAura(target, icon, item->fortificationLevel, zoom);
                    }

                    target.draw(icon);
                }
            } catch (...) {}
        }

        // Draw Item Name & Cultivation Level
        text.setColor(sf::Color::White);
        text.setString(item->name);
        text.setPosition({ panelX + mItemNameOffset.x * zoom, panelY + mItemNameOffset.y * zoom });
        target.draw(text);

        text.setColor(sf::Color(100, 220, 255)); // Light cyan
        text.setString(mItemLevelPrefix + std::to_string(item->cultivoLevel));
        text.setPosition({ panelX + mItemLevelOffset.x * zoom, panelY + mItemLevelOffset.y * zoom });
        target.draw(text);

        // Draw EXP Bar (Texture or Fallback)
        int reqExp = CultivoSystem::getExpForNextLevel(item->cultivoLevel + 1);
        sf::Vector2f expPos = mPosition + mExpBarOffset * zoom;
        float fillRatio = (reqExp > 0) ? std::clamp((float)item->cultivoExp / (float)reqExp, 0.f, 1.f) : 0.f;

        bool expDrawn = false;
        try {
            sf::Texture& emptyBarTex = res.getTexture("assets/ui/cultivo-panel/empty_exp_bar.png");
            sf::Sprite emptyBarSprite(emptyBarTex);
            emptyBarSprite.setScale({ zoom, zoom });
            emptyBarSprite.setPosition(expPos);
            target.draw(emptyBarSprite);

            sf::Texture& fullBarTex = res.getTexture("assets/ui/cultivo-panel/full_exp_bar.png");
            int fillW = static_cast<int>(fullBarTex.getSize().x * fillRatio);
            if (fillW > 0) {
                sf::Sprite fullBarSprite(fullBarTex);
                fullBarSprite.setTextureRect(sf::IntRect({0, 0}, {fillW, (int)fullBarTex.getSize().y}));
                fullBarSprite.setScale({ zoom, zoom });
                fullBarSprite.setPosition(expPos);
                target.draw(fullBarSprite);
            }
            expDrawn = true;
        } catch (...) {}

        if (!expDrawn) {
            float barW = (mSize.x - 40.f) * zoom;
            float barH = 8.f * zoom;
            sf::RectangleShape barBg({ barW, barH });
            barBg.setPosition(expPos);
            barBg.setFillColor(sf::Color(40, 40, 50));
            target.draw(barBg);

            sf::RectangleShape barFill({ barW * fillRatio, barH });
            barFill.setPosition(expPos);
            barFill.setFillColor(sf::Color(0, 200, 120));
            target.draw(barFill);
        }

        // Draw EXP Text
        text.setColor(sf::Color(200, 200, 200));
        text.setString(mExpTextPrefix + std::to_string(item->cultivoExp) + " / " + std::to_string(reqExp));
        text.setPosition({ panelX + mExpTextOffset.x * zoom, panelY + mExpTextOffset.y * zoom });
        target.draw(text);
    } else {
        text.setColor(sf::Color(150, 150, 150));
        text.setString(mPlaceholderText);
        text.setPosition({ panelX + mPlaceholderOffset.x * zoom, panelY + mPlaceholderOffset.y * zoom });
        target.draw(text);
    }

    // 5. Stat Selection Section
    if (item) {
        text.setColor(sf::Color(255, 230, 130));
        text.setString(item->cultivoLocked ? mStatsHeaderLocked : mStatsHeaderUnlocked);
        text.setPosition({ panelX + mStatsHeaderOffset.x * zoom, panelY + mStatsHeaderOffset.y * zoom });
        target.draw(text);

        float socketY = panelY + mSocketsOffset.y * zoom;
        float labelY = panelY + mSocketLabelsOffset.y * zoom;
        for (size_t i = 0; i < mAvailableStats.size() && i < 6; ++i) {
            bool isChecked = (i < mSelectedFlags.size() && mSelectedFlags[i]);

            // Checkbox Sprite or Fallback Box
            bool boxDrawn = false;
            try {
                std::string boxTexPath = isChecked ? "assets/ui/cultivo-panel/selection_socket_on.png" 
                                                   : "assets/ui/cultivo-panel/selection_socket_off.png";
                sf::Texture& boxTex = res.getTexture(boxTexPath);
                sf::Sprite boxSprite(boxTex);
                boxSprite.setScale({ zoom, zoom });
                boxSprite.setPosition({ panelX + mSocketsOffset.x * zoom, socketY });
                target.draw(boxSprite);
                boxDrawn = true;
            } catch (...) {}

            if (!boxDrawn) {
                sf::RectangleShape box({ 12.f * zoom, 12.f * zoom });
                box.setPosition({ panelX + mSocketsOffset.x * zoom, socketY });
                box.setFillColor(isChecked ? sf::Color(0, 180, 252) : sf::Color(40, 40, 50));
                box.setOutlineThickness(1.f);
                box.setOutlineColor(sf::Color::White);
                target.draw(box);
            }

            std::string bonusStr = "";
            const auto& cs = item->cultivoBonusStats;
            const std::string& statId = mAvailableStats[i].first;
            std::stringstream ss;

            if (statId == "STR" || statId == "strength") {
                if (cs.strength > 0) ss << " +" << cs.strength;
            } else if (statId == "AGI" || statId == "agility") {
                if (cs.agility > 0) ss << " +" << cs.agility;
            } else if (statId == "INT" || statId == "intelligence") {
                if (cs.intelligence > 0) ss << " +" << cs.intelligence;
            } else if (statId == "VIT" || statId == "vitality") {
                if (cs.vitality > 0) ss << " +" << cs.vitality;
            } else if (statId == "physicalDamage" || statId == "attack") {
                if (cs.physicalDamage > 0) ss << " +" << cs.physicalDamage;
            } else if (statId == "defense") {
                if (cs.defense > 0) ss << " +" << cs.defense;
            } else if (statId == "hpBonus" || statId == "hp") {
                if (cs.hpBonus > 0) ss << " +" << cs.hpBonus;
            } else if (statId == "mpBonus" || statId == "mp") {
                if (cs.mpBonus > 0) ss << " +" << cs.mpBonus;
            } else if (statId == "critChance") {
                if (cs.critChance > 0.001f) ss << " +" << std::fixed << std::setprecision(1) << cs.critChance << "%";
            } else if (statId == "critDamage") {
                if (cs.critDamage > 0.001f) ss << " +" << std::fixed << std::setprecision(1) << cs.critDamage << "%";
            } else if (statId == "lifestealPercent") {
                if (cs.lifestealPercent > 0.001f) ss << " +" << std::fixed << std::setprecision(1) << cs.lifestealPercent << "%";
            } else if (statId == "cooldownReductionPercent") {
                if (cs.cooldownReductionPercent > 0.001f) ss << " +" << std::fixed << std::setprecision(1) << cs.cooldownReductionPercent << "%";
            } else if (statId == "attackPercent") {
                if (cs.attackPercent > 0.001f) ss << " +" << std::fixed << std::setprecision(1) << cs.attackPercent << "%";
            } else if (statId == "defensePercent") {
                if (cs.defensePercent > 0.001f) ss << " +" << std::fixed << std::setprecision(1) << cs.defensePercent << "%";
            } else if (statId == "hpPercent") {
                if (cs.hpPercent > 0.001f) ss << " +" << std::fixed << std::setprecision(1) << cs.hpPercent << "%";
            } else if (statId == "physicalDamageBonus") {
                if (cs.physicalDamageBonus > 0.001f) ss << " +" << std::fixed << std::setprecision(1) << cs.physicalDamageBonus << "%";
            } else if (statId == "armorPenetration") {
                if (cs.armorPenetration > 0.001f) ss << " +" << std::fixed << std::setprecision(1) << cs.armorPenetration << "%";
            }
            bonusStr = ss.str();

            text.setColor(isChecked ? (bonusStr.empty() ? sf::Color::White : sf::Color(100, 255, 200)) : sf::Color(170, 170, 170));
            text.setString(mAvailableStats[i].second + bonusStr);
            text.setPosition({ panelX + mSocketLabelsOffset.x * zoom, labelY });
            target.draw(text);

            socketY += mStatLineSpacing * zoom;
            labelY += mStatLineSpacing * zoom;
        }

        // 6. Cultivar Button (Active / Desactive)
        sf::Vector2f btnPos = mPosition + mButtonCultivarOffset * zoom;
        bool btnDrawn = false;
        try {
            std::string btnTexPath = !item->cultivoLocked ? "assets/ui/cultivo-panel/active_button.png"
                                                          : "assets/ui/cultivo-panel/desactive_button.png";
            sf::Texture& btnTex = res.getTexture(btnTexPath);
            sf::Sprite btnSprite(btnTex);
            btnSprite.setScale({ zoom, zoom });
            btnSprite.setPosition(btnPos);
            target.draw(btnSprite);
            btnDrawn = true;
        } catch (...) {}

        if (!btnDrawn) {
            if (!item->cultivoLocked) {
                sf::RectangleShape confirmBtn({ (mSize.x - 40.f) * zoom, 24.f * zoom });
                confirmBtn.setPosition(btnPos);
                confirmBtn.setFillColor(sf::Color(40, 140, 60));
                confirmBtn.setOutlineThickness(1.f);
                confirmBtn.setOutlineColor(sf::Color::White);
                target.draw(confirmBtn);

                BitmapText btnText;
                btnText.setTexture(mFontTexture);
                btnText.setScale({ cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE });
                btnText.setColor(sf::Color::White);
                btnText.setString(mButtonTextActive);
                btnText.setPosition({ panelX + mButtonTextOffset.x * zoom, panelY + mButtonTextOffset.y * zoom });
                target.draw(btnText);
            } else {
                text.setColor(sf::Color(0, 220, 130));
                text.setString(mButtonTextLocked);
                text.setPosition({ panelX + mButtonTextOffset.x * zoom, panelY + mButtonTextOffset.y * zoom });
                target.draw(text);
            }
        }
    }
}

bool CultivoPanel::onMousePress(sf::Vector2f mousePos) {
    if (!mVisible) return false;

    float zoom = cfg::Map::ZOOM_FACTOR;
    const float panelX = mPosition.x;
    const float panelY = mPosition.y;

    // Check Close Button
    float closeX = panelX + (mSize.x - 22.f) * zoom;
    float closeY = panelY + 8.f * zoom;
    sf::FloatRect closeBounds({ closeX, closeY }, { 18.f * zoom, 18.f * zoom });
    if (closeBounds.contains(mousePos)) {
        if (mOnCloseCallback) mOnCloseCallback();
        return true;
    }

    auto item = getItem();
    if (item && getBounds().contains(mousePos)) {
        // Check Stat Selection checkboxes (if not locked)
        if (!item->cultivoLocked) {
            float socketY = panelY + mSocketsOffset.y * zoom;
            for (size_t i = 0; i < mAvailableStats.size() && i < 6; ++i) {
                sf::FloatRect checkBounds({ panelX + mSocketsOffset.x * zoom, socketY }, { (mSize.x - 40.f) * zoom, mStatLineSpacing * zoom });
                if (checkBounds.contains(mousePos)) {
                    if (i < mSelectedFlags.size()) {
                        mSelectedFlags[i] = !mSelectedFlags[i];
                    }
                    return true;
                }
                socketY += mStatLineSpacing * zoom;
            }

            // Check Cultivar Button
            sf::Vector2f btnPos = mPosition + mButtonCultivarOffset * zoom;
            sf::FloatRect confirmBounds(btnPos, { 100.f * zoom, 30.f * zoom });
            if (confirmBounds.contains(mousePos)) {
                // Collect selected stats
                item->cultivoSelectedStats.clear();
                for (size_t i = 0; i < mAvailableStats.size() && i < mSelectedFlags.size(); ++i) {
                    if (mSelectedFlags[i]) {
                        item->cultivoSelectedStats.push_back(mAvailableStats[i].first);
                    }
                }

                if (!item->cultivoSelectedStats.empty()) {
                    item->cultivoLocked = true;
                    if (item->cultivoLevel < 1) item->cultivoLevel = 1;
                    CultivoSystem::recalculateCultivoBonusStats(*item);
                    if (mOnConfirmCallback) {
                        mOnConfirmCallback();
                    }
                    std::cout << "[CULTIVO] Confirmada seleccion de " << item->cultivoSelectedStats.size() 
                              << " stat(s) para " << item->name << "\n";
                }
                return true;
            }
        }
    }

    // Window Dragging
    sf::FloatRect titleBar(mPosition, { mSize.x * zoom, 24.f * zoom });
    if (titleBar.contains(mousePos)) {
        mIsBeingDragged = true;
        mDragOffset = mousePos - mPosition;
        return true;
    }

    if (getBounds().contains(mousePos)) {
        return true;
    }

    return false;
}

void CultivoPanel::onMouseMove(sf::Vector2f mousePos) {
    if (mIsBeingDragged) {
        mPosition = mousePos - mDragOffset;
    }
}

void CultivoPanel::onMouseRelease() {
    mIsBeingDragged = false;
}
