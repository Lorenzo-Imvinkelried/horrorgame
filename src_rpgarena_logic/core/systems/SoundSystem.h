#pragma once
#include <SFML/Audio.hpp>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

class ResourceManager;

struct SoundSettings {
    float volume = 100.f;
    float pitch = 1.0f;
    bool priority = true;
    bool preload = false;
};

class SoundSystem {
public:
    SoundSystem(ResourceManager& res);

    ~SoundSystem(); // destructor, limpia la memoria

    static SoundSystem* getInstance() { return sInstance; }

    // Carga la configuración de sonidos desde assets/data/sounds.json
    bool loadConfig(const std::string& configPath = "assets/data/sounds.json");

    // Reproduce un sonido desde la ruta dada. Si defaultVolume < 0, se usa el de sounds.json
    void playSound(const std::string& path, float defaultVolume = -1.f, bool priority = true);

    // Detener todos los sonidos activos
    void stopAllSounds();

    // Estado global de mute
    static bool isMuted() { return sMuted; }
    static void setMuted(bool muted);

    // Actualiza la lógica de los sonidos si es necesario (ej: limpiar detenidos)
    void update();

    static inline bool sMuted = false; // [NEW] Persist mute state across states

private:
    ResourceManager& mRes;
    
    struct PlaybackInfo {
        std::unique_ptr<sf::Sound> sound;
        bool priority = false;
    };

    // Pool de reproductores activos
    std::vector<PlaybackInfo> mActiveSounds;

    // Mapa de configuraciones cargadas desde sounds.json
    std::unordered_map<std::string, SoundSettings> mSoundConfigs;
    float mMasterVolume = 100.f;

    // Instance estatica para el Singleton
    static inline SoundSystem* sInstance = nullptr;

};
