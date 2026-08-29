#include "PixelSpriteRenderer.h"
#include <cmath>
#include <iostream>

const sf::Image& PixelSpriteRenderer::getSubImage(const sf::Texture& texture, const sf::IntRect& rect) {
    static std::map<CacheKey, sf::Image> s_subImageCache;
    CacheKey key{ &texture, rect };
    auto it = s_subImageCache.find(key);
    if (it != s_subImageCache.end()) {
        return it->second;
    }

    sf::Image atlasImg = texture.copyToImage();
    sf::Image subImg;
    subImg.resize(sf::Vector2u(rect.size.x, rect.size.y));

    for (int y = 0; y < rect.size.y; ++y) {
        for (int x = 0; x < rect.size.x; ++x) {
            sf::Color color = atlasImg.getPixel({
                static_cast<unsigned int>(rect.position.x + x),
                static_cast<unsigned int>(rect.position.y + y)
            });
            subImg.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, color);
        }
    }

    s_subImageCache[key] = std::move(subImg);
    std::cout << "[PixelSpriteRenderer] Extracted and cached sub-image of size " << rect.size.x << "x" << rect.size.y << "\n";
    return s_subImageCache[key];
}

void cleanupPixelArt(sf::Image& image) {
    sf::Vector2u size = image.getSize();
    int width = static_cast<int>(size.x);
    int height = static_cast<int>(size.y);

    auto isSolid = [&](int x, int y) -> bool {
        if (x < 0 || x >= width || y < 0 || y >= height) return false;
        return image.getPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}).a > 15;
    };

    auto getSolidNeighbors = [&](int x, int y) -> int {
        int count = 0;
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) continue;
                if (isSolid(x + dx, y + dy)) count++;
            }
        }
        return count;
    };

    auto getOrthogonalNeighbors = [&](int x, int y) -> int {
        int count = 0;
        if (isSolid(x - 1, y)) count++;
        if (isSolid(x + 1, y)) count++;
        if (isSolid(x, y - 1)) count++;
        if (isSolid(x, y + 1)) count++;
        return count;
    };

    auto packColor = [](sf::Color c) -> std::uint32_t {
        return (static_cast<std::uint32_t>(c.r)) | 
               (static_cast<std::uint32_t>(c.g) << 8) | 
               (static_cast<std::uint32_t>(c.b) << 16) | 
               (static_cast<std::uint32_t>(c.a) << 24);
    };

    auto unpackColor = [](std::uint32_t val) -> sf::Color {
        return sf::Color(
            static_cast<std::uint8_t>(val & 0xFF),
            static_cast<std::uint8_t>((val >> 8) & 0xFF),
            static_cast<std::uint8_t>((val >> 16) & 0xFF),
            static_cast<std::uint8_t>((val >> 24) & 0xFF)
        );
    };

    // Run passes in-place until convergence or up to 4 iterations (Stage 6)
    bool changed = true;
    for (int pass = 0; pass < 4 && changed; ++pass) {
        changed = false;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                bool solid = isSolid(x, y);

                if (solid) {
                    // Check if it is an outline pixel (at least one transparent orthogonal neighbor).
                    // Interior pixels (all 4 orthogonal neighbors solid) must NEVER be pruned.
                    bool isOutline = !isSolid(x - 1, y) || !isSolid(x + 1, y) || !isSolid(x, y - 1) || !isSolid(x, y + 1);

                    if (isOutline) {
                        // 1. Remove isolated pixels (islands)
                        int neighbors = getSolidNeighbors(x, y);
                        if (neighbors <= 1) {
                            image.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color::Transparent);
                            changed = true;
                            continue;
                        }

                        // 2. Prune redundant corners (doubles) to ensure strictly 1-pixel thick outlines
                        bool N = isSolid(x, y - 1);
                        bool S = isSolid(x, y + 1);
                        bool W = isSolid(x - 1, y);
                        bool E = isSolid(x + 1, y);
                        bool NW = isSolid(x - 1, y - 1);
                        bool NE = isSolid(x + 1, y - 1);
                        bool SW = isSolid(x - 1, y + 1);
                        bool SE = isSolid(x + 1, y + 1);

                        if (N && W && !NW) {
                            image.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color::Transparent);
                            changed = true;
                        }
                        else if (N && E && !NE) {
                            image.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color::Transparent);
                            changed = true;
                        }
                        else if (S && W && !SW) {
                            image.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color::Transparent);
                            changed = true;
                        }
                        else if (S && E && !SE) {
                            image.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color::Transparent);
                            changed = true;
                        }
                    }
                } else {
                    // 3. Fill isolated 1x1 holes using majority color voting (Stage 7)
                    int ortho = getOrthogonalNeighbors(x, y);
                    if (ortho >= 3) {
                        std::map<std::uint32_t, int> colorCounts;
                        auto addNeighborColor = [&](int nx, int ny) {
                            if (isSolid(nx, ny)) {
                                sf::Color c = image.getPixel({static_cast<unsigned int>(nx), static_cast<unsigned int>(ny)});
                                colorCounts[packColor(c)]++;
                            }
                        };
                        addNeighborColor(x - 1, y);
                        addNeighborColor(x + 1, y);
                        addNeighborColor(x, y - 1);
                        addNeighborColor(x, y + 1);

                        std::uint32_t majorityPacked = 0;
                        int maxCount = 0;
                        for (const auto& pair : colorCounts) {
                            if (pair.second > maxCount) {
                                maxCount = pair.second;
                                majorityPacked = pair.first;
                            }
                        }

                        if (maxCount > 0) {
                            sf::Color fillCol = unpackColor(majorityPacked);
                            fillCol.a = 255;
                            image.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, fillCol);
                            changed = true;
                        }
                    }
                }
            }
        }
    }
}

