#include "TerrainDeformSystem.h"
#include "Config.h"
#include <iostream>

TerrainDeformSystem::TerrainDeformSystem() {
    constexpr float R = 10.f;
    mEraserCircle.setRadius(R);
    mEraserCircle.setOrigin({R, R});
    mEraserCircle.setFillColor(sf::Color(0, 0, 0, 255));

    mEraseStates.blendMode = sf::BlendMode(
        sf::BlendMode::Factor::Zero,
        sf::BlendMode::Factor::One,
        sf::BlendMode::Equation::Add,
        sf::BlendMode::Factor::Zero,
        sf::BlendMode::Factor::OneMinusSrcAlpha,
        sf::BlendMode::Equation::Add
    );

    mPendingFootprints.reserve(8);

    mRegenGrassStates.blendMode = sf::BlendMode(
        sf::BlendMode::Factor::Zero, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add,
        sf::BlendMode::Factor::One, sf::BlendMode::Factor::One,  sf::BlendMode::Equation::Add
    );
    mRegenDepthStates.blendMode = sf::BlendMode(
        sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::ReverseSubtract,
        sf::BlendMode::Factor::Zero, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Add
    );
}

bool TerrainDeformSystem::init(sf::Vector2u mapSizePx, sf::Color initialColor) {
    if (mapSizePx.x == 0 || mapSizePx.y == 0) {
        std::cerr << "[TerrainDeformSystem] ERROR: Tamaño de mapa inválido.\n";
        return false;
    }

    mCpuMapSize = mapSizePx;
    mMaxDepthMap.assign(mapSizePx.x * mapSizePx.y, 0);
    mDeformTimeMap.assign(mapSizePx.x * mapSizePx.y, 0.f);
    mTotalElapsedTime = 0.f;
    mPendingFootprints.clear();
    mPendingExplosions.clear();
    mChunkPool.clear();

    (void)mGrassTexture.resize({1, 1});
    mGrassTexture.clear(sf::Color::White);
    mGrassTexture.display();
    (void)mDepthRT.resize({1, 1});
    mDepthRT.clear(sf::Color::Black);
    mDepthRT.display();

    mVisualChunkGridSize.x = (mapSizePx.x + VISUAL_CHUNK_SIZE - 1) / VISUAL_CHUNK_SIZE;
    mVisualChunkGridSize.y = (mapSizePx.y + VISUAL_CHUNK_SIZE - 1) / VISUAL_CHUNK_SIZE;
    mVisualChunks.clear();
    mVisualChunks.resize(mVisualChunkGridSize.x * mVisualChunkGridSize.y);
    mChunkHealTime.assign(mVisualChunkGridSize.x * mVisualChunkGridSize.y, 0.f);

    mDepthAddStates.blendMode = sf::BlendMode(
        sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Max,
        sf::BlendMode::Factor::One, sf::BlendMode::Factor::One, sf::BlendMode::Equation::Max
    );

    mGrassBaked  = false;
    mShallowDirtBaked = false;
    mInitialized = true;

    mRegenRect.setSize(sf::Vector2f(VISUAL_CHUNK_SIZE, VISUAL_CHUNK_SIZE));
    mRegenRect.setPosition({0.f, 0.f});

    std::cout << "[TerrainDeformSystem] Visual Chunks grid inicializada (" 
              << mVisualChunkGridSize.x << "x" << mVisualChunkGridSize.y << ") para mapa de "
              << mapSizePx.x << "x" << mapSizePx.y << " px\n";
    return true;
}

