#include "FXSystem.h"
#include <string>
#include <algorithm>
#include <cmath>
#include <cstring>
#include "Config.h" 
#include <iostream>
#include <cstdio>

FXSystem* FXSystem::sInstance = nullptr; 

FXSystem::FXSystem()
    : mBatchArray(sf::PrimitiveType::Triangles) // Single batch
    , mCritBatchArray(sf::PrimitiveType::Triangles) // [NEW] Critical batch
    , mRingBatch(sf::PrimitiveType::Triangles) // HitRing batch
{
    // [BITMAP FONT] Load Texture
    if (!mNumberTexture.loadFromFile("assets/fonts/font.png")) { // [UPDATED] Full alphabet
        // Fallback
        sf::Image img;
        img.resize({160, 16}, sf::Color::White);
        if (!mNumberTexture.loadFromImage(img)) {
            std::cerr << "FXSystem: Failed to load fallback texture from image." << std::endl;
        }
    }
    mNumberTexture.setSmooth(false);

    // [NEW] Load critical font (31x7)
    if (!mCritFontTexture.loadFromFile("assets/fonts/crit_font.png")) {
        std::cerr << "FXSystem: Failed to load crit_font.png" << std::endl;
    }
    mCritFontTexture.setSmooth(false);

    // [VFX] Load HitRing texture
    if (!mRingTexture.loadFromFile("assets/textures/fx/spritesheet_ring.png")) {
        std::cerr << "FXSystem: Failed to load spritesheet_ring.png" << std::endl;
    }

    if (!sInstance) {               
        sInstance = this;           
    }   

    mTexts.reserve(cfg::Player::FX_MAX_TEXTS); 
    mActiveCount = 0; 
    mHitRings.reserve(100);
    mActiveRingCount = 0;
}

FloatingText& FXSystem::getNextFreeText() {
    if (mActiveCount < mTexts.size()) {
        FloatingText& ft = mTexts[mActiveCount];
        mActiveCount++; 
        ft.isActive = true;
        ft.type = FloatingTextType::Generic;
        ft.currentScale = 1.0f;
        ft.targetScale = 1.0f;
        ft.owner = nullptr;
        ft.offsetY = 0.0f;
        return ft;
    }
    
    if (mTexts.size() >= cfg::Player::FX_MAX_TEXTS) {
        if (mActiveCount > 0) return mTexts[mActiveCount - 1];
        // Only if size > 0, otherwise crash? 
        // cfg::Player::FX_MAX_TEXTS should be > 0.
        // But reserve doesn't fill.
        // We need to ensure vector has elements.
    }
    
    // If we are here, we need to push back (if under capacity but vector not full?)
    // Actually simplicity:
    if (mTexts.size() < cfg::Player::FX_MAX_TEXTS) {
        mTexts.emplace_back();
        mActiveCount++;
        mTexts.back().isActive = true;
        mTexts.back().type = FloatingTextType::Generic;
        mTexts.back().currentScale = 1.0f;
        mTexts.back().targetScale = 1.0f;
        mTexts.back().owner = nullptr;
        mTexts.back().offsetY = 0.0f;
        return mTexts.back();
    }

    // Wrap around fallback
    return mTexts[0]; 
}

void FXSystem::update(sf::Time dt) {
    float s = dt.asSeconds();

    for (size_t i = 0; i < mActiveCount; ++i) {
        FloatingText& ft = mTexts[i];

        // Physics
        ft.position += ft.velocity * s;
        ft.lifetime -= s;

        // Animation (Lerp Scale)
        bool useLerp = false;
        if (useLerp && std::abs(ft.currentScale - ft.targetScale) > 0.01f) {
            ft.currentScale += (ft.targetScale - ft.currentScale) * 5.0f * s;
        } else {
             if (!useLerp) ft.currentScale = ft.targetScale;
        }

        // Deactivate
        if (ft.lifetime <= 0.f) {
            ft.isActive = false;
            // Swap Remove
            if (i < mActiveCount - 1) {
                mTexts[i] = mTexts[mActiveCount - 1];
                i--; 
            }
            mActiveCount--;
        }
    }

    // [VFX] Update Hit Rings
    for (size_t i = 0; i < mActiveRingCount; ++i) {
        HitRing& ring = mHitRings[i];
        ring.timer += s;
        float frameDuration = 1.0f / ring.fps;
        
        if (ring.timer >= frameDuration) {
            ring.timer -= frameDuration;
            ring.currentFrame++;
            
            if (ring.currentFrame >= 8) { // 8 frames total
                ring.active = false;
                // Swap Remove
                if (i < mActiveRingCount - 1) {
                    mHitRings[i] = mHitRings[mActiveRingCount - 1];
                    i--;
                }
                mActiveRingCount--;
            }
        }
    }
}

