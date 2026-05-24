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
uniform vec3 u_PlayerPos;
uniform vec3 u_PlayerFront;

// Terrain Math REMOVED (CPU Only)

void main()
{
    vec3 worldPos = aPos;
    if (u_IsInstanced) {
        // SELECT SOURCE: Birds or Trees
        vec4 iData = u_UseBirdAttribs ? aBirdInstanceData : aInstanceData;
        float iYaw = u_UseBirdAttribs ? aBirdYaw : aInstanceYaw;

        // Apply scaling
        worldPos *= iData.w;
        
        // Apply Rotation (Yaw around Y)
        float c = cos(iYaw);
        float s = sin(iYaw);
        // Rotation Matrix for Y axis:
        // [ c  0  s ]
        // [ 0  1  0 ]
        // [-s  0  c ]
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

            // Simple Height Factor
            float normalizedH = clamp((aPos.y - 6.0) / 9.0, 0.0, 1.0);
            float hFactor = 0.5 + 1.5 * (normalizedH * normalizedH);

            // Apply Wind
            worldPos.xz += u_WindDirection * wave * 1.0 * hFactor * u_WindStrength * windEnabled;
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
        
        // Add Flashlight spotlight effect at night
        if (u_IsNight) {
            vec3 flashPos = u_PlayerPos + vec3(0.0, 1.2, 0.0); // flash source below eye line
            vec3 toVertex = worldPos - flashPos;
            float dist = length(toVertex);
            
            if (dist < 40.0) {
                vec3 lightVec = normalize(toVertex);
                float spotDot = dot(lightVec, u_PlayerFront);
                
                // Spotlight cone: active if spotDot > 0.84 (approx 30 deg half-angle)
                if (spotDot > 0.84) {
                    float attenuation = clamp(1.0 - (dist / 40.0), 0.0, 1.0);
                    float spotIntensity = smoothstep(0.84, 0.90, spotDot);
                    float flashlightPower = spotIntensity * attenuation * 0.95;
                    
                    // Diffuse lighting for flashlight
                    float flashDiff = max(dot(aNormal, -lightVec), 0.0);
                    light += vec3(flashlightPower * (0.3 + flashDiff * 0.7));
                }
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
}
