#include "SoundSystem.h"
#include "../engine/ResourceManager.h"
#include "utils/TinyJson.h"
#include "Config.h"
#include <iostream>
#include <algorithm>

SoundSystem::SoundSystem(ResourceManager& res) : mRes(res) {
    sInstance = this; // Registrar instancia activa

    // 1. Cargar configuraciones desde assets/data/sounds.json
    loadConfig("assets/data/sounds.json");

    // 2. Pre-cargar sonidos críticos o marcados con preload = true
    for (const auto& kv : mSoundConfigs) {
        if (kv.second.preload) {
            mRes.getSoundBuffer(kv.first);
        }
    }

    // Fallbacks directos para asegurar buffers base
    sf::SoundBuffer* hitBuffer = mRes.getSoundBuffer("assets/sounds/barehand_hit.wav");
    if (!hitBuffer) hitBuffer = mRes.getSoundBuffer("assets/sounds/block.wav");

    // 3. Pre-instanciar el pool de reproductores (Object Pooling)
    mActiveSounds.reserve(cfg::Audio::MAX_SOUNDS);
    if (hitBuffer) {
        for (int i = 0; i < cfg::Audio::MAX_SOUNDS; ++i) {
            mActiveSounds.push_back({ std::make_unique<sf::Sound>(*hitBuffer), false });
        }
    }
}

SoundSystem::~SoundSystem() {
    if (sInstance == this){
        sInstance = nullptr; // Limpiar puntero cuando el sistema se destruye
    }
}

bool SoundSystem::loadConfig(const std::string& configPath) {
    try {
        json::Value root = json::parseFile(configPath);
        if (root.type != json::Type::Object) {
            std::cerr << "[SoundSystem] ERROR: " << configPath << " is not a valid JSON object.\n";
            return false;
        }

        auto rootObj = root.asObject();
        if (rootObj.count("master_volume")) {
            mMasterVolume = static_cast<float>(rootObj.at("master_volume").asDouble());
        }

        if (rootObj.count("sounds") && rootObj.at("sounds").type == json::Type::Object) {
            auto soundsObj = rootObj.at("sounds").asObject();
            for (const auto& kv : soundsObj) {
                if (kv.second.type == json::Type::Object) {
                    auto sObj = kv.second.asObject();
                    SoundSettings settings;
                    if (sObj.count("volume")) settings.volume = static_cast<float>(sObj.at("volume").asDouble());
                    if (sObj.count("pitch")) settings.pitch = static_cast<float>(sObj.at("pitch").asDouble());
                    if (sObj.count("priority")) settings.priority = sObj.at("priority").asBool();
                    if (sObj.count("preload")) settings.preload = sObj.at("preload").asBool();

                    mSoundConfigs[kv.first] = settings;
                }
            }
        }
        std::cout << "[SoundSystem] Loaded " << mSoundConfigs.size() << " sound configurations from " << configPath << "\n";
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[SoundSystem] Exception loading " << configPath << ": " << e.what() << "\n";
        return false;
    }
}

void SoundSystem::playSound(const std::string& path, float volume, bool priority) {
    if (sMuted) return; // Return early if muted

    float targetVolume = volume;
    float targetPitch = 1.0f;
    bool targetPriority = priority;

    auto it = mSoundConfigs.find(path);
    if (it != mSoundConfigs.end()) {
        const auto& cfg = it->second;
        if (volume < 0.f) {
            targetVolume = cfg.volume;
        }
        targetPitch = cfg.pitch;
        targetPriority = priority && cfg.priority;
    } else {
        if (targetVolume < 0.f) {
            targetVolume = 100.f;
        }
    }

    float finalVolume = std::clamp(targetVolume * (mMasterVolume / 100.f), 0.f, 100.f);

    sf::SoundBuffer* buffer = mRes.getSoundBuffer(path);
    if (!buffer) return; // Falla silenciosamente si no hay archivo

    // 1. Buscar un reproductor que esté libre en el pool pre-allocado
    for (auto& playback : mActiveSounds) {
        if (playback.sound->getStatus() == sf::SoundSource::Status::Stopped) {
            playback.sound->setBuffer(*buffer);
            playback.sound->setVolume(finalVolume);
            playback.sound->setPitch(targetPitch);
            playback.sound->play();
            playback.priority = targetPriority;
            return; // ¡Listo, reciclado!
        }
    }

    // Si todos están ocupados pero aún no llegamos al límite, instanciamos uno nuevo
    if (mActiveSounds.size() < cfg::Audio::MAX_SOUNDS) {
        auto sound = std::make_unique<sf::Sound>(*buffer);
        sound->setVolume(finalVolume);
        sound->setPitch(targetPitch);
        sound->play();
        mActiveSounds.push_back({ std::move(sound), targetPriority });
        return;
    }

    // Si el pool está lleno y esta petición es de prioridad, robamos un canal
    if (targetPriority) {
        PlaybackInfo* candidate = nullptr;
        float minVolume = 999.0f;
        
        // 1. Buscar primero entre sonidos SIN prioridad (ej. pisadas)
        for (auto& playback : mActiveSounds) {
            if (!playback.priority) {
                float vol = playback.sound->getVolume();
                if (vol < minVolume) {
                    minVolume = vol;
                    candidate = &playback;
                }
            }
        }

        // 2. Si todos los canales activos son de prioridad, buscar el sonido de prioridad con menor volumen que el nuevo
        if (!candidate) {
            for (auto& playback : mActiveSounds) {
                float vol = playback.sound->getVolume();
                if (vol < minVolume && vol < finalVolume) {
                    minVolume = vol;
                    candidate = &playback;
                }
            }
        }

        if (candidate) {
            candidate->sound->stop();
            candidate->sound->setBuffer(*buffer);
            candidate->sound->setVolume(finalVolume);
            candidate->sound->setPitch(targetPitch);
            candidate->sound->play();
            candidate->priority = targetPriority;
        }
    }
}

void SoundSystem::stopAllSounds() {
    for (auto& playback : mActiveSounds) {
        playback.sound->stop();
    }
}

void SoundSystem::setMuted(bool muted) {
    sMuted = muted;
    if (muted && sInstance) {
        sInstance->stopAllSounds();
    }
}

void SoundSystem::update() {
    // Ya no destruimos los sonidos aquí. Los mantenemos vivos en memoria 
    // y los reutilizamos en playSound() para no sobrecargar a la CPU.
}