void FXSystem::draw(sf::RenderTarget& target) {
    mBatchArray.clear();
    
    // Frustum Culling
    sf::View view = target.getView();
    sf::FloatRect screenRect(
        view.getCenter() - view.getSize() / 2.f, 
        view.getSize()
    );
    screenRect.position.x -= 50; screenRect.position.y -= 50;
    screenRect.size.x += 100; screenRect.size.y += 100;

    static const int GLYPH_WIDTH = 3;  // [PIXEL PERFECT] 3px ancho
    static const int GLYPH_HEIGHT = 5; // [PIXEL PERFECT] 5px alto
    static const int GLYPH_PAD = 0;    // [REVERT] Texture is contiguous, but we keep screen spacing code

    mBatchArray.clear();
    mCritBatchArray.clear(); // [NEW]

    for (size_t i = 0; i < mActiveCount; ++i) {
        const FloatingText& ft = mTexts[i];
        float currentScale = ft.currentScale;
        
        // Culling
        if (!screenRect.contains(ft.position)) continue;

        // [OPTIMIZATION] Use cached C-string
        const char* p = ft.cachedText;
        
        // Count valid digits (length)
        int infoLen = 0;
        const char* pCnt = p;
        while(*pCnt) {
            char c = *pCnt;
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ' ' || c == '!' || c == '+' || c == '-') infoLen++;
            pCnt++;
        }

        if (infoLen == 0) continue;

        // Width includes 1px spacing between digits
        float width = 0.f;
        if (ft.type == FloatingTextType::Crit) {
            const char* pW = p;
            while (*pW) {
                char c = *pW;
                if (c >= '0' && c <= '9') {
                    width += 3.f;
                } else if (c == '!') {
                    width += 1.f;
                }
                if (*(pW + 1) != '\0') {
                    width += 1.f; // Spacing
                }
                pW++;
            }
            width *= currentScale;
        } else {
            width = (infoLen * GLYPH_WIDTH + (infoLen - 1)) * currentScale;
        }
        
        float height = (ft.type == FloatingTextType::Crit ? 7.f : 5.f) * currentScale;
        
        // Origin centered
        float startX = std::floor(ft.position.x - width / 2.f);
        float startY = std::floor(ft.position.y - height / 2.f);

        // --- SUB-DRAW 1: SHADOW ---
        float x = startX + 1.0f; // Shadow Offset (Reduced from 2.0)
        float y = startY + 1.0f;
        
        // Shadow Loop
        const char* pShadow = p;
        while(*pShadow) {
            unsigned char rawC = static_cast<unsigned char>(*pShadow);
            pShadow++;

            // Handle UTF-8 (C3 prefix for Spanish chars)
            if (rawC == 0xC3 && *pShadow != '\0') {
                unsigned char nextC = static_cast<unsigned char>(*pShadow);
                // Ñ = C3 91, ñ = C3 B1
                if (nextC == 0x91 || nextC == 0xB1) { 
                    rawC = 209; // Map to our Ñ logic
                    pShadow++; 
                }
                else if (nextC == 0x81 || nextC == 0xA1) { rawC = 'A'; pShadow++; } // Á
                else if (nextC == 0x89 || nextC == 0xA9) { rawC = 'E'; pShadow++; } // É
                else if (nextC == 0x8D || nextC == 0xAD) { rawC = 'I'; pShadow++; } // Í
                else if (nextC == 0x93 || nextC == 0xB3) { rawC = 'O'; pShadow++; } // Ó
                else if (nextC == 0x9A || nextC == 0xBA) { rawC = 'U'; pShadow++; } // Ú
            }

            float tu = 0.f;
            float glyphW = 3.f;
            bool valid = false;

            if (rawC == ' ') {
                x += 4.f * currentScale;
                continue;
            }

            char cDigit = static_cast<char>(rawC);
            if (ft.type == FloatingTextType::Crit) {
                if (cDigit >= '0' && cDigit <= '9') {
                    tu = (cDigit - '0') * 3.f;
                    glyphW = 3.f;
                    valid = true;
                }
                else if (cDigit == '!') {
                    tu = 30.f;
                    glyphW = 1.f;
                    valid = true;
                }
            } else {
                if (cDigit >= '0' && cDigit <= '9') {
                    tu = 85.f + (cDigit - '0') * 3.f;
                    glyphW = 3.f;
                    valid = true;
                }
                else if (cDigit == '+') {
                    tu = 127.f;
                    glyphW = 3.f;
                    valid = true;
                }
                else if (cDigit == '-') {
                    tu = 130.f;
                    glyphW = 3.f;
                    valid = true;
                }
                else {
                    char upper = cDigit;
                    if (cDigit >= 'a' && cDigit <= 'z') upper = cDigit - 'a' + 'A';
                    
                    unsigned char uChar = static_cast<unsigned char>(upper);
                    if (uChar == 209 || uChar == 241) { 
                        tu = 44.f;
                        glyphW = 3.f;
                        valid = true;
                    }
                    else if (upper >= 'A' && upper <= 'Z') {
                        valid = true;
                        if (upper <= 'L') {
                            tu = (upper - 'A') * 3.f;
                            glyphW = 3.f;
                        }
                        else if (upper == 'M') {
                            tu = 36.f;
                            glyphW = 5.f;
                        }
                        else if (upper == 'N') {
                            tu = 41.f;
                            glyphW = 3.f;
                        }
                        else if (upper >= 'O' && upper <= 'V') {
                            tu = 47.f + (upper - 'O') * 3.f;
                            glyphW = 3.f;
                        }
                        else if (upper == 'W') {
                            tu = 71.f;
                            glyphW = 5.f;
                        }
                        else if (upper >= 'X' && upper <= 'Z') {
                            tu = 76.f + (upper - 'X') * 3.f;
                            glyphW = 3.f;
                        }
                        else {
                            valid = false;
                        }
                    }
                }
            }

            if (valid) {
                 float w = glyphW * currentScale;
                 float h = (ft.type == FloatingTextType::Crit ? 7.f : 5.f) * currentScale;

                sf::Vector2f tl(x, y);
                sf::Vector2f tr(x + w, y);
                sf::Vector2f br(x + w, y + h);
                sf::Vector2f bl(x, y + h);
                
                sf::Vector2f t_tl(tu, 0.f);
                sf::Vector2f t_tr(tu + glyphW, 0.f);
                sf::Vector2f t_br(tu + glyphW, (ft.type == FloatingTextType::Crit ? 7.f : 5.f));
                sf::Vector2f t_bl(tu, (ft.type == FloatingTextType::Crit ? 7.f : 5.f));

                sf::Color shadowColor = sf::Color::Black;
                float alpha = 255;
                if (ft.lifetime < 0.5f) alpha = (ft.lifetime / 0.5f) * 255.f;
                shadowColor.a = static_cast<uint8_t>(alpha);

                sf::VertexArray& currentBatch = (ft.type == FloatingTextType::Crit) ? mCritBatchArray : mBatchArray;
                currentBatch.append(sf::Vertex{tl, shadowColor, t_tl});
                currentBatch.append(sf::Vertex{tr, shadowColor, t_tr});
                currentBatch.append(sf::Vertex{br, shadowColor, t_br});
                currentBatch.append(sf::Vertex{br, shadowColor, t_br});
                currentBatch.append(sf::Vertex{bl, shadowColor, t_bl});
                currentBatch.append(sf::Vertex{tl, shadowColor, t_tl});

                // Advance x
                x += w + (1.0f * currentScale); 
            }
        }

        // --- SUB-DRAW 2: MAIN TEXT ---
        x = startX;
        y = startY;
        
        const char* pMain = p;
        while(*pMain) {
            unsigned char rawC = static_cast<unsigned char>(*pMain);
            pMain++;

            // Handle UTF-8 (C3 prefix for Spanish chars)
            if (rawC == 0xC3 && *pMain != '\0') {
                unsigned char nextC = static_cast<unsigned char>(*pMain);
                // Ñ = C3 91, ñ = C3 B1
                if (nextC == 0x91 || nextC == 0xB1) { 
                    rawC = 209; // Map to our Ñ logic
                    pMain++; 
                }
                else if (nextC == 0x81 || nextC == 0xA1) { rawC = 'A'; pMain++; } // Á
                else if (nextC == 0x89 || nextC == 0xA9) { rawC = 'E'; pMain++; } // É
                else if (nextC == 0x8D || nextC == 0xAD) { rawC = 'I'; pMain++; } // Í
                else if (nextC == 0x93 || nextC == 0xB3) { rawC = 'O'; pMain++; } // Ó
                else if (nextC == 0x9A || nextC == 0xBA) { rawC = 'U'; pMain++; } // Ú
            }

            float tu = 0.f;
            float glyphW = 3.f;
            bool valid = false;

            if (rawC == ' ') {
                x += 4.f * currentScale;
                continue;
            }

            char cDigit = static_cast<char>(rawC);
            if (ft.type == FloatingTextType::Crit) {
                if (cDigit >= '0' && cDigit <= '9') {
                    tu = (cDigit - '0') * 3.f;
                    glyphW = 3.f;
                    valid = true;
                }
                else if (cDigit == '!') {
                    tu = 30.f;
                    glyphW = 1.f;
                    valid = true;
                }
            } else {
                if (cDigit >= '0' && cDigit <= '9') {
                    tu = 85.f + (cDigit - '0') * 3.f;
                    glyphW = 3.f;
                    valid = true;
                }
                else if (cDigit == '+') {
                    tu = 127.f;
                    glyphW = 3.f;
                    valid = true;
                }
                else if (cDigit == '-') {
                    tu = 130.f;
                    glyphW = 3.f;
                    valid = true;
                }
                else {
                    char upper = cDigit;
                    if (cDigit >= 'a' && cDigit <= 'z') upper = cDigit - 'a' + 'A';
                    
                    unsigned char uChar = static_cast<unsigned char>(upper);
                    if (uChar == 209 || uChar == 241) { 
                        tu = 44.f;
                        glyphW = 3.f;
                        valid = true;
                    }
                    else if (upper >= 'A' && upper <= 'Z') {
                        valid = true;
                        if (upper <= 'L') {
                            tu = (upper - 'A') * 3.f;
                            glyphW = 3.f;
                        }
                        else if (upper == 'M') {
                            tu = 36.f;
                            glyphW = 5.f;
                        }
                        else if (upper == 'N') {
                            tu = 41.f;
                            glyphW = 3.f;
                        }
                        else if (upper >= 'O' && upper <= 'V') {
                            tu = 47.f + (upper - 'O') * 3.f;
                            glyphW = 3.f;
                        }
                        else if (upper == 'W') {
                            tu = 71.f;
                            glyphW = 5.f;
                        }
                        else if (upper >= 'X' && upper <= 'Z') {
                            tu = 76.f + (upper - 'X') * 3.f;
                            glyphW = 3.f;
                        }
                        else {
                            valid = false;
                        }
                    }
                }
            }

            if (valid) {
                float w = glyphW * currentScale;
                float h = (ft.type == FloatingTextType::Crit ? 7.f : 5.f) * currentScale;

                sf::Vector2f tl(x, y);
                sf::Vector2f tr(x + w, y);
                sf::Vector2f br(x + w, y + h);
                sf::Vector2f bl(x, y + h);
                
                sf::Vector2f t_tl(tu, 0.f);
                sf::Vector2f t_tr(tu + glyphW, 0.f);
                sf::Vector2f t_br(tu + glyphW, (ft.type == FloatingTextType::Crit ? 7.f : 5.f));
                sf::Vector2f t_bl(tu, (ft.type == FloatingTextType::Crit ? 7.f : 5.f));

                sf::Color mainColor = ft.color;
                float alpha = 255;
                if (ft.lifetime < 0.5f) alpha = (ft.lifetime / 0.5f) * 255.f;
                mainColor.a = static_cast<uint8_t>(alpha);

                sf::VertexArray& currentBatch = (ft.type == FloatingTextType::Crit) ? mCritBatchArray : mBatchArray;
                currentBatch.append(sf::Vertex{tl, mainColor, t_tl});
                currentBatch.append(sf::Vertex{tr, mainColor, t_tr});
                currentBatch.append(sf::Vertex{br, mainColor, t_br});
                currentBatch.append(sf::Vertex{br, mainColor, t_br});
                currentBatch.append(sf::Vertex{bl, mainColor, t_bl});
                currentBatch.append(sf::Vertex{tl, mainColor, t_tl});

                // Advance x with spacing
                x += w + (1.0f * currentScale);
            }
        }
    }

    if (mBatchArray.getVertexCount() > 0) {
        target.draw(mBatchArray, &mNumberTexture);
    }
    if (mCritBatchArray.getVertexCount() > 0) {
        target.draw(mCritBatchArray, &mCritFontTexture);
    }

    // [VFX] Draw Hit Rings
    mRingBatch.clear();
    for (size_t i = 0; i < mActiveRingCount; ++i) {
        const HitRing& ring = mHitRings[i];
        
        // 32x32 frames. 8 frames horizontally
        int u = ring.currentFrame * 32;
        int v = 0;
        
        float hw = 16.f; // half width (32/2)
        float hh = 16.f;
        
        sf::Vector2f tl(ring.position.x - hw, ring.position.y - hh);
        sf::Vector2f tr(ring.position.x + hw, ring.position.y - hh);
        sf::Vector2f br(ring.position.x + hw, ring.position.y + hh);
        sf::Vector2f bl(ring.position.x - hw, ring.position.y + hh);
        
        sf::Vector2f t_tl(u, v);
        sf::Vector2f t_tr(u + 32, v);
        sf::Vector2f t_br(u + 32, v + 32);
        sf::Vector2f t_bl(u, v + 32);
        
        sf::Color ringColor = sf::Color::White;
        ringColor.a = static_cast<uint8_t>(cfg::FX::HIT_RING_ALPHA * 255.f);

        mRingBatch.append(sf::Vertex{tl, ringColor, t_tl});
        mRingBatch.append(sf::Vertex{tr, ringColor, t_tr});
        mRingBatch.append(sf::Vertex{br, ringColor, t_br});
        mRingBatch.append(sf::Vertex{br, ringColor, t_br});
        mRingBatch.append(sf::Vertex{bl, ringColor, t_bl});
        mRingBatch.append(sf::Vertex{tl, ringColor, t_tl});
    }
    
    if (mRingBatch.getVertexCount() > 0) {
        target.draw(mRingBatch, &mRingTexture);
    }
}

