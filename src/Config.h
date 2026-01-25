#pragma once

namespace Config {
    namespace Terrain {
        // Base Hills (The main shape of the world)
        constexpr float BaseFreqX = 0.02f;   // Lower = Wider hills (Reduced from 0.50)
        constexpr float BaseFreqZ = 0.02f;   // Reduced from 0.65
        constexpr float BaseAmplitude = 20.0f; // Higher = Taller hills

        // Detail Noise (Roughness)
        constexpr float DetailFreqX = 0.2f;
        constexpr float DetailFreqZ = 0.1f;
        constexpr float DetailAmplitude = 1.5f;
    }

    namespace Graphics {
        //320x240, 640x480, 800x600, 960x720, 1280x720, 1920x1080
        constexpr int InternalWidth = 640;
        constexpr int InternalHeight = 480;

        // Distance Scaling (For visibility in low res)
        constexpr float DistantScaleStart = 20.0f;
        constexpr float DistantScaleFactor = 0.05f; // 5% growth per unit
        constexpr float DistantScaleMax = 5.0f;

        constexpr bool VSyncEnabled = true; // Use VSync to cap FPS and prevent tearing
    }

    namespace World {
        constexpr int ChunkSize = 16;
        constexpr float ChunkScale = 2.0f;
        constexpr int MapRadius = 24; // Reduce to 24 (Width 49) to prevent VSync drops. 32 was hitting GPU limits.
        constexpr int RenderDistance = 24; // Fog/Culling distance (keep high)
        constexpr int RenderBatchSize = 2; // NxN chunks per batch (2x2 = 4 chunks). 4x4 was too heavy on CPU gen.
        constexpr float FogDistStart = 600.0f;
        constexpr float FogDistEnd = 750.0f; // Max visibility
    }

    namespace Trees {
        constexpr float MinScale = 0.8f;
        constexpr float MaxScale = 1.6f; 
    }
    
    namespace Gameplay {
        constexpr float PlayerSpeed = 6.0f;
        constexpr float DebugCamSpeed = 60.0f;
        constexpr float MonsterSpeed = 5.5f; // Initial value
        
        // Monster Spawn - Donut Distribution //550, 600
        constexpr float MonsterSpawnMinRadius = 150.0f;
        constexpr float MonsterSpawnMaxRadius = 260.0f;
        
        // Projectile Physics (Exaggerated for gameplay feel)
        constexpr float ProjectileSpeed = 300.0f;       // Slower to see arc (was 400)
        constexpr float ProjectileGravity = -35.0f;     // Heavy drop (approx 3x real gravity)
        constexpr float ProjectileWindInfluence = 15.0f; // Strong wind effect
        constexpr float ProjectileDrag = 1.5f;          // Air resistance
    }
}
