#pragma once

namespace Config {
    namespace Terrain {
        // Base Hills (The main shape of the world)
        inline float BaseFreqX = 0.02f;   // Lower = Wider hills (Reduced from 0.50)
        inline float BaseFreqZ = 0.02f;   // Reduced from 0.65
        inline float BaseAmplitude = 22.0f; // Higher = Taller hills

        // Detail Noise (Roughness)
        inline float DetailFreqX = 0.2f;
        inline float DetailFreqZ = 0.1f;
        inline float DetailAmplitude = 1.5f;
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
        constexpr int RenderDistance = 16; // Reduced from 24 to 16 for FPS Stability
        constexpr int RenderBatchSize = 2; // NxN chunks per batch (2x2 = 4 chunks). 4x4 was too heavy on CPU gen.
        constexpr float FogDistStart = 35.0f; // Adjusted for new distance (Dense horror fog)
        constexpr float FogDistEnd = 65.0f; // Max visibility
    }

    namespace Trees {
        constexpr float MinScale = 0.8f;
        constexpr float MaxScale = 1.6f; 
        
        // Wind Animation LOD (Distance in meters)
        // Trees beyond Near: Low FPS (5 FPS)
        // Trees beyond Far: No Animation
        constexpr float WindLodNear = 60.0f; // Increased from 40.0f as per user request (was hardcoded)
        constexpr float WindLodFar = 100.0f; // Increased from 60.0f (was hardcoded)
    }
    
    namespace Gameplay {
        constexpr float PlayerSpeed = 6.0f;
        constexpr float DebugCamSpeed = 60.0f;
        constexpr float MonsterSpeed = 6.8f; // Initial value (Slightly faster than player)
        
        // Monster Spawn - Donut Distribution //550, 600
        inline float MonsterSpawnMinRadius = 150.0f;
        inline float MonsterSpawnMaxRadius = 260.0f;
        
        // Projectile Physics (Exaggerated for gameplay feel)
        constexpr float ProjectileSpeed = 600.0f;       // Fast/Heavy (was 300)
        constexpr float ProjectileGravity = -45.0f;     // Heavy drop
        constexpr float ProjectileWindInfluence = 0.5f; // Negligible wind (Lead is heavy)
        constexpr float ProjectileDrag = 0.1f;          // Low drag (Aerodynamic)
        constexpr float GunshotSoundRange = 999999.0f; // Infinite sound range
    }

    namespace Monster {
         constexpr float TreeScanRadius = 35.0f;   // Radius of the scan
         constexpr float SteerAvoidanceForce = 6.0f; // Strength of steering away from trees
         constexpr float FlankStalkDistance = 10.0f; // Distance behind player to flank to
         constexpr float StartleDuration = 0.5f;    // Time spent frozen / screaming
         constexpr float ScentTrackSpeed = 3.3f;    // Walk speed while tracking scent
         constexpr float OrbitDistance = 1.6f;      // Ideal distance behind cover tree trunk
    }

    namespace Bird {
        constexpr float SpawnChance = 0.3f; // REDUCIDO de 1.0f (30% chance)
        constexpr float TriggerDistance = 12.0f; // REDUCIDO de 25.0f
        constexpr float FlySpeed = 6.0f; // Aumentado de 4.5f
        constexpr float HideDistance = 40.0f; // Aumentado de 30.0f
        constexpr float MaxFlightDistance = 80.0f; // NUEVO: limite distancia
        constexpr float MaxFlightTime = 8.0f; // NUEVO: tiempo maximo vuelo
        constexpr float SoundRange = 45.0f; // Range of the sound emitted when birds startle/fly away
    }
    
    namespace Scent {
        constexpr float MaxDistance = 250.0f; // Max distance scent is tracked (was 150)
        constexpr float MaxLifeTime = 120.0f;  // Seconds scent stays alive
        constexpr float SpawnInterval = 3.0f; // Seconds between puffs
        constexpr float BaseWidth = 1.0f;     // Initial width of puff
        constexpr float ExpansionRate = 0.3f; // How fast it widens
        constexpr float WindSpeed = 5.0f;     // Speed of scent travel
    }

    namespace Water {
        constexpr float Level = 2.0f; // Height of water surface
        constexpr float Chance = 0.10f; // Chance of a low area becoming a lagoon
        constexpr float Depth = 4.5f; // How much to sink the terrain in lagoons
    }
}