void FXSystem::clear() {
    mActiveCount = 0;
    mActiveRingCount = 0;
}

void FXSystem::addMiss(const sf::FloatRect& mobBounds, const void* owner) {
    // [OPTIMIZATION] Search backwards because recently created texts are at the end
    if (owner != nullptr && mActiveCount > 0) {
        for (int i = static_cast<int>(mActiveCount) - 1; i >= 0; --i) {
            // Safety: ensure index is valid (should always be true)
            if (i >= static_cast<int>(mTexts.size())) break;

            if (mTexts[i].owner == owner) {
                // 1. If it's a Miss from same owner, refresh it
                if (mTexts[i].type == FloatingTextType::Miss) {
                    mTexts[i].lifetime = 1.0f;
                    mTexts[i].position.x = mobBounds.position.x + mobBounds.size.x * 0.5f;
                    mTexts[i].position.y = mobBounds.position.y - cfg::UI::FloatingText::OFFSET_Y_MISS;
                    return;
                }
                // 2. Clear any Hit if we just missed
                else if (mTexts[i].type == FloatingTextType::Damage || 
                         mTexts[i].type == FloatingTextType::Crit ||
                         mTexts[i].type == FloatingTextType::TrueDamage) {
                    mTexts[i].lifetime = 0.0f; // Fast clear
                }
            }
        }
    }

    FloatingText& ft = getNextFreeText();
    ft.owner = owner;
    ft.type = FloatingTextType::Miss;
    ft.value = 0;
    
    // Set text to MISS
    std::strncpy(ft.cachedText, "MISS", sizeof(ft.cachedText));
    ft.cachedText[sizeof(ft.cachedText) - 1] = '\0';

    ft.color = sf::Color(200, 200, 200); // Silver/Gray for Miss

    ft.position.x = mobBounds.position.x + mobBounds.size.x * 0.5f;
    ft.position.y = mobBounds.position.y - cfg::UI::FloatingText::OFFSET_Y_MISS;

    ft.velocity = {0.f, cfg::UI::FloatingText::VELOCITY_Y_NORMAL}; 
    ft.lifetime = 1.0f;
    ft.maxLifetime = 1.0f;
    ft.currentScale = 1.0f;
    ft.targetScale = 1.0f;
}

