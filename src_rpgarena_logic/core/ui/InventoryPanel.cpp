//InventoryPanel.cpp
#include "InventoryPanel.h"
#include "../graphics/BitmapText.h"
#include "../systems/InteractionSystem.h"
#include "../systems/GoldSystem.h"
#include <iostream> // (Para std::to_string, aunque no se usa aquí)
#include <sstream>
#include <iomanip>

// --- ¡CONSTRUCTOR MODIFICADO! ---
InventoryPanel::InventoryPanel(sf::Texture* fontTexture, std::optional<sf::Sprite>& slotSprite)
    : mFontTexture(fontTexture), mSlotSprite(slotSprite)
{
    // Tamaño por defecto (fallback) en caso de que no haya textura de fondo
    float zoom = cfg::Map::ZOOM_FACTOR;
    float slotSize = cfg::UI::BASE_SLOT_SIZE * zoom;
    float slotMargin = cfg::UI::UNIFIED_SLOT_MARGIN;
    const float gridW = cfg::UI::Inventory::COLS * slotSize + (cfg::UI::Inventory::COLS - 1) * slotMargin;
    const float gridH = cfg::UI::Inventory::ROWS * slotSize + (cfg::UI::Inventory::ROWS - 1) * slotMargin;
    mSize = { gridW + 2.f*cfg::UI::COMMON_MARGIN, gridH + 2.f*cfg::UI::COMMON_MARGIN + cfg::UI::PANEL_TITLE_HEIGHT * zoom };

    mPosition = { 0.f, 0.f }; 
    mIsBeingDragged = false;
    mItems.resize(cfg::UI::Inventory::TOTAL_SLOTS, nullptr);
}

void InventoryPanel::setOnCloseCallback(std::function<void()> callback) {
    mOnCloseCallback = callback;
}

