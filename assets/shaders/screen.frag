#version 330 core
out vec4 FragColor;
  
in vec2 vTexCoord;

uniform sampler2D u_ScreenTexture;
uniform int u_IsGameOver;
uniform float u_GameOverTime;
uniform float u_Time;

// Simple pseudo-random generator
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{ 
    if (u_IsGameOver == 1) {
        vec2 uv = vTexCoord;
        
        // Calculate progress of the death animation (from 0.0 to 1.0)
        // u_GameOverTime starts at 3.5 and goes down to 0.0
        float progress = clamp((3.5 - u_GameOverTime) / 3.5, 0.0, 1.0);
        
        // Fetch original texture color
        vec4 col = texture(u_ScreenTexture, uv);
        
        // Realistic blood vignette overlay
        vec2 centerDist = uv - vec2(0.5);
        float dist = length(centerDist);
        
        // Blood vignette gets stronger and spreads towards the center as progress increases
        // At progress=0: vignette covers only the outer edges
        // At progress=1: vignette covers almost the entire screen
        float bloodStrength = clamp(progress * 1.5, 0.0, 1.0);
        float vignette = smoothstep(0.2, 0.7 - bloodStrength * 0.4, dist);
        
        // Deep realistic blood color (dark crimson/maroon)
        vec3 bloodColor = vec3(0.35, 0.01, 0.01);
        
        // Mix game color with the blood vignette
        vec3 finalColor = mix(col.rgb, bloodColor, vignette * 0.9 * bloodStrength);
        
        // Also apply a general red desaturation/tint as player loses consciousness
        // We darken the image and shift it slightly redder overall
        vec3 redLethargy = vec3(finalColor.r * 1.1, finalColor.g * 0.6, finalColor.b * 0.6);
        finalColor = mix(finalColor, redLethargy, progress * 0.7);
        
        // Darken the entire screen as vision fades
        finalColor = mix(finalColor, vec3(0.0), progress * 0.5);
        
        // Smooth fade out to complete black at the end
        // Start fading to black rapidly after 40% progress, reaching 100% black at 95% progress
        float fadeToBlack = smoothstep(0.4, 0.95, progress);
        finalColor = mix(finalColor, vec3(0.0), fadeToBlack);
        
        FragColor = vec4(finalColor, 1.0);
    } else {
        FragColor = texture(u_ScreenTexture, vTexCoord);
    }
}
