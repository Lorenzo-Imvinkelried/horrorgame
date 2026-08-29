// PostFXSystem.cpp
// =====================================================================
// Self-contained post-processing pass for pixel art polish.
// See PostFXSystem.h for overview.
// =====================================================================
#include "PostFXSystem.h"
#include "Config.h"
#include <iostream>

PostFXSystem::PostFXSystem() {}

bool PostFXSystem::init() {
    mActive = cfg::PostFX::ENABLED;

    if (!sf::Shader::isAvailable()) {
        std::cerr << "[PostFXSystem] GPU does not support shaders. PostFX disabled.\n";
        mShaderOK = false;
        mGrayscaleShaderOK = false;
        return false;
    }

    // Always attempt to load the death grayscale shader
    mGrayscaleShaderOK = mGrayscaleShader.loadFromFile("assets/shaders/grayscale.frag", sf::Shader::Type::Fragment);
    if (!mGrayscaleShaderOK) {
        std::cerr << "[PostFXSystem] ERROR: Failed to load assets/shaders/grayscale.frag\n";
    }

    // Always load post_fx.frag so sunset/sunrise weather color grading works
    mShaderOK = mShader.loadFromFile("assets/shaders/post_fx.frag", sf::Shader::Type::Fragment);
    if (!mShaderOK) {
        std::cerr << "[PostFXSystem] ERROR: Failed to load PostFX shader from file.\n";
        return false;
    }

    mShader.setUniform("u_Texture", sf::Shader::CurrentTexture);
    std::cout << "[PostFXSystem] PostFX shader loaded OK.\n";
    return true;
}

void PostFXSystem::update(sf::Time dt) {
    mTime += dt.asSeconds();
    // Wrap to avoid precision loss on long sessions
    if (mTime > 1000.f) mTime -= 1000.f;

    // Fast transition to/from desaturate target (grayscale upon death)
    const float fadeSpeed = 2.5f; // transition takes ~0.4s
    if (mDesaturate < mDesaturateTarget) {
        mDesaturate = std::min(1.f, mDesaturate + dt.asSeconds() * fadeSpeed);
    } else if (mDesaturate > mDesaturateTarget) {
        mDesaturate = std::max(0.f, mDesaturate - dt.asSeconds() * fadeSpeed);
    }
}

template <typename T>
T lerp(const T& a, const T& b, float t) {
    return a + t * (b - a);
}

void PostFXSystem::apply(sf::Sprite& sprite, sf::RenderTarget& window, float scale) {
    if (!mShaderOK) return;

    static bool firstFrame = true;
    if (firstFrame) {
        std::cout << "[PostFXSystem] Applying shader for the first time. Scale: " << scale << "\n";
        firstFrame = false;
    }

    // ---- Set Uniforms from config ----
    mShader.setUniform("u_Time", mTime);
    mShader.setUniform("u_Scale", scale);

    mShader.setUniform("u_DitherStrength",   cfg::PostFX::DITHER_STRENGTH);
    mShader.setUniform("u_GrainStrength",    cfg::PostFX::GRAIN_STRENGTH);
    mShader.setUniform("u_GridStrength",     cfg::PostFX::GRID_STRENGTH);
    mShader.setUniform("u_VignetteStrength", cfg::PostFX::VIGNETTE_STRENGTH);

    mShader.setUniform("u_PaletteLevels",    cfg::PostFX::PALETTE_LEVELS);
    mShader.setUniform("u_UsePalette",       cfg::PostFX::PALETTE_ENABLED ? 1.0f : 0.0f);

    // Dynamic Time-of-Day Color Grading based on cfg::Shadow::SUN_ANGLE
    float angle = cfg::Shadow::SUN_ANGLE;
    if (angle < -75.f) angle = -75.f;
    if (angle > 75.f) angle = 75.f;

    float brightness = cfg::PostFX::BRIGHTNESS;
    float contrast = cfg::PostFX::CONTRAST;
    sf::Glsl::Vec3 tint(cfg::PostFX::TINT_R, cfg::PostFX::TINT_G, cfg::PostFX::TINT_B);

    if (angle < 0.f) {
        // Lerp Morning (-75) -> Noon (0)
        float t = (angle - (-75.f)) / 75.f;
        brightness = lerp(-0.07f, 0.02f, t);
        contrast = lerp(0.95f, 1.05f, t);
        sf::Vector3f morning(0.82f, 0.88f, 1.05f);
        sf::Vector3f noon(1.02f, 1.00f, 0.95f);
        sf::Vector3f finalTint = lerp(morning, noon, t);
        tint = sf::Glsl::Vec3(finalTint.x, finalTint.y, finalTint.z);
    } else {
        // Lerp Noon (0) -> Sunset (75)
        float t = angle / 75.f;
        brightness = lerp(0.02f, -0.02f, t);
        contrast = lerp(1.05f, 1.12f, t);
        sf::Vector3f noon(1.02f, 1.00f, 0.95f);
        sf::Vector3f sunset(1.22f, 0.82f, 0.62f);
        sf::Vector3f finalTint = lerp(noon, sunset, t);
        tint = sf::Glsl::Vec3(finalTint.x, finalTint.y, finalTint.z);
    }

    mShader.setUniform("u_Brightness",       brightness);
    mShader.setUniform("u_Contrast",         contrast);
    mShader.setUniform("u_TintColor",        tint);
    mShader.setUniform("u_Desaturate",       0.f);

    // Resolution of the low-res texture (for grid / dither pixel alignment)
    const sf::Texture& tex = sprite.getTexture();
    sf::Vector2u sz = tex.getSize();
    mShader.setUniform("u_Resolution", sf::Glsl::Vec2(
        static_cast<float>(sz.x), static_cast<float>(sz.y)));

    // Attach shader to the sprite draw
    sf::RenderStates states;
    states.shader = &mShader;
    window.draw(sprite, states);
}

void PostFXSystem::applyGrayscale(sf::Sprite& sprite, sf::RenderTarget& target, float desaturate) {
    if (!mGrayscaleShaderOK || desaturate <= 0.001f) {
        target.draw(sprite);
        return;
    }
    mGrayscaleShader.setUniform("texture", sf::Shader::CurrentTexture);
    mGrayscaleShader.setUniform("u_Desaturate", desaturate);
    sf::RenderStates states;
    states.shader = &mGrayscaleShader;
    target.draw(sprite, states);
}
