#pragma once
#include <string>

class ConfigManager {
public:
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

    // Loads configuration from JSON and applies it to Config.h variables
    void loadConfig(const std::string& path);
    void loadIKConfig(const std::string& path);
    void loadWindConfig(const std::string& path = "assets/data/wind.json");

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
};
