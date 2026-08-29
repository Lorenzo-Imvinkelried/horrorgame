#include "SkillDebugPanel.h"
#include "core/skills/SkillManager.h"
#include "core/skills/Skill.h"
#include "entities/player/Player.h"
#include "Config.h"
#include <iostream>
#include <algorithm>
#include <cmath>

SkillDebugPanel::SkillDebugPanel(sf::Texture* fontTexture)
    : mFontTexture(fontTexture) {
    mPosition = sf::Vector2f(120.f, 70.f);
    mSize = sf::Vector2f(420.f, 360.f);
}

void SkillDebugPanel::setPosition(sf::Vector2f pos) {
    mPosition = pos;
}

sf::FloatRect SkillDebugPanel::getBounds() const {
    return sf::FloatRect(mPosition, mSize);
}

void SkillDebugPanel::drawText(sf::RenderTarget& target, const std::string& text, sf::Vector2f pos, float scale, sf::Color color) {
    if (!mFontTexture || text.empty()) return;
    BitmapText bt;
    bt.setTexture(mFontTexture);
    bt.setScale({scale * cfg::UI::FONT_SCALE, scale * cfg::UI::FONT_SCALE});
    bt.setString(text);
    bt.setColor(color);
    bt.setPosition({std::floor(pos.x), std::floor(pos.y)});
    target.draw(bt);
}