void FXSystem::addDamageNumber(int damage, const sf::FloatRect& mobBounds, float offsetY, bool isCrit, float scale, sf::Color colorOverride, const void* owner) {
    if (offsetY < 0.1f) offsetY = cfg::UI::DAMAGE_OFFSET_BASE;
    
    // [OPTIMIZATION] Search and Replace for same owner AND same offset slot
    if (owner != nullptr && mActiveCount > 0) {
        for (int i = static_cast<int>(mActiveCount) - 1; i >= 0; --i) {
            if (i >= static_cast<int>(mTexts.size())) break;

            if (mTexts[i].owner == owner) {
                // [FIX] intelligent overwriting: only replace if the slots match
                // std::abs(mTexts[i].offsetY - offsetY) < 5.0f handles slight variations
                if (std::abs(mTexts[i].offsetY - offsetY) < 5.0f) {
                    
                    if (mTexts[i].type == FloatingTextType::Damage || mTexts[i].type == FloatingTextType::Crit) {
                        FloatingText& ft = mTexts[i];
                        ft.value = damage;
                        if (isCrit) {
                            std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d!", std::abs(damage));
                            ft.type = FloatingTextType::Crit;
                        } else {
                            std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d", std::abs(damage));
                            ft.type = FloatingTextType::Damage;
                        }
                        ft.lifetime = 1.0f;
                        ft.currentScale = 1.0f;
                        ft.targetScale = ft.currentScale;
                        ft.position.x = mobBounds.position.x + mobBounds.size.x * 0.5f;
                        ft.position.y = mobBounds.position.y - offsetY;

                        if (colorOverride != sf::Color::Transparent) {
                            ft.color = colorOverride;
                        } else {
                            ft.color = sf::Color::Red;
                        }
                        return;
                    }
                }

                // Extra: replace Miss with a hit
                if (mTexts[i].type == FloatingTextType::Miss) {
                    mTexts[i].lifetime = 0.0f; 
                }
            }
        }
    }
    
    FloatingText& ft = getNextFreeText();
    ft.owner = owner;
    ft.offsetY = offsetY; // Store the slot index
    ft.type = isCrit ? FloatingTextType::Crit : FloatingTextType::Damage;
    ft.value = damage;
    
    if (isCrit) {
        std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d!", std::abs(damage));
    } else {
        std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d", std::abs(damage));
    }
    
    ft.color = sf::Color::Red;
    if (colorOverride != sf::Color::Transparent) ft.color = colorOverride;
    
    ft.position.x = mobBounds.position.x + mobBounds.size.x * 0.5f;
    ft.position.y = mobBounds.position.y - offsetY;

    ft.velocity = {0.f, cfg::UI::FloatingText::VELOCITY_Y_NORMAL}; // Uses -10.f now
    ft.lifetime = cfg::UI::FloatingText::LIFETIME_NORMAL; // [BUG FIX] This was missing!
    ft.maxLifetime = cfg::UI::FloatingText::LIFETIME_NORMAL;
    ft.currentScale = 1.0f;
    ft.targetScale = ft.currentScale;
}