// --- ¡FUNCIÓN 'DRAW' MODIFICADA! ---
void InventoryPanel::draw(sf::RenderTarget& target, ResourceManager& res) {
    float zoom = cfg::Map::ZOOM_FACTOR; // Zoom global para UI pixel-perfect

    // 1. Cargar y Dibujar Fondo Estandarizado
    try {
        sf::Texture& bgTex = res.getTexture("assets/ui/inventory_bg.png");
        if (!mBackgroundSprite) {
            mBackgroundSprite.emplace(bgTex);
        } else {
            mBackgroundSprite->setTexture(bgTex, true);
        }
        mBackgroundSprite->setScale({zoom, zoom});
        mBackgroundSprite->setPosition(mPosition);
        
        // Actualizamos el tamaño del panel basado EXACTAMENTE en la textura x2
        mSize = mBackgroundSprite->getGlobalBounds().size;
        mHasValidBackground = true;
        
        target.draw(*mBackgroundSprite);

        // --- DIBUJAR EL BOTON DE CERRAR EXTRUIDO ---
        try {
            sf::Texture& btnTex = res.getTexture("assets/ui/button_close.png");
            sf::Sprite closeBtn(btnTex);
            closeBtn.setScale({zoom, zoom});
            closeBtn.setPosition({
                mPosition.x + cfg::UI::Inventory::CLOSE_BTN_X * zoom,
                mPosition.y + cfg::UI::Inventory::CLOSE_BTN_Y * zoom
            });
            
            // Efecto sutil si el mouse estuviera encima (opcional, para sentirlo responsive)
            // Calculamos rápido si está adentro
            sf::Vector2i mousePosI = sf::Mouse::getPosition(); // esto asume coordenadas de screen pero
            // Lo más seguro es dibujarlo liso por ahora.
            
            target.draw(closeBtn);
        } catch (...) {
            // No pasa nada si no lo encuentra, ya es opcional
        }
    } catch (...) {
        // Fallback: Si no existe la imagen dibujada por el usuario
        mHasValidBackground = false;
        sf::RectangleShape bg(mSize);
        bg.setPosition(mPosition);
        bg.setFillColor(sf::Color(0,0,0,160));
        bg.setOutlineThickness(1.f);
        bg.setOutlineColor(sf::Color(60,60,60));
        target.draw(bg);
    }

    // 2. Calcular origen de la grilla interna
    float slotSize = cfg::UI::BASE_SLOT_SIZE * zoom;
    float slotMargin;
    
    // Utilizamos el GridOffset (es fácilmente editable desde InventoryPanel.h)
    // Se escala por el zoom global para mantener todo pixel perfect.
    sf::Vector2f origin;
    if (mHasValidBackground) {
        origin = { mPosition.x + cfg::UI::Inventory::GRID_OFFSET_X * zoom, mPosition.y + cfg::UI::Inventory::GRID_OFFSET_Y * zoom };
        slotMargin = 2.f * zoom; // <--- 2 px de gap en arte 1x
    } else {
        // Fallback origen (comportamiento antiguo)
        origin = { mPosition.x + cfg::UI::COMMON_MARGIN, mPosition.y + cfg::UI::COMMON_MARGIN + cfg::UI::PANEL_TITLE_HEIGHT * zoom };
        slotMargin = cfg::UI::UNIFIED_SLOT_MARGIN;
    }

    // 3. Recorrer filas y columnas
    for (int r = 0; r < cfg::UI::Inventory::ROWS; ++r) {
        for (int c = 0; c < cfg::UI::Inventory::COLS; ++c) {
            
            int index = r * cfg::UI::Inventory::COLS + c;

            const sf::Vector2f slotPos{
                origin.x + c * (slotSize + slotMargin),
                origin.y + r * (slotSize + slotMargin)
            };

            // A) Dibujar la caja del slot (siempre lo dibujamos sobre el fondo)
            if (mSlotSprite) {
                mSlotSprite->setPosition(slotPos);
                target.draw(*mSlotSprite);
            } else {
                drawSlotFallback(target, slotPos);
            }

            // B) Dibujar el ITEM (si existe en este índice)
            auto item = mItems[index];
            if (item) {
                try {
                    sf::Texture& tex = res.getTexture(item->texturePath);
                    sf::Sprite icon(tex);

                    if (item->textureRect.size.x > 0 && item->textureRect.size.y > 0) {
                        icon.setTextureRect(item->textureRect);
                    }
                    
                    sf::FloatRect localB = icon.getLocalBounds();
                    if (localB.size.x > 0 && localB.size.y > 0) {
                        float finalW = localB.size.x * zoom;
                        float finalH = localB.size.y * zoom;
                        
                        float currentScale = zoom;
                        float padding = 4.f * zoom;
                        bool is32x32Item = (localB.size.x == 32 || localB.size.y == 32);
                        if (!is32x32Item && (finalW > slotSize - padding || finalH > slotSize - padding)) {
                            float maxDim = std::max((float)localB.size.x, (float)localB.size.y);
                            currentScale = (slotSize - padding) / maxDim;
                            finalW = localB.size.x * currentScale;
                            finalH = localB.size.y * currentScale;
                        }
                        icon.setScale({currentScale, currentScale});
                        
                        float offsetX = std::floor((slotSize - finalW) * 0.5f);
                        float offsetY = std::floor((slotSize - finalH) * 0.5f);
                        
                        icon.setPosition({std::floor(slotPos.x + offsetX), std::floor(slotPos.y + offsetY)});

                        // Draw aura if fortified >= 6
                        if (item->fortificationLevel >= 6) {
                            ItemAuraRenderer::drawAura(target, icon, item->fortificationLevel, zoom);
                        }

                        target.draw(icon);

                        // Overlay de Calidad Correcto (solo para tamaño estándar 16x16)
                        if (item->quality != ItemQuality::Common && item->type == ItemType::Weapon && item->textureRect.size.x == 16) {
                            try {
                                sf::Texture& qTex = res.getTexture("assets/items/weapons/weapons_layout.png");
                                sf::Sprite qIcon(qTex);
                                
                                if (item->textureRect.size.x > 0 && item->textureRect.size.y > 0) {
                                    qIcon.setTextureRect(item->textureRect);
                                }
                                
                                qIcon.setScale({zoom, zoom});
                                qIcon.setPosition({slotPos.x + offsetX, slotPos.y + offsetY});
                                
                                sf::Color color = getQualityColor(item->quality);
                                qIcon.setColor(color);
                                target.draw(qIcon);
                            } catch(...) { /* ignore */ }
                        }

                        // Draw stack count for consumables
                        if (item->type == ItemType::Potion && item->stackCount > 1) {
                            BitmapText countText;
                            countText.setTexture(mFontTexture);
                            countText.setString(std::to_string(item->stackCount));
                            countText.setColor(sf::Color::White);
                            countText.setScale({zoom, zoom});

                            float tw = countText.getWidth() * zoom;
                            float th = 5.f * zoom;

                            float posX = slotPos.x + offsetX + finalW - tw - 1.f * zoom;
                            float posY = slotPos.y + offsetY + finalH - th - 1.f * zoom;

                            countText.setPosition({posX, posY});
                            target.draw(countText);
                        }
                    }
                } catch (const std::exception& e) { // Fallback Item
                    sf::RectangleShape err({slotSize/2.f, slotSize/2.f});
                    err.setPosition({slotPos.x + 10.f, slotPos.y + 10.f});
                    err.setFillColor(sf::Color::Red);
                    target.draw(err);
                }
            }

            // Interaction feedback overlay
            if (InteractionSystem::getInstance().isActive()) {
                if (InteractionSystem::getInstance().getSourceSlot() == index) {
                    sf::RectangleShape selectedOverlay({slotSize, slotSize});
                    selectedOverlay.setPosition(slotPos);
                    selectedOverlay.setFillColor(sf::Color(0, 0, 0, 150)); // Selected stone slot darkened
                    target.draw(selectedOverlay);
                } else if (item) {
                    if (!InteractionSystem::getInstance().isCompatible(item)) {
                        sf::RectangleShape dimOverlay({slotSize, slotSize});
                        dimOverlay.setPosition(slotPos);
                        dimOverlay.setFillColor(sf::Color(0, 0, 0, 180)); // Incompatible slots dimmed
                        target.draw(dimOverlay);
                    }
                }
            }
        }
    }

    // --- RENDERIZADO DEL MONEDERO (GOLD SYSTEM) ---
    if (mPlayerPtr && mFontTexture) {
        GoldSplit split = GoldSystem::splitCoins(mPlayerPtr->getBronzeCoins());
        drawCoinText(target, zoom, std::to_string(split.bronze), 163.f, 120.f);
        drawCoinText(target, zoom, std::to_string(split.silver), 145.f, 120.f);
        drawCoinText(target, zoom, std::to_string(split.gold), 127.f, 120.f);
    }
}

