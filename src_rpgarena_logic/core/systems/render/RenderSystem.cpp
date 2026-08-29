#include "RenderSystem.h"
#include "Config.h"
#include <algorithm>
#include <iostream>

RenderSystem::RenderSystem() {
  mRenderQueueDynamic.reserve(128);
  mRenderQueueCombined.reserve(4200);
  mVisibleDecorCache.reserve(2048);
  mTempVerts.reserve(64);

  (void)mSelectionRT.resize({256, 256});
  mSelectionRT.setSmooth(false);

  if (sf::Shader::isAvailable()) {
    sf::Image contourImg({1, 1}, sf::Color(
        static_cast<uint8_t>(std::clamp(cfg::Shadow::CONTOUR_COLOR_R, 0, 255)),
        static_cast<uint8_t>(std::clamp(cfg::Shadow::CONTOUR_COLOR_G, 0, 255)),
        static_cast<uint8_t>(std::clamp(cfg::Shadow::CONTOUR_COLOR_B, 0, 255))
    ));
    if (!mContourTexture.loadFromImage(contourImg)) {
      (void)mContourTexture.loadFromFile(
          "assets/shaders/contorno_sombra_shader.png");
    }
    mContourTexture.setSmooth(false);

    const std::string activeFragShader = "assets/shaders/occlusion_sobel.frag";

    if (mOcclusionShader.loadFromFile("assets/shaders/occlusion_25d.vert",
                                      activeFragShader)) {
      mOcclusionShaderLoaded = true;
      mOcclusionShader.setUniform("playerTexture", sf::Shader::CurrentTexture);
      mOcclusionShader.setUniform("u_ContourTexture", mContourTexture);
      std::cout << "[RenderSystem] Cargado shader activo de oclusión: "
                << activeFragShader << std::endl;
    } else {
      std::cerr << "[RenderSystem] ERROR: No se pudo cargar "
                << activeFragShader << "\n";
    }

    if (mGroundShadowShader.loadFromFile("assets/shaders/ground_shadow.frag",
                                         sf::Shader::Type::Fragment)) {
      mGroundShadowShaderLoaded = true;
      mGroundShadowShader.setUniform("texture", sf::Shader::CurrentTexture);
    } else {
      std::cerr
          << "[RenderSystem] ERROR: No se pudo cargar ground_shadow.frag.\n";
    }

    if (mShadowEncodeShader.loadFromFile("assets/shaders/shadow_encode.frag",
                                         sf::Shader::Type::Fragment)) {
      mShadowEncodeShaderLoaded = true;
      mShadowEncodeShader.setUniform("texture", sf::Shader::CurrentTexture);
    } else {
      std::cerr
          << "[RenderSystem] ERROR: No se pudo cargar shadow_encode.frag.\n";
    }

    if (mDynamicShadowEncodeShader.loadFromFile(
            "assets/shaders/occlusion_25d.vert",
            "assets/shaders/dynamic_shadow_encode.frag")) {
      mDynamicShadowShaderLoaded = true;
      mDynamicShadowEncodeShader.setUniform("texture",
                                            sf::Shader::CurrentTexture);
    } else {
      std::cerr << "[RenderSystem] ERROR: No se pudo cargar "
                   "dynamic_shadow_encode.frag.\n";
    }

    if (mHeightEncodeShader.loadFromFile("assets/shaders/occlusion_25d.vert",
                                         "assets/shaders/height_encode.frag")) {
      mHeightEncodeShaderLoaded = true;
      mHeightEncodeShader.setUniform("texture", sf::Shader::CurrentTexture);
    } else {
      std::cerr
          << "[RenderSystem] ERROR: No se pudo cargar height_encode.frag.\n";
    }

    if (mSelectionOutlineShader.loadFromFile(
            "assets/shaders/selection_outline.frag",
            sf::Shader::Type::Fragment)) {
      mSelectionOutlineShaderLoaded = true;
      mSelectionOutlineShader.setUniform("texture", sf::Shader::CurrentTexture);
    } else {
      std::cerr << "[RenderSystem] ERROR: No se pudo cargar "
                   "selection_outline.frag.\n";
    }
  }
}
