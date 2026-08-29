#include "PlayingState.h"
#include "Config.h"
#include "core/engine/ResourceManager.h"
#include "core/engine/WorldRegistry.h"
#include "core/items/ItemManager.h"
#include "core/managers/TitleManager.h"
#include "core/managers/StatusEffectManager.h"
#include "core/systems/CultivoSystem.h"
#include "core/skills/mage/basic-orb.h"
#include "core/skills/active/TotemHeal_1/TotemHeal_1.h"
#include "utils/Random.h"
#include <iostream>

PlayingState::PlayingState(Game &game)
    : mWorldManager(game.getResources()), mUIManager(game.getResources()),
      mEntityManager(game.getResources(), game.getItemManager()),
      mSkillManager(game.getResources()), mParticleSystem(game.getResources()),
      mCombatSystem(mUIManager.getHud(), mUIManager.getFXSystem(),
                    &mParticleSystem),
      mGoreSystem(), mRenderSystem(),
      mItemDrops(),
      mSoundSystem(game.getResources()),
      mShieldSystem(),
      mPlayerController(mCombatSystem, mSkillManager, mParticleSystem,
                        mEntityManager, mUIManager),
      mWasInCombat(false), mIsTransitioning(false), mDustTimer(0.f),
      mArrowSprite(mArrowTexture)
{
  mTargetWorldID = cfg::World::INITIAL_WORLD;
  mSpawnPos = {cfg::World::INITIAL_SPAWN_X, cfg::World::INITIAL_SPAWN_Y};
  mInitialized = false;
  mInitStep = 0;
  game.getPostFX().setDesaturateInstantly(0.f);
}

PlayingState::~PlayingState() {
  mPlayerController.reset();
  mCombatSystem.setCombatTarget(nullptr);
  mParticleSystem.clear();
  mEntityManager.clear();
  CultivoSystem::getInstance().reset();
}

void PlayingState::initializeStep(Game &game, int step) {
  auto &res = game.getResources();
  auto &win = game.getWindow();

  switch (step) {
  case 0: {
    CultivoSystem::getInstance().reset();
    mUIManager.getHud().setOnWorldDropCallback(
        [this, &game](std::shared_ptr<Item> item) {
          if (mPlayerPtr) {
            float sideSpeed = Random::Float(-35.f, 35.f);
            sf::Vector2f velocity = { sideSpeed, 0.f };
            float floorY = mPlayerPtr->getGroundPosition().y;
            mItemDrops.dropItem(item, mPlayerPtr->getGroundPosition() - sf::Vector2f(0.f, 32.f), game.getResources(),
                                velocity, floorY, 0.f);
          }
        });
    mParticleSystem.init();
    break;
  }
  case 1: {
    WorldRegistry::load("assets/data/worlds.dat");
    break;
  }
  case 2: {
    if (!mSkillManager.loadSkills("assets/data/skills.json")) {
      std::cerr << "[PlayingState] ERROR: Failed to load skills.json\n";
    }
    break;
  }
  case 3: {
    if (!TitleManager::getInstance().loadTitles("assets/data/title.json")) {
      std::cerr << "[PlayingState] ERROR: Failed to load title.json\n";
    }
    if (!StatusEffectManager::getInstance().loadConfig("assets/data/statuseffectui.json")) {
      std::cerr << "[PlayingState] ERROR: Failed to load statuseffectui.json\n";
    }
    break;
  }
  case 4: {
    mEntityManager.setSkillManager(&mSkillManager);
    mEntityManager.loadMobBlueprints();
    break;
  }
  case 5: {
    mView = win.getDefaultView();

    if (mSavedPlayer) {
      mEntityManager.spawnPlayer(std::move(mSavedPlayer));
      mPlayerPtr = mEntityManager.getPlayer();

      mCombatSystem.setPlayer(mPlayerPtr);
      mCombatSystem.setEntityManager(&mEntityManager);
      mCombatSystem.setParticleSystem(&mParticleSystem);
      mCombatSystem.setSoundSystem(&mSoundSystem);
      mPlayerController.setPlayer(mPlayerPtr);
    } else {
      auto player = std::make_unique<Player>();
      if (!player->loadSkin(res)) {
        std::cerr << "[ERROR] Skin player falló (faltan texturas en "
                     "assets/textures/player/parts/partes/)\n";
      } else {
        player->setPosition(mSpawnPos);
        player->setSpeed(cfg::Player::SPEED);

        mEntityManager.spawnPlayer(std::move(player));
        mPlayerPtr = mEntityManager.getPlayer();

        mCombatSystem.setPlayer(mPlayerPtr);
        mPlayerController.setPlayer(mPlayerPtr);
        mCombatSystem.setEntityManager(&mEntityManager);
        mCombatSystem.setParticleSystem(&mParticleSystem);
        mCombatSystem.setSoundSystem(&mSoundSystem);

        mEntityManager.setCombatSystem(&mCombatSystem);

        for (const auto& pair : mSkillManager.getSkills()) {
            const Skill* skill = pair.second.get();
            if (skill && skill->defaultSlot >= 0 && skill->defaultSlot < 12) {
                mPlayerPtr->equipSkill(skill->defaultSlot, skill->id);
            }
        }
      }
    }
    break;
  }
  case 6: {
    if (!mWorldManager.loadLevel(mTargetWorldID, res)) {
      std::cerr << "[CRITICAL] WorldManager failed to load level: "
                << mTargetWorldID << "!\n";
    }
    break;
  }
  case 7: {
    if (cfg::Terrain::ENABLE_TERRAIN_DEFORM) {
      sf::Vector2u mapPx = mWorldManager.getMapSizePx();
      const auto &worldData = WorldRegistry::get(mTargetWorldID);

      mTerrainDeform.clear(mapPx);

      try {
        const sf::Texture &footTex =
            res.getTexture("assets/textures/player/parts/partes/huella.png");
        sf::Vector2u sz = footTex.getSize();
        mTerrainDeform.setFootTexture(&footTex, {sz.x * 0.5f, sz.y * 0.5f});
      } catch (...) {
      }

      try {
        const sf::Texture &expTex = res.getTexture(
            "assets/textures/player/parts/debug_explosion.png");
        sf::Vector2u eSz = expTex.getSize();
        mTerrainDeform.setExplosionTexture(&expTex,
                                           {eSz.x * 0.5f, eSz.y * 0.5f});
      } catch (...) {
      }

      mTerrainDeform.initDirtLayer(
          worldData.map, "assets/tiles/tileset_0px.png",
          "assets/tiles/tileset_tierra.png", cfg::Map::TILE_SIZE,
          cfg::Map::CHUNK_SIZE, mapPx);

      mTerrainDeform.bakeGrassLayer(mWorldManager.getMap(), mapPx);
      mTerrainDeform.initEdgeShader();
    }
    break;
  }
  case 8: {
    const auto &worldData = WorldRegistry::get(mTargetWorldID);
    mEntityManager.loadEntitiesFromFile(res, mTargetWorldID, worldData);

    mCollisionCache.reserve(50);
    break;
  }
  case 9: {
    sf::Vector2u mapPx = mWorldManager.getMapSizePx();
    if (mPlayerPtr) {
      mPlayerPtr->setPosition(mSpawnPos);
      mPlayerPtr->resetIK();
      mPlayerPtr->setWorldBounds(
          sf::FloatRect(sf::Vector2f(0.f, 0.f),
                        sf::Vector2f((float)mapPx.x, (float)mapPx.y)));

      const float halfW = mView.getSize().x * 0.5f;
      const float halfH = mView.getSize().y * 0.5f;
      float cx =
          std::clamp(mPlayerPtr->getPosition().x, halfW, mapPx.x - halfW);
      float cy =
          std::clamp(mPlayerPtr->getPosition().y, halfH, mapPx.y - halfH);
      mView.setCenter({cx, cy});
    }

    if (cfg::Debug::ENABLE_WEAPONS_DEBUG) {
      mUIManager.getHud().addAllItemsToInventory(
          game.getItemManager(), mPlayerPtr, mPlayerPtr ? mPlayerPtr->getLevel() : 1);
    }

    if (!mArrowTexture.loadFromFile("assets/ui/arrow.png")) {
      std::cerr << "[PlayingState] ERROR: Could not load assets/ui/arrow.png\n";
    } else {
      mArrowSprite.setTexture(mArrowTexture, true);
      sf::Vector2u sz = mArrowTexture.getSize();
      mArrowSprite.setOrigin({sz.x * 0.5f, (float)sz.y});
    }

    if (cfg::Debug::ENABLE_DEBUG_OVERLAY) {
      mDebugOverlay.loadAssets("assets/data/debug_assets.json");
    }

    onResize(game, win.getSize().x, win.getSize().y);
    break;
  }
  }
}

