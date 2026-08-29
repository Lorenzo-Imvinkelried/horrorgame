#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <string>
#include <tuple>
#include <sstream>
#include <iomanip>
#include <functional>
#include "entities/player/Player.h"
#include "core/engine/ResourceManager.h"
#include "core/ui/UIPanel.h"
#include "Config.h"
#include "core/graphics/BitmapText.h"

class CharacterPanel : public UIPanel {
public:
    CharacterPanel(sf::Texture* fontTexture, std::optional<sf::Sprite>& slotSprite);

    void setEntity(Entity* entity);
    void notifyEntityDeath(Entity* entity);
    void draw(sf::RenderTarget& target, ResourceManager& res) override;
    void draw(sf::RenderTarget& target, Entity* entity, ResourceManager &res);
    Entity* getEntity() const { return mEntity; }

    void setHiddenSlot(int slotIndex);

    int getSlotAt(sf::Vector2f mousePos) const;
    std::string getSlotName(int index) const;

    std::shared_ptr<Item> getItemAt(sf::Vector2f mousePos, Entity* entity) const;

    void setPosition(sf::Vector2f pos) override;
    sf::FloatRect getBounds() const override;
    void setVisible(bool visible) override;
    
    void setOnCloseCallback(std::function<void()> callback);

    bool onMousePress(sf::Vector2f mousePos) override;
    void onMouseRelease() override;
    void onMouseMove(sf::Vector2f mousePos) override;
    bool isBeingDragged() const override { return mIsBeingDragged; }

    void setDebugLayoutMode(bool enabled);
    bool isDebugLayoutMode() const { return mDebugLayoutMode; }
    static void loadLayout();
    static void saveLayout();

    struct SlotConfig {
        std::string name;
        sf::Vector2f offset;
    };
    static std::vector<SlotConfig> sSlotLayouts;
    static bool sLayoutInitialized;

private:
    void drawSlotFallback(sf::RenderTarget& target, sf::Vector2f pos);
    void updateTexts();
   
    Entity* mEntity = nullptr;
    int mObserverId = -1;
    std::function<void()> mOnCloseCallback;

    // Cached Strings
    std::string mLvlStr;
    std::string mStrStr, mAgiStr, mIntStr, mVitStr;
    std::string mHpStr, mMpStr;
    std::string mWeightStr;
    std::string mAtkStr, mDefStr, mAtkSpdStr, mCritStr, mCritDmgStr;
    std::string mPenStr, mPenPctStr, mAccStr, mLifeStealStr;
    std::string mAoeRngStr, mAoeDmgStr, mDblHitStr, mTriHitStr, mHpDmgStr;
    std::string mMoveSpdStr;
    std::string mBlockStr, mThornsStr, mEvasionStr, mHpRegenStr, mMpRegenStr;
    std::string mExecDmgStr, mExecThrStr, mTrueDmgStr;
    std::string mBleedDurationFlatStr, mBleedFlatDmgStr, mBleedDurationPctStr, mBleedPctDmgStr;
    std::string mStunStr, mSlowMovStr, mSlowAtkStr;
    std::string mAtkBonusStr, mRangeStr; 
   
    std::string mTenacityStr, mDmgRedStr, mCritAvoidStr;
    std::string mAntiPenStr, mManaStealStr, mXpBonusStr, mCdrStr; 
    std::string mMaliceStr; 

    // Cached Colors
    sf::Color mAtkSpdColor = sf::Color::White;
    sf::Color mMoveSpdColor = sf::Color::White;

private:
    sf::Texture* mFontTexture;
    std::optional<sf::Sprite>& mSlotSprite;

    sf::Vector2f mPosition;
    sf::Vector2f mSize;
    bool         mIsBeingDragged = false;
    sf::Vector2f mDragOffset;
    
    std::optional<sf::Sprite> mBackgroundSprite;
    bool mHasValidBackground = false;

    int mHiddenSlotIndex = -1;

    bool mDebugLayoutMode = false;
    int mSelectedDebugSlotIndex = -1;
    sf::Vector2f mDebugDragStartOffset;
    sf::Vector2f mDebugDragStartMousePos;

    static constexpr float UNIFIED_SLOT_SIZE = 60.f;
    static constexpr float UNIFIED_SLOT_MARGIN = 6.f;
    static constexpr float UI_MARGIN = 10.f;
};
