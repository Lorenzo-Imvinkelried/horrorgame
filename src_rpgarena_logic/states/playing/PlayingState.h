#pragma once
#include "states/GameState.h"
#include "core/engine/Game.h"
#include "core/systems/WorldManager.h"
#include "core/managers/EntityManager.h"
#include "core/managers/UIManager.h"
#include "core/systems/combat/CombatSystem.h"
#include "core/systems/ParticleSystem.h"
#include "core/engine/SpriteBatcher.h"
#include "core/engine/IRenderable.h"
#include "core/systems/RespawnSystem.h"
#include <vector>
#include <memory>
#include "core/systems/gore/GoreSystem.h"
#include "core/skills/SkillManager.h"
#include "core/input/PlayerController.h"
#include "core/systems/render/RenderSystem.h"
#include "core/systems/DebugOverlaySystem.h"
#include "core/systems/terrain/TerrainDeformSystem.h"
#include "core/systems/ItemDropSystem.h"
#include "core/systems/SoundSystem.h"
#include "core/systems/ShieldSystem.h"
#include "core/systems/AggroSystem.h"
#include "core/systems/DashSystem.h"

class PlayingState : public GameState {
public:
    PlayingState(Game& game); 
    ~PlayingState();

    void handleInput(Game& game, sf::Time dt) override;
    void update(Game& game, sf::Time dt) override;
    
    void drawWorld(Game& game, sf::RenderTarget& target) override;
    void drawUI(Game& game, sf::RenderTarget& target) override;
    void draw(Game& game, sf::RenderTarget& target) override;
    void onResize(Game& game, int w, int h) override;
    void clearState();
    void handleEvent(Game& game, const sf::Event& ev) override;

private:
    void performLevelChange(const Portal& p, Game& game);
    void initializeStep(Game& game, int step);

private:
    WorldManager     mWorldManager;
    UIManager        mUIManager;
    ParticleSystem   mParticleSystem;
    CombatSystem     mCombatSystem;
    GoreSystem       mGoreSystem;
    RenderSystem     mRenderSystem;
    DebugOverlaySystem mDebugOverlay;
    TerrainDeformSystem mTerrainDeform;
    ItemDropSystem   mItemDrops;
    SoundSystem      mSoundSystem;
    ShieldSystem     mShieldSystem;
    DashSystem       mDashSystem;
    AggroSystem      mAggroSystem;

    EntityManager mEntityManager;
    SkillManager mSkillManager;

    Player* mPlayerPtr = nullptr;
    Entity* mTargetedEntity = nullptr;

    sf::View mView;

    bool mLoadedOk = false;
    bool mWasInCombat = false;
    bool mIsTransitioning = false;
    bool mPlayerDeathTriggered = false;
    float mDustTimer = 0.f;
    
    bool mInitialized = false;
    int mInitStep = 0;
    std::string mTargetWorldID = "level1";
    sf::Vector2f mSpawnPos = {400.f, 300.f};
    std::unique_ptr<Player> mSavedPlayer = nullptr;
    
    std::vector<sf::FloatRect> mCollisionCache; 

    sf::Texture mArrowTexture;
    sf::Sprite mArrowSprite;
    float mArrowTimer = 0.f;

    PlayerController mPlayerController;
};
