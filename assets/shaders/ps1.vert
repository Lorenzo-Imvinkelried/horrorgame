#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;
// Tree/Default Attributes
layout (location = 4) in vec4 aInstanceData; // .xyz = pos, .w = scale
layout (location = 5) in float aInstanceYaw;  // Rotation (Radians)

// Bird Attributes (Separate to avoid conflict)
layout (location = 6) in vec4 aBirdInstanceData;
layout (location = 7) in float aBirdYaw;

out vec3 vColor;
out vec2 vTexCoord;
out float vDist; // Distance for Fog
out float vTreeAlphaFactor;

uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform float u_Time;
uniform bool u_IsInstanced;
uniform float u_WindStrength; // Control wind intensity from CPU (0.0 = static)

uniform vec2 u_Resolution;
uniform bool u_Snap;
uniform bool u_ConformToTerrain;
uniform bool u_ParticleMode; // Skip lighting for particles
uniform bool u_UseBirdAttribs; // Toggle for Bird attribute source

uniform vec2 u_WindDirection;

uniform float u_LodDistNear; // Config::Trees::WindLodNear
uniform float u_LodDistFar;  // Config::Trees::WindLodFar

uniform bool u_IsNight;
uniform float u_Darkness;
uniform bool u_FlashlightEnabled;
uniform vec3 u_PlayerPos;
uniform vec3 u_PlayerFront;
uniform bool u_IsPlayerTreePass;
uniform vec4 u_PlayerTreeData;

// Terrain Math REMOVED (CPU Only)

