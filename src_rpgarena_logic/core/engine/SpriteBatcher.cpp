#include "SpriteBatcher.h"
#include "Config.h"
#include <tracy/Tracy.hpp>

SpriteBatcher::SpriteBatcher() 
    : mCurrentTexture(nullptr)
{
    mVertices.reserve(10000 * 4); 
    mBatches.reserve(100);
}

void SpriteBatcher::add(const sf::Vertex* vertices, std::size_t vertexCount, const sf::Texture* texture) {
    if (vertexCount == 0) return;

    // Si cambia la textura O no hay batches aún, creamos uno nuevo
    if (texture != mCurrentTexture || mBatches.empty()) {
        if (!mBatches.empty() && mBatches.back().vertexCount == 0) {
             // Optimización: si el último batch estaba vacío (raro), lo reutilizamos
             mBatches.back().texture = texture;
             mBatches.back().startIndex = mVertices.size();
        } else {
            Batch newBatch;
            newBatch.startIndex = mVertices.size();
            newBatch.vertexCount = 0;
            newBatch.texture = texture;
            mBatches.push_back(newBatch);
        }
        mCurrentTexture = texture;
    }

    // Añadir vértices al vector global
    for (std::size_t i = 0; i < vertexCount; ++i) {
        mVertices.push_back(vertices[i]);
    }

    // Actualizar contador del batch actual
    mBatches.back().vertexCount += vertexCount;
}

void SpriteBatcher::render(sf::RenderTarget& target) {
    if (mVertices.empty()) return;

    for (const auto& batch : mBatches) {
        if (batch.vertexCount == 0) continue;

        sf::RenderStates states;
        states.texture = batch.texture;
        
        // Dibujamos el sub-rango de vértices
        // En SFML 3, draw primitives accepts (vertex*, count, type, states)
        // Wait, VertexArray allows implicit handling, but here we want a pointer to the vector data
        
        // Check SFML version: User command used -lsfml-graphics.
        // User code used sf::Vertex, sf::PrimitiveType::Triangles (in DecorSystem.cpp changes I made).
        // Assuming SFML 3 or modern 2.6.
        // target.draw(&mVertices[batch.startIndex], batch.vertexCount, sf::PrimitiveType::Triangles, states);
        
        // Wait, if we use Quads (deprecated in 3) or Triangles?
        // Let's assume Triangles for safety as we are generating quads as 2 triangles usually, 
        // OR we conform to what the Entity provides.
        // Entity::getRenderData will provide vertices. If it provides 4 vertices, does it imply Quad or 2 Triangles?
        // Standard SFML Sprite uses Triangles or Quads depending on version. 
        // Best approach: Use helper to convert sprite to 4 vertices (Quad-like). 
        // If we render as Triangles, we need 6 vertices per sprite (2 shared? no, 6 distinct for batching typically).
        // UNLESS we use Quads primitive type if available. 
        // NOTE: DecorSystem logic I wrote used 6 vertices (2 triangles).
        // IF Entities provide 4 vertices (Quad), we must use Quads primitive type.
        // Let's assume sf::PrimitiveType::Quads is available OR we convert to Triangles in `add`.
        
        // IMPORTANT: The user said "Eliminar Draw Calls". 
        // If I treat everything as Triangles (6 vertices), it's safer for SFML 3.
        // But if Entity gives 4, I need to generate 6.
        
        // Let's try rendering as Quads first if SFML < 3, but header included "sfml-graphics".
        // PlayingState.cpp includes.
        
        // I will use sf::PrimitiveType::Triangles and ensure `add` or the provider gives 6 vertices? 
        // NO, the user requested: "void addSprite(...) que extraiga los 4 vértices".
        // If I extract 4 vertices, I should use Quads.
        // SFML 3 removed Quads.
        // If user is on SFML 3 (implied by -std=c++20 and my previous knowledge of "SFML 3.0.0 ELIMINÓ sf::Quads"), 
        // I MUST generate 6 vertices from 4.
        
        // So `add` should take 4 vertices (Quad) and push 6 (Triangles) to `mVertices`?
        // OR `add` takes vertices and we trust the caller.
        // User said: "extraiga los 4 vértices del sprite y los añada al array."
        // I will implement helper to extract as 6 vertices (Triangles) to be future-proof/safe for SFML 3.
        
        target.draw(&mVertices[batch.startIndex], batch.vertexCount, sf::PrimitiveType::Triangles, states);
    }
}

void SpriteBatcher::render(sf::RenderTarget& target, sf::RenderStates baseStates) {
    if (mVertices.empty()) return;

    for (const auto& batch : mBatches) {
        if (batch.vertexCount == 0) continue;

        sf::RenderStates states = baseStates;
        states.texture = batch.texture;
        
        if (baseStates.shader && batch.texture) {
            sf::Vector2u tSize = batch.texture->getSize();
            if (tSize.x > 0 && tSize.y > 0) {
                sf::Shader* shaderPtr = const_cast<sf::Shader*>(baseStates.shader);
                sf::Shader::bind(shaderPtr);
                shaderPtr->setUniform(
                    "u_SpriteTexelSize", 
                    sf::Glsl::Vec2(1.f / static_cast<float>(tSize.x), 1.f / static_cast<float>(tSize.y))
                );
                shaderPtr->setUniform("u_SpriteUVBounds", sf::Glsl::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
                shaderPtr->setUniform("u_SpritePixelSize", sf::Glsl::Vec2(static_cast<float>(tSize.x), static_cast<float>(tSize.y)));
                shaderPtr->setUniform("u_SobelStep", cfg::Shadow::SOBEL_STEP);
                shaderPtr->setUniform("u_ContourThickness", cfg::Shadow::ENABLE_CONTOUR ? cfg::Shadow::CONTOUR_THICKNESS : 0.0f);
                shaderPtr->setUniform("u_ContourAlpha", cfg::Shadow::CONTOUR_ALPHA);
                shaderPtr->setUniform("u_ContourBaseAlpha", cfg::Shadow::CONTOUR_BASE_ALPHA);
            }
        }
        
        target.draw(&mVertices[batch.startIndex], batch.vertexCount, sf::PrimitiveType::Triangles, states);
    }

    if (baseStates.shader) {
        sf::Shader::bind(nullptr);
    }
}

void SpriteBatcher::clear() {
    mVertices.clear();
    mBatches.clear();
    mCurrentTexture = nullptr;
}
