#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "core/ui/hud/Hud.h"
#include "core/ui/ExpBar.h"
#include "core/ui/LoadingScreen.h"
#include "core/ui/ContextMenu.h"
#include "core/ui/character/CharacterPanel.h"
#include "core/systems/FXSystem.h"
#include "core/systems/HealthBarSystem.h"
#include "core/engine/ResourceManager.h"
#include "core/managers/EntityManager.h" // For entity access in HealthBar/Inspection
#include "core/systems/WorldManager.h" // For Minimap rendering

class Game; // Forward decl
class SkillManager; // Forward decl [SKILLS]

class UIManager {
public:
    UIManager(ResourceManager& res);
    ~UIManager();

    // Core Loop
    void update(Game& game, sf::Time dt, Player* player, int activeEnts = 0, int renderedEnts = 0, int activeChunks = 0);
    void updateRTs(Game& game, WorldManager& worldManager, EntityManager& entityMgr, Player* player, Entity* targetEntity, const class TerrainDeformSystem* terrainDeform = nullptr);
    void draw(Game& game, sf::RenderTarget& target, const sf::View& worldView, EntityManager& entityMgr, WorldManager& worldMgr, Player* player, Entity* targetEntity, const SkillManager& skillMgr, const class AggroSystem* aggroSystem = nullptr);
    void onResize(int w, int h);
    
    // Input Handling
    void handleInput(Game& game, sf::Time dt, Player* player);
    void handleEvent(Game& game, const sf::Event& ev, Player* player);
    
    // Interaction Returns true if UI handled it
    bool handleInteract(sf::Vector2f mousePos, Player* player); 
    bool handleRightClick(sf::Vector2f mousePos, Player* player, Entity* targetedEntity, EntityManager& em, ResourceManager& res, const InputManager& input);

    bool isMouseOverUI(sf::Vector2f mousePos) const;
    bool isChatFocused() const { return mHud.isChatFocused(); }

    // Accessors for other systems
    Hud& getHud();
    FXSystem& getFXSystem();
    const sf::View& getUIView() const { return mUIView; }
    void notifyEntityDeath(Entity* entity); // [SAFETY]
    
    // Specific UI logic helpers
    void toggleInventory();
    void toggleCharacterPanel();
    void toggleMap();
    void toggleFortify(); // [NEW]
    void toggleTitlesPanel(); // [NEW]
    void toggleCultivoPanel(); // [CULTIVO SYSTEM]
    void toggleSkillLevelUp(); // [NEW]
    
    // Inspection
    void inspectEntity(Entity* entity);
    void closeInspection();

    // Loading Screen
    void showLoadingScreen(sf::RenderWindow& window, const std::string& worldID);
    void drawLoadingScreen(sf::RenderTarget& target, float progress, const std::string& worldID);
    void waitForLoading(sf::RenderWindow& window, sf::Time time);
    LoadingScreen& getLoadingScreen() { return mLoadingScreen; }

    // Minimap access for updating layout
    // (Minimap implementation details hidden in cpp potentially, or we keep public helpers)

private:
    void renderMinimap(Game& game, sf::RenderTarget& target, WorldManager& worldManager, Player* player);
    void drawMinimapMask(sf::RenderTarget& target);
    sf::Vector2f clampViewCenter(sf::Vector2f desired, sf::Vector2f viewSize, sf::Vector2u mapPx) const;

private:
    // Systems
    Hud mHud;
    FXSystem mFXSystem;
    HealthBarSystem mHealthBarSystem;
    ExpBar mExpBar;
    LoadingScreen mLoadingScreen;
    
    // Inspection
    ContextMenu mContextMenu;
    CharacterPanel mInspectionPanel;
    std::optional<sf::Sprite> mInspectSlotSprite; 
    Entity* mInspectedEntity = nullptr;
    bool mShowInspectionPanel = false;

    // UI Scaling
    sf::View mUIView;

    // Minimap Data
    sf::View mMiniView;
    sf::RenderTexture mMiniRT;
    sf::CircleShape   mMiniMask;
    sf::Clock         mMiniMapTimer;
    float mMiniDiameterPx;
    float mMiniMarginPx;
    std::optional<sf::Sprite> mMinimapBgSprite; // [NEW] Fondo manual UI para Minimapa
    std::optional<sf::Sprite> mMinimapPlayerSprite; // [NEW] Icono PNG para el jugador en el minimapa
    std::optional<sf::Sprite> mCursorSprite; // [NEW] Cursor PNG personalizado
    bool mCursorInitialized = false;
};

