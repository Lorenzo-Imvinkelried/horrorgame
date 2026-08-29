#include "SkillLevelUpPanel.h"
#include "entities/player/Player.h"
#include "core/skills/SkillManager.h"
#include "core/skills/Skill.h"
#include "core/systems/SkillUpgradeSystem.h"
#include "core/systems/GoldSystem.h"
#include "Config.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <iomanip>

SkillLevelUpPanel::SkillLevelUpPanel(sf::Texture* fontTexture)
    : mFontTexture(fontTexture)
{
    float zoom = cfg::Map::ZOOM_FACTOR;
    mSize = { 260.f * zoom, 280.f * zoom };
    mPosition = { 150.f, 100.f };
}

void SkillLevelUpPanel::setPosition(sf::Vector2f pos) {
    mPosition = pos;
}

sf::FloatRect SkillLevelUpPanel::getBounds() const {
    return sf::FloatRect(mPosition, mSize);
}

void SkillLevelUpPanel::drawText(sf::RenderTarget& target, const std::string& text, sf::Vector2f pos, float scale, sf::Color color) {
    if (!mFontTexture || text.empty()) return;
    BitmapText bt;
    bt.setTexture(mFontTexture);
    bt.setScale({scale * cfg::UI::FONT_SCALE, scale * cfg::UI::FONT_SCALE});
    bt.setString(text);
    bt.setColor(color);
    bt.setPosition({std::floor(pos.x), std::floor(pos.y)});
    target.draw(bt);
}