void main()
{
    vec3 worldPos = aPos;
    if (u_IsInstanced) {
        vec4 iData;
        float iYaw;
        if (u_IsPlayerTreePass) {
            iData = u_PlayerTreeData;
            iYaw = 0.0;
        } else {
            iData = u_UseBirdAttribs ? aBirdInstanceData : aInstanceData;
            iYaw = u_UseBirdAttribs ? aBirdYaw : aInstanceYaw;

            // Clip player's tree instance in normal pass
            if (!u_UseBirdAttribs && u_PlayerTreeData.w > 0.0) {
                if (distance(iData.xz, u_PlayerTreeData.xz) < 0.1) {
                    gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
                    return;
                }
            }
        }

        // Apply scaling
        worldPos *= iData.w;
        
        // Apply Rotation (Yaw around Y)
        float c = cos(iYaw);
        float s = sin(iYaw);
        float newX = worldPos.x * c + worldPos.z * s;
        float newZ = worldPos.x * -s + worldPos.z * c;
        worldPos.x = newX;
        worldPos.z = newZ;

        // Apply Translation
        worldPos += iData.xyz;
    }

    // -- SHADOW/FOOTPRINT SNAPPING --
    // REMOVED: Procedural Height Snapping (caused mismatch with CPU Noise Terrain).
    // Now we rely strictly on the Y coordinate passed in aInstanceData or aPos.

    // -- WIND SYSTEM --
    // STRUCTURAL SOLUTION (Final): Explicit Control from CPU.
    // Trunks are drawn with u_WindStrength = 0.0 (Static).
    // Leaves are drawn with u_WindStrength = 1.0 (Mobile).
    // OPTIMIZED WIND SYSTEM (LOD + DistSq)
    if (u_WindStrength > 0.0) {
        float windTime = u_Time;
        float windEnabled = 1.0;
        
        // Use View Space position of the INSTANCE to get distance
        vec4 iData = u_UseBirdAttribs ? aBirdInstanceData : aInstanceData;
        vec4 viewInst = u_View * vec4(iData.xyz, 1.0);
        float distSq = dot(viewInst.xyz, viewInst.xyz);
        
        float nearSq = u_LodDistNear * u_LodDistNear;
        float farSq = u_LodDistFar * u_LodDistFar;
        
        if (distSq > farSq) { 
            // Far: DISABLE WIND COMPLETELY
            windEnabled = 0.0;
        } 
        else if (distSq > nearSq) {
            // Mid: Low FPS (5 FPS)
            windTime = floor(windTime * 5.0) * 0.2; 
            windEnabled = 0.5; // Reduced amplitude
        }
        
        if (windEnabled > 0.0) {
            // Bias wave to be [0.0, 1.0]
            float rawWave = sin(windTime * 3.0 + worldPos.x * 0.2 + worldPos.z * 0.2); 
            float wave = (rawWave + 1.0) * 0.5;
            wave = wave * wave; // Sharp gusts

            // Height Factor: 0.0 at branch connection (aPos.y <= 4.0), smoothly scaling to top
            float normalizedH = clamp((aPos.y - 4.0) / 10.0, 0.0, 1.0);
            float hFactor = normalizedH * normalizedH; // EXACTLY 0.0 at trunk connection!

            // Apply Wind
            worldPos.xz += u_WindDirection * (wave * 0.40) * hFactor * u_WindStrength * windEnabled;
        }
    }

    // -- LIGHTING --
    vec3 litColor;
    if (u_ParticleMode) {
        litColor = aColor; // Use raw color
    } else {
        float ambientStrength = 0.6;
        float diffFactor = 0.4;
        vec3 lightDir = normalize(vec3(0.8, 0.6, 0.3)); // 10 AM Sun Angle
        
        if (u_IsNight) {
            ambientStrength = u_Darkness;
            diffFactor = 0.0; // Disable daylight directional light
        }
        
        float diff = max(dot(aNormal, lightDir), 0.0);
        vec3 light = vec3(ambientStrength + diff * diffFactor);
        
        // --- TORCH FIRE LIGHTING (Warm Amber Glow & Flicker) ---
        vec3 fireColor = vec3(1.0, 0.74, 0.38); // Warm fire amber
        float flameFlicker = 0.88 + 0.12 * sin(u_Time * 14.0 + sin(u_Time * 32.0));

        // 1. Player Hand Torch (Omnidirectional Point Light)
        if (u_TorchActive) {
            vec3 toTorch = u_TorchPos - worldPos;
            float dist = length(toTorch);
            if (dist < 24.0) {
                vec3 lightVec = normalize(toTorch);
                float att = clamp(1.0 - (dist / 24.0), 0.0, 1.0);
                att = att * att;
                float torchDiff = max(dot(aNormal, lightVec), 0.0);
                light += fireColor * (att * flameFlicker * (0.35 + torchDiff * 0.75));
            }
        }

        // 2. Placed World Torches
        for (int i = 0; i < 8; ++i) {
            if (i >= u_NumWorldTorches) break;
            vec3 toT = u_WorldTorches[i].xyz - worldPos;
            float dist = length(toT);
            if (dist < 20.0) {
                vec3 lightVec = normalize(toT);
                float att = clamp(1.0 - (dist / 20.0), 0.0, 1.0);
                att = att * att;
                float tDiff = max(dot(aNormal, lightVec), lightVec.y > 0.0 ? lightVec.y : 0.0);
                light += fireColor * (att * flameFlicker * u_WorldTorches[i].w * (0.32 + tDiff * 0.72));
            }
        }
        
        litColor = aColor * light;
    }

    // -- INSTANCE VARIATION (Trees) --
    if (u_IsInstanced) {
        vec4 iData = u_UseBirdAttribs ? aBirdInstanceData : aInstanceData;
        
        // Pseudo-random hash based on tree position (XZ)
        float hash = fract(sin(dot(iData.xz, vec2(12.9898, 78.233))) * 43758.5453);
        // Vary mostly Green, slightly Red/Blue for "Tonal" difference
        // range: 0.0 to 0.15 extra brightness/color
        vec3 tint = vec3(hash * 0.05, hash * 0.15, hash * 0.05);
        litColor += tint * 0.5; // Subtle blend
    }

    // -- SNAP & TRANSFORM --
    vec4 viewPos = u_View * u_Model * vec4(worldPos, 1.0);
    vDist = length(viewPos.xyz); // Linear distance to camera

    vec4 clipPos = u_Projection * viewPos;
    vec4 snappedPos = clipPos;
    // NUCLEAR BYPASS: FORCE OFF SNAPPING
    if (u_Snap && clipPos.w > 0.0) {
        vec2 grid = u_Resolution;
        vec2 snap = clipPos.xy / clipPos.w;
        snap = floor(snap * grid) / grid;
        snappedPos.xy = snap * clipPos.w;
    }
    
    gl_Position = snappedPos;
    vColor = litColor;
    vTexCoord = aTexCoord;

    // Foliage transparency calculation
    vTreeAlphaFactor = 1.0;
    if (u_IsInstanced && !u_UseBirdAttribs) {
        vec4 iData = u_IsPlayerTreePass ? u_PlayerTreeData : aInstanceData;
        vec2 diff2D = vec2(u_PlayerPos.x - iData.x, u_PlayerPos.z - iData.z);
        float dist2D = length(diff2D);
        
        float leavesBase = iData.y + 3.0 * iData.w;
        float leavesTop = iData.y + 25.0 * iData.w;
        bool insideFoliageHeight = (u_PlayerPos.y >= leavesBase && u_PlayerPos.y <= leavesTop);
        float leavesRadius = 3.8 * iData.w;
        
        if (dist2D < leavesRadius && insideFoliageHeight) {
            float distToCam = length(worldPos - u_PlayerPos);
            // Smoothly fade from 0.0 (within 1.5m) to 0.12 (at 6.0m)
            float closeFade = clamp((distToCam - 1.5) / 4.5, 0.0, 1.0);
            vTreeAlphaFactor = 0.12 * closeFade;
        }
    }
}
