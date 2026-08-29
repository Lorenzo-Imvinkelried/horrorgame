#include "Tooltip.h"
#include "core/engine/ResourceManager.h"
#include "Config.h"
#include <cmath>

void Tooltip::draw(sf::RenderTarget &target, ResourceManager &res) {
  if (!mVisible)
    return;

  auto drawPanel = [&](const std::vector<TooltipLine> &lines,
                       sf::RectangleShape &bg) {
    target.draw(bg);
    sf::Vector2f pos = bg.getPosition();
    float currentY = std::floor(pos.y + PADDING);
    float startX = std::floor(pos.x + PADDING);

    BitmapText tempText;
    tempText.setTexture(mFontTexture);
    tempText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});

    for (const auto &line : lines) {
      if (line.parts.empty()) {
        currentY += 8.f;
        continue;
      }
      float currentX = startX;
      float maxH = 0.f;
      for (const auto &part : line.parts) {
        tempText.setString(part.text);
        tempText.setColor(part.color);
        tempText.setPosition({std::floor(currentX), std::floor(currentY)});
        target.draw(tempText);
        sf::FloatRect b = tempText.getGlobalBounds();
        currentX += b.size.x;
        if (b.size.y > maxH)
          maxH = b.size.y;
      }
      currentY += std::floor(maxH + LINE_SPACING);
    }

    // Draw sockets at the bottom of the tooltip panel
    const Item* panelItem = (&bg == &mBackground) ? mCurrentItem : mCurrentEquippedItem;
    if (panelItem && panelItem->maxSockets > 0) {
        float socketSize = 7.f * 2.f;
        float rowHeight = 16.f;
        float startSocketX = bg.getPosition().x + PADDING;
        float startSocketY = bg.getPosition().y + bg.getSize().y - PADDING - (panelItem->maxSockets * rowHeight);
        
        BitmapText socketText;
        socketText.setTexture(mFontTexture);
        socketText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
        
        for (int i = 0; i < panelItem->maxSockets; ++i) {
            sf::Vector2f socketPos(startSocketX, startSocketY + i * rowHeight);
            
            try {
                sf::Texture& socketTex = res.getTexture("assets/ui/stones_system/socket_stone.png");
                sf::Sprite socketSprite(socketTex);
                socketSprite.setScale({2.f, 2.f});
                socketSprite.setPosition(socketPos);
                target.draw(socketSprite);
            } catch (...) {}
            
            bool occupied = (i < (int)panelItem->socketedStones.size() && panelItem->socketedStones[i]);
            if (occupied) {
                auto stone = panelItem->socketedStones[i];
                try {
                    sf::Texture& stoneTex = res.getTexture(stone->texturePath);
                    sf::Sprite stoneSprite(stoneTex);
                    stoneSprite.setTextureRect(stone->textureRect);
                    stoneSprite.setScale({2.f, 2.f});
                    stoneSprite.setPosition(socketPos + sf::Vector2f(2.f, 2.f));
                    target.draw(stoneSprite);
                } catch (...) {}
            }
            
            if (occupied) {
                auto stone = panelItem->socketedStones[i];
                std::string textStr = stone->name + " (" + getStoneStatsString(*stone) + ")";
                sf::Color textColor = getQualityColor(stone->quality);
                if (textColor == sf::Color::Transparent) {
                    textColor = sf::Color(220, 220, 220);
                }
                
                socketText.setString(textStr);
                socketText.setColor(textColor);
                
                float textYOffset = (14.f - socketText.getGlobalBounds().size.y) * 0.5f;
                socketText.setPosition({socketPos.x + 14.f + 8.f, socketPos.y + textYOffset});
                target.draw(socketText);
            }
        }
    }
  };

  drawPanel(mLines, mBackground);

  if (mShowComparison) {
    drawPanel(mEquippedLines, mEquippedBackground);
  }
}