void FXSystem::addBleedNumber(int damage, const sf::FloatRect& mobBounds, const void* owner) {
    float offsetY = cfg::UI::FloatingText::OFFSET_Y_DAMAGE; // Same offset
    
    // [BLEED FIX] Independent logic. Only overwrites other BLEED texts from same owner.
    if (owner != nullptr && mActiveCount > 0) {
        for (int i = static_cast<int>(mActiveCount) - 1; i >= 0; --i) {
            if (i >= static_cast<int>(mTexts.size())) break;

            if (mTexts[i].owner == owner && mTexts[i].type == FloatingTextType::Bleed) {
                 // Refresh existing Bleed
                 FloatingText& ft = mTexts[i];
                 ft.value = damage;
                 std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d", std::abs(damage));
                 ft.lifetime = 1.0f;
                 ft.currentScale = 1.0f;
                 ft.targetScale = ft.currentScale;
                 ft.position.x = mobBounds.position.x + mobBounds.size.x * 0.5f;
                 ft.position.y = mobBounds.position.y - offsetY;
                 return;
            }
        }
    }

    FloatingText& ft = getNextFreeText();
    ft.owner = owner;
    ft.type = FloatingTextType::Bleed; // DISTINCT TYPE
    ft.value = damage;
    
    std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d", std::abs(damage));
    
    // Dark Red for Bleed
    ft.color = sf::Color(180, 0, 0); 
    
    ft.position.x = mobBounds.position.x + mobBounds.size.x * 0.5f;
    ft.position.y = mobBounds.position.y - offsetY;

    ft.velocity = {0.f, cfg::UI::FloatingText::VELOCITY_Y_NORMAL}; 
    ft.lifetime = cfg::UI::FloatingText::LIFETIME_NORMAL; 
    ft.maxLifetime = cfg::UI::FloatingText::LIFETIME_NORMAL;
    ft.currentScale = 1.0f;
    ft.targetScale = 1.0f;
}