void TerrainDeformSystem::initDirtLayer(const std::string& csvPath,
                                        const std::string& dirtTilesetPath,
                                        const std::string& debugTilesetPath,
                                        unsigned tileSize,
                                        unsigned chunkSize,
                                        sf::Vector2u mapSizePx)
{
    if (!mShallowDirtMap.load(dirtTilesetPath, csvPath, tileSize, chunkSize)) {
        std::cerr << "[TerrainDeformSystem] ERROR: No se pudo cargar shallow dirt layer.\n"
                  << "  Tileset: " << dirtTilesetPath << "\n";
        mDirtLayerReady = false;
        return;
    }
    mShallowDirtBaked = true;

    if (!mDeepDirtMap.load(debugTilesetPath, csvPath, tileSize, chunkSize)) {
        std::cout << "[TerrainDeformSystem] INFO: '" << debugTilesetPath << "' no encontrado. Usando fallback.\n";
        if (!mDeepDirtMap.load(dirtTilesetPath, csvPath, tileSize, chunkSize)) {
            mDirtLayerReady = false;
            return;
        }
    }
    
    mShallowDirtMap.setAllChunksVisible(true);
    mDeepDirtMap.setAllChunksVisible(true);
    mDirtLayerReady = true;
    std::cout << "[TerrainDeformSystem] Dirt layers (A1 y A2) cargadas y bakeadas.\n";
}

void TerrainDeformSystem::bakeGrassLayer(const ChunkedTileMap& grassMap, sf::Vector2u mapSizePx) {
    if (!mInitialized) {
        std::cerr << "[TerrainDeformSystem] ERROR: bakeGrassLayer() antes de init()\n";
        return;
    }

    mGrassMap = grassMap;
    mGrassMap.setAllChunksVisible(true);
    mGrassBaked = true;
    std::cout << "[TerrainDeformSystem] Grass layer configurado para instanciación por chunks.\n";
}

void TerrainDeformSystem::clear(sf::Vector2u newMapSizePx) {
    mPendingFootprints.clear();
    mPendingExplosions.clear();
    mGrassBaked       = false;
    mShallowDirtBaked = false;
    mDirtLayerReady   = false;
    mInitialized      = false;
    mVisualChunks.clear();
    mChunkPool.clear();
    init(newMapSizePx, sf::Color::Transparent);
}

