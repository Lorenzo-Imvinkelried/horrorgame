#include "BitmapText.h"
#include <cmath>

BitmapText::BitmapText() 
    : mTexture(nullptr)
    , mVertices(sf::PrimitiveType::Triangles) // SFML 3.0: No Quads
    , mColor(sf::Color::White)
    , mShadowOffset(0.f, 0.f)
    , mShadowColor(sf::Color::Black)
{
}

void BitmapText::setTexture(const sf::Texture* texture) {
    mTexture = texture;
    // Force geometry update if string exists
    if (!mString.empty()) updateGeometry();
}

void BitmapText::setString(const std::string& str) {
    if (mString != str) {
        mString = str;
        updateGeometry();
    }
}

void BitmapText::setColor(sf::Color color) {
    mColor = color;
}

void BitmapText::setShadowOffset(sf::Vector2f offset) {
    mShadowOffset = offset;
}

void BitmapText::setShadowColor(sf::Color color) {
    mShadowColor = color;
}

sf::FloatRect BitmapText::getLocalBounds() const {
    if (!mTexture) return sf::FloatRect();
    return mVertices.getBounds();
}

sf::FloatRect BitmapText::getGlobalBounds() const {
    return getTransform().transformRect(getLocalBounds());
}

void BitmapText::updateGeometry() {
    if (!mTexture) return;

    mVertices.clear();
    
    float x = 0.f;
    float y = 0.f;

    for (size_t i = 0; i < mString.size(); ++i) {
        unsigned char rawC = static_cast<unsigned char>(mString[i]);
        
        // Handle UTF-8 (C2 and C3 prefix for Spanish chars)
        if (rawC == 0xC3 && i + 1 < mString.size()) {
            unsigned char nextC = static_cast<unsigned char>(mString[i+1]);
            // Ñ = C3 91, ñ = C3 B1
            if (nextC == 0x91 || nextC == 0xB1) { 
                rawC = 209; // Map to our Ñ logic
                i++; 
            }
            // Optional: Map accents to base letters to avoid DAO vs DAÑO-like issues if user uses them
            else if (nextC == 0x81 || nextC == 0xA1) { rawC = 'A'; i++; } // Á, á
            else if (nextC == 0x89 || nextC == 0xA9) { rawC = 'E'; i++; } // É, é
            else if (nextC == 0x8D || nextC == 0xAD) { rawC = 'I'; i++; } // Í, í
            else if (nextC == 0x93 || nextC == 0xB3) { rawC = 'O'; i++; } // Ó, ó
            else if (nextC == 0x9A || nextC == 0xBA) { rawC = 'U'; i++; } // Ú, ú
        }
        else if (rawC == 0xC2 && i + 1 < mString.size()) {
            unsigned char nextC = static_cast<unsigned char>(mString[i+1]);
            if (nextC == 0xA1) { // ¡
                rawC = 161;
                i++;
            }
            else if (nextC == 0xBF) { // ¿
                rawC = 191;
                i++;
            }
        }

        float uStrStart = 0.f;
        float glyphW = 3.f; 
        bool valid = false;

        // [USER REQUEST] Custom Glyphs
        if (rawC == ' ') {
            uStrStart = 144.f; 
            glyphW = 3.f; 
            valid = true;
        }
        else if (rawC == '|') {
            uStrStart = 143.f;
            glyphW = 1.f;
            valid = true;
        }
        
        char c = static_cast<char>(rawC);

        if (valid) {
            // Already handled Space/Pipe
        }
        else if (c >= '0' && c <= '9') {
            uStrStart = 85.f + (c - '0') * 3.f;
            glyphW = 3.f;
            valid = true;
        } 
        else {
            char upper = c;
            if (c >= 'a' && c <= 'z') upper = c - 'a' + 'A';
            
            // Handle Ñ (User Specific)
            unsigned char uChar = static_cast<unsigned char>(upper);
            if (uChar == 209 || uChar == 241) { 
                uStrStart = 44.f;
                glyphW = 3.f;
                valid = true;
            }
            else if (upper >= 'A' && upper <= 'Z') {
                valid = true;
                if (upper <= 'L') {
                    uStrStart = (upper - 'A') * 3.f;
                    glyphW = 3.f;
                }
                else if (upper == 'M') {
                    uStrStart = 36.f;
                    glyphW = 5.f;
                }
                else if (upper == 'N') {
                    uStrStart = 41.f;
                    glyphW = 3.f;
                }
                else if (upper >= 'O' && upper <= 'V') {
                    uStrStart = 47.f + (upper - 'O') * 3.f;
                    glyphW = 3.f;
                }
                else if (upper == 'W') {
                    uStrStart = 71.f;
                    glyphW = 5.f;
                }
                else if (upper >= 'X' && upper <= 'Z') {
                    uStrStart = 76.f + (upper - 'X') * 3.f;
                    glyphW = 3.f;
                }
                else {
                    valid = false;
                }
            }
        }
        
        // [SYMBOLS SUPPORT]
        if (!valid) {
            if      (c == '.') { uStrStart = 115.f; glyphW = 1.f; valid = true; }
            else if (c == ',') { uStrStart = 115.f; glyphW = 1.f; valid = true; } // Alias to Dot
            else if (c == ':') { uStrStart = 116.f; glyphW = 1.f; valid = true; }
            else if (c == '!') { uStrStart = 117.f; glyphW = 1.f; valid = true; }
            else if (rawC == 161) { uStrStart = 118.f; glyphW = 1.f; valid = true; } // ¡
            else if (c == '\'') { uStrStart = 119.f; glyphW = 1.f; valid = true; }
            else if (c == '=') { uStrStart = 120.f; glyphW = 3.f; valid = true; }
            else if (c == '(') { uStrStart = 123.f; glyphW = 2.f; valid = true; }
            else if (c == ')') { uStrStart = 125.f; glyphW = 2.f; valid = true; }
            else if (c == '+') { uStrStart = 127.f; glyphW = 3.f; valid = true; }
            else if (c == '-') { uStrStart = 130.f; glyphW = 3.f; valid = true; }
            else if (c == '?') { uStrStart = 133.f; glyphW = 2.f; valid = true; }
            else if (rawC == 191) { uStrStart = 135.f; glyphW = 2.f; valid = true; } // ¿
            else if (c == '/') { uStrStart = 137.f; glyphW = 3.f; valid = true; }
            else if (c == '%') { uStrStart = 140.f; glyphW = 3.f; valid = true; }
        }

        if (valid) {
            float tu = uStrStart;
            float tv = 0.f;

            sf::Vector2f tl(x, y);
            sf::Vector2f tr(x + glyphW, y);
            sf::Vector2f br(x + glyphW, y + GLYPH_HEIGHT);
            sf::Vector2f bl(x, y + GLYPH_HEIGHT);
            
            sf::Vector2f t_tl(tu, tv);
            sf::Vector2f t_tr(tu + glyphW, tv);
            sf::Vector2f t_br(tu + glyphW, tv + GLYPH_HEIGHT);
            sf::Vector2f t_bl(tu, tv + GLYPH_HEIGHT);
            
            mVertices.append(sf::Vertex{tl, sf::Color::White, t_tl});
            mVertices.append(sf::Vertex{tr, sf::Color::White, t_tr});
            mVertices.append(sf::Vertex{br, sf::Color::White, t_br});
            mVertices.append(sf::Vertex{br, sf::Color::White, t_br});
            mVertices.append(sf::Vertex{bl, sf::Color::White, t_bl});
            mVertices.append(sf::Vertex{tl, sf::Color::White, t_tl});

            
            x += glyphW + 1.0f; 
        } else {
            // [FALLBACK] If character is unknown, at least advance space so numbers don't overlap!
            x += 4.0f; // Treat as space width
        }
    }
    mTextWidth = x; // [NEW] Store total width
}

