#include "RenderSystem.h"
#include "Config.h"
#include "core/systems/WorldManager.h"
#include "core/managers/EntityManager.h"
#include "core/systems/ParticleSystem.h"
#include "core/systems/gore/GoreSystem.h"
#include "core/systems/terrain/TerrainDeformSystem.h"
#include "core/systems/ItemDropSystem.h"
#include "core/skills/mage/basic-orb.h"
#include "core/skills/active/TotemHeal_1/TotemHeal_1.h"
#include "core/systems/combat/GhostSlash.h"
#include "entities/mob/Mob.h"
#include <algorithm>
#include <iostream>
#include <tracy/Tracy.hpp>

void RenderSystem::render(sf::RenderTarget &target, sf::View &view,
                          WorldManager &worldManager,
                          EntityManager &entityManager,
                          ParticleSystem &particleSystem,
                          GoreSystem &goreSystem,
                          TerrainDeformSystem &terrainDeform,
                          ItemDropSystem &itemDrops, Entity *targetedEntity) {
  ZoneScoped;

  target.clear(sf::Color::Black);

  // ===== EL SÁNDWICH DE TILES =====
  target.setView(view);
  if (terrainDeform.hasDirtLayer()) {
    terrainDeform.drawDirt(target, view);
  } else {
    worldManager.drawMap(target, view);
  }

  if (terrainDeform.isInitialized()) {
    terrainDeform.applyFootprintsAndDraw(target, view);
  }

  worldManager.drawDecorBottomLayer(target);
  particleSystem.drawBottom(target);

  // Culling y Sorting
  sf::Vector2f center = view.getCenter();
  sf::Vector2f size = view.getSize();
  float margin = cfg::Map::CULLING_MARGIN_PX;
  sf::FloatRect viewRect(
      {center.x - size.x / 2.f - margin, center.y - size.y / 2.f - margin},
      {size.x + margin * 2.f, size.y + margin * 2.f});

  // 1. Filtrar Estáticos
  mVisibleDecorCache.clear();
  worldManager.getDecorSystem().getVisibleInstances(viewRect,
                                                    mVisibleDecorCache);

  // 2. Llenar Cola de Renderizado
  mRenderQueueCombined.clear();
  const auto &activeEnts = entityManager.getActiveEntities();
  mRenderQueueCombined.reserve(mVisibleDecorCache.size() + activeEnts.size());

  for (const auto *inst : mVisibleDecorCache) {
    mRenderQueueCombined.emplace_back(inst->getY(),
                                      static_cast<const IRenderable *>(inst));
  }

  for (const auto &e : activeEnts) {
    if (viewRect.contains(e->getPosition())) {
      mRenderQueueCombined.emplace_back(e->getLayerSortingY(1), e.get(), 1);
      mRenderQueueCombined.emplace_back(e->getLayerSortingY(2), e.get(), 2);
      mRenderQueueCombined.emplace_back(e->getLayerSortingY(3), e.get(), 3);
    }
  }

  const auto &sleepingEnts = entityManager.getSleepingEntities();
  for (const auto &e : sleepingEnts) {
    if (viewRect.contains(e->getPosition())) {
      mRenderQueueCombined.emplace_back(e->getLayerSortingY(1), e.get(), 1);
      mRenderQueueCombined.emplace_back(e->getLayerSortingY(2), e.get(), 2);
      mRenderQueueCombined.emplace_back(e->getLayerSortingY(3), e.get(), 3);
    }
  }

  const auto &independentParticles = particleSystem.getIndependentRenderables();
  for (const auto *pRenderable : independentParticles) {
    mRenderQueueCombined.emplace_back(
        pRenderable->getY(), static_cast<const IRenderable *>(pRenderable));
  }

  for (const auto &drop : itemDrops.getDroppedItems()) {
    if (viewRect.contains(drop.position)) {
      mRenderQueueCombined.emplace_back(
          drop.position.y + cfg::YSorting::ITEM_DROP,
          static_cast<const IRenderable *>(&drop));
    }
  }

  for (const auto &p : worldManager.getPortals()) {
    if (viewRect.findIntersection(p->bounds)) {
      mRenderQueueCombined.emplace_back(
          p->getY(), static_cast<const IRenderable *>(p.get()));
    }
  }

  for (const auto &orb : BasicOrb::getActiveOrbs()) {
    if (orb.alive && viewRect.contains(orb.position)) {
      mRenderQueueCombined.emplace_back(
          orb.getY(), static_cast<const IRenderable *>(&orb));
    }
  }

  for (const auto &totem : TotemHeal_1::getActiveTotems()) {
    if (totem.alive && viewRect.contains(totem.position)) {
      mRenderQueueCombined.emplace_back(
          totem.getY(), static_cast<const IRenderable *>(&totem));
    }
  }

  for (const auto &slash : GhostSlashSystem::getActiveSlashes()) {
    if (slash.isStarted() && !slash.isFinished()) {
      mRenderQueueCombined.emplace_back(
          slash.getY(), static_cast<const IRenderable *>(&slash));
    }
  }

  const auto &activeGibs = goreSystem.getGibs();
  int activeGibCount = goreSystem.getActiveCount();
  for (int i = 0; i < activeGibCount; ++i) {
    const auto &gib = activeGibs[i];
    if (gib.active && (gib.texture || gib.armorTexture || gib.boneTexture)) {
      float gibY = gib.onGround ? gib.groundY : gib.deathSortY;
      gibY += gib.layerPriority * 0.01f;
      sf::Vector2f c = gib.getCenter();
      if (viewRect.contains(c) || viewRect.contains({c.x, gibY})) {
        mRenderQueueCombined.emplace_back(gibY, static_cast<const IRenderable *>(&gib));
      }
    }
  }

  // 3. Ordenar TODO por Y (Z-Ordering)
  std::sort(mRenderQueueCombined.begin(), mRenderQueueCombined.end());

  sf::Vector2f shadowViewTopLeft;
  sf::Vector2f shadowViewSize;
  bool shadowViewValid = false;

  // 3.5. SHADOW PASS
  renderShadowPass(target, view, terrainDeform, goreSystem, shadowViewTopLeft,
                   shadowViewSize, shadowViewValid);

  // 4. ENTITY & BATCH DRAW PASS
  renderQueue(target, view, terrainDeform, particleSystem, targetedEntity,
              shadowViewTopLeft, shadowViewSize, shadowViewValid);

  particleSystem.drawUnownedParticles(target);

  // Debug culling
  if (cfg::Debug::ENABLE_CULLING_DEBUG) {
    const auto &sleeping = entityManager.getSleepingEntities();
    for (const auto &e : sleeping) {
      if (auto *mob = dynamic_cast<Mob *>(e.get())) {
        if (auto *sprite = mob->getSprite()) {
          sf::Color old = sprite->getColor();
          sprite->setColor(sf::Color::Blue);
          mob->draw(target);
          sprite->setColor(old);
        }
      } else {
        e->draw(target);
      }
    }
  }

  // Partículas superiores
  particleSystem.drawTop(target);

  // Rendimiento y métricas
  static int frameCount = 0;
  static sf::Clock perfClock;
  frameCount++;
  if (perfClock.getElapsedTime().asSeconds() >= 1.0f) {
    if (cfg::Debug::ENABLE_PERF_LOG) {
      float fps = (float)frameCount / perfClock.getElapsedTime().asSeconds();
      std::cout << "[PROFILE] FPS: " << fps
                << " | Mobs: " << entityManager.getActiveEntities().size()
                << " | Chunks: " << terrainDeform.getDebugActiveVisualChunks()
                << " | Draw Calls (Batches): " << mBatcher.getBatchCount()
                << " | Queue: " << mRenderQueueCombined.size() << "\n";
    }
    frameCount = 0;
    perfClock.restart();
  }

  mLastRenderCount = mRenderQueueCombined.size();
}