const sf::Texture& PixelSpriteRenderer::getRotatedTexture(const sf::Texture& texture, const sf::IntRect& rect, 
                                                          int angleKey, sf::Vector2f origin, sf::Vector2f& outDestOrigin) {
    struct CacheKeyWithAngle {
        const sf::Texture* texture;
        sf::IntRect rect;
        int angleKey;
        int originX;
        int originY;

        bool operator<(const CacheKeyWithAngle& other) const {
            if (texture != other.texture) return texture < other.texture;
            if (rect.position.x != other.rect.position.x) return rect.position.x < other.rect.position.x;
            if (rect.position.y != other.rect.position.y) return rect.position.y < other.rect.position.y;
            if (rect.size.x != other.rect.size.x) return rect.size.x < other.rect.size.x;
            if (rect.size.y != other.rect.size.y) return rect.size.y < other.rect.size.y;
            if (angleKey != other.angleKey) return angleKey < other.angleKey;
            if (originX != other.originX) return originX < other.originX;
            return originY < other.originY;
        }
    };

    CacheKeyWithAngle key{ &texture, rect, angleKey, static_cast<int>(origin.x), static_cast<int>(origin.y) };
    static std::map<CacheKeyWithAngle, sf::Texture> s_rotatedTextureCache;

    // Calculate destSize first to make it available for cache hits
    int srcW = rect.size.x;
    int srcH = rect.size.y;
    float diag = std::ceil(std::sqrt(srcW * srcW + srcH * srcH));
    int destSize = static_cast<int>(diag) + 4; // Add padding to avoid edge clipping
    if (destSize % 2 != 0) destSize += 1;      // Keep it even for centering stability

    float cx_dest = destSize * 0.5f;
    float cy_dest = destSize * 0.5f;
    outDestOrigin = sf::Vector2f(cx_dest, cy_dest);

    auto it = s_rotatedTextureCache.find(key);
    if (it != s_rotatedTextureCache.end()) {
        return it->second;
    }

    const sf::Image& srcImg = getSubImage(texture, rect);

    sf::Image destImg;
    destImg.resize(sf::Vector2u(destSize, destSize), sf::Color::Transparent);

    float cx_src = origin.x;
    float cy_src = origin.y;

    float rad = angleKey * 3.14159265f / 180.f;
    float cosA = std::cos(rad);
    float sinA = std::sin(rad);

    // CPU inverse mapping with 4x4 supersampling (16 sub-pixels per destination pixel)
    for (int y = 0; y < destSize; ++y) {
        for (int x = 0; x < destSize; ++x) {
            // 1. Sample nearest-neighbor (center of the pixel)
            float dx_center = x + 0.5f - cx_dest;
            float dy_center = y + 0.5f - cy_dest;
            float src_x_center = dx_center * cosA + dy_center * sinA + cx_src;
            float src_y_center = -dx_center * sinA + dy_center * cosA + cy_src;

            int isrc_x_center = static_cast<int>(std::floor(src_x_center));
            int isrc_y_center = static_cast<int>(std::floor(src_y_center));

            sf::Color nearestColor = sf::Color::Transparent;
            if (isrc_x_center >= 0 && isrc_x_center < srcW && isrc_y_center >= 0 && isrc_y_center < srcH) {
                nearestColor = srcImg.getPixel({static_cast<unsigned int>(isrc_x_center), static_cast<unsigned int>(isrc_y_center)});
            }

            // 2. Scan 4x4 sub-pixel grid for coverage and representative color
            int solidCount = 0;
            sf::Color repColor = sf::Color::Transparent;

            for (int sy = 0; sy < 4; ++sy) {
                float sub_y = y + (sy + 0.5f) / 4.f;
                float dy = sub_y - cy_dest;

                for (int sx = 0; sx < 4; ++sx) {
                    float sub_x = x + (sx + 0.5f) / 4.f;
                    float dx = sub_x - cx_dest;

                    float src_x = dx * cosA + dy * sinA + cx_src;
                    float src_y = -dx * sinA + dy * cosA + cy_src;

                    int isrc_x = static_cast<int>(std::floor(src_x));
                    int isrc_y = static_cast<int>(std::floor(src_y));

                    if (isrc_x >= 0 && isrc_x < srcW && isrc_y >= 0 && isrc_y < srcH) {
                        sf::Color color = srcImg.getPixel({static_cast<unsigned int>(isrc_x), static_cast<unsigned int>(isrc_y)});
                        if (color.a > 15) {
                            solidCount++;
                            if (repColor == sf::Color::Transparent) {
                                repColor = color; // Capture the first non-transparent color
                            }
                        }
                    }
                }
            }

            // 3. Binary decision rule:
            // - If nearest-neighbor is solid, we draw it (alpha=255).
            // - If nearest-neighbor is empty but coverage is high (solidCount >= 5), we fill it using repColor to preserve connectivity.
            if (nearestColor.a > 15) {
                nearestColor.a = 255; // Ensure binary alpha (no blur!)
                destImg.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, nearestColor);
            } else if (solidCount >= 5) {
                repColor.a = 255; // Ensure binary alpha (no blur!)
                destImg.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, repColor);
            }
        }
    }

    cleanupPixelArt(destImg);

    sf::Texture destTex;
    (void)destTex.loadFromImage(destImg);
    destTex.setSmooth(false); // Sharp retro pixels

    s_rotatedTextureCache[key] = std::move(destTex);
    std::cout << "[PixelSpriteRenderer] Generated and cached rotated texture for angle " << angleKey << " deg (" << destSize << "x" << destSize << ")\n";
    return s_rotatedTextureCache[key];
}