void BitmapText::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    if (!mTexture || mVertices.getVertexCount() == 0) return;

    sf::Transform snappedTransform = getTransform();
    sf::Vector2f pos = getPosition();
    sf::Vector2f snappedPos(std::floor(pos.x), std::floor(pos.y));
    
    // Create a NEW transform that is snapped to pixels
    // We recreate it because modifying the existing one's internal matrix is complex
    sf::Transform t;
    t.translate(snappedPos);
    t.rotate(getRotation());
    t.scale(getScale());
    t.translate(-getOrigin());

    states.transform *= t;
    states.texture = mTexture;

    // --- PASS 1: SHADOW ---
    if (mShadowOffset.x != 0.f || mShadowOffset.y != 0.f) {
        sf::RenderStates shadowStates = states;
        shadowStates.transform.translate(mShadowOffset);

        BitmapText* mutableThis = const_cast<BitmapText*>(this);
        
        // 1. Set to Shadow
        for (size_t i = 0; i < mutableThis->mVertices.getVertexCount(); ++i) {
            mutableThis->mVertices[i].color = mShadowColor;
        }
        target.draw(mVertices, shadowStates);
    }

    // --- PASS 2: MAIN TEXT ---
    // 2. Set to Tint Color
    BitmapText* mutableThis = const_cast<BitmapText*>(this);
    for (size_t i = 0; i < mutableThis->mVertices.getVertexCount(); ++i) {
        mutableThis->mVertices[i].color = mColor;
    }
    target.draw(mVertices, states);
}
