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
        
        // 1. Horizontal screen tearing glitch
        float glitchTime = floor(u_Time * 20.0); // Step time for chunky glitch
        float noiseVal = rand(vec2(glitchTime, 0.0));
        
        // Random horizontal offsets at specific vertical bands
        float band = step(0.9, rand(vec2(floor(uv.y * 12.0), glitchTime)));
        uv.x += band * (noiseVal - 0.5) * 0.15;
        
        // Additional high-frequency fine lines jitter
        if (rand(vec2(uv.y, u_Time)) > 0.96) {
            uv.x += (rand(vec2(u_Time)) - 0.5) * 0.05;
        }

        // 2. Fetch texture color with glitched coordinates
        vec4 col = texture(u_ScreenTexture, uv);
        
        // 3. Chromatic aberration (Split channels for glitch feel)
        float splitAmount = 0.03 * sin(u_Time * 40.0) * (1.5 - u_GameOverTime);
        col.r = texture(u_ScreenTexture, uv + vec2(splitAmount, 0.0)).r;
        col.b = texture(u_ScreenTexture, uv - vec2(splitAmount, 0.0)).b;

        // 4. Glitched blood overlay (Vignette + static noise + red tint)
        vec2 centerDist = uv - vec2(0.5);
        float vignette = dot(centerDist, centerDist) * 2.0; // Vignette strength
        
        // High-frequency screen static
        float staticNoise = rand(uv * sin(u_Time)) * 0.25;
        
        // Glitch flash: rapidly invert or flash screen
        float flash = step(0.85, rand(vec2(floor(u_Time * 30.0))));
        
        // Dark crimson red blood color
        vec3 bloodColor = vec3(0.7, 0.0, 0.0);
        
        // Mix original game color with blood and static
        vec3 finalColor = mix(col.rgb, bloodColor, clamp(vignette + 0.3 + staticNoise, 0.0, 1.0));
        
        // Intensify red channel
        finalColor.r += 0.3 + sin(u_Time * 50.0) * 0.1;
        
        // Glitch color inversion flashes
        if (flash > 0.5) {
            finalColor = vec3(1.0) - finalColor;
            finalColor.r += 0.5; // keep it red
        }
        
        FragColor = vec4(finalColor, 1.0);
    } else {
        FragColor = texture(u_ScreenTexture, vTexCoord);
    }
}
