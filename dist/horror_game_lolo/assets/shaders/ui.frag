#version 330 core
out vec4 FragColor;

in vec2 vTexCoord;

uniform sampler2D u_Texture;
uniform vec3 u_Color;

void main()
{
    // Sample texture but multiply by color (u_Color allows tinting or white)
    vec4 texColor = texture(u_Texture, vTexCoord);
    FragColor = texColor * vec4(u_Color, 1.0);
}