void TerrainDeformSystem::initEdgeShader() {
    if (!sf::Shader::isAvailable()) {
        std::cerr << "[TerrainDeformSystem] GPU no soporta shaders. Edge shader desactivado.\n";
        return;
    }

    static constexpr std::string_view kVertSrc = R"GLSL(
        void main() {
            gl_Position   = gl_ModelViewProjectionMatrix * gl_Vertex;
            gl_TexCoord[0]= gl_TextureMatrix[0] * gl_MultiTexCoord0;
            gl_FrontColor = gl_Color;
        }
    )GLSL";

    static constexpr std::string_view kFragSrc = R"GLSL(
        uniform sampler2D texture;
        uniform sampler2D u_DepthTex;
        uniform float u_UseDepthTex;
        uniform vec2 u_TexelSize;
        uniform float u_Offset;

        float getAbsoluteDepth(vec2 sampleUv) {
            float alpha = texture2D(texture, sampleUv).a;
            if (alpha > 0.1) {
                return 0.0;
            } else {
                float depthPx = 0.0;
                if (u_UseDepthTex > 0.5) {
                    depthPx = texture2D(u_DepthTex, sampleUv).r * 255.0;
                }
                return u_Offset + depthPx;
            }
        }

        void main() {
            vec2 uv = gl_TexCoord[0].xy;
            vec4 pixel = texture2D(texture, uv);
            float alpha = pixel.a;

            if (alpha > 0.1) {
                float distanceToHole = 999.0;
                for (int i = 1; i <= 35; ++i) {
                    vec2 checkUv = uv + vec2(0.0, u_TexelSize.y * float(i));
                    float solidity = texture2D(texture, checkUv).a;
                    if (solidity <= 0.1) {
                        distanceToHole = float(i);
                        break;
                    }
                }

                bool inOcclusionZone = false;
                if (distanceToHole < 999.0) {
                    vec2 holeUv = uv + vec2(0.0, u_TexelSize.y * distanceToHole);
                    float edgeDepth = 0.0;
                    if (u_UseDepthTex > 0.5) {
                        edgeDepth = texture2D(u_DepthTex, holeUv).r * 255.0;
                    }
                    float actualDepth = u_Offset + edgeDepth;
                    if (distanceToHole <= actualDepth) {
                        inOcclusionZone = true;
                    }
                }

                if (inOcclusionZone) {
                    gl_FragColor = vec4(0.7, 0.0, 0.9, 1.0);
                } else {
                    vec2 southUv = uv - vec2(0.0, u_TexelSize.y * 1.5);
                    float aSouth = texture2D(texture, southUv).a;
                    
                    float depthSouth = 0.0;
                    if (u_UseDepthTex > 0.5) {
                        depthSouth = texture2D(u_DepthTex, southUv).r * 255.0;
                    }
                    
                    float totalDepth = u_Offset + depthSouth;
                    float depthFactor = clamp(totalDepth * 0.5, 0.0, 1.0);
                    
                    float lip = (1.0 - aSouth) * 0.5 * depthFactor;
                    gl_FragColor = gl_Color * vec4(pixel.rgb * (1.0 - lip), 1.0);
                }
            } 
            else {
                float depthPx = 0.0;
                if (u_UseDepthTex > 0.5) {
                    depthPx = texture2D(u_DepthTex, uv).r * 255.0;
                }
                
                float actualDepth = u_Offset + depthPx;
                float totalDepth = clamp(actualDepth, 0.0, 6.0);
                
                float wallDarkness = 0.0;
                if (totalDepth > 0.0) {
                    float D_curr = getAbsoluteDepth(uv);
                    
                    float dist1 = totalDepth * 0.2;
                    float dist2 = totalDepth * 0.6;
                    float dist3 = totalDepth;
                    
                    vec2 uvN1 = uv + vec2(0.0, u_TexelSize.y * dist1);
                    vec2 uvN2 = uv + vec2(0.0, u_TexelSize.y * dist2);
                    vec2 uvN3 = uv + vec2(0.0, u_TexelSize.y * dist3);
                    
                    float D_n1 = getAbsoluteDepth(uvN1);
                    float D_n2 = getAbsoluteDepth(uvN2);
                    float D_n3 = getAbsoluteDepth(uvN3);
                    
                    float diff1 = D_curr - D_n1;
                    float diff2 = D_curr - D_n2;
                    float diff3 = D_curr - D_n3;
                    
                    float s1 = (diff1 > 0.5) ? clamp(diff1 - dist1 + 1.0, 0.0, 1.0) * clamp(diff1, 0.0, 1.0) : 0.0;
                    float s2 = (diff2 > 0.5) ? clamp(diff2 - dist2 + 1.0, 0.0, 1.0) * clamp(diff2, 0.0, 1.0) : 0.0;
                    float s3 = (diff3 > 0.5) ? clamp(diff3 - dist3 + 1.0, 0.0, 1.0) * clamp(diff3, 0.0, 1.0) : 0.0;
                    
                    wallDarkness = max(s1, max(s2, s3)) * 0.7;
                }
                
                if (wallDarkness > 0.05) {
                    gl_FragColor = vec4(0.0, 0.0, 0.0, wallDarkness);
                } else {
                    gl_FragColor = vec4(0.0);
                }
            }
        }
    )GLSL";

    if (!mEdgeShader.loadFromMemory(kVertSrc, kFragSrc)) {
        std::cerr << "[TerrainDeformSystem] ERROR: No se pudo compilar edge shader.\n";
        mEdgeShaderLoaded = false;
    } else {
        mEdgeShader.setUniform("texture", sf::Shader::CurrentTexture);
        mEdgeShaderLoaded = true;
        std::cout << "[TerrainDeformSystem] Edge shader compilado OK.\n";
    }

    static constexpr std::string_view kRegenFragSrc = R"GLSL(
        uniform sampler2D u_DepthTex;
        uniform float u_Amount;
        uniform vec2 u_TexelSize;
        uniform float u_IsGrass;

        void main() {
            vec2 uv = gl_TexCoord[0].xy;
            float dC = texture2D(u_DepthTex, uv).r;
            float dN = texture2D(u_DepthTex, uv + vec2(0.0, u_TexelSize.y * 12.0)).r;
            float dS = texture2D(u_DepthTex, uv - vec2(0.0, u_TexelSize.y * 12.0)).r;
            float dE = texture2D(u_DepthTex, uv + vec2(u_TexelSize.x * 12.0, 0.0)).r;
            float dW = texture2D(u_DepthTex, uv - vec2(u_TexelSize.x * 12.0, 0.0)).r;
            
            float maxDepth = max(dC, max(max(dN, dS), max(dE, dW)));
            float threshold = (u_IsGrass > 0.5) ? 0.001 : 0.08;
            
            if (maxDepth <= threshold) {
                gl_FragColor = vec4(0.0, 0.0, 0.0, u_Amount);
            } else {
                gl_FragColor = vec4(0.0);
            }
        }
    )GLSL";

    if (!mRegenShader.loadFromMemory(kVertSrc, kRegenFragSrc)) {
        std::cerr << "[TerrainDeformSystem] ERROR: No se pudo compilar regen shader.\n";
        mRegenShaderLoaded = false;
    } else {
        mRegenShaderLoaded = true;
        std::cout << "[TerrainDeformSystem] Regen shader compilado OK.\n";
    }

    static constexpr std::string_view kDepthStampFragSrc = R"GLSL(
        uniform sampler2D texture;
        void main() {
            vec4 texColor = texture2D(texture, gl_TexCoord[0].xy);
            gl_FragColor = vec4(gl_Color.r * texColor.a, 0.0, 0.0, 1.0);
        }
    )GLSL";

    if (!mDepthStampShader.loadFromMemory(kVertSrc, kDepthStampFragSrc)) {
        std::cerr << "[TerrainDeformSystem] ERROR: No se pudo compilar depth stamp shader.\n";
        mDepthStampShaderLoaded = false;
    } else {
        mDepthStampShader.setUniform("texture", sf::Shader::CurrentTexture);
        mDepthStampShaderLoaded = true;
        std::cout << "[TerrainDeformSystem] Depth Stamp shader compilado OK.\n";
    }
}