void SkillLevelUpPanel::draw(sf::RenderTarget& target, ResourceManager& res) {
    if (!mVisible) return;

    float zoom = cfg::Map::ZOOM_FACTOR;
    mSize = { 260.f * zoom, 280.f * zoom };

    const float panelX = std::floor(mPosition.x);
    const float panelY = std::floor(mPosition.y);
    const float padding = PADDING * zoom;

    // 1. Draw Panel Background
    sf::RectangleShape bg(mSize);
    bg.setPosition({panelX, panelY});
    bg.setFillColor(sf::Color(15, 15, 22, 235));
    bg.setOutlineThickness(1.5f * zoom);
    bg.setOutlineColor(sf::Color(212, 175, 55, 255)); // Gold border
    target.draw(bg);

    // 2. Draw Panel Header
    drawText(target, "MEJORA DE HABILIDADES", {panelX + padding, panelY + padding}, 1.1f, sf::Color(255, 215, 0));

    // 3. Draw Close Button [X]
    sf::FloatRect closeBtnRect(
        { panelX + mSize.x - 22.f * zoom, panelY + padding },
        { 16.f * zoom, 16.f * zoom }
    );
    sf::RectangleShape closeBg(closeBtnRect.size);
    closeBg.setPosition(closeBtnRect.position);
    closeBg.setFillColor(sf::Color(80, 20, 20, 200));
    target.draw(closeBg);
    drawText(target, "X", {closeBtnRect.position.x + 3.f * zoom, closeBtnRect.position.y + 1.f * zoom}, 1.0f, sf::Color::White);

    // 4. Scrollable Container Viewport
    const float listStartY = panelY + HEADER_HEIGHT * zoom + padding;
    const float listHeight = mSize.y - (HEADER_HEIGHT * zoom + padding * 2.f);
    const float itemWidth = mSize.x - padding * 2.f;

    if (!mSkillMgr) return;

    auto& skillMap = mSkillMgr->getSkills();
    std::vector<const Skill*> skillsList;
    for (const auto& pair : skillMap) {
        if (pair.second) {
            skillsList.push_back(pair.second.get());
        }
    }

    // Sort by skill ID
    std::sort(skillsList.begin(), skillsList.end(), [](const Skill* a, const Skill* b) {
        return a->id < b->id;
    });

    float contentHeight = skillsList.size() * (ROW_HEIGHT * zoom + 4.f * zoom);
    float maxScroll = std::max(0.f, contentHeight - listHeight);
    mScrollOffset = std::clamp(mScrollOffset, 0.f, maxScroll);

    // Set scissor/clip rect if needed, or simple position bounds check
    float currentY = listStartY - mScrollOffset;
    int index = 0;

    for (const Skill* skill : skillsList) {
        if (!skill) continue;

        float itemY = currentY;
        currentY += ROW_HEIGHT * zoom + 4.f * zoom;

        // Clip items outside visible list area
        if (itemY + ROW_HEIGHT * zoom < listStartY || itemY > listStartY + listHeight) {
            index++;
            continue;
        }

        sf::FloatRect rowRect({panelX + padding, itemY}, {itemWidth, ROW_HEIGHT * zoom});

        // Row background
        sf::RectangleShape rowBg(rowRect.size);
        rowBg.setPosition(rowRect.position);
        rowBg.setFillColor(sf::Color(25, 28, 38, 220));
        rowBg.setOutlineThickness(1.f * zoom);
        rowBg.setOutlineColor(sf::Color(50, 60, 80, 200));
        target.draw(rowBg);

        // Draw Icon if available (PowerStrike fallback if zero/missing)
        float iconSize = 24.f * zoom;
        sf::Vector2f iconPos = {rowRect.position.x + 4.f * zoom, rowRect.position.y + 7.f * zoom};
        const sf::Texture* tex = skill->iconTexture;
        if (!tex) {
            try {
                std::string path = (!skill->iconPath.empty()) ? skill->iconPath : "assets/ui/skills/atlas_skills_18x18x10.png";
                tex = &res.getTexture(path);
            } catch (...) {
                try { tex = &res.getTexture("assets/ui/skills/atlas_skills_18x18x10.png"); } catch(...) {}
            }
        }

        int drawAtlasX = (skill->atlasX != 0 || skill->atlasY != 0) ? skill->atlasX : 91;
        int drawAtlasY = (skill->atlasX != 0 || skill->atlasY != 0) ? skill->atlasY : 1;

        if (tex) {
            sf::Sprite iconSprite(*tex);
            iconSprite.setTextureRect(sf::IntRect({drawAtlasX, drawAtlasY}, {16, 16}));
            iconSprite.setScale({iconSize / 16.f, iconSize / 16.f});
            iconSprite.setPosition(iconPos);
            target.draw(iconSprite);
        } else {
            sf::RectangleShape fallbackIcon({iconSize, iconSize});
            fallbackIcon.setPosition(iconPos);
            fallbackIcon.setFillColor(sf::Color(60, 60, 80));
            target.draw(fallbackIcon);
        }

        // Skill Name & Level
        int level = SkillUpgradeSystem::getInstance().getSkillLevel(skill->id);
        double bonus = SkillUpgradeSystem::getInstance().getBonusPercent(skill->id);

        std::string nameText = skill->name;
        if (nameText.size() > 14) nameText = nameText.substr(0, 12) + "..";
        drawText(target, nameText, {rowRect.position.x + iconSize + 8.f * zoom, rowRect.position.y + 3.f * zoom}, 0.95f, sf::Color(240, 240, 255));

        std::stringstream ssLvl;
        ssLvl << "Niv." << level << " (+" << static_cast<int>(std::round(bonus)) << "%)";
        drawText(target, ssLvl.str(), {rowRect.position.x + iconSize + 8.f * zoom, rowRect.position.y + 18.f * zoom}, 0.8f, sf::Color(180, 220, 180));

        // Cost & Upgrade Button
        uint64_t costBronze = SkillUpgradeSystem::getInstance().getUpgradeCost(skill->id);
        GoldSplit split = GoldSystem::splitCoins(costBronze);

        std::stringstream ssCost;
        if (split.gold > 0) ssCost << split.gold << "g ";
        if (split.silver > 0 || split.gold > 0) ssCost << split.silver << "s ";
        ssCost << (int)split.bronze << "b";

        // Button [ + ] / [ MEJORAR ]
        sf::FloatRect btnRect(
            {rowRect.position.x + rowRect.size.x - 62.f * zoom, rowRect.position.y + 6.f * zoom},
            {58.f * zoom, 24.f * zoom}
        );

        bool canAfford = SkillUpgradeSystem::getInstance().canUpgrade(mPlayer, skill->id);
        bool isHovered = (mHoveredUpgradeIndex == index);

        sf::RectangleShape btn(btnRect.size);
        btn.setPosition(btnRect.position);

        if (!canAfford) {
            btn.setFillColor(sf::Color(50, 40, 40, 180));
            btn.setOutlineColor(sf::Color(80, 60, 60, 200));
        } else if (isHovered) {
            btn.setFillColor(sf::Color(60, 160, 70, 240));
            btn.setOutlineColor(sf::Color(255, 220, 100, 255));
        } else {
            btn.setFillColor(sf::Color(40, 120, 50, 220));
            btn.setOutlineColor(sf::Color(100, 200, 110, 255));
        }
        btn.setOutlineThickness(1.f * zoom);
        target.draw(btn);

        // Draw Cost & "+1" text inside/above button
        drawText(target, "+1 Lvl", {btnRect.position.x + 8.f * zoom, btnRect.position.y + 2.f * zoom}, 0.75f, sf::Color::White);
        drawText(target, ssCost.str(), {btnRect.position.x + 3.f * zoom, btnRect.position.y + 12.f * zoom}, 0.75f, canAfford ? sf::Color(255, 230, 150) : sf::Color(220, 120, 120));

        index++;
    }
}

