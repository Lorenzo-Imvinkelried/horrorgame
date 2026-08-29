#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <memory> // Para std::make_unique si hiciera falta, pero aquí usamos stack/heap simple
#include "Config.h"
#include "AnimCore.h"

class ResourceManager {
public:
    ResourceManager() {
        // --- CREAR TEXTURA DE ERROR (Fucsia Mágico) ---
        // Creamos una imagen de 32x32 píxeles en memoria
        sf::Image errorImg;
        errorImg.resize(sf::Vector2u(cfg::Resources::ERROR_TEXTURE_SIZE, cfg::Resources::ERROR_TEXTURE_SIZE), sf::Color::Magenta); // Magenta = Error estándar en gamedev
        
        // La cargamos en la textura de fallback
        if (!mErrorTexture.loadFromImage(errorImg)) {
            std::cerr << "[ResourceManager] FATAL: No se pudo crear la textura de error.\n";
        }
        mErrorTexture.setSmooth(false);
    }

    // La función principal: dame una textura para esta ruta
    sf::Texture& getTexture(const std::string& path) {
        // 1. ¿Ya la tenemos cargada?
        auto it = mTextureCache.find(path);
        if (it != mTextureCache.end()) {
            return it->second; // Devolvemos la textura existente
        }

        // 2. Si no, intentamos cargarla
        sf::Texture newTexture;
        if (!newTexture.loadFromFile(path)) {
            std::cerr << "[ResourceManager] ERROR: No se encontró: " << path << " -> Usando fallback.\n";
            
            // --- AQUÍ ESTÁ EL CAMBIO ---
            // En lugar de lanzar excepción, guardamos la textura de error en el caché
            // bajo el nombre de la ruta fallida. Así no intenta cargarla de nuevo cada frame.
            mTextureCache[path] = mErrorTexture; 
            return mTextureCache[path];
        }

        newTexture.setSmooth(false); // Ideal para pixel art
        
        // 3. Guardarla en el caché y devolverla
        mTextureCache[path] = newTexture; 
        return mTextureCache[path];
    }

    // [OPTIMIZATION] Bitmask System for Pixel Perfect Collision
    // Returns a fast-lookup boolean vector representing transparency (true = solid, false = transparent/empty)
    const std::vector<bool>& getBitmask(const std::string& path) {
        // 1. Check Cache
        auto it = mBitmaskCache.find(path);
        if (it != mBitmaskCache.end()) {
            return it->second;
        }

        // 2. Generate Bitmask (Lazy Load)
        sf::Texture& tex = getTexture(path); 
        
        sf::Image img = tex.copyToImage(); // Stop pipeline stall here (once per asset)
        sf::Vector2u sz = img.getSize();
        
        std::vector<bool> mask;
        mask.reserve(sz.x * sz.y); // Use reserve instead of resize+operator[] for bool vector potentially
        // Actually resize is fine for bool vector specialisation.
        mask.resize(sz.x * sz.y);
        
        for (unsigned int y = 0; y < sz.y; ++y) {
            for (unsigned int x = 0; x < sz.x; ++x) {
                // [STRICT TRANSPARENCY] Alpha > 0
                mask[y * sz.x + x] = (img.getPixel({x, y}).a > 0);
            }
        }
        
        mBitmaskCache[path] = std::move(mask);
        return mBitmaskCache[path];
    }
    
    // [OPTIMIZATION] Reverse Lookup for Entity Sprites
    const std::vector<bool>* getBitmask(const sf::Texture* tex) {
        if (!tex) return nullptr;
        
        // 1. Find Path by Texture Pointer (Linear Search, but cache is small)
        std::string path;
        for (const auto& pair : mTextureCache) {
            if (&pair.second == tex) {
                path = pair.first;
                break;
            }
        }
        
        if (path.empty()) {
            std::cerr << "[ResourceManager] ERROR: Texture not found in cache for Bitmask lookup.\n";
            return nullptr;
        }
        
        // 2. Wrap existing method
        return &getBitmask(path);
    }

    // [SOUND SYSTEM]
    sf::SoundBuffer* getSoundBuffer(const std::string& path) {
        if (mFailedSounds.count(path)) {
            return nullptr;
        }

        auto it = mSoundCache.find(path);
        if (it != mSoundCache.end()) {
            return &it->second; // Already loaded
        }

        sf::SoundBuffer newBuffer;
        if (!newBuffer.loadFromFile(path)) {
            std::cerr << "[ResourceManager] ERROR: No se pudo cargar el audio: " << path << " (future warnings silenced)\n";
            mFailedSounds.insert(path);
            return nullptr; 
        }

        mSoundCache[path] = std::move(newBuffer);
        return &mSoundCache[path];
    }

    // [ANIMATION CACHE]
    const AnimationClip* getAnimationClip(const std::string& path) {
        if (mFailedAnimations.count(path)) {
            return nullptr;
        }

        auto it = mAnimationCache.find(path);
        if (it != mAnimationCache.end()) {
            return &it->second;
        }

        AnimationClip clip;
        if (!clip.loadFromFile(path)) {
            // Return nullptr as animations can be optional, no loud error
            mFailedAnimations.insert(path);
            return nullptr;
        }

        mAnimationCache[path] = std::move(clip);
        return &mAnimationCache[path];
    }

    // [SKELETON CACHE]
    const SkeletonData* getSkeleton(const std::string& path) {
        if (mFailedSkeletons.count(path)) {
            return nullptr;
        }

        auto it = mSkeletonCache.find(path);
        if (it != mSkeletonCache.end()) {
            return &it->second;
        }

        SkeletonData sk;
        if (!sk.loadFromFile(path)) {
            mFailedSkeletons.insert(path);
            return nullptr;
        }

        mSkeletonCache[path] = std::move(sk);
        return &mSkeletonCache[path];
    }

    void clearAnimationCache() {
        mAnimationCache.clear();
        mFailedAnimations.clear();
    }

    size_t getTextureCacheSize() const { return mTextureCache.size(); }

    friend class Animation; // [ATLAS] Allow Animation to cache runtime atlases

private:
    std::map<std::string, sf::Texture> mTextureCache;
    std::map<std::string, std::vector<bool>> mBitmaskCache; // [NEW] RAM Cache
    std::map<std::string, sf::SoundBuffer> mSoundCache; // [AUDIO]
    std::map<std::string, AnimationClip> mAnimationCache; // [ANIMATION]
    std::map<std::string, SkeletonData> mSkeletonCache; // [SKELETON]
    std::set<std::string> mFailedAnimations; // [CACHE FAILURE]
    std::set<std::string> mFailedSounds; // [CACHE FAILURE]
    std::set<std::string> mFailedSkeletons; // [CACHE FAILURE]
    sf::Texture mErrorTexture; // La textura de seguridad
};