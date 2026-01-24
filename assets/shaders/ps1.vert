#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aNormal;
layout (location = 4) in vec4 aInstanceData; // .xyz = pos, .w = scale

out vec3 vColor;
out vec2 vTexCoord;
out float vDist; // Distance for Fog

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

uniform vec2 u_WindDirection;

// Terrain Math (Must match WorldGenerator.cpp)
float GetVisualHeight(float x, float z) {
    float y = abs(sin(x * 0.05)) * abs(cos(z * 0.05)) * 4.0;
    y += abs(sin(x * 0.2 + z * 0.1)) * 1.5;
    return floor(y * 2.0) / 2.0;
}

void main()
{
    vec3 worldPos = aPos;
    if (u_IsInstanced) {
        // Apply scaling before translation
        worldPos *= aInstanceData.w;
        worldPos += aInstanceData.xyz;
    }
    
    // -- SHADOW/FOOTPRINT SNAPPING --
    // If drawing a shadow/footprint, snap strictly to terrain height
    if (u_ConformToTerrain) {
        float terrainY = GetVisualHeight(worldPos.x, worldPos.z);
        worldPos.y = terrainY + 0.001; // Minimal bias, rely on PolygonOffset
    } else {
        // -- WIND SYSTEM --
        // STRUCTURAL SOLUTION (Final): Explicit Control from CPU.
        // Trunks are drawn with u_WindStrength = 0.0 (Static).
        // Leaves are drawn with u_WindStrength = 1.0 (Mobile).
        if (u_WindStrength > 0.0) {
            // Use direction from CPU WindSystem
            // Uniform wave for the entire object (e.g. leaves mesh)
            // BIASED WAVE: (sin(t) + 1.0) * 0.5 -> Range [0.0, 1.0]
            // This ensures trees lean WITH the wind, not against it.
            float rawWave = sin(u_Time * 3.0 + worldPos.x * 0.2 + worldPos.z * 0.2); 
            // Squaring it makes the "low" periods flatter and "high" periods sharper (gusty feel)
            float wave = (rawWave + 1.0) * 0.5;
            wave = wave * wave; 

            // HEIGHT FACTOR:
            // Normalize height 0..1 (Base to Tip)
            float normalizedH = clamp((aPos.y - 6.0) / 9.0, 0.0, 1.0);
            
            // Base Sway (0.5) + Tip Extra (1.5 * curve)
            // This ensures the whole canopy moves (0.5) but the tip "whips" (2.0 total)
            float hFactor = 0.5 + 1.5 * (normalizedH * normalizedH);

            // Apply sway
            worldPos.xz += u_WindDirection * wave * 1.0 * hFactor * u_WindStrength;
        }
    }

    // -- LIGHTING --
    vec3 litColor;
    if (u_ParticleMode) {
        litColor = aColor; // Use raw color
    } else {
        float ambientStrength = 0.6;
        vec3 lightDir = normalize(vec3(0.8, 0.6, 0.3)); // 10 AM Sun Angle
        float diff = max(dot(aNormal, lightDir), 0.0);
        vec3 light = vec3(ambientStrength + diff * 0.4);
        litColor = aColor * light;
    }

    // -- INSTANCE VARIATION (Trees) --
    if (u_IsInstanced) {
        // Pseudo-random hash based on tree position (XZ)
        float hash = fract(sin(dot(aInstanceData.xz, vec2(12.9898, 78.233))) * 43758.5453);
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
}
