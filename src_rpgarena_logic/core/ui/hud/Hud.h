#pragma once
#include <SFML/Graphics.hpp>
#include <optional>
#include <vector>
#include <memory>
#include <algorithm> 
#include <cstddef> // size_t
#include "core/engine/ResourceManager.h"
#include "core/ui/PlayerFrame.h"
#include "core/ui/character/CharacterPanel.h" 
#include "core/ui/InventoryPanel.h" 
#include "core/items/Item.h"
#include "core/ui/TargetFrame.h" 
#include "core/ui/tooltip/Tooltip.h" 
#include "core/ui/MapPanel.h"
#include "core/systems/DragDropSystem.h"
#include "core/ui/ChatBox.h"
#include "core/ui/FortifyPanel.h"
#include "core/ui/ItemDebugPanel.h"
#include "core/graphics/BitmapText.h"
#include "core/ui/TitlesPanel.h"
#include "core/ui/CultivoPanel.h"
#include "core/ui/SkillLevelUpPanel.h"
#include "core/ui/SkillDebugPanel.h"
#include "core/systems/StatusEffectSystem.h"

class Player; 
class Entity; 
class ItemManager;
class InputManager;
class ConsoleRedirector;
class SkillManager;
class CommandManager;
class CombatSystem;

class Hud {
public:
    Hud();
    ~Hud();

    void load(ResourceManager& res);
    void updateLayout(float width, float height);
    sf::Texture& getFontTexture() { return mFontTexture; } 
    void updateRTs(Player* player, Entity* targetEntity, const ChunkedTileMap& map, const DecorSystem& decor, const class EntityManager* entityMgr = nullptr, const class TerrainDeformSystem* terrainDeform = nullptr);
    void draw(sf::RenderTarget& target, Player* player, Entity* targetedEntity, ResourceManager& res, const SkillManager& skillMgr);
    
    void setTarget(Entity* target);
    void notifyEntityDeath(Entity* entity);
    
    void updateMap(const ChunkedTileMap& map, const DecorSystem& decor, sf::Vector2f playerPos);
    
    void setCombatStatus(const std::string& text, sf::Color color = sf::Color::White);
    void addTestItemToInventory(ItemManager& itemMgr); 
    void addAllItemsToInventory(ItemManager& itemMgr, Player* player = nullptr, int targetLevel = -1); 
    void setItemManager(class ItemManager* itemMgr);

    void handleEvent(const sf::Event& event, Player* player);

    // Input Handling
    bool handleMousePress(sf::Vector2f mousePos);
    void handleMouseRelease();
    void handleMouseMove(sf::Vector2f mousePos);
    void handleScroll(int delta);

    // Chat text-field mouse (pass pre-mapped logical coordinates)
    bool handleChatTextPress(sf::Vector2f logicalPos);
    void handleChatTextMove(sf::Vector2f logicalPos);
    void handleChatTextRelease();

    bool handleRightClick(sf::Vector2f mousePos, Player* player, ResourceManager& res, const InputManager& input);

    using WorldDropCallback = std::function<void(std::shared_ptr<Item>)>;
    void setOnWorldDropCallback(WorldDropCallback cb) { mWorldDropCallback = cb; }

    void setHoveredWorldItem(std::shared_ptr<Item> item) { mHoveredWorldItem = item; }

    void clearInventory(); 
    int findEmptyInventorySlot();
    bool addItemToInventory(std::shared_ptr<Item> item);
    sf::FloatRect getTargetFrameBounds() const { return mTargetFrame.getBounds(); } 

    // Toggles
    void toggleInventory();
    bool isInventoryOpen() const { return mInventoryOpen; }
    
    void toggleFortify();
    bool isFortifyOpen() const { return mFortifyPanel.isVisible(); }

    void toggleItemDebug();
    bool isItemDebugOpen() const { return mItemDebugPanel.isVisible(); }

    void toggleCharacterPanel();
    bool isCharacterPanelOpen() const { return mCharacterPanelOpen; }

    void toggleTitlesPanel();
    bool isTitlesPanelOpen() const { return mTitlesPanelOpen; }

    void toggleCultivoPanel();
    bool isCultivoPanelOpen() const { return mCultivoPanel.isVisible(); }

    void toggleSkillLevelUp();
    bool isSkillLevelUpOpen() const { return mSkillLevelUpPanel.isVisible(); }

    void toggleSkillDebug();
    bool isSkillDebugOpen() const { return mSkillDebugPanel.isVisible(); }
    SkillDebugPanel& getSkillDebugPanel() { return mSkillDebugPanel; }

    void setInspectionPanel(CharacterPanel* panel) {
        mInspectionPanelPtr = panel;
        if (panel) {
            if (std::find(mPanels.begin(), mPanels.end(), panel) == mPanels.end()) {
                mPanels.push_back(panel);
            }
        }
    }
    void setCombatSystem(class CombatSystem* cs) { mCombatSystem = cs; }
    void bringToFront(class UIPanel* panel) {
        if (!panel) return;
        auto it = std::find(mPanels.begin(), mPanels.end(), panel);
        if (it != mPanels.end()) {
            mPanels.erase(it);
            mPanels.push_back(panel);
        }
    }

    void toggleMap() { mMapPanel.toggle(); }
    bool isMapOpen() const { return mMapPanel.isOpen(); }

