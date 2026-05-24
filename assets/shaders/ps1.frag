#version 330 core
in vec3 vColor;
in vec2 vTexCoord;
in float vDist;

out vec4 FragColor;

uniform sampler2D u_Texture;
uniform float u_Alpha; // Particle transparency override
uniform int u_IsDebug; // 0 = Normal, 1 = Red, 2 = Blue

uniform vec3 u_FogColor;

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
    // Exponential-squared fog (smoothly gets denser, avoiding sharp linear cuts)
    float density = 0.016; // Adjusted for a smooth 120m blend (further away)
    float fogFactor = 1.0 - exp(-pow(density * vDist, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);
    
    vec3 fogColor = u_FogColor;
    
    // Apply u_Alpha if not 1.0
    float finalAlpha = texColor.a;
    if (u_Alpha < 1.0) finalAlpha *= u_Alpha;

    FragColor = vec4(mix(outColor, fogColor, fogFactor), finalAlpha);
}
