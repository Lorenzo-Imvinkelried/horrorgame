#include "RenderSystem.h"
#include "Config.h"
#include "core/systems/terrain/TerrainDeformSystem.h"
#include "core/systems/ParticleSystem.h"
#include "entities/Entity.h"
#include <algorithm>
#include <cmath>
#include <iostream>

void RenderSystem::renderQueue(sf::RenderTarget &target, const sf::View &view,
                               TerrainDeformSystem &terrainDeform,
                               ParticleSystem &particleSystem,
                               Entity *targetedEntity,
                               const sf::Vector2f &shadowViewTopLeft,
                               const sf::Vector2f &shadowViewSize,
                               bool shadowViewValid) {
  sf::Vector2f center = view.getCenter();
  mBatcher.clear();

  for (const auto &item : mRenderQueueCombined) {
    const Entity *entityToDraw = item.entity;

    float distX = 10000.f;
    float distY = 10000.f;

    bool isOccluded = false;
    float entityBaseY = 0.f;
    float pureDepth = 0.f;

    if (entityToDraw) {
      sf::Vector2f entPos = entityToDraw->getPosition();
      distX = std::abs(entPos.x - center.x);
      distY = std::abs(entPos.y - center.y);

      if (mOcclusionShaderLoaded) {
        entityBaseY = entityToDraw->getSortingY();
        sf::FloatRect bounds = entityToDraw->getGlobalBounds();
        float entityBaseX = bounds.position.x + bounds.size.x / 2.f;

        float depthL =
            terrainDeform.getDepthAt(entityToDraw->getLeftFootPosition());
        float depthR =
            terrainDeform.getDepthAt(entityToDraw->getRightFootPosition());
        float depthC = terrainDeform.getDepthAt({entityBaseX, entityBaseY});

        pureDepth = std::max({depthL, depthR, depthC});

        static sf::Clock debugPrintClock;
        if (cfg::Debug::ENABLE_PERF_LOG && pureDepth > 0.0f &&
            debugPrintClock.getElapsedTime().asSeconds() > 0.2f) {
          std::cout << "[DEBUG OCCLUSION] pureDepth: " << pureDepth
                    << " | depthL: " << depthL << " | depthR: " << depthR
                    << " | depthC: " << depthC << " | posL: ("
                    << entityToDraw->getLeftFootPosition().x << ","
                    << entityToDraw->getLeftFootPosition().y << ")"
                    << " | posR: (" << entityToDraw->getRightFootPosition().x
                    << "," << entityToDraw->getRightFootPosition().y << ")"
                    << std::endl;
          debugPrintClock.restart();
        }

        if (pureDepth > 2.0f) {
          isOccluded = true;
        }
      }
    } else if (item.renderable && item.renderable->getRenderType() ==
               IRenderable::RenderType::Entity) {
      sf::Vector2f entPos =
          static_cast<const Entity *>(item.renderable)->getPosition();
      distX = std::abs(entPos.x - center.x);
      distY = std::abs(entPos.y - center.y);
    }

    bool forceImmediate = false;
    if (entityToDraw) {
      forceImmediate = true;
    }

    if (forceImmediate && entityToDraw) {
      sf::RenderStates batchStates;
      if (mOcclusionShaderLoaded) {
        mOcclusionShader.setUniform("u_EntityBaseY", 0.f);
        mOcclusionShader.setUniform("u_Offset", cfg::Terrain::DIRT_OFFSET_PX);
        if (shadowViewValid) {
          mOcclusionShader.setUniform("u_ShadowMap",
                                      mStaticShadowRT.getTexture());
          mOcclusionShader.setUniform("u_HeightMap", mHeightMapRT.getTexture());
          mOcclusionShader.setUniform("u_ShadowViewTopLeft", shadowViewTopLeft);
          mOcclusionShader.setUniform("u_ShadowViewSize", shadowViewSize);
          mOcclusionShader.setUniform("u_ShadowAlpha",
                                      cfg::Shadow::ALPHA / 255.f);
          mOcclusionShader.setUniform("u_ShadowSkewX", cfg::Shadow::SKEW_X);
          mOcclusionShader.setUniform("u_ShadowScaleY", cfg::Shadow::SCALE_Y);
          mOcclusionShader.setUniform("u_ShadowOffsetX", cfg::Shadow::OFFSET_X);
          mOcclusionShader.setUniform("u_ShadowOffsetY", cfg::Shadow::OFFSET_Y);
          mOcclusionShader.setUniform("u_EnableShadows", 1.0f);
        } else {
          mOcclusionShader.setUniform("u_EnableShadows", 0.0f);
        }
        mOcclusionShader.setUniform("u_IsChunked", 0.0f);
        mOcclusionShader.setUniform("u_IsFlatGore", 0.0f);
        mOcclusionShader.setUniform("u_ContourTexture", mContourTexture);
        mOcclusionShader.setUniform("u_ContourThickness", cfg::Shadow::ENABLE_CONTOUR ? cfg::Shadow::CONTOUR_THICKNESS : 0.0f);
        mOcclusionShader.setUniform("u_ContourAlpha", cfg::Shadow::CONTOUR_ALPHA);
        mOcclusionShader.setUniform("u_ContourBaseAlpha", cfg::Shadow::CONTOUR_BASE_ALPHA);
        mOcclusionShader.setUniform("u_SobelStep", cfg::Shadow::SOBEL_STEP);
        batchStates.shader = &mOcclusionShader;
      }
      mBatcher.render(target, batchStates);
      mBatcher.clear();

      // Outline de Selección detrás de la entidad
      if (entityToDraw == targetedEntity && mSelectionOutlineShaderLoaded) {
        if (mCurrentTargetedEntity != targetedEntity) {
          mCurrentTargetedEntity = targetedEntity;
          sf::FloatRect initialBounds = targetedEntity->getGlobalBounds();
          mCurrentTargetedEntityHeight =
              (initialBounds.size.y > 0.f) ? initialBounds.size.y : 64.f;
        }

        sf::Vector2f vSize = view.getSize();
        sf::Vector2u tSize = target.getSize();
        float scaleX = 1.f;
        float scaleY = 1.f;
        if (vSize.x > 0.f && vSize.y > 0.f) {
          scaleX = static_cast<float>(tSize.x) / vSize.x;
          scaleY = static_cast<float>(tSize.y) / vSize.y;
        }

        unsigned int rtWidth =
            static_cast<unsigned int>(std::round(256.f * scaleX));
        unsigned int rtHeight =
            static_cast<unsigned int>(std::round(256.f * scaleY));
        rtWidth = std::max(1u, rtWidth);
        rtHeight = std::max(1u, rtHeight);

        if (mSelectionRT.getSize() != sf::Vector2u(rtWidth, rtHeight)) {
          (void)mSelectionRT.resize({rtWidth, rtHeight});
          mSelectionRT.setSmooth(false);
        }

        mSelectionRT.clear(sf::Color::Transparent);

        sf::Vector2f entityPos = entityToDraw->getPosition();
        sf::Vector2f entityCenter;
        entityCenter.x = std::round(entityPos.x);
        entityCenter.y =
            std::round(entityPos.y - mCurrentTargetedEntityHeight * 0.5f);

        sf::View selectionView(entityCenter, sf::Vector2f(256.f, 256.f));
        mSelectionRT.setView(selectionView);

        const_cast<Entity *>(entityToDraw)->draw(mSelectionRT);
        mSelectionRT.display();

        sf::Sprite outlineSprite(mSelectionRT.getTexture());
        outlineSprite.setOrigin({static_cast<float>(rtWidth) * 0.5f,
                                 static_cast<float>(rtHeight) * 0.5f});
        outlineSprite.setPosition(entityCenter);
        outlineSprite.setScale({1.f / scaleX, 1.f / scaleY});

        mSelectionOutlineShader.setUniform("u_texSize",
                                           sf::Vector2f(rtWidth, rtHeight));
        mSelectionOutlineShader.setUniform(
            "u_color", sf::Glsl::Vec4(1.f, 0.f, 0.f, 1.f));

        sf::RenderStates outlineStates;
        outlineStates.shader = &mSelectionOutlineShader;
        target.draw(outlineSprite, outlineStates);
      }

      if (mOcclusionShaderLoaded) {
        mOcclusionShader.setUniform("u_EntityBaseY", entityBaseY);
        mOcclusionShader.setUniform("u_Offset", cfg::Terrain::DIRT_OFFSET_PX);

        if (shadowViewValid) {
          mOcclusionShader.setUniform("u_ShadowMap",
                                      mStaticShadowRT.getTexture());
          mOcclusionShader.setUniform("u_HeightMap", mHeightMapRT.getTexture());
          mOcclusionShader.setUniform("u_ShadowViewTopLeft", shadowViewTopLeft);
          mOcclusionShader.setUniform("u_ShadowViewSize", shadowViewSize);
          mOcclusionShader.setUniform("u_ShadowAlpha",
                                      cfg::Shadow::ALPHA / 255.f);
          mOcclusionShader.setUniform("u_ShadowSkewX", cfg::Shadow::SKEW_X);
          mOcclusionShader.setUniform("u_ShadowScaleY", cfg::Shadow::SCALE_Y);
          mOcclusionShader.setUniform("u_ShadowOffsetX", cfg::Shadow::OFFSET_X);
          mOcclusionShader.setUniform("u_ShadowOffsetY", cfg::Shadow::OFFSET_Y);
          mOcclusionShader.setUniform("u_EnableShadows", 1.0f);
        } else {
          mOcclusionShader.setUniform("u_EnableShadows", 0.0f);
        }

        if (isOccluded) {
          sf::FloatRect bounds2 = entityToDraw->getGlobalBounds();
          float entityBaseX2 = bounds2.position.x + bounds2.size.x / 2.f;
          TerrainDeformSystem::ChunkInfo info =
              terrainDeform.getGrassChunkInfo({entityBaseX2, entityBaseY});

          mOcclusionShader.setUniform("maskTexture", *info.texture);
          if (info.depthTexture) {
            mOcclusionShader.setUniform("depthTexture", *info.depthTexture);
          }
          mOcclusionShader.setUniform("u_ChunkOffset",
                                      sf::Glsl::Vec2(info.offset));
          mOcclusionShader.setUniform("u_MapSize",
                                      sf::Glsl::Vec2(info.size, info.size));
          mOcclusionShader.setUniform("u_IsChunked", 1.0f);
        } else {
          mOcclusionShader.setUniform("u_IsChunked", 0.0f);
        }
        mOcclusionShader.setUniform("u_IsFlatGore", 0.0f);
        mOcclusionShader.setUniform("u_ContourTexture", mContourTexture);
        mOcclusionShader.setUniform("u_ContourThickness", cfg::Shadow::ENABLE_CONTOUR ? cfg::Shadow::CONTOUR_THICKNESS : 0.0f);
        mOcclusionShader.setUniform("u_ContourAlpha", cfg::Shadow::CONTOUR_ALPHA);
        mOcclusionShader.setUniform("u_ContourBaseAlpha", cfg::Shadow::CONTOUR_BASE_ALPHA);
        mOcclusionShader.setUniform("u_SobelStep", cfg::Shadow::SOBEL_STEP);

        sf::RenderStates states;
        states.shader = &mOcclusionShader;
        const_cast<Entity *>(entityToDraw)->drawLayer(target, item.partLayer, states);
      } else {
        const_cast<Entity *>(entityToDraw)->drawLayer(target, item.partLayer);
      }

      if (cfg::Debug::SHOW_PART_SORTING_POINTS) {
        sf::CircleShape debugPoint(3.f);
        sf::Color debugColor = sf::Color::Green;
        if (item.partLayer == 1) {
          debugColor = sf::Color::Blue;
        } else if (item.partLayer == 3) {
          debugColor = sf::Color::Red;
        }
        debugPoint.setFillColor(debugColor);
        debugPoint.setOrigin({1.5f, 1.5f});
        sf::FloatRect bounds = entityToDraw->getGlobalBounds();
        float entityBaseX = bounds.position.x + bounds.size.x / 2.f;
        debugPoint.setPosition({entityBaseX, item.y});
        target.draw(debugPoint);
      }

      if (item.partLayer == 3) {
        particleSystem.drawOwnedParticles(entityToDraw, target);
      }
    } else {
      const IRenderable *renderable =
          (entityToDraw) ? static_cast<const IRenderable *>(entityToDraw)
                         : item.renderable;

      sf::RenderStates states;
      if (mOcclusionShaderLoaded) {
        mOcclusionShader.setUniform("u_EntityBaseY", 0.f);
        mOcclusionShader.setUniform("u_Offset", cfg::Terrain::DIRT_OFFSET_PX);
        if (shadowViewValid) {
          mOcclusionShader.setUniform("u_ShadowMap",
                                      mStaticShadowRT.getTexture());
          mOcclusionShader.setUniform("u_HeightMap", mHeightMapRT.getTexture());
          mOcclusionShader.setUniform("u_ShadowViewTopLeft", shadowViewTopLeft);
          mOcclusionShader.setUniform("u_ShadowViewSize", shadowViewSize);
          mOcclusionShader.setUniform("u_ShadowAlpha",
                                      cfg::Shadow::ALPHA / 255.f);
          mOcclusionShader.setUniform("u_ShadowSkewX", cfg::Shadow::SKEW_X);
          mOcclusionShader.setUniform("u_ShadowScaleY", cfg::Shadow::SCALE_Y);
          mOcclusionShader.setUniform("u_ShadowOffsetX", cfg::Shadow::OFFSET_X);
          mOcclusionShader.setUniform("u_ShadowOffsetY", cfg::Shadow::OFFSET_Y);
          mOcclusionShader.setUniform("u_EnableShadows", 1.0f);
        } else {
          mOcclusionShader.setUniform("u_EnableShadows", 0.0f);
        }
        mOcclusionShader.setUniform("u_IsChunked", 0.0f);
        mOcclusionShader.setUniform("u_IsFlatGore", 0.0f);
        mOcclusionShader.setUniform("u_ContourTexture", mContourTexture);
        mOcclusionShader.setUniform("u_SpriteTexelSize",
                                    sf::Glsl::Vec2(1.f / 256.f, 1.f / 256.f));
        mOcclusionShader.setUniform("u_ContourThickness", cfg::Shadow::ENABLE_CONTOUR ? cfg::Shadow::CONTOUR_THICKNESS : 0.0f);
        mOcclusionShader.setUniform("u_ContourAlpha", cfg::Shadow::CONTOUR_ALPHA);
        mOcclusionShader.setUniform("u_ContourBaseAlpha", cfg::Shadow::CONTOUR_BASE_ALPHA);
        mOcclusionShader.setUniform("u_SpriteUVBounds",
                                    sf::Glsl::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        mOcclusionShader.setUniform("u_SpritePixelSize",
                                    sf::Glsl::Vec2(256.0f, 256.0f));
        mOcclusionShader.setUniform("u_SobelStep", cfg::Shadow::SOBEL_STEP);
        states.shader = &mOcclusionShader;
      }

      if (renderable) {
        const sf::Drawable *drawable = renderable->getDrawable();
        if (drawable) {
          mBatcher.render(target, states);
          mBatcher.clear();

          if (mOcclusionShaderLoaded) {
            const sf::Texture *dTex = nullptr;
            mTempVerts.clear();
            renderable->getRenderData(mTempVerts, dTex);
            if (dTex && dTex->getSize().x > 0) {
              sf::Vector2u dtSize = dTex->getSize();
              float minU = 0.0f, maxU = 1.0f, minV = 0.0f, maxV = 1.0f;
              if (!mTempVerts.empty()) {
                minU = mTempVerts[0].texCoords.x;
                maxU = mTempVerts[0].texCoords.x;
                minV = mTempVerts[0].texCoords.y;
                maxV = mTempVerts[0].texCoords.y;
                for (const auto &v : mTempVerts) {
                  minU = std::min(minU, v.texCoords.x);
                  maxU = std::max(maxU, v.texCoords.x);
                  minV = std::min(minV, v.texCoords.y);
                  maxV = std::max(maxV, v.texCoords.y);
                }
                if (maxU > 1.0f || maxV > 1.0f) {
                  minU /= static_cast<float>(dtSize.x);
                  maxU /= static_cast<float>(dtSize.x);
                  minV /= static_cast<float>(dtSize.y);
                  maxV /= static_cast<float>(dtSize.y);
                }
              }
              float uvW = std::max(0.0001f, maxU - minU);
              float uvH = std::max(0.0001f, maxV - minV);
              float pixW =
                  std::max(1.0f, std::round(uvW * static_cast<float>(dtSize.x)));
              float pixH =
                  std::max(1.0f, std::round(uvH * static_cast<float>(dtSize.y)));

              sf::Shader::bind(&mOcclusionShader);
              mOcclusionShader.setUniform(
                  "u_SpriteTexelSize",
                  sf::Glsl::Vec2(1.f / static_cast<float>(dtSize.x),
                                 1.f / static_cast<float>(dtSize.y)));
              mOcclusionShader.setUniform("u_ContourTexture", mContourTexture);
              mOcclusionShader.setUniform("u_ContourThickness", cfg::Shadow::ENABLE_CONTOUR ? cfg::Shadow::CONTOUR_THICKNESS : 0.0f);
              mOcclusionShader.setUniform("u_ContourAlpha", cfg::Shadow::CONTOUR_ALPHA);
              mOcclusionShader.setUniform("u_ContourBaseAlpha", cfg::Shadow::CONTOUR_BASE_ALPHA);
              mOcclusionShader.setUniform("u_SpriteUVBounds",
                                          sf::Glsl::Vec4(minU, minV, uvW, uvH));
              mOcclusionShader.setUniform("u_SpritePixelSize",
                                          sf::Glsl::Vec2(pixW, pixH));
              mOcclusionShader.setUniform("u_SobelStep", cfg::Shadow::SOBEL_STEP);
            }
          }
          target.draw(*drawable, states);
          if (mOcclusionShaderLoaded) {
            sf::Shader::bind(nullptr);
          }
        } else {
          const sf::Texture *tex = nullptr;
          mTempVerts.clear();
          renderable->getRenderData(mTempVerts, tex);

          if (tex && !mTempVerts.empty()) {
            mBatcher.add(mTempVerts.data(), mTempVerts.size(), tex);
          } else if (!mTempVerts.empty()) {
            mBatcher.render(target, states);
            mBatcher.clear();
            target.draw(mTempVerts.data(), mTempVerts.size(),
                        sf::PrimitiveType::Triangles, states);
          }
        }
      }
    }
  }

  sf::RenderStates finalBatchStates;
  if (mOcclusionShaderLoaded) {
    mOcclusionShader.setUniform("u_EntityBaseY", 0.f);
    mOcclusionShader.setUniform("u_Offset", cfg::Terrain::DIRT_OFFSET_PX);
    if (shadowViewValid) {
      mOcclusionShader.setUniform("u_ShadowMap", mStaticShadowRT.getTexture());
      mOcclusionShader.setUniform("u_HeightMap", mHeightMapRT.getTexture());
      mOcclusionShader.setUniform("u_ShadowViewTopLeft", shadowViewTopLeft);
      mOcclusionShader.setUniform("u_ShadowViewSize", shadowViewSize);
      mOcclusionShader.setUniform("u_ShadowAlpha", cfg::Shadow::ALPHA / 255.f);
      mOcclusionShader.setUniform("u_ShadowSkewX", cfg::Shadow::SKEW_X);
      mOcclusionShader.setUniform("u_ShadowScaleY", cfg::Shadow::SCALE_Y);
      mOcclusionShader.setUniform("u_ShadowOffsetX", cfg::Shadow::OFFSET_X);
      mOcclusionShader.setUniform("u_ShadowOffsetY", cfg::Shadow::OFFSET_Y);
      mOcclusionShader.setUniform("u_EnableShadows", 1.0f);
    } else {
      mOcclusionShader.setUniform("u_EnableShadows", 0.0f);
    }
    mOcclusionShader.setUniform("u_IsChunked", 0.0f);
    mOcclusionShader.setUniform("u_IsFlatGore", 0.0f);
    mOcclusionShader.setUniform("u_ContourTexture", mContourTexture);
    mOcclusionShader.setUniform("u_ContourThickness", cfg::Shadow::ENABLE_CONTOUR ? cfg::Shadow::CONTOUR_THICKNESS : 0.0f);
    mOcclusionShader.setUniform("u_ContourAlpha", cfg::Shadow::CONTOUR_ALPHA);
    mOcclusionShader.setUniform("u_ContourBaseAlpha", cfg::Shadow::CONTOUR_BASE_ALPHA);
    mOcclusionShader.setUniform("u_SobelStep", cfg::Shadow::SOBEL_STEP);
    finalBatchStates.shader = &mOcclusionShader;
  }
  mBatcher.render(target, finalBatchStates);
}