void SkillDebugPanel::draw(sf::RenderTarget& target, ResourceManager& res) {
    if (!mVisible || !mSkillMgr) return;

    float zoom = cfg::Map::ZOOM_FACTOR;
    mSize = { 430.f * zoom, 360.f * zoom };

    const float panelX = std::floor(mPosition.x);
    const float panelY = std::floor(mPosition.y);
    const float padding = PADDING * zoom;
    const float headerH = HEADER_HEIGHT * zoom;

    // 1. Panel Background
    sf::RectangleShape bg(mSize);
    bg.setPosition({panelX, panelY});
    bg.setFillColor(sf::Color(18, 22, 32, 235));
    bg.setOutlineThickness(2.f * zoom);
    bg.setOutlineColor(sf::Color(70, 140, 220, 200));
    target.draw(bg);

    // 2. Header Bar (Draggable)
    sf::RectangleShape header({mSize.x, headerH});
    header.setPosition({panelX, panelY});
    header.setFillColor(sf::Color(32, 45, 70, 240));
    target.draw(header);

    // Title text
    drawText(target, "[DEBUG] SKILL PALETTE (TECLA V)", {panelX + padding, panelY + 4.f * zoom}, 1.1f, sf::Color(255, 215, 0));

    // Close button [X]
    float closeSize = 18.f * zoom;
    sf::RectangleShape closeBtn({closeSize, closeSize});
    closeBtn.setPosition({panelX + mSize.x - closeSize - 4.f * zoom, panelY + 4.f * zoom});
    closeBtn.setFillColor(sf::Color(180, 50, 50, 200));
    target.draw(closeBtn);
    drawText(target, "X", {panelX + mSize.x - closeSize + 2.f * zoom, panelY + 4.f * zoom}, 0.9f, sf::Color::White);

    // Subtitle instruction
    drawText(target, "Click a skill -> Press 1-9 to equip into Hotbar slot", 
             {panelX + padding, panelY + headerH + 4.f * zoom}, 0.85f, sf::Color(180, 220, 255));

    // Divider line
    sf::RectangleShape line({mSize.x - padding * 2.f, 1.f * zoom});
    line.setPosition({panelX + padding, panelY + headerH + 20.f * zoom});
    line.setFillColor(sf::Color(70, 140, 220, 100));
    target.draw(line);

    // 3. Grid area for Skills
    const float gridStartY = panelY + headerH + 26.f * zoom - mScrollOffset;
    const float cardW = 60.f * zoom;
    const float cardH = 64.f * zoom;
    const float marginX = 12.f * zoom;
    const float marginY = 12.f * zoom;
    const int cols = 6;

    const auto& skillsMap = mSkillMgr->getSkills();
    int index = 0;

    mHoveredSkillId = -1;
    sf::Vector2i mousePosI = sf::Mouse::getPosition(); // Screen mouse check via bounds
    sf::Vector2f mousePos = target.mapPixelToCoords(mousePosI);

    // Scissor / clipping area check (keep skills inside panel content area)
    float contentTop = panelY + headerH + 22.f * zoom;
    float contentBottom = panelY + mSize.y - 70.f * zoom; // leave space at bottom for skill preview info

    for (const auto& [skillId, skillPtr] : skillsMap) {
        if (!skillPtr) continue;

        int row = index / cols;
        int col = index % cols;
        float cardX = panelX + padding + col * (cardW + marginX);
        float cardY = gridStartY + row * (cardH + marginY);

        index++;

        // Only draw if within visible content bounds
        if (cardY + cardH < contentTop || cardY > contentBottom) {
            continue;
        }

        sf::FloatRect cardRect({cardX, cardY}, {cardW, cardH});
        bool isHovered = cardRect.contains(mousePos);
        bool isSelected = (skillId == mSelectedSkillId);

        if (isHovered) {
            mHoveredSkillId = skillId;
        }

        // Card Background
        sf::RectangleShape cardBg({cardW, cardH});
        cardBg.setPosition({cardX, cardY});
        if (isSelected) {
            cardBg.setFillColor(sf::Color(70, 80, 40, 230));
            cardBg.setOutlineThickness(2.f * zoom);
            cardBg.setOutlineColor(sf::Color(255, 215, 0, 255)); // Gold border
        } else if (isHovered) {
            cardBg.setFillColor(sf::Color(45, 65, 95, 230));
            cardBg.setOutlineThickness(1.5f * zoom);
            cardBg.setOutlineColor(sf::Color(100, 200, 255, 255)); // Cyan border
        } else {
            cardBg.setFillColor(sf::Color(25, 32, 48, 200));
            cardBg.setOutlineThickness(1.f * zoom);
            cardBg.setOutlineColor(sf::Color(60, 80, 110, 160));
        }
        target.draw(cardBg);

        // Skill Icon rendering with PowerStrike default fallback
        float iconSize = 36.f * zoom;
        float iconX = cardX + (cardW - iconSize) / 2.f;
        float iconY = cardY + 4.f * zoom;

        const sf::Texture* tex = skillPtr ? skillPtr->iconTexture : nullptr;
        if (!tex) {
            try {
                std::string path = (skillPtr && !skillPtr->iconPath.empty()) 
                    ? skillPtr->iconPath 
                    : "assets/ui/skills/atlas_skills_18x18x10.png";
                tex = &res.getTexture(path);
            } catch (...) {
                try {
                    tex = &res.getTexture("assets/ui/skills/atlas_skills_18x18x10.png");
                } catch (...) {}
            }
        }

        int drawAtlasX = 91; // Default PowerStrike icon (Col 5, Row 0)
        int drawAtlasY = 1;

        if (skillPtr) {
            if (skillPtr->atlasX != 0 || skillPtr->atlasY != 0) {
                drawAtlasX = skillPtr->atlasX;
                drawAtlasY = skillPtr->atlasY;
            }
        } else {
            // Empty / unused slot: icon at bottom right of atlas (Col 9, Row 9)
            drawAtlasX = 163;
            drawAtlasY = 163;
        }

        if (tex) {
            sf::Sprite iconSprite(*tex);
            iconSprite.setTextureRect(sf::IntRect({drawAtlasX, drawAtlasY}, {16, 16}));
            float scale = iconSize / 16.f;
            iconSprite.setScale({scale, scale});
            iconSprite.setPosition({iconX, iconY});
            target.draw(iconSprite);
        } else {
            sf::RectangleShape fallbackIcon({iconSize, iconSize});
            fallbackIcon.setPosition({iconX, iconY});
            fallbackIcon.setFillColor(sf::Color(50, 50, 70));
            target.draw(fallbackIcon);
        }

        // Skill ID Badge
        std::string idStr = "ID:" + std::to_string(skillId);
        drawText(target, idStr, {cardX + 2.f * zoom, cardY + cardH - 18.f * zoom}, 0.7f, 
                 isSelected ? sf::Color(255, 215, 0) : sf::Color(200, 200, 200));
    }

    // 4. Bottom Info Box for Hovered / Selected Skill
    sf::RectangleShape infoBox({mSize.x - padding * 2.f, 58.f * zoom});
    infoBox.setPosition({panelX + padding, panelY + mSize.y - 64.f * zoom});
    infoBox.setFillColor(sf::Color(12, 16, 24, 230));
    infoBox.setOutlineThickness(1.f * zoom);
    infoBox.setOutlineColor(sf::Color(60, 90, 130, 180));
    target.draw(infoBox);

    int activeInfoId = (mHoveredSkillId != -1) ? mHoveredSkillId : mSelectedSkillId;
    const Skill* activeSkill = mSkillMgr->getSkill(activeInfoId);

    if (activeSkill) {
        std::string nameLine = "[" + std::to_string(activeSkill->id) + "] " + activeSkill->name;
        std::string statLine = "CD: " + std::to_string(activeSkill->cooldown) + "s | Mana: " + std::to_string(activeSkill->manaCost);
        std::string descLine = activeSkill->description;
        if (descLine.length() > 42) descLine = descLine.substr(0, 40) + "...";

        drawText(target, nameLine, {panelX + padding + 6.f * zoom, panelY + mSize.y - 60.f * zoom}, 0.85f, sf::Color(255, 215, 0));
        drawText(target, statLine, {panelX + padding + 6.f * zoom, panelY + mSize.y - 44.f * zoom}, 0.75f, sf::Color(180, 220, 255));
        drawText(target, descLine, {panelX + padding + 6.f * zoom, panelY + mSize.y - 28.f * zoom}, 0.7f, sf::Color(200, 200, 200));
    } else {
        drawText(target, "Haz clic en una habilidad para seleccionarla.", 
                 {panelX + padding + 6.f * zoom, panelY + mSize.y - 42.f * zoom}, 0.8f, sf::Color(140, 160, 180));
    }
}