void TerrainDeformSystem::setFootTexture(const sf::Texture* tex, sf::Vector2f localOrigin) {
    mFootTex    = tex;
    mFootOrigin = localOrigin;
    if (tex) {
        mFootImage = tex->copyToImage();
        mEraserSprite = std::make_unique<sf::Sprite>(*tex);
        mEraserSprite->setOrigin(localOrigin);
        mEraserSprite->setColor(sf::Color::White);
        std::cout << "[TerrainDeformSystem] Foot texture: "
                  << tex->getSize().x << "x" << tex->getSize().y
                  << " | Origin: " << localOrigin.x << ", " << localOrigin.y << "\n";
    } else {
        mEraserSprite.reset();
    }
}

void TerrainDeformSystem::setExplosionTexture(const sf::Texture* tex, sf::Vector2f localOrigin) {
    mExplosionTex    = tex;
    mExplosionOrigin = localOrigin;
    if (tex) {
        mExplosionImage = tex->copyToImage();
        mExplosionSprite = std::make_unique<sf::Sprite>(*tex);
        mExplosionSprite->setOrigin(localOrigin);
        mExplosionSprite->setColor(sf::Color::White); 
        std::cout << "[TerrainDeformSystem] Explosion texture: "
                  << tex->getSize().x << "x" << tex->getSize().y
                  << " | Origin: " << localOrigin.x << ", " << localOrigin.y << "\n";
    } else {
        mExplosionSprite.reset();
    }
}