void SkillLevelUpPanel::onMouseMove(sf::Vector2f mousePos) {
    if (!mVisible) return;

    if (mIsBeingDragged) {
        mPosition = mousePos - mDragOffset;
        return;
    }

    float zoom = cfg::Map::ZOOM_FACTOR;
    const float panelX = std::floor(mPosition.x);
    const float panelY = std::floor(mPosition.y);
    const float padding = PADDING * zoom;
    const float listStartY = panelY + HEADER_HEIGHT * zoom + padding;
    const float listHeight = mSize.y - (HEADER_HEIGHT * zoom + padding * 2.f);
    const float itemWidth = mSize.x - padding * 2.f;

    mHoveredUpgradeIndex = -1;
    mHoveredSkillId = -1;

    if (!mSkillMgr) return;

    auto& skillMap = mSkillMgr->getSkills();
    std::vector<const Skill*> skillsList;
    for (const auto& pair : skillMap) {
        if (pair.second) skillsList.push_back(pair.second.get());
    }
    std::sort(skillsList.begin(), skillsList.end(), [](const Skill* a, const Skill* b) { return a->id < b->id; });

    float currentY = listStartY - mScrollOffset;
    int index = 0;

    for (const Skill* skill : skillsList) {
        if (!skill) continue;

        float itemY = currentY;
        currentY += ROW_HEIGHT * zoom + 4.f * zoom;

        if (itemY + ROW_HEIGHT * zoom < listStartY || itemY > listStartY + listHeight) {
            index++;
            continue;
        }

        sf::FloatRect rowRect({panelX + padding, itemY}, {itemWidth, ROW_HEIGHT * zoom});
        if (rowRect.contains(mousePos)) {
            mHoveredSkillId = skill->id;
        }

        sf::FloatRect btnRect(
            {rowRect.position.x + rowRect.size.x - 62.f * zoom, rowRect.position.y + 6.f * zoom},
            {58.f * zoom, 24.f * zoom}
        );

        if (btnRect.contains(mousePos)) {
            mHoveredUpgradeIndex = index;
            break;
        }
        index++;
    }
}

bool SkillLevelUpPanel::onMousePress(sf::Vector2f mousePos) {
    if (!mVisible) return false;

    sf::FloatRect panelBounds = getBounds();
    if (!panelBounds.contains(mousePos)) return false;

    float zoom = cfg::Map::ZOOM_FACTOR;
    const float panelX = std::floor(mPosition.x);
    const float panelY = std::floor(mPosition.y);
    const float padding = PADDING * zoom;

    // Check Close Button [X]
    sf::FloatRect closeBtnRect(
        { panelX + mSize.x - 22.f * zoom, panelY + padding },
        { 16.f * zoom, 16.f * zoom }
    );
    if (closeBtnRect.contains(mousePos)) {
        if (mOnCloseCallback) mOnCloseCallback();
        return true;
    }

    // Check Header Area for Dragging
    sf::FloatRect headerRect({panelX, panelY}, {mSize.x - 30.f * zoom, HEADER_HEIGHT * zoom + padding});
    if (headerRect.contains(mousePos)) {
        mIsBeingDragged = true;
        mDragOffset = mousePos - mPosition;
        return true;
    }

    // Check Upgrade Button Clicks
    const float listStartY = panelY + HEADER_HEIGHT * zoom + padding;
    const float listHeight = mSize.y - (HEADER_HEIGHT * zoom + padding * 2.f);
    const float itemWidth = mSize.x - padding * 2.f;

    if (mSkillMgr) {
        auto& skillMap = mSkillMgr->getSkills();
        std::vector<const Skill*> skillsList;
        for (const auto& pair : skillMap) {
            if (pair.second) skillsList.push_back(pair.second.get());
        }
        std::sort(skillsList.begin(), skillsList.end(), [](const Skill* a, const Skill* b) { return a->id < b->id; });

        float currentY = listStartY - mScrollOffset;
        int index = 0;

        for (const Skill* skill : skillsList) {
            if (!skill) continue;

            float itemY = currentY;
            currentY += ROW_HEIGHT * zoom + 4.f * zoom;

            if (itemY + ROW_HEIGHT * zoom < listStartY || itemY > listStartY + listHeight) {
                index++;
                continue;
            }

            sf::FloatRect rowRect({panelX + padding, itemY}, {itemWidth, ROW_HEIGHT * zoom});
            sf::FloatRect btnRect(
                {rowRect.position.x + rowRect.size.x - 62.f * zoom, rowRect.position.y + 6.f * zoom},
                {58.f * zoom, 24.f * zoom}
            );

            if (btnRect.contains(mousePos)) {
                if (mPlayer) {
                    bool upgraded = SkillUpgradeSystem::getInstance().upgradeSkill(mPlayer, skill->id);
                    if (upgraded) {
                        std::cout << "[SkillLevelUpPanel] ¡Habilidad " << skill->name << " mejorada con éxito!\n";
                    }
                }
                return true;
            }
            index++;
        }
    }

    return true; // Click consumed by panel
}

void SkillLevelUpPanel::onMouseRelease() {
    mIsBeingDragged = false;
}

void SkillLevelUpPanel::onMouseWheel(int delta) {
    if (!mVisible || !mSkillMgr) return;
    float zoom = cfg::Map::ZOOM_FACTOR;
    const float padding = PADDING * zoom;
    const float listHeight = mSize.y - (HEADER_HEIGHT * zoom + padding * 2.f);

    float contentHeight = mSkillMgr->getSkills().size() * (ROW_HEIGHT * zoom + 4.f * zoom);
    float maxScroll = std::max(0.f, contentHeight - listHeight);

    mScrollOffset -= delta * 20.f * zoom;
    mScrollOffset = std::clamp(mScrollOffset, 0.f, maxScroll);
}