void InventoryPanel::drawCoinText(sf::RenderTarget& target, float zoom, const std::string& str, float basePointX, float baseY) {
    if (str.empty()) return;
    int k = static_cast<int>(str.size());
    float startX = basePointX - (k - 1) * 4.f;
    BitmapText txt;
    txt.setTexture(mFontTexture);
    txt.setString(str);
    txt.setScale({zoom, zoom});
    txt.setPosition({mPosition.x + startX * zoom, mPosition.y + baseY * zoom});
    target.draw(txt);
}

// ... Resto de funciones (getSlotAt, setItem, etc. del paso anterior) ...
int InventoryPanel::getSlotAt(sf::Vector2f mousePos) const {
    float zoom = cfg::Map::ZOOM_FACTOR;
    float slotSize = cfg::UI::BASE_SLOT_SIZE * zoom;
    float slotMargin;
    
    // Utilizamos el GridOffset (es fácilmente editable desde InventoryPanel.h)
    // Se escala por el zoom global para mantener todo pixel perfect.
    sf::Vector2f origin;
    if (mHasValidBackground) {
        origin = { mPosition.x + cfg::UI::Inventory::GRID_OFFSET_X * zoom, mPosition.y + cfg::UI::Inventory::GRID_OFFSET_Y * zoom };
        slotMargin = 2.f * zoom; // <--- 2 px de gap en arte 1x (escalado x zoom)
    } else {
        // Fallback origen (comportamiento antiguo)
        origin = { mPosition.x + cfg::UI::COMMON_MARGIN, mPosition.y + cfg::UI::COMMON_MARGIN + cfg::UI::PANEL_TITLE_HEIGHT * zoom };
        slotMargin = cfg::UI::UNIFIED_SLOT_MARGIN; // Comportamiento viejo sin textura
    }

    if (!getBounds().contains(mousePos)) return -1;
    
    for (int r = 0; r < cfg::UI::Inventory::ROWS; ++r) {
        for (int c = 0; c < cfg::UI::Inventory::COLS; ++c) {
            sf::Vector2f p{
                origin.x + c * (slotSize + slotMargin),
                origin.y + r * (slotSize + slotMargin)
            };
            
            if (sf::FloatRect(p, {slotSize, slotSize}).contains(mousePos))
                return r * cfg::UI::Inventory::COLS + c;
        }
    }
    return -1;
}