    bool isPanelOpen() const { 
        return mInventoryOpen || mCharacterPanelOpen || mMapPanel.isOpen() || mFortifyPanel.isVisible() || mItemDebugPanel.isVisible(); 
    }

    void closeAllPanels(Player* player);

    bool isChatFocused() const { return mChatBox.isFocused(); }
    bool isMouseOverChat(sf::Vector2f pos) const { return mChatBox.contains(pos); }
    void setChatFocused(bool focused) { mChatBox.setFocus(focused); }
    
    // FPS Counter
    void updateFps(sf::Time dt, int activeEntities = 0, int renderedEntities = 0, int activeChunks = 0);
    void toggleFps() { 
        mShowFps = !mShowFps; 
        if (mShowFps) { 
            mFpsFrameCount = 0; 
            mFpsClock.restart(); 
        } 
    }
    
    // Check if mouse is over UI (blocks world interaction)
    bool isMouseOverUI(sf::Vector2f mousePos) const {
        if (checkPanelClick(mousePos)) return true;
        if (checkBagClick(mousePos)) return true;
        if (checkCharacterClick(mousePos)) return true;
        if (mChatBox.contains(mousePos)) return true;
        if (mTargetFrame.getBounds().contains(mousePos)) return true;
        return false;
    }

    bool checkBagClick(sf::Vector2f mousePos) const;
    bool checkCharacterClick(sf::Vector2f mousePos) const;
    bool checkPanelClick(sf::Vector2f mousePos) const {
        for (const auto* panel : mPanels) {
            if (panel && panel->isVisible() && panel->getBounds().contains(mousePos)) return true;
        }
        if (mMapPanel.isOpen() && mMapPanel.getBounds().contains(mousePos)) return true;
        return false;
    }

private:
    void drawCombatStatus(sf::RenderTarget& target);
    void drawHotbarBottomCenter(sf::RenderTarget& target, bool isStunned, Player* player, const SkillManager* skillMgr);
    void drawBagIconBottomRight(sf::RenderTarget& target); 
    void drawCharacterIcon(sf::RenderTarget& target);
    void drawSlotFallback(sf::RenderTarget& target, sf::Vector2f pos);

private:
    std::optional<sf::Sprite> mSlotSprite; 
    std::optional<sf::Sprite> mBagSprite;     
    sf::FloatRect             mBagIconBounds; 
    std::optional<sf::Sprite> mCharacterSprite;     
    sf::FloatRect             mCharacterIconBounds; 
    
    std::optional<sf::Sprite> mHudBgSprite;
    bool mHasValidHudBg = false;
    
    sf::Texture mFontTexture; 

    PlayerFrame      mPlayerFrame;
    CharacterPanel   mCharacterPanel;
    InventoryPanel   mInventoryPanel; 
    FortifyPanel     mFortifyPanel;
    ItemDebugPanel   mItemDebugPanel;
    TitlesPanel      mTitlesPanel;
    CultivoPanel     mCultivoPanel;
    SkillLevelUpPanel mSkillLevelUpPanel;
    SkillDebugPanel  mSkillDebugPanel;
    TargetFrame      mTargetFrame; 
    Tooltip          mTooltip; 
    StatusEffectSystem mStatusEffectSystem;
    const StatusEffectInfo* mHoveredStatusEffect = nullptr;
    float            mHoveredStatusEffectDuration = 0.f;
    MapPanel         mMapPanel; 
    DragDropSystem   mDragSystem; 

    // Chat
    ChatBox mChatBox;
    std::unique_ptr<ConsoleRedirector> mConsoleRedirector;
    std::unique_ptr<CommandManager> mCommandManager;

    void updateTooltip(sf::Vector2f mousePos, const SkillManager* skillMgr); 
    bool isPointOccludedByUpperUI(sf::Vector2f pos) const;

    BitmapText  mCombatStatusText;
    sf::Clock mCombatStatusClock;
    float     mCombatStatusDuration = 2.0f;

    static constexpr float UI_MARGIN     = 10.f;  
    float mSlotSize = 60.f; 

    bool mInventoryOpen = false;
    bool mCharacterPanelOpen = false;
    bool mTitlesPanelOpen = false;
    std::vector<UIPanel*> mPanels;
    bool mInventoryPosInitialized = false; 

    sf::Vector2f mCurrentMousePos;  
    ResourceManager* mRes = nullptr; 
    CharacterPanel* mInspectionPanelPtr = nullptr;
    class ItemManager* mItemMgr = nullptr;

private:
    WorldDropCallback mWorldDropCallback;
    std::shared_ptr<Item> mHoveredWorldItem = nullptr;

    int mDragCandidateSlot = -1;
    DragSource mDragCandidateSource = DragSource::Inventory;
    sf::Vector2f mDragStartPos;
    bool mDragCandidateIsSplit = false;
    static constexpr float DRAG_THRESHOLD = 5.0f;
    
    sf::Vector2u mWindowSize;

    // FPS Counter
    std::optional<BitmapText> mFpsText;
    int mFpsFrameCount = 0;
    sf::Clock mFpsClock;
    bool mShowFps = false;
    
    // Performance Cache
    int mCachedActiveEnts = 0;
    int mCachedRenderedEnts = 0;
    int mCachedActiveChunks = 0;

    Entity* mTargetedEntity = nullptr;

    class CombatSystem* mCombatSystem = nullptr;
    std::optional<sf::Sprite> mCastBarBgSprite;
    std::optional<sf::Sprite> mCastBarFillSprite;
};
