#pragma once

namespace Config {
    namespace Terrain {
        // Base Hills (The main shape of the world)
        constexpr float BaseFreqX = 0.04f;   // Lower = Wider hills
        constexpr float BaseFreqZ = 0.05f;
        constexpr float BaseAmplitude = 6.0f; // Higher = Taller hills

        // Detail Noise (Roughness)
        constexpr float DetailFreqX = 0.2f;
        constexpr float DetailFreqZ = 0.1f;
        constexpr float DetailAmplitude = 1.5f;
    }

    namespace Graphics {
        //320x240, 640x480, 800x600, 960x720, 1280x720, 1920x1080
        constexpr int InternalWidth = 800;
        constexpr int InternalHeight = 600;
    }

    namespace Gameplay {
        constexpr float PlayerSpeed = 6.0f;
        constexpr float DebugCamSpeed = 40.0f;
        constexpr float MonsterSpeed = 3.5f; // Initial value
    }
}
