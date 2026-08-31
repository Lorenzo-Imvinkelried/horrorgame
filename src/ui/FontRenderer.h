#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

/**
 * @brief FontRenderer: Renderizador de tipografía TrueType (Symtext.ttf) para la interfaz de usuario.
 * Utiliza stb_truetype para rasterizar un atlas bitmap nítido de estilo pixel-art y generar quads UV optimizados.
 */
class FontRenderer {
public:
    static bool Init(const std::string& fontPath = "assets/fonts/Symtext.ttf");
    static void Shutdown();

    /**
     * @brief Dibuja una cadena de texto en coordenadas NDC (-1.0 a 1.0) utilizando Symtext.ttf.
     */
    static void DrawString(const std::string& text, float x, float y, float size, glm::vec3 color, 
                           GLuint uiProgram, GLuint uiVAO, GLuint uiVBO);

    /**
     * @brief Calcula el ancho aproximado del texto en espacio NDC.
     */
    static float GetTextWidth(const std::string& text, float size);

    static GLuint GetTextureID() noexcept;
    static bool IsLoaded() noexcept;

private:
    static void pushGlyphQuad(std::vector<float>& data, 
                              float x0, float y0, float x1, float y1, 
                              float s0, float t0, float s1, float t1);
};