void FXSystem::addTrueDamageNumber(int damage, const sf::FloatRect& mobBounds, float offsetY, bool isCrit, float scale, const void* owner) {
    if (offsetY < 0.1f) offsetY = cfg::UI::DAMAGE_OFFSET_BASE + 10.f; // Default for True damage

    // [OPTIMIZATION] Search and Replace for same owner AND same offset slot
    if (owner != nullptr && mActiveCount > 0) {
        for (int i = static_cast<int>(mActiveCount) - 1; i >= 0; --i) {
            if (i >= static_cast<int>(mTexts.size())) break;

            if (mTexts[i].owner == owner && std::abs(mTexts[i].offsetY - offsetY) < 5.0f) {
                // 1. Refresh existing TrueDamage
                if (mTexts[i].type == FloatingTextType::TrueDamage || mTexts[i].type == FloatingTextType::Crit) {
                    FloatingText& ft = mTexts[i];
                    ft.value = damage; 
                    if (isCrit) {
                        std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d!", std::abs(damage));
                        ft.type = FloatingTextType::Crit;
                    } else {
                        std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d", std::abs(damage));
                        ft.type = FloatingTextType::TrueDamage;
                    }
                    ft.lifetime = 1.0f;
                    ft.currentScale = 1.0f;
                    ft.targetScale = ft.currentScale;
                    ft.position.x = mobBounds.position.x + mobBounds.size.x * 0.5f;
                    ft.position.y = mobBounds.position.y - offsetY;
                    return;
                }
                // 2. Clear Miss
                else if (mTexts[i].type == FloatingTextType::Miss) {
                    mTexts[i].lifetime = 0.0f;
                }
            }
        }
    }
    
    FloatingText& ft = getNextFreeText();
    ft.owner = owner;
    ft.offsetY = offsetY;
    ft.type = isCrit ? FloatingTextType::Crit : FloatingTextType::TrueDamage;
    ft.value = damage;
     
     // [OPTIMIZATION]
     if (isCrit) {
         std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d!", std::abs(damage));
     } else {
         std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d", std::abs(damage));
     }
     
     ft.color = sf::Color::White;

     ft.position.x = mobBounds.position.x + mobBounds.size.x * 0.5f;
     ft.position.y = mobBounds.position.y - offsetY;
     
     ft.velocity = {0.f, cfg::UI::FloatingText::VELOCITY_Y_FAST}; 
     ft.lifetime = cfg::UI::FloatingText::LIFETIME_NORMAL;
     ft.maxLifetime = cfg::UI::FloatingText::LIFETIME_NORMAL;
     ft.currentScale = 1.0f;
     ft.targetScale = 1.0f;
}

