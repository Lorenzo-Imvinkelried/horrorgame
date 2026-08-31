#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D u_Texture;
uniform vec3 u_Color;
uniform int u_UseTexture; // 0 = Solido, 1 = Fuente Pixel Art Symtext

void main()
{
    if (u_UseTexture == 1) {
        float alpha = texture(u_Texture, vTexCoord).a;
        // Binarizacion nitida para fuentes pixel art: elimina cualquier desenfoque o halo
        if (alpha < 0.35) {
            discard;
        }
        FragColor = vec4(u_Color, 1.0);
    } else {
        FragColor = vec4(u_Color, 1.0);
    }
}
