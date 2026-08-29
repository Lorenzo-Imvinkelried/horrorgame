#include "RenderSystem.h"
#include "Config.h"
#include "core/systems/gore/GoreSystem.h"
#include "core/systems/terrain/TerrainDeformSystem.h"
#include "entities/Entity.h"
#include <algorithm>
#include <cmath>

void RenderSystem::renderShadowPass(sf::RenderTarget &target, const sf::View &view,
                                    TerrainDeformSystem &terrainDeform, GoreSystem &goreSystem,
                                    sf::Vector2f &shadowViewTopLeft, sf::Vector2f &shadowViewSize,
                                    bool &shadowViewValid) {
  sf::Vector2f viewSize = view.getSize();
  sf::Vector2u virtualSize(static_cast<unsigned int>(std::round(viewSize.x)),
                           static_cast<unsigned int>(std::round(viewSize.y)));
  sf::Vector2u targetSize = target.getSize();

  if (virtualSize.x == 0 || virtualSize.y == 0 || targetSize.x == 0 || targetSize.y == 0) {
    return;
  }

  if (mShadowRT.getSize() != virtualSize) {
    (void)mShadowRT.resize(virtualSize);
    mShadowRT.setSmooth(false);
  }
  if (mStaticShadowRT.getSize() != virtualSize) {
    (void)mStaticShadowRT.resize(virtualSize);
    mStaticShadowRT.setSmooth(false);
  }
  if (mHeightMapRT.getSize() != virtualSize) {
    (void)mHeightMapRT.resize(virtualSize);
    mHeightMapRT.setSmooth(false);
  }

  mShadowRT.clear(sf::Color::Transparent);
  mStaticShadowRT.clear(sf::Color::Transparent);
  mHeightMapRT.clear(sf::Color::Transparent);

  sf::View shadowView = view;
  sf::Vector2f actualCenter = view.getCenter();
  float halfW = viewSize.x / 2.f;
  float halfH = viewSize.y / 2.f;
  float fracX = halfW - std::floor(halfW);
  float fracY = halfH - std::floor(halfH);

  sf::Vector2f roundedCenter(std::round(actualCenter.x - fracX) + fracX,
                             std::round(actualCenter.y - fracY) + fracY);
  shadowView.setCenter(roundedCenter);
  mShadowRT.setView(shadowView);
  mStaticShadowRT.setView(shadowView);
  mHeightMapRT.setView(shadowView);

  shadowViewTopLeft = shadowView.getCenter() - shadowView.getSize() / 2.f;
  shadowViewSize = shadowView.getSize();
  shadowViewValid = true;

  // 1. Sombras estáticas
  mBatcher.clear();
  for (const auto &item : mRenderQueueCombined) {
    const Entity *entityToDraw = item.entity;
    if (entityToDraw)
      continue;

    const IRenderable *renderable = item.renderable;
    if (renderable && renderable->castsShadow()) {
      const sf::Texture *tex = nullptr;
      mTempVerts.clear();
      renderable->getShadowRenderData(mTempVerts, tex);
      if (tex && !mTempVerts.empty()) {
        float casterY = item.y;
        int yInt = static_cast<int>(std::round(casterY));
        if (yInt < 0)
          yInt = 0;
        std::uint8_t r = static_cast<std::uint8_t>((yInt >> 8) & 0xFF);
        std::uint8_t g = static_cast<std::uint8_t>(yInt & 0xFF);
        for (auto &v : mTempVerts) {
          v.color = sf::Color(r, g, 0, v.color.a);
        }
        mBatcher.add(mTempVerts.data(), mTempVerts.size(), tex);
      }
    }
  }

  sf::RenderStates staticEncodeStates;
  if (mShadowEncodeShaderLoaded) {
    staticEncodeStates.shader = &mShadowEncodeShader;
  }
  staticEncodeStates.blendMode = sf::BlendNone;
  mBatcher.render(mStaticShadowRT, staticEncodeStates);
  mStaticShadowRT.display();

  // 1.5. Alturas estáticas en mHeightMapRT
  mBatcher.clear();
  for (const auto &item : mRenderQueueCombined) {
    const Entity *entityToDraw = item.entity;
    if (entityToDraw)
      continue;

    const IRenderable *renderable = item.renderable;
    if (renderable && renderable->castsShadow()) {
      const sf::Texture *tex = nullptr;
      mTempVerts.clear();
      renderable->getRenderData(mTempVerts, tex);
      if (tex && !mTempVerts.empty()) {
        float casterY = item.y;
        int yInt = static_cast<int>(std::round(casterY));
        if (yInt < 0)
          yInt = 0;
        std::uint8_t r = static_cast<std::uint8_t>((yInt >> 8) & 0xFF);
        std::uint8_t g = static_cast<std::uint8_t>(yInt & 0xFF);
        for (auto &v : mTempVerts) {
          v.color = sf::Color(r, g, 0, v.color.a);
        }
        mBatcher.add(mTempVerts.data(), mTempVerts.size(), tex);
      }
    }
  }

  sf::RenderStates heightEncodeStates;
  if (mHeightEncodeShaderLoaded) {
    heightEncodeStates.shader = &mHeightEncodeShader;
  }
  heightEncodeStates.blendMode = sf::BlendNone;
  mBatcher.render(mHeightMapRT, heightEncodeStates);
  mHeightMapRT.display();

  // 2. Copiar mStaticShadowRT a mShadowRT
  mShadowRT.setView(mShadowRT.getDefaultView());
  sf::Sprite staticShadowSprite(mStaticShadowRT.getTexture());
  mShadowRT.draw(staticShadowSprite, sf::BlendNone);
  mShadowRT.setView(shadowView);

  // 3. Sombras dinámicas (Player y Mobs)
  if (mDynamicShadowShaderLoaded) {
    for (const auto &item : mRenderQueueCombined) {
      const Entity *entityToDraw = item.entity;
      if (!entityToDraw)
        continue;
      if (item.partLayer != 2)
        continue;

      bool isOccluded = false;
      float entityBaseY = entityToDraw->getSortingY();
      TerrainDeformSystem::ChunkInfo info;

      if (mOcclusionShaderLoaded && terrainDeform.isInitialized()) {
        sf::FloatRect bounds = entityToDraw->getGlobalBounds();
        float entityBaseX = bounds.position.x + bounds.size.x / 2.f;

        float depthL =
            terrainDeform.getDepthAt(entityToDraw->getLeftFootPosition());
        float depthR =
            terrainDeform.getDepthAt(entityToDraw->getRightFootPosition());
        float depthC = terrainDeform.getDepthAt({entityBaseX, entityBaseY});

        float pureDepth = std::max({depthL, depthR, depthC});
        if (pureDepth > 2.0f) {
          isOccluded = true;
          info = terrainDeform.getGrassChunkInfo({entityBaseX, entityBaseY});
        }
      }

      sf::RenderStates shadowStates;
      shadowStates.shader = &mDynamicShadowEncodeShader;
      shadowStates.blendMode = sf::BlendNone;

      mDynamicShadowEncodeShader.setUniform("u_StaticShadowMap",
                                            mStaticShadowRT.getTexture());
      mDynamicShadowEncodeShader.setUniform("u_ShadowViewTopLeft",
                                            shadowViewTopLeft);
      mDynamicShadowEncodeShader.setUniform("u_ShadowViewSize",
                                            shadowViewSize);
      mDynamicShadowEncodeShader.setUniform("u_Offset",
                                            cfg::Terrain::DIRT_OFFSET_PX);

      if (isOccluded && info.texture && info.depthTexture) {
        mDynamicShadowEncodeShader.setUniform("maskTexture", *info.texture);
        mDynamicShadowEncodeShader.setUniform("depthTexture",
                                              *info.depthTexture);
        mDynamicShadowEncodeShader.setUniform("u_ChunkOffset",
                                              sf::Glsl::Vec2(info.offset));
        mDynamicShadowEncodeShader.setUniform(
            "u_MapSize", sf::Glsl::Vec2(info.size, info.size));
        mDynamicShadowEncodeShader.setUniform("u_IsChunked", 1.0f);
      } else {
        mDynamicShadowEncodeShader.setUniform("u_IsChunked", 0.0f);
      }

      auto drawPartShadow = [&](const std::vector<sf::Vertex> &verts,
                                const sf::Texture *tex) {
        if (!tex || verts.empty())
          return;

        float casterY = item.y;
        int yInt = static_cast<int>(std::round(casterY));
        if (yInt < 0)
          yInt = 0;
        std::uint8_t r = static_cast<std::uint8_t>((yInt >> 8) & 0xFF);
        std::uint8_t g = static_cast<std::uint8_t>(yInt & 0xFF);

        std::vector<sf::Vertex> coloredVerts = verts;
        for (auto &v : coloredVerts) {
          v.color = sf::Color(r, g, 0, v.color.a);
        }

        sf::RenderStates states = shadowStates;
        states.texture = tex;
        mShadowRT.draw(coloredVerts.data(), coloredVerts.size(),
                       sf::PrimitiveType::Triangles, states);
      };

      const IRenderable *renderable =
          static_cast<const IRenderable *>(entityToDraw);
      if (renderable->castsShadow()) {
        const sf::Texture *tex = nullptr;
        mTempVerts.clear();
        renderable->getShadowRenderData(mTempVerts, tex);
        drawPartShadow(mTempVerts, tex);
      }

      for (int slot = 0; slot < 2; ++slot) {
        const sf::Texture *wTex = nullptr;
        mTempVerts.clear();
        entityToDraw->getWeaponShadowRenderData(mTempVerts, wTex, slot);
        drawPartShadow(mTempVerts, wTex);
      }

      const int armorSlots[] = {0, 2, 3, 8};
      for (int slot : armorSlots) {
        const sf::Texture *aTex = nullptr;
        mTempVerts.clear();
        entityToDraw->getArmorShadowRenderData(mTempVerts, aTex, slot);
        drawPartShadow(mTempVerts, aTex);
      }
    }
  } else {
    mBatcher.clear();
    for (const auto &item : mRenderQueueCombined) {
      const Entity *entityToDraw = item.entity;
      if (!entityToDraw)
        continue;
      if (item.partLayer != 2)
        continue;

      const IRenderable *renderable =
          static_cast<const IRenderable *>(entityToDraw);
      if (renderable->castsShadow()) {
        const sf::Texture *tex = nullptr;
        mTempVerts.clear();
        renderable->getShadowRenderData(mTempVerts, tex);
        if (tex && !mTempVerts.empty()) {
          float casterY = item.y;
          int yInt = static_cast<int>(std::round(casterY));
          if (yInt < 0)
            yInt = 0;
          std::uint8_t r = static_cast<std::uint8_t>((yInt >> 8) & 0xFF);
          std::uint8_t g = static_cast<std::uint8_t>(yInt & 0xFF);

          for (auto &v : mTempVerts) {
            v.color = sf::Color(r, g, 0, v.color.a);
          }
          mBatcher.add(mTempVerts.data(), mTempVerts.size(), tex);
        }
      }

      for (int slot = 0; slot < 2; ++slot) {
        const sf::Texture *wTex = nullptr;
        mTempVerts.clear();
        entityToDraw->getWeaponShadowRenderData(mTempVerts, wTex, slot);
        if (wTex && !mTempVerts.empty()) {
          float casterY = item.y;
          int yInt = static_cast<int>(std::round(casterY));
          if (yInt < 0)
            yInt = 0;
          std::uint8_t r = static_cast<std::uint8_t>((yInt >> 8) & 0xFF);
          std::uint8_t g = static_cast<std::uint8_t>(yInt & 0xFF);

          for (auto &v : mTempVerts) {
            v.color = sf::Color(r, g, 0, v.color.a);
          }
          mBatcher.add(mTempVerts.data(), mTempVerts.size(), wTex);
        }
      }

      const int armorSlots[] = {0, 2, 3, 8};
      for (int slot : armorSlots) {
        const sf::Texture *aTex = nullptr;
        mTempVerts.clear();
        entityToDraw->getArmorShadowRenderData(mTempVerts, aTex, slot);
        if (aTex && !mTempVerts.empty()) {
          float casterY = item.y;
          int yInt = static_cast<int>(std::round(casterY));
          if (yInt < 0)
            yInt = 0;
          std::uint8_t r = static_cast<std::uint8_t>((yInt >> 8) & 0xFF);
          std::uint8_t g = static_cast<std::uint8_t>(yInt & 0xFF);

          for (auto &v : mTempVerts) {
            v.color = sf::Color(r, g, 0, v.color.a);
          }
          mBatcher.add(mTempVerts.data(), mTempVerts.size(), aTex);
        }
      }
    }
    sf::RenderStates dynamicEncodeStates;
    if (mShadowEncodeShaderLoaded) {
      dynamicEncodeStates.shader = &mShadowEncodeShader;
    }
    dynamicEncodeStates.blendMode = sf::BlendNone;
    mBatcher.render(mShadowRT, dynamicEncodeStates);
  }
  mShadowRT.display();

  // 4. Sombras de piezas de gore (Gibs)
  const auto &activeGibs = goreSystem.getGibs();
  int activeGibCount = goreSystem.getActiveCount();
  for (int i = 0; i < activeGibCount; ++i) {
    const auto &gib = activeGibs[i];
    if (!gib.texture)
      continue;

    float shadowScaleY = cfg::Shadow::SCALE_Y;
    float shadowScaleX = cfg::Shadow::SCALE_X;
    float shadowSkewX = cfg::Shadow::SKEW_X;
    float shOffsetX = cfg::Shadow::OFFSET_X * gib.facingDir;
    float shOffsetY = cfg::Shadow::OFFSET_Y - 10.f;

    float basePosLeftX =
        (gib.mobBaseX != 0.f) ? gib.mobBaseX : gib.getCenter().x;

    auto projectPoint = [&](sf::Vector2f p) -> sf::Vector2f {
      float relX = p.x - basePosLeftX;
      float height = gib.groundY - p.y;
      float projX = basePosLeftX + relX * shadowScaleX + shOffsetX +
                    height * shadowSkewX;
      float projY =
          gib.groundY + (p.y - gib.groundY) * shadowScaleY + shOffsetY;
      return {projX, projY};
    };

    float casterY = gib.groundY;
    int yInt = static_cast<int>(std::round(casterY));
    if (yInt < 0)
      yInt = 0;
    std::uint8_t r = static_cast<std::uint8_t>((yInt >> 8) & 0xFF);
    std::uint8_t g = static_cast<std::uint8_t>(yInt & 0xFF);

    float fleshAlpha = 255.f;
    float armorAlpha = 255.f;
    float boneAlpha = 0.f;

    float fleshSinkY = 0.f;
    float armorSinkY = 0.f;
    float boneSinkY = 0.f;

    if (cfg::Gore::ENABLE_BONE_DECAY && gib.boneTexture && gib.onGround) {
      float t = gib.decayTimer;
      float delay = cfg::Gore::DECAY_DELAY_SEC;
      float fadeDur = cfg::Gore::DECAY_FADE_DURATION;
      float boneLife = cfg::Gore::BONE_LIFETIME_SEC;

      if (t < delay) {
        fleshAlpha = 255.f;
        armorAlpha = 255.f;
        boneAlpha = 0.f;
      } else if (t < delay + fadeDur) {
        float p = (t - delay) / fadeDur;
        fleshAlpha = (1.0f - p) * 255.f;
        armorAlpha = (1.0f - p) * 255.f;
        boneAlpha = p * 255.f;

        float sink = p * cfg::Gore::SINK_DISTANCE;
        fleshSinkY = sink;
        armorSinkY = sink;
      } else if (t < delay + fadeDur + boneLife) {
        fleshAlpha = 0.f;
        armorAlpha = 0.f;
        boneAlpha = 255.f;
      } else {
        float p = (t - (delay + fadeDur + boneLife)) /
                  cfg::Gore::BONE_FADE_DURATION;
        fleshAlpha = 0.f;
        armorAlpha = 0.f;
        boneAlpha = std::max(0.f, (1.0f - p) * 255.f);

        boneSinkY = p * cfg::Gore::SINK_DISTANCE;
      }
    } else {
      if (gib.lifetime <= cfg::Gore::FADE_DURATION) {
        float p = std::max(0.f, gib.lifetime / cfg::Gore::FADE_DURATION);
        float fadeProgress = 1.0f - p;
        fleshAlpha = p * 255.f;
        armorAlpha = p * 255.f;

        float sink = fadeProgress * cfg::Gore::SINK_DISTANCE;
        fleshSinkY = sink;
        armorSinkY = sink;
      }
    }

    if (gib.texture && fleshAlpha > 0.05f) {
      mTempVerts.clear();
      for (int j = 0; j < 6; ++j) {
        sf::Vertex v = gib.vertices[j];
        v.position.y += fleshSinkY;
        v.position = projectPoint(v.position);
        std::uint8_t finalA = static_cast<std::uint8_t>(
            (static_cast<float>(v.color.a) / 255.f) * fleshAlpha);
        v.color = sf::Color(r, g, 0, finalA);
        mTempVerts.push_back(v);
      }
      mBatcher.add(mTempVerts.data(), mTempVerts.size(), gib.texture);
    }

    if (gib.armorTexture && armorAlpha > 0.05f) {
      mTempVerts.clear();
      for (int j = 0; j < 6; ++j) {
        sf::Vertex v = gib.armorVertices[j];
        v.position.y += armorSinkY;
        v.position = projectPoint(v.position);
        std::uint8_t finalA = static_cast<std::uint8_t>(
            (static_cast<float>(v.color.a) / 255.f) * armorAlpha);
        v.color = sf::Color(r, g, 0, finalA);
        mTempVerts.push_back(v);
      }
      mBatcher.add(mTempVerts.data(), mTempVerts.size(), gib.armorTexture);
    }

    if (gib.boneTexture && boneAlpha > 0.05f) {
      mTempVerts.clear();
      for (int j = 0; j < 6; ++j) {
        sf::Vertex v = gib.boneVertices[j];
        v.position.y += boneSinkY;
        v.position = projectPoint(v.position);
        std::uint8_t finalA = static_cast<std::uint8_t>(
            (static_cast<float>(v.color.a) / 255.f) * boneAlpha);
        v.color = sf::Color(r, g, 0, finalA);
        mTempVerts.push_back(v);
      }
      mBatcher.add(mTempVerts.data(), mTempVerts.size(), gib.boneTexture);
    }
  }

  sf::RenderStates dynamicEncodeStates;
  if (mDynamicShadowShaderLoaded) {
    dynamicEncodeStates.shader = &mDynamicShadowEncodeShader;
    mDynamicShadowEncodeShader.setUniform("u_StaticShadowMap",
                                          mStaticShadowRT.getTexture());
    mDynamicShadowEncodeShader.setUniform("u_ShadowViewTopLeft",
                                          shadowViewTopLeft);
    mDynamicShadowEncodeShader.setUniform("u_ShadowViewSize",
                                          shadowViewSize);
    mDynamicShadowEncodeShader.setUniform("u_IsChunked", 0.0f);
  } else if (mShadowEncodeShaderLoaded) {
    dynamicEncodeStates.shader = &mShadowEncodeShader;
  }
  dynamicEncodeStates.blendMode = sf::BlendNone;
  mBatcher.render(mShadowRT, dynamicEncodeStates);
  mShadowRT.display();

  // Composición final de sombra en target
  sf::View oldView = target.getView();
  target.setView(
      sf::View(sf::FloatRect({0.f, 0.f}, sf::Vector2f(targetSize))));

  sf::Sprite shadowSprite(mShadowRT.getTexture());
  shadowSprite.setColor(sf::Color(255, 255, 255, 255));

  float scaleX = static_cast<float>(targetSize.x) / virtualSize.x;
  float scaleY = static_cast<float>(targetSize.y) / virtualSize.y;
  shadowSprite.setScale({scaleX, scaleY});

  sf::Vector2f diff = roundedCenter - actualCenter;
  float snapX = std::round(diff.x * scaleX);
  float snapY = std::round(diff.y * scaleY);
  shadowSprite.setPosition({snapX, snapY});

  if (mGroundShadowShaderLoaded) {
    sf::RenderStates states;
    states.shader = &mGroundShadowShader;
    mGroundShadowShader.setUniform("u_Alpha", cfg::Shadow::ALPHA / 255.f);
    target.draw(shadowSprite, states);
  } else {
    shadowSprite.setColor(sf::Color(
        255, 255, 255, static_cast<std::uint8_t>(cfg::Shadow::ALPHA)));
    target.draw(shadowSprite);
  }

  target.setView(oldView);
}