void FXSystem::addHealNumber(int amount, const sf::FloatRect& targetBounds, float offsetY, const void* owner) {
    // [LIFESTEAL FIX] User wants REPLACE behavior, same as Damage.
    // If a heal text exists for this owner, overwrite it with the new value.
    if (owner != nullptr) {
        for (int i = static_cast<int>(mActiveCount) - 1; i >= 0; --i) {
            // [FIX] Match by owner, type and offset to allow multi-hit healing to stack/refresh correctly
            if (mTexts[i].owner == owner && mTexts[i].type == FloatingTextType::Heal && std::abs(mTexts[i].offsetY - offsetY) < 5.0f) {
                 FloatingText& ft = mTexts[i];
                 ft.value = amount; 
                 
                 std::snprintf(ft.cachedText, sizeof(ft.cachedText), "+%d", std::abs(ft.value));
                 
                 ft.lifetime = 1.5f;
                 ft.maxLifetime = 1.5f;
                 ft.currentScale = 1.0f;
                 ft.targetScale = ft.currentScale;
                 ft.position.x = targetBounds.position.x + targetBounds.size.x * 0.5f;
                 ft.position.y = targetBounds.position.y - offsetY;
                 return;
            }
        }
    }

    FloatingText& ft = getNextFreeText();
    ft.owner = owner;
    ft.offsetY = offsetY;
    ft.type = FloatingTextType::Heal;
    ft.value = amount;
    
    // [OPTIMIZATION]
    std::snprintf(ft.cachedText, sizeof(ft.cachedText), "+%d", std::abs(amount));
    
    ft.color = sf::Color::Green;

    ft.position.x = targetBounds.position.x + targetBounds.size.x * 0.5f;
    ft.position.y = targetBounds.position.y - offsetY;

    ft.velocity = {0.f, cfg::UI::FloatingText::VELOCITY_Y_NORMAL}; 
    ft.lifetime = cfg::UI::FloatingText::LIFETIME_NORMAL;
    ft.maxLifetime = cfg::UI::FloatingText::LIFETIME_NORMAL;
}