void PlayingState::performLevelChange(const Portal &p, Game &game) {
  const WorldData &worldData = WorldRegistry::get(p.targetWorldID);
  if (worldData.map.empty()) {
    std::cerr << "[ERROR] Level Change Failed: Invalid World ID '"
              << p.targetWorldID << "'\n";
    return;
  }

  std::cout << "[PlayingState] Transitioning to " << p.targetWorldID << "...\n";
  mIsTransitioning = true;

  auto &win = game.getWindow();

  mUIManager.showLoadingScreen(win, p.targetWorldID);

  std::unique_ptr<Entity> savedPlayer = nullptr;
  auto &activeEnts = mEntityManager.getActiveEntities();
  for (auto it = activeEnts.begin(); it != activeEnts.end(); ++it) {
    if (it->get() == mPlayerPtr) {
      savedPlayer = std::move(*it);
      activeEnts.erase(it);
      break;
    }
  }

  mTargetedEntity = nullptr;
  mCombatSystem.setCombatTarget(nullptr);
  mPlayerController.reset();

  mCombatSystem.notifyPlayerMoved();
  if (mPlayerPtr) {
    mPlayerPtr->setFollowTarget(nullptr);
    mPlayerPtr->clearAggroList();
  }

  mEntityManager.clear();
  mAggroSystem.clear();
  mCollisionCache.clear();
  mUIManager.getFXSystem().clear();
  mParticleSystem.clear();
  mGoreSystem.clear();
  mItemDrops.clear();
  BasicOrb::clearAll();
  TotemHeal_1::clearAll();

  if (savedPlayer) {
    savedPlayer->resetIK();
    mSavedPlayer =
        std::unique_ptr<Player>(static_cast<Player *>(savedPlayer.release()));
  }
  mTargetWorldID = p.targetWorldID;
  mSpawnPos = p.spawnPos;

  mInitialized = false;
  mInitStep = 5;
}