bool SkillDebugPanel::onMousePress(sf::Vector2f mousePos) {
    if (!mVisible) return false;

    float zoom = cfg::Map::ZOOM_FACTOR;
    const float panelX = mPosition.x;
    const float panelY = mPosition.y;
    const float headerH = HEADER_HEIGHT * zoom;

    sf::FloatRect panelBounds(mPosition, mSize);
    if (!panelBounds.contains(mousePos)) {
        return false;
    }

    // Close button click
    float closeSize = 18.f * zoom;
    sf::FloatRect closeBounds({panelX + mSize.x - closeSize - 4.f * zoom, panelY + 4.f * zoom}, {closeSize, closeSize});
    if (closeBounds.contains(mousePos)) {
        mVisible = false;
        if (mOnCloseCallback) mOnCloseCallback();
        return true;
    }

    // Header drag
    sf::FloatRect headerBounds(mPosition, {mSize.x, headerH});
    if (headerBounds.contains(mousePos)) {
        mIsBeingDragged = true;
        mDragOffset = mousePos - mPosition;
        return true;
    }

    // Skill card selection
    if (mSkillMgr) {
        const float padding = PADDING * zoom;
        const float gridStartY = panelY + headerH + 26.f * zoom - mScrollOffset;
        const float cardW = 60.f * zoom;
        const float cardH = 64.f * zoom;
        const float marginX = 12.f * zoom;
        const float marginY = 12.f * zoom;
        const int cols = 6;

        const auto& skillsMap = mSkillMgr->getSkills();
        int index = 0;

        for (const auto& [skillId, skillPtr] : skillsMap) {
            if (!skillPtr) continue;

            int row = index / cols;
            int col = index % cols;
            float cardX = panelX + padding + col * (cardW + marginX);
            float cardY = gridStartY + row * (cardH + marginY);
            index++;

            sf::FloatRect cardRect({cardX, cardY}, {cardW, cardH});
            if (cardRect.contains(mousePos)) {
                mSelectedSkillId = skillId;
                std::cout << "[SkillDebugPanel] Selected skill ID " << skillId << " (" << skillPtr->name << ")\n";
                return true;
            }
        }
    }

    return true;
}

