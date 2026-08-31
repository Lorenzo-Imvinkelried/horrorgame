#include "FontRenderer.h"
#include <fstream>
#include <iostream>
#include <algorithm>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

static stbtt_bakedchar g_cdata[96]; // ASCII 32..127
static GLuint g_fontTexture = 0;
static bool g_isFontLoaded = false;
static const int FONT_ATLAS_WIDTH = 512;
static const int FONT_ATLAS_HEIGHT = 512;
// Symtext rasterizado a 32px para densidad de píxeles HD de máxima nitidez
static const float FONT_NATIVE_HEIGHT = 32.0f;

bool FontRenderer::Init(const std::string& fontPath) {
    if (g_isFontLoaded) return true;

    std::vector<std::string> candidatePaths = {
        fontPath,
        "assets/fonts/Symtext.ttf",
        "symtext/Symtext.ttf",
        "../symtext/Symtext.ttf",
        "assets/Symtext.ttf",
        "Symtext.ttf"
    };

    std::ifstream file;
    std::string foundPath = "";
    for (const auto& p : candidatePaths) {
        file.open(p, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            foundPath = p;
            break;
        }
    }

    if (!file.is_open()) {
        std::cerr << "[FontRenderer] Warning: Symtext.ttf not found in standard paths." << std::endl;
        return false;
    }

    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<unsigned char> ttfBuffer(fileSize);
    if (!file.read(reinterpret_cast<char*>(ttfBuffer.data()), fileSize)) {
        std::cerr << "[FontRenderer] Error reading Symtext.ttf data." << std::endl;
        return false;
    }
    file.close();

    // Rasterizar atlas con alta densidad y bordes nítidos
    std::vector<unsigned char> tempAlphaBitmap(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT);
    int bakeResult = stbtt_BakeFontBitmap(ttfBuffer.data(), 0, FONT_NATIVE_HEIGHT, 
                                          tempAlphaBitmap.data(), FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 
                                          32, 96, g_cdata);

    if (bakeResult <= 0) {
        std::cerr << "[FontRenderer] Warning: Not all glyphs fit in font bitmap (" << bakeResult << ")" << std::endl;
    }

    // Convertir a RGBA (Blanco puro con canal alfa nítido)
    std::vector<unsigned char> rgbaBitmap(FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT * 4);
    for (int i = 0; i < FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT; ++i) {
        rgbaBitmap[i * 4 + 0] = 255;
        rgbaBitmap[i * 4 + 1] = 255;
        rgbaBitmap[i * 4 + 2] = 255;
        rgbaBitmap[i * 4 + 3] = tempAlphaBitmap[i];
    }

    glGenTextures(1, &g_fontTexture);
    glBindTexture(GL_TEXTURE_2D, g_fontTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBitmap.data());
    
    // Filtro Nearest para pixel art cristalino
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    g_isFontLoaded = true;
    std::cout << "[FontRenderer] Symtext.ttf cargado HD (32px, Nearest): " << foundPath << std::endl;
    return true;
}

void FontRenderer::Shutdown() {
    if (g_fontTexture != 0) {
        glDeleteTextures(1, &g_fontTexture);
        g_fontTexture = 0;
    }
    g_isFontLoaded = false;
}

GLuint FontRenderer::GetTextureID() noexcept {
    return g_fontTexture;
}

bool FontRenderer::IsLoaded() noexcept {
    return g_isFontLoaded;
}

void FontRenderer::pushGlyphQuad(std::vector<float>& data, 
                                 float x0, float y0, float x1, float y1, 
                                 float s0, float t0, float s1, float t1) 
{
    // Formato por vértice: (x, y, z, u, v) -> 5 floats
    // Triángulo 1
    data.push_back(x0); data.push_back(y0); data.push_back(0.0f); data.push_back(s0); data.push_back(t1);
    data.push_back(x0); data.push_back(y1); data.push_back(0.0f); data.push_back(s0); data.push_back(t0);
    data.push_back(x1); data.push_back(y0); data.push_back(0.0f); data.push_back(s1); data.push_back(t1);

    // Triángulo 2
    data.push_back(x1); data.push_back(y0); data.push_back(0.0f); data.push_back(s1); data.push_back(t1);
    data.push_back(x0); data.push_back(y1); data.push_back(0.0f); data.push_back(s0); data.push_back(t0);
    data.push_back(x1); data.push_back(y1); data.push_back(0.0f); data.push_back(s1); data.push_back(t0);
}

float FontRenderer::GetTextWidth(const std::string& text, float size) {
    if (!g_isFontLoaded || text.empty()) {
        return static_cast<float>(text.length()) * (size * 0.5f);
    }

    float glyphScale = (size * 1.80f) / FONT_NATIVE_HEIGHT;
    float scaleX = glyphScale * 0.60f;
    float totalW = 0.0f;

    for (char c : text) {
        if (c >= 32 && c < 128) {
            const stbtt_bakedchar& b = g_cdata[c - 32];
            totalW += b.xadvance * scaleX;
        } else if (c == ' ') {
            totalW += g_cdata[0].xadvance * scaleX;
        }
    }
    return totalW;
}

void FontRenderer::DrawString(const std::string& text, float x, float y, float size, glm::vec3 color, 
                              GLuint uiProgram, GLuint uiVAO, GLuint uiVBO) 
{
    if (text.empty()) return;

    if (!g_isFontLoaded) {
        if (!Init()) {
            return;
        }
    }

    // Proporciones NDC de alta definición
    float glyphScale = (size * 1.80f) / FONT_NATIVE_HEIGHT;
    float scaleX = glyphScale * 0.60f;
    float scaleY = glyphScale;

    float curX = x;
    float curY = y;

    std::vector<float> vertexData;
    vertexData.reserve(text.length() * 30); // 6 vértices * 5 floats

    for (char c : text) {
        if (c < 32 || c >= 128) {
            c = '?';
        }

        const stbtt_bakedchar& b = g_cdata[c - 32];

        // Coordenadas locales del glifo
        float gx0 = curX + b.xoff * scaleX;
        float gy1 = curY - b.yoff * scaleY;
        float gx1 = gx0 + (b.x1 - b.x0) * scaleX;
        float gy0 = gy1 - (b.y1 - b.y0) * scaleY;

        float s0 = static_cast<float>(b.x0) / static_cast<float>(FONT_ATLAS_WIDTH);
        float t0 = static_cast<float>(b.y0) / static_cast<float>(FONT_ATLAS_HEIGHT);
        float s1 = static_cast<float>(b.x1) / static_cast<float>(FONT_ATLAS_WIDTH);
        float t1 = static_cast<float>(b.y1) / static_cast<float>(FONT_ATLAS_HEIGHT);

        pushGlyphQuad(vertexData, gx0, gy0, gx1, gy1, s0, t0, s1, t1);

        curX += b.xadvance * scaleX;
    }

    if (vertexData.empty()) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUniform1i(glGetUniformLocation(uiProgram, "u_UseTexture"), 1);
    glUniform3f(glGetUniformLocation(uiProgram, "u_Color"), color.r, color.g, color.b);
    glUniform1i(glGetUniformLocation(uiProgram, "u_Texture"), 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, g_fontTexture);

    glBindVertexArray(uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_DYNAMIC_DRAW);

    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertexData.size() / 5));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