void PixelSpriteRenderer::draw(sf::RenderTarget& target, const sf::Texture& texture, const sf::IntRect& rect, 
                               sf::Vector2f position, sf::Vector2f origin, sf::Vector2f scale, float angleDeg, 
                               sf::Color colorTint, sf::RenderStates states) {
    // 1. Quantize the angle to the nearest multiple of 3 degrees to optimize cache size and generation time.
    int angleVal = static_cast<int>(std::round(angleDeg)) % 360;
    if (angleVal < 0) angleVal += 360;
    int angleKey = (static_cast<int>(std::round(static_cast<float>(angleVal) / 3.f)) * 3) % 360;

    // 2. Bypass custom rendering at orthogonal angles (0, 90, 180, 270 degrees)
    // This uses standard sf::Sprite rendering to prevent sub-pixel snapping jitter during movement.
    if (angleKey == 0 || angleKey == 90 || angleKey == 180 || angleKey == 270) {
        sf::Sprite sprite(texture);
        sprite.setTextureRect(rect);
        sprite.setOrigin(origin);
        sprite.setScale(scale);
        sprite.setPosition(position);
        sprite.setRotation(sf::degrees(static_cast<float>(angleKey)));
        sprite.setColor(colorTint);
        target.draw(sprite, states);
        return;
    }

    // 2. Fetch or generate the software-rotated texture
    sf::Vector2f destOrigin;
    const sf::Texture& rotatedTex = getRotatedTexture(texture, rect, angleKey, origin, destOrigin);

    // 3. Draw using a standard, axis-aligned sf::Sprite!
    // Since the texture itself is already rotated on the CPU, we draw with rotation = 0.
    // This completely bypasses OpenGL's rotated geometry rasterizer and prevents any distortion.
    sf::Sprite sprite(rotatedTex);
    sprite.setOrigin(destOrigin);
    sprite.setScale(scale);
    
    // Snap draw position to game's virtual pixels
    float snapX = std::round(position.x / std::abs(scale.x)) * std::abs(scale.x);
    float snapY = std::round(position.y / std::abs(scale.y)) * std::abs(scale.y);
    sprite.setPosition({snapX, snapY});
    sprite.setColor(colorTint);

    target.draw(sprite, states);
}