void SkillDebugPanel::onMouseMove(sf::Vector2f mousePos) {
    if (mIsBeingDragged) {
        mPosition = mousePos - mDragOffset;
    }
}

void SkillDebugPanel::onMouseRelease() {
    mIsBeingDragged = false;
}

void SkillDebugPanel::onMouseWheel(int delta) {
    if (!mVisible || !mSkillMgr) return;
    float zoom = cfg::Map::ZOOM_FACTOR;
    const float headerH = HEADER_HEIGHT * zoom;
    const float cardH = 64.f * zoom;
    const float marginY = 12.f * zoom;
    const int cols = 6;

    int totalSkills = static_cast<int>(mSkillMgr->getSkills().size());
    int totalRows = (totalSkills + cols - 1) / cols;
    float totalGridH = totalRows * (cardH + marginY);
    float visibleH = mSize.y - (headerH + 26.f * zoom + 70.f * zoom);
    float maxScroll = std::max(0.f, totalGridH - visibleH + marginY);

    mScrollOffset -= delta * 20.f;
    if (mScrollOffset < 0.f) mScrollOffset = 0.f;
    if (mScrollOffset > maxScroll) mScrollOffset = maxScroll;
}

bool SkillDebugPanel::onKeyPress(sf::Keyboard::Key key) {
    if (!mVisible || mSelectedSkillId == -1 || !mPlayer) {
        return false;
    }

    int slotIndex = -1;

    // Detect keys 1..9 and 0 (Number row and Numpad)
    if (key == sf::Keyboard::Key::Num1 || key == sf::Keyboard::Key::Numpad1) slotIndex = 0;
    else if (key == sf::Keyboard::Key::Num2 || key == sf::Keyboard::Key::Numpad2) slotIndex = 1;
    else if (key == sf::Keyboard::Key::Num3 || key == sf::Keyboard::Key::Numpad3) slotIndex = 2;
    else if (key == sf::Keyboard::Key::Num4 || key == sf::Keyboard::Key::Numpad4) slotIndex = 3;
    else if (key == sf::Keyboard::Key::Num5 || key == sf::Keyboard::Key::Numpad5) slotIndex = 4;
    else if (key == sf::Keyboard::Key::Num6 || key == sf::Keyboard::Key::Numpad6) slotIndex = 5;
    else if (key == sf::Keyboard::Key::Num7 || key == sf::Keyboard::Key::Numpad7) slotIndex = 6;
    else if (key == sf::Keyboard::Key::Num8 || key == sf::Keyboard::Key::Numpad8) slotIndex = 7;
    else if (key == sf::Keyboard::Key::Num9 || key == sf::Keyboard::Key::Numpad9) slotIndex = 8;
    else if (key == sf::Keyboard::Key::Num0 || key == sf::Keyboard::Key::Numpad0) slotIndex = 9;

    if (slotIndex != -1) {
        mPlayer->equipSkill(slotIndex, mSelectedSkillId);
        std::cout << "[SkillDebugPanel] Equipada habilidad ID " << mSelectedSkillId 
                  << " en slot de Hotbar " << (slotIndex + 1) << "!\n";
        return true;
    }

    return false;
}
