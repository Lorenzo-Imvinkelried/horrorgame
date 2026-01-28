#version 330 core
in vec3 vColor;
in vec2 vTexCoord;
in float vDist;

out vec4 FragColor;

uniform sampler2D u_Texture;
uniform float u_Alpha; // Particle transparency override
uniform int u_IsDebug; // 0 = Normal, 1 = Red, 2 = Blue

void main()
{
    if (u_IsDebug == 1) {
        FragColor = vec4(1.0, 0.0, 0.0, 1.0); // Bright Red
        return;
    }
    if (u_IsDebug == 2) {
        FragColor = vec4(0.0, 0.5, 1.0, 1.0); // Bright Blue
        return;
    }
    if (u_IsDebug == 3) {
        FragColor = vec4(vColor, 1.0); // Vertex Color Passthrough
        return;
    }
    vec4 texColor = texture(u_Texture, vTexCoord);
    
    // OPTIMIZATION: Early Discard
    if (texColor.a < 0.1) discard;

    vec3 outColor = vColor * texColor.rgb;

    // -- DISTANCE FOG --
    // Linear or Exponential    // Fog (Sky Blue PS1 style)
    // HARDCODED FIX: Uniforms causing issues, using fixed values for now.
    float fogStart = 450.0;
    float fogEnd = 550.0;
    
    // DEBUG: Force Fog OFF to see if geometry exists
    // float fogFactor = 0.0; 
    float fogFactor = clamp((vDist - fogStart) / (fogEnd - fogStart), 0.0, 1.0);
    
    vec3 fogColor = vec3(0.4, 0.6, 1.0);
    
    // Apply u_Alpha if not 1.0
    float finalAlpha = texColor.a;
    if (u_Alpha < 1.0) finalAlpha *= u_Alpha;

    FragColor = vec4(mix(outColor, fogColor, fogFactor), finalAlpha);
}