std::shared_ptr<Item> InventoryPanel::getItem(int index) {
    if (index >= 0 && index < (int)mItems.size()) {
       return mItems[index];
    }
    return nullptr;
}

void InventoryPanel::setItem(int index, std::shared_ptr<Item> item) {
    if (index >= 0 && index < (int)mItems.size()) {
        mItems[index] = item;
    }
}

void InventoryPanel::setVisible(bool visible) {
    UIPanel::setVisible(visible);
    if (!visible) {
        mIsBeingDragged = false;
    }
}

int InventoryPanel::findEmptySlot() const {
    for (int i = 0; i < (int)mItems.size(); ++i) {
        if (!mItems[i]) return i;
    }
    return -1;
}

void InventoryPanel::setPosition(sf::Vector2f pos) {
    mPosition = pos;
}

sf::FloatRect InventoryPanel::getBounds() const {
    return sf::FloatRect(mPosition, mSize);
}

bool InventoryPanel::onMousePress(sf::Vector2f mousePos) {
    float zoom = cfg::Map::ZOOM_FACTOR;
    
    // 1. Chequear si se hizo click en el botón de cerrar
    sf::FloatRect closeBtnRect;
    if (mHasValidBackground) {
        float cx = mPosition.x + cfg::UI::Inventory::CLOSE_BTN_X * zoom;
        float cy = mPosition.y + cfg::UI::Inventory::CLOSE_BTN_Y * zoom;
        float csize = cfg::UI::Inventory::CLOSE_BTN_SIZE * zoom;
        closeBtnRect = sf::FloatRect({cx, cy}, {csize, csize});
    } else {
        float oldTitleH = cfg::UI::PANEL_TITLE_HEIGHT * zoom;
        closeBtnRect = sf::FloatRect({mPosition.x + mSize.x - oldTitleH, mPosition.y}, {oldTitleH, oldTitleH});
    }

    if (closeBtnRect.contains(mousePos)) {
        if (mOnCloseCallback) mOnCloseCallback();
        return true; 
    }

    // 2. Chequear arrastre de panel
    sf::FloatRect titleBar;
    if (mHasValidBackground) {
        // La barra de título ahora es el espacio "arriba" del GridOffset
        titleBar = sf::FloatRect(mPosition, { mSize.x, cfg::UI::Inventory::GRID_OFFSET_Y * zoom });
    } else {
        titleBar = sf::FloatRect(mPosition, { mSize.x, cfg::UI::PANEL_TITLE_HEIGHT * zoom });
    }

    if (titleBar.contains(mousePos)) {
        mIsBeingDragged = true;
        mDragOffset = mousePos - mPosition;
        return true;
    }
    
    if (getBounds().contains(mousePos)) {
        return true;
    }

    return false;
}

void InventoryPanel::onMouseRelease() {
    mIsBeingDragged = false;
}

void InventoryPanel::onMouseMove(sf::Vector2f mousePos) {
    if (mIsBeingDragged) {
        mPosition.x = std::floor(mousePos.x - mDragOffset.x);
        mPosition.y = std::floor(mousePos.y - mDragOffset.y);
    }
}

// (La función drawSlotFallback)
void InventoryPanel::drawSlotFallback(sf::RenderTarget& target, sf::Vector2f pos) {
    sf::RectangleShape r({cfg::UI::UNIFIED_SLOT_SIZE, cfg::UI::UNIFIED_SLOT_SIZE});
    r.setPosition(pos);
    r.setFillColor(sf::Color(30,30,30,220));
    r.setOutlineThickness(1.f);
    r.setOutlineColor(sf::Color(90,90,90));
    target.draw(r);
}