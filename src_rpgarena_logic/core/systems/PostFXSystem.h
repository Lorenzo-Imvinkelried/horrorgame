#pragma once
// PostFXSystem.h
// =====================================================================
// Post-processing visual polish for pixel art.
// Applied as a single full-screen shader pass on the final rendered
// frame AFTER the world is drawn to the RenderTexture and BEFORE
// it is blitted to the window.
//
// Effects (all configurable via config.json → "post_fx"):
//   - Ordered Dithering (Bayer 4×4): subtle color banding reduction
//   - Film Grain: animated noise overlay
//   - Color Grading: warm/cool tone shift + slight contrast boost
//   - Pixel Grid: very faint scanline overlay
//   - Vignette: darkened edges
//   - Palette Limiting: partial posterization
//
// This system is 100% self-contained. It does NOT depend on any
// game logic classes — only on SFML + the cfg namespace.
// =====================================================================

#include <SFML/Graphics.hpp>

class PostFXSystem {
public:
    PostFXSystem();

    /// Compile the shader. Call once after the config is loaded.
    /// Returns false if shaders aren't supported or compilation fails.
    bool init();

    /// Advance internal timers (grain animation).
    void update(sf::Time dt);

    /// Apply the post-processing shader to the given sprite that is
    /// about to be drawn to the window. The sprite's texture should
    /// be the low-res RenderTexture of the world.
    /// Call this right before window.draw(sprite).
    void apply(sf::Sprite& sprite, sf::RenderTarget& window, float scale);

    /// Set desaturation level target (0.0 = normal colors, 1.0 = fully grayscale)
    void setDesaturateTarget(float target) { mDesaturateTarget = target; }
    void setDesaturateInstantly(float value) { mDesaturate = value; mDesaturateTarget = value; }

    /// Apply grayscale only shader to the given sprite (used for full screen transition on death)
    void applyGrayscale(sf::Sprite& sprite, sf::RenderTarget& target, float desaturate);

    float getDesaturate() const { return mDesaturate; }

    /// Whether the system is active and the shader compiled OK.
    bool isActive() const { return mShaderOK; }
    bool isShaderOK() const { return mShaderOK; }

private:
    sf::Shader mShader;
    bool       mShaderOK = false;
    sf::Shader mGrayscaleShader;
    bool       mGrayscaleShaderOK = false;
    bool       mActive   = false;  // from config
    float      mTime     = 0.f;    // animated timer for grain
    float      mDesaturate = 0.f;        // current grayscale level (0 to 1)
    float      mDesaturateTarget = 0.f;  // target grayscale level
};
