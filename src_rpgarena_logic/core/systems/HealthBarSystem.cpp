//HealthBarSystem.cpp
#include "HealthBarSystem.h"
#include "AggroSystem.h"
#include "entities/mob/Mob.h"
#include "Config.h"
#include "../graphics/BitmapText.h"
#include "core/engine/ResourceManager.h"
#include <string>
#include <algorithm> // Para std::max

// --- ¡CONSTRUCTOR CORREGIDO (SFML 3)! ---
// (Inicializa mNameText en la lista)
HealthBarSystem::HealthBarSystem(sf::Texture* fontTexture)
    : mFontTexture(fontTexture)
{
    // Configura el texto (se reutilizará)
    mNameText.setTexture(mFontTexture);
    mNameText.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
    mNameText.setColor(sf::Color::White);
}

// Configura las barras (VertexArray se limpia cada frame)
void HealthBarSystem::load(ResourceManager& res) {
    try {
        mGreenTexture = &res.getTexture("assets/ui/healthbar_green.png");
        mRedTexture = &res.getTexture("assets/ui/healthbar_red.png");
    } catch (...) {
        // Fallback or ignore
    }
}

// --- ¡FUNCIÓN 'draw' CORREGIDA & OPTIMIZADA (BATCHING)! ---
void HealthBarSystem::draw(sf::RenderTarget& target, 
                           const sf::View& worldView,
                           const std::vector<std::unique_ptr<Entity>>& entities, 
                           Entity* localPlayer,
                           Entity* targetEntity,
                           sf::Vector2f mouseWorldPos,
                           const AggroSystem* aggroSystem) 
{
    // [OPTIMIZATION] Calculate View Bounds in WORLD Coords
    sf::Vector2f center = worldView.getCenter();
    sf::Vector2f viewSz = worldView.getSize();
    float margin = 100.f; 
    sf::FloatRect viewRect({center.x - viewSz.x / 2.f - margin, 
                           center.y - viewSz.y / 2.f - margin}, 
                           {viewSz.x + margin * 2.f, 
                           viewSz.y + margin * 2.f});

    // Collect visible, alive entities
    std::vector<Entity*> sortedEntities;
    sortedEntities.reserve(entities.size());
    for (const auto& entity : entities) {
        if (!entity) continue;
        if (!entity->isAlive()) continue;
        if (!viewRect.contains(entity->getPosition())) continue;
        sortedEntities.push_back(entity.get());
    }

    // Sort by Y-coordinate to respect Y-sorting (smaller Y is further back, drawn first)
    std::sort(sortedEntities.begin(), sortedEntities.end(), [](const Entity* a, const Entity* b) {
        return a->getSortingY() < b->getSortingY();
    });

    // Set primitive types for vertices
    mBgVertices.setPrimitiveType(sf::PrimitiveType::Triangles); 
    mFillVertices.setPrimitiveType(sf::PrimitiveType::Triangles);
    mTextVertices.setPrimitiveType(sf::PrimitiveType::Triangles);

    // Layout Constants (Lógicas)
    float zoom = cfg::Map::ZOOM_FACTOR;
    const float padding = 1.0f * zoom;
    const float namePadding = 2.f * zoom;

    for (Entity* entity : sortedEntities) {
        // --- VALIDAR TEXTURAS ---
        if (!mRedTexture || !mGreenTexture) continue;
        sf::Vector2f barSize = static_cast<sf::Vector2f>(mRedTexture->getSize());

        // --- Calcular HP ---
        float hpPercent = 0.f;
        if (entity->getMaxHp() > 0) {
            hpPercent = std::clamp((float)entity->getCurrentHp() / (float)entity->getMaxHp(), 0.f, 1.f);
        }

        // --- MAP TO VIEW SPACE (Logical UI) ---
        sf::Vector2f worldPos = entity->getPosition();
        
        // [ESTABILIZADO] Usamos getVisualHeight() en lugar de getGlobalBounds().position.y
        // para que la barra no oscile con la animación de respiración.
        float spriteWorldTopY = worldPos.y - entity->getVisualHeight();
        
        sf::Vector2i pixOrigin = target.mapCoordsToPixel(worldPos, worldView);
        sf::Vector2f screenOrigin = target.mapPixelToCoords(pixOrigin, target.getView());
        
        sf::Vector2i pixTop = target.mapCoordsToPixel({worldPos.x, spriteWorldTopY}, worldView);
        sf::Vector2f screenTop = target.mapPixelToCoords(pixTop, target.getView());

        // Position Logic (Considerando zoom)
        sf::Vector2f barPos(
            std::round(screenOrigin.x - (barSize.x * zoom) * 0.5f), 
            std::round(screenTop.y - (barSize.y * zoom) - padding)
        );
        
        // --- HP Bar Visibility Conditions ---
        bool showHpBar = false;
        if (entity == localPlayer) {
            showHpBar = true;
        } else {
            bool isSelected = (entity == targetEntity);
            bool isHovered = entity->getGlobalBounds().contains(mouseWorldPos);
            if (isSelected || isHovered) {
                showHpBar = true;
            }
        }

        mBgVertices.clear();
        mFillVertices.clear();
        mTextVertices.clear();

        if (showHpBar) {
            // Background Quad (RED TEXTURE - Full Width)
            {
                float x = barPos.x;
                float y = barPos.y;
                float w = barSize.x * zoom;
                float h = barSize.y * zoom;
                
                float tw = barSize.x;
                float th = barSize.y;

                mBgVertices.append(sf::Vertex{{x, y}, sf::Color::White, {0, 0}});
                mBgVertices.append(sf::Vertex{{x + w, y}, sf::Color::White, {tw, 0}});
                mBgVertices.append(sf::Vertex{{x, y + h}, sf::Color::White, {0, th}});
                mBgVertices.append(sf::Vertex{{x + w, y}, sf::Color::White, {tw, 0}});
                mBgVertices.append(sf::Vertex{{x + w, y + h}, sf::Color::White, {tw, th}});
                mBgVertices.append(sf::Vertex{{x, y + h}, sf::Color::White, {0, th}});
            }

            // Fill Quad (GREEN TEXTURE - Snapped to integer pixels)
            {
                float x = barPos.x;
                float y = barPos.y;
                
                int fillWidth = 0;
                if (hpPercent > 0.f) {
                    fillWidth = std::max(1, static_cast<int>(barSize.x * hpPercent));
                }
                
                float w = fillWidth * zoom;
                float h = barSize.y * zoom;
                
                // Texture Coords dinámicas (snapped to integer pixels)
                float tx = static_cast<float>(fillWidth);
                float ty = barSize.y;

                if (w > 0) {
                    mFillVertices.append(sf::Vertex{{x, y}, sf::Color::White, {0, 0}});
                    mFillVertices.append(sf::Vertex{{x + w, y}, sf::Color::White, {tx, 0}});
                    mFillVertices.append(sf::Vertex{{x, y + h}, sf::Color::White, {0, ty}});
                    mFillVertices.append(sf::Vertex{{x + w, y}, sf::Color::White, {tx, 0}});
                    mFillVertices.append(sf::Vertex{{x + w, y + h}, sf::Color::White, {tx, ty}});
                    mFillVertices.append(sf::Vertex{{x, y + h}, sf::Color::White, {0, ty}});
                }
            }
        }

        // --- DRAW TEXT (BATCHED) ---
        auto& textObj = mTextCache[entity->getName()];
        if (textObj.getString().empty()) {
            textObj.setTexture(mFontTexture);
            textObj.setString(entity->getName());
            sf::FloatRect textBounds = textObj.getLocalBounds(); 
            textObj.setOrigin({textBounds.size.x / 2.f, textBounds.size.y + textBounds.position.y}); 
            textObj.setScale({2.f, 2.f}); 
        }

        bool isSuspicious = false;
        bool isAggro = false;
        sf::Color tint = sf::Color::White;
        if (auto* mob = dynamic_cast<Mob*>(entity)) {
            if (aggroSystem && aggroSystem->isSuspicious(mob)) {
                isSuspicious = true;
            }
            if (mob->isAggro()) {
                isAggro = true;
            }

            if (mob->getStance() == MobStance::Violent || mob->isAggro()) {
                tint = sf::Color(255, 60, 60); // Violent / Aggro mob names are ALWAYS RED
            } else if (mob->getStance() == MobStance::Passive) {
                tint = sf::Color(100, 230, 120); // Green for Passive
            } else {
                tint = sf::Color(255, 230, 120); // Yellow for Neutral
            }
        } else if (entity->isAggro()) {
            isAggro = true;
            tint = sf::Color(255, 60, 60);
        }
        
        textObj.setPosition({ screenOrigin.x, barPos.y - namePadding });
        
        // Manual Transform & Append to Batch
        sf::Transform transform = textObj.getTransform();
        const sf::VertexArray& sourceVerts = textObj.getVertices();
        
        // Pass 2: Main Text (Shadow pass removed)
        for (size_t i = 0; i < sourceVerts.getVertexCount(); ++i) {
            sf::Vertex v = sourceVerts[i];
            v.position = transform.transformPoint(v.position);
            v.color = tint; // Apply color directly
            mTextVertices.append(v);
        }

        // --- DRAW OVERHEAD SYMBOL (? or !) WITH BITMAP FONT (mFontTexture) ---
        if (isSuspicious || isAggro) {
            std::string symbol = isSuspicious ? "?" : "!";
            auto& symbolObj = mTextCache["__symbol_" + symbol];
            if (symbolObj.getString().empty()) {
                symbolObj.setTexture(mFontTexture);
                symbolObj.setString(symbol);
                sf::FloatRect symbolBounds = symbolObj.getLocalBounds(); 
                symbolObj.setOrigin({std::floor(symbolBounds.size.x * 0.5f), std::floor(symbolBounds.size.y + symbolBounds.position.y)}); 
                symbolObj.setScale({2.f, 2.f}); // Clean 2.f scale matching text
            }

            sf::Color symbolColor = isSuspicious ? sf::Color(255, 230, 80) : sf::Color(255, 60, 60);
            float symbolX = std::round(screenOrigin.x);
            float symbolY = std::round(barPos.y - namePadding - 14.f * zoom);
            symbolObj.setPosition({ symbolX, symbolY });

            sf::Transform sTransform = symbolObj.getTransform();
            const sf::VertexArray& sVerts = symbolObj.getVertices();
            for (size_t i = 0; i < sVerts.getVertexCount(); ++i) {
                sf::Vertex v = sVerts[i];
                v.position = sTransform.transformPoint(v.position);
                v.color = symbolColor;
                mTextVertices.append(v);
            }
        }

        // --- DRAW TITLE (IF ANY) ---
        std::string titleName = entity->getTitleName();
        if (!titleName.empty()) {
            std::string titleCacheKey = entity->getName() + "_title_" + titleName;
            auto& titleTextObj = mTextCache[titleCacheKey];
            if (titleTextObj.getString().empty()) {
                titleTextObj.setTexture(mFontTexture);
                titleTextObj.setString(titleName);
                sf::FloatRect titleBounds = titleTextObj.getLocalBounds();
                titleTextObj.setOrigin({titleBounds.size.x / 2.f, titleBounds.size.y + titleBounds.position.y});
                titleTextObj.setScale({2.f, 2.f});
            }

            float titleY = barPos.y - namePadding - 10.f * zoom;
            titleTextObj.setPosition({ screenOrigin.x, titleY });

            sf::Transform titleTransform = titleTextObj.getTransform();
            const sf::VertexArray& titleVerts = titleTextObj.getVertices();
            sf::Color titleColor = entity->getTitleColor();

            for (size_t i = 0; i < titleVerts.getVertexCount(); ++i) {
                sf::Vertex v = titleVerts[i];
                v.position = titleTransform.transformPoint(v.position);
                v.color = titleColor;
                mTextVertices.append(v);
            }
        }

        // Render the current entity's elements to enforce relative sorting between entities
        if (showHpBar) {
            if (mRedTexture) target.draw(mBgVertices, mRedTexture);
            else target.draw(mBgVertices);

            if (mGreenTexture) target.draw(mFillVertices, mGreenTexture);
            else target.draw(mFillVertices);
        }

        if (mFontTexture && mTextVertices.getVertexCount() > 0) {
            target.draw(mTextVertices, mFontTexture);
        }
    }
}