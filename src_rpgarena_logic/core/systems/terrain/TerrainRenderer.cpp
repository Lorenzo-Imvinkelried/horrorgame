#include "TerrainDeformSystem.h"
#include "Config.h"

void TerrainDeformSystem::drawDirt(sf::RenderTarget& target, const sf::View& view) {
    if (!mDirtLayerReady) return;

    // 1. Deep Dirt (Fondo absoluto)
    sf::View dirtView = view;
    dirtView.setCenter(sf::Vector2f(view.getCenter().x, view.getCenter().y - cfg::Terrain::EXPLOSION_OFFSET_PX));
    target.setView(dirtView);
    mDeepDirtMap.drawVisible(target, dirtView);

    // 2. Base Shallow Dirt (Solo chunks intactos)
    sf::View a1View = view;
    a1View.setCenter(sf::Vector2f(view.getCenter().x, view.getCenter().y - cfg::Terrain::DIRT_OFFSET_PX));
    target.setView(a1View);
    mShallowDirtMap.drawVisible(target, a1View);

    // 3. Deformed Shallow Dirt Chunks
    for (const auto& chunk : mVisualChunks) {
        if (chunk.active && chunk.shallowDirtSprite) {
            target.draw(*chunk.shallowDirtSprite);
        }
    }

    target.setView(view);
}

void TerrainDeformSystem::applyFootprintsAndDraw(sf::RenderTarget& target, const sf::View& worldView) {
    if (!mInitialized) return;

    mCurrentCameraCenter = worldView.getCenter();

    // 1. Pasto base intacto
    target.setView(worldView);
    mGrassMap.drawVisible(target, worldView);

    // 2. Chunks deformados con Edge Shader
    sf::RenderStates states;
    if (mEdgeShaderLoaded) {
        states.shader = &mEdgeShader;
        mEdgeShader.setUniform("u_Offset", cfg::Terrain::DIRT_OFFSET_PX);
        mEdgeShader.setUniform("u_UseDepthTex", 1.0f);
        mEdgeShader.setUniform("u_TexelSize", sf::Glsl::Vec2(1.f / VISUAL_CHUNK_SIZE, 1.f / VISUAL_CHUNK_SIZE));
    }

    for (const auto& chunk : mVisualChunks) {
        if (chunk.active && chunk.grassSprite) {
            if (mEdgeShaderLoaded) {
                mEdgeShader.setUniform("u_DepthTex", chunk.vram.depthRT->getTexture());
            }
            target.draw(*chunk.grassSprite, states);

            if (cfg::Terrain::DEBUG_PAINT_ACTIVE_CHUNKS) {
                sf::RectangleShape rect(sf::Vector2f(VISUAL_CHUNK_SIZE, VISUAL_CHUNK_SIZE));
                rect.setPosition(chunk.grassSprite->getPosition());
                rect.setFillColor(sf::Color(0, 255, 0, 40));
                rect.setOutlineColor(sf::Color::Green);
                rect.setOutlineThickness(1.0f);
                target.draw(rect);
            }
        }
    }
}