void FXSystem::addManaNumber(int amount, const sf::FloatRect& targetBounds, float offsetY, const void* owner) {
    if (owner != nullptr) {
        for (int i = static_cast<int>(mActiveCount) - 1; i >= 0; --i) {
            if (mTexts[i].owner == owner && mTexts[i].type == FloatingTextType::Mana && std::abs(mTexts[i].offsetY - offsetY) < 5.0f) {
                 FloatingText& ft = mTexts[i];
                 ft.value = amount; 
                 
                 std::snprintf(ft.cachedText, sizeof(ft.cachedText), "+%d", std::abs(ft.value));
                 
                 ft.lifetime = 1.5f;
                 ft.maxLifetime = 1.5f;
                 ft.currentScale = 1.0f;
                 ft.targetScale = ft.currentScale;
                 ft.position.x = targetBounds.position.x + targetBounds.size.x * 0.5f;
                 ft.position.y = targetBounds.position.y - offsetY;
                 return;
            }
        }
    }

    FloatingText& ft = getNextFreeText();
    ft.owner = owner;
    ft.offsetY = offsetY;
    ft.type = FloatingTextType::Mana;
    ft.value = amount;
    
    std::snprintf(ft.cachedText, sizeof(ft.cachedText), "+%d", std::abs(amount));
    
    ft.color = sf::Color(0, 162, 255); // A nice vibrant sky blue

    ft.position.x = targetBounds.position.x + targetBounds.size.x * 0.5f;
    ft.position.y = targetBounds.position.y - offsetY;

    ft.velocity = {0.f, cfg::UI::FloatingText::VELOCITY_Y_NORMAL}; 
    ft.lifetime = cfg::UI::FloatingText::LIFETIME_NORMAL;
    ft.maxLifetime = cfg::UI::FloatingText::LIFETIME_NORMAL;
}


void FXSystem::addExperienceNumber(int amount, const sf::FloatRect& mobBounds, const void* owner) {
    if (owner != nullptr) {
        for (int i = static_cast<int>(mActiveCount) - 1; i >= 0; --i) {
            if (mTexts[i].owner == owner && mTexts[i].type == FloatingTextType::XP) { 
                 FloatingText& ft = mTexts[i];
                 ft.value += amount; 
                 std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d", std::abs(ft.value)); // [OPTIMIZATION]

                 ft.lifetime = cfg::UI::FloatingText::LIFETIME_XP;
                 ft.maxLifetime = cfg::UI::FloatingText::LIFETIME_XP;
                 ft.currentScale = 1.0f; // [PIXEL PERFECT] No scaling 1.2
                 ft.position.x = mobBounds.position.x + mobBounds.size.x * 0.5f;
                 ft.position.y = mobBounds.position.y - cfg::UI::FloatingText::OFFSET_Y_XP; 
                 return;
            }
        }
    }

    FloatingText& ft = getNextFreeText();
    ft.owner = owner;
    ft.type = FloatingTextType::XP;
    ft.value = amount;
    
    std::snprintf(ft.cachedText, sizeof(ft.cachedText), "%d", std::abs(amount)); // [OPTIMIZATION]

    ft.color = sf::Color(255, 215, 0); 

    ft.position.x = mobBounds.position.x + mobBounds.size.x * 0.5f;
    ft.position.y = mobBounds.position.y - cfg::UI::FloatingText::OFFSET_Y_XP;

    ft.velocity = {0.f, cfg::UI::FloatingText::VELOCITY_Y_SLOW}; 
    ft.lifetime = cfg::UI::FloatingText::LIFETIME_XP; 
    ft.maxLifetime = cfg::UI::FloatingText::LIFETIME_XP;
}

void FXSystem::createFloatingText(sf::Vector2f pos, const std::string& msg, sf::Color color, int fontSize, float scale, float offsetY) {
    FloatingText& ft = getNextFreeText();
    ft.value = 0;
    ft.type = FloatingTextType::Generic;
    
    // Copy message to buffer
    std::strncpy(ft.cachedText, msg.c_str(), sizeof(ft.cachedText));
    ft.cachedText[sizeof(ft.cachedText) - 1] = '\0';

    ft.color = color;
    ft.position = {pos.x, pos.y + offsetY};
    ft.velocity = {0.f, -25.f};
    ft.lifetime = 1.5f; 
    ft.maxLifetime = 1.5f;
    ft.currentScale = scale;
    ft.targetScale = scale;
}

void FXSystem::addHitRing(sf::Vector2f pos, float fps) {
    if (mActiveRingCount < mHitRings.size()) {
        HitRing& ring = mHitRings[mActiveRingCount];
        mActiveRingCount++;
        ring.position = pos;
        ring.timer = 0.f;
        ring.fps = fps;
        ring.currentFrame = 0;
        ring.active = true;
    } else if (mHitRings.size() < 500) { // arbitrary safe limit
        HitRing ring;
        ring.position = pos;
        ring.timer = 0.f;
        ring.fps = fps;
        ring.currentFrame = 0;
        ring.active = true;
        mHitRings.push_back(ring);
        mActiveRingCount++;
    }
}

void FXSystem::setInstance(FXSystem* instance) { 
    sInstance = instance;                       
}                                                

FXSystem* FXSystem::getInstance() {              
    return sInstance;                            
}   