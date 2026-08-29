#pragma once
#include <SFML/Graphics.hpp>
#include "core/items/Item.h"
#include "core/stats/Stats.h"
#include <string>
#include <vector>
#include "core/graphics/BitmapText.h"

class Skill;
struct Title;
class Player;
class ResourceManager;

class Tooltip {
public:
    Tooltip(sf::Texture* fontTexture);

    void show(const Item& item, sf::Vector2f position, sf::Vector2u windowSize, const Item* equippedItem = nullptr);
    void show(const Skill& skill, sf::Vector2f position, sf::Vector2u windowSize, Player* player = nullptr);
    void show(const Title& title, sf::Vector2f position, sf::Vector2u windowSize);
    void show(const std::string& name, const std::string& description, float remainingDuration, sf::Vector2f position, sf::Vector2u windowSize);

    void hide();
    void setPosition(sf::Vector2f position, sf::Vector2u windowSize);
    void draw(sf::RenderTarget& target, ResourceManager& res);

    struct TextPart {
        std::string text;
        sf::Color color;
    };
    
    struct TooltipLine {
        std::vector<TextPart> parts;
    };

    static std::string getStatDisplayName(Stat stat);
    static bool isStatInteger(Stat stat);
    static bool isStatPercent(Stat stat);
    static bool isDiffSignificant(float diff);
    static std::string getStoneStatsString(const Item& stone);

private:
    void addLine(std::vector<TooltipLine>& targetLines, const std::string& text, sf::Color color = sf::Color::White);
    void addStatLine(std::vector<TooltipLine>& targetLines, const std::string& label, float value, bool isInt = false, bool isPercent = false, sf::Color valueColor = sf::Color::White);
    void addComparisonStatLine(const std::string& label, float value, float equippedValue, bool isInt, bool isPercent, bool isLowerBetter, sf::Color textColor = sf::Color::White, int fortLevel = 0, float baseValue = 0.f, int eqFortLevel = 0, float eqBaseValue = 0.f);
    void populateItemContent(const Item& item, std::vector<TooltipLine>& lines, bool isEquippedHeader);
    void calculateSize(const std::vector<TooltipLine>& lines, sf::RectangleShape& bg, const Item* panelItem = nullptr);

private:
    sf::Texture* mFontTexture;
    std::vector<TooltipLine> mLines;
    std::vector<TooltipLine> mEquippedLines;
    
    sf::RectangleShape mBackground;
    sf::RectangleShape mEquippedBackground;
    
    bool mVisible;
    bool mShowComparison;
    
    const Item* mCurrentItem = nullptr;
    const Item* mCurrentEquippedItem = nullptr;
    
    static constexpr float PADDING = 10.f;
    static constexpr float LINE_SPACING = 4.f; 
};
