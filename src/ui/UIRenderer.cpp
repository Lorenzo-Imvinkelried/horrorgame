#include "UIRenderer.h"
#include "FontRenderer.h"
#include <cmath>
#include <algorithm>

UIRenderer::UIRenderer() {}
UIRenderer::~UIRenderer() {}

void UIRenderer::pushQuad(std::vector<float>& data, float x, float y, float w, float h) {
    // Formato unificado de 5 floats: (x, y, z, u, v)
    data.push_back(x);     data.push_back(y);     data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x);     data.push_back(y + h); data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x + w); data.push_back(y);     data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);

    data.push_back(x + w); data.push_back(y);     data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x);     data.push_back(y + h); data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
    data.push_back(x + w); data.push_back(y + h); data.push_back(0.0f); data.push_back(0.0f); data.push_back(0.0f);
}

void UIRenderer::drawColoredQuad(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float x, float y, float w, float h, glm::vec3 color) {
    std::vector<float> data;
    pushQuad(data, x, y, w, h);

    glUniform1i(glGetUniformLocation(uiProgram, "u_UseTexture"), 0);
    glUniform3f(glGetUniformLocation(uiProgram, "u_Color"), color.r, color.g, color.b);

    glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_DYNAMIC_DRAW);
    glBindVertexArray(uiVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void UIRenderer::DrawWin98Window(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, 
                                float x, float y, float w, float h, 
                                const std::string& title, bool hasCloseBtn) 
{
    // Outer black boundary
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x - 0.004f, y - 0.004f, w + 0.008f, h + 0.008f, glm::vec3(0.05f, 0.05f, 0.05f));

    // 3D Bevel: White Top & Left
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y + h - 0.005f, w, 0.005f, glm::vec3(1.0f, 1.0f, 1.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y, 0.004f, h, glm::vec3(1.0f, 1.0f, 1.0f));

    // 3D Bevel: Dark Gray Bottom & Right
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 0.004f, y, w - 0.004f, 0.005f, glm::vec3(0.40f, 0.40f, 0.42f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + w - 0.004f, y, 0.004f, h - 0.004f, glm::vec3(0.40f, 0.40f, 0.42f));

    // Window Body Fill: Classic Windows 98 Light Gray
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 0.004f, y + 0.005f, w - 0.008f, h - 0.010f, glm::vec3(0.76f, 0.76f, 0.78f));

    // Title Bar: Navy Blue
    float titleH = 0.065f;
    float titleY = y + h - titleH - 0.008f;
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 0.008f, titleY, w - 0.016f, titleH, glm::vec3(0.06f, 0.14f, 0.48f));

    // Title Text (Symtext.ttf)
    DrawString(title, x + 0.020f, titleY + 0.018f, 0.028f, glm::vec3(1.0f, 1.0f, 1.0f), uiProgram, uiVAO, uiVBO);

    // Close Button [X]
    if (hasCloseBtn) {
        float btnW = 0.045f, btnH = 0.045f;
        float btnX = x + w - btnW - 0.014f;
        float btnY = titleY + 0.010f;
        drawColoredQuad(uiProgram, uiVAO, uiVBO, btnX, btnY, btnW, btnH, glm::vec3(0.76f, 0.76f, 0.78f));
        drawColoredQuad(uiProgram, uiVAO, uiVBO, btnX, btnY + btnH - 0.003f, btnW, 0.003f, glm::vec3(1.0f, 1.0f, 1.0f));
        drawColoredQuad(uiProgram, uiVAO, uiVBO, btnX, btnY, 0.003f, btnH, glm::vec3(1.0f, 1.0f, 1.0f));
        drawColoredQuad(uiProgram, uiVAO, uiVBO, btnX, btnY, btnW, 0.003f, glm::vec3(0.35f, 0.35f, 0.35f));
        drawColoredQuad(uiProgram, uiVAO, uiVBO, btnX + btnW - 0.003f, btnY, 0.003f, btnH, glm::vec3(0.35f, 0.35f, 0.35f));
        DrawString("X", btnX + 0.012f, btnY + 0.009f, 0.026f, glm::vec3(0.10f, 0.10f, 0.10f), uiProgram, uiVAO, uiVBO);
    }
}

void UIRenderer::DrawWin98Button(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO,
                                float x, float y, float w, float h,
                                const std::string& text, bool isHovered,
                                float textSize) 
{
    glm::vec3 fillCol = isHovered ? glm::vec3(0.88f, 0.88f, 0.92f) : glm::vec3(0.76f, 0.76f, 0.78f);

    // Black outline
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x - 0.002f, y - 0.002f, w + 0.004f, h + 0.004f, glm::vec3(0.08f, 0.08f, 0.10f));

    // Body Fill
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y, w, h, fillCol);

    // 3D Bevel: White Top/Left, Dark Bottom/Right
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y + h - 0.004f, w, 0.004f, glm::vec3(1.0f, 1.0f, 1.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y, 0.003f, h, glm::vec3(1.0f, 1.0f, 1.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y, w, 0.004f, glm::vec3(0.35f, 0.35f, 0.38f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + w - 0.003f, y, 0.003f, h, glm::vec3(0.35f, 0.35f, 0.38f));

    // Center Text con métricas de Symtext
    float textW = FontRenderer::GetTextWidth(text, textSize);
    float textX = x + std::max(0.008f, (w - textW) * 0.5f);
    float textY = y + (h - textSize) * 0.5f + 0.005f;

    glm::vec3 textCol = isHovered ? glm::vec3(0.06f, 0.14f, 0.48f) : glm::vec3(0.08f, 0.08f, 0.12f);
    DrawString(text, textX, textY, textSize, textCol, uiProgram, uiVAO, uiVBO);
}

void UIRenderer::DrawString(const std::string& text, float x, float y, float size, glm::vec3 color, GLuint uiProgram, GLuint uiVAO, GLuint uiVBO) {
    FontRenderer::DrawString(text, x, y, size, color, uiProgram, uiVAO, uiVBO);
}

void UIRenderer::drawDigit(int d, float x, float y, float size, glm::vec3 color, GLuint uiProgram, GLuint uiVAO, GLuint uiVBO) {
    if (d >= 0 && d <= 9) {
        DrawString(std::to_string(d), x, y, size, color, uiProgram, uiVAO, uiVBO);
    }
}

void UIRenderer::drawNumber(int num, float x, float y, float size, glm::vec3 color, GLuint uiProgram, GLuint uiVAO, GLuint uiVBO) {
    DrawString(std::to_string(num), x, y, size, color, uiProgram, uiVAO, uiVBO);
}

void UIRenderer::drawECGWave(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, 
                            float x, float y, float w, float h, 
                            float healthPct, float dangerLevel, float globalTime) 
{
    // Sunken Display Bevel
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y, w, h, glm::vec3(0.015f, 0.04f, 0.02f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y + h - 0.003f, w, 0.003f, glm::vec3(0.35f, 0.35f, 0.35f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y, 0.003f, h, glm::vec3(0.35f, 0.35f, 0.35f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y, w, 0.003f, glm::vec3(0.9f, 0.9f, 0.9f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + w - 0.003f, y, 0.003f, h, glm::vec3(0.9f, 0.9f, 0.9f));

    // Subtle CRT green oscilloscope grid lines
    for (int i = 1; i < 4; ++i) {
        float gy = y + (h / 4.0f) * (float)i;
        drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 0.005f, gy, w - 0.010f, 0.002f, glm::vec3(0.03f, 0.10f, 0.04f));
    }

    // Heartbeat Frequency (BPM)
    float baseBpm = 75.0f;
    if (healthPct < 0.40f) baseBpm += (0.40f - healthPct) * 220.0f;
    baseBpm += dangerLevel * 45.0f;
    int currentBpm = std::clamp((int)baseBpm, 55, 210);

    float freq = (float)currentBpm / 60.0f;
    float centerY = y + h * 0.50f;
    float amp = h * 0.38f;

    std::vector<float> waveData;
    const int numSamples = 48;

    auto evalEcgPoint = [&](float t) -> float {
        float phase = fmod(t * freq, 1.0f);
        float v = 0.0f;
        if (phase < 0.12f) {
            v = sin(phase / 0.12f * 3.14159f) * 0.18f;
        } else if (phase >= 0.18f && phase < 0.22f) {
            v = -0.22f;
        } else if (phase >= 0.22f && phase < 0.28f) {
            float rNorm = (phase - 0.22f) / 0.06f;
            v = (rNorm < 0.5f) ? (-0.22f + rNorm * 2.0f * 1.22f) : (1.0f - (rNorm - 0.5f) * 2.0f * 1.35f);
        } else if (phase >= 0.28f && phase < 0.33f) {
            v = -0.35f;
        } else if (phase >= 0.42f && phase < 0.60f) {
            float tNorm = (phase - 0.42f) / 0.18f;
            v = sin(tNorm * 3.14159f) * 0.28f;
        }
        if (dangerLevel > 0.1f || healthPct < 0.35f) {
            v += sin(t * 45.0f) * 0.08f * (dangerLevel + 0.5f);
        }
        return v;
    };

    glm::vec3 phosphorCol = (healthPct < 0.30f) ? glm::vec3(0.95f, 0.20f, 0.15f) : (dangerLevel > 0.5f ? glm::vec3(0.95f, 0.85f, 0.15f) : glm::vec3(0.15f, 0.98f, 0.25f));

    for (int i = 0; i < numSamples; ++i) {
        float normX = (float)i / (float)(numSamples - 1);
        float px = x + 0.006f + normX * (w - 0.012f);
        float timeOffset = globalTime - (1.0f - normX) * 1.2f;
        float val = evalEcgPoint(timeOffset);
        float py = centerY + val * amp;

        pushQuad(waveData, px, py - 0.003f, (w / (float)numSamples) * 1.05f, 0.006f);
    }

    if (!waveData.empty()) {
        glUniform1i(glGetUniformLocation(uiProgram, "u_UseTexture"), 0);
        glUniform3f(glGetUniformLocation(uiProgram, "u_Color"), phosphorCol.r, phosphorCol.g, phosphorCol.b);
        glBindBuffer(GL_ARRAY_BUFFER, uiVBO);
        glBufferData(GL_ARRAY_BUFFER, waveData.size() * sizeof(float), waveData.data(), GL_DYNAMIC_DRAW);
        glBindVertexArray(uiVAO);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(waveData.size() / 5));
        glBindVertexArray(0);
    }

    // BPM & Status Overlay
    std::string bpmStr = "PULSO: " + std::to_string(currentBpm) + " BPM";
    DrawString(bpmStr, x + 0.015f, y + h - 0.040f, 0.024f, phosphorCol, uiProgram, uiVAO, uiVBO);

    std::string statusStr = (healthPct < 0.30f) ? "ESTADO: CRITICO!" : ((dangerLevel > 0.4f) ? "PELIGRO INMINENTE" : "ESTADO: ESTABLE");
    DrawString(statusStr, x + 0.015f, y + 0.012f, 0.022f, phosphorCol * 0.9f, uiProgram, uiVAO, uiVBO);
}

void UIRenderer::RenderHUD(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, 
                           const PlayerStats& stats, 
                           const TargetingSystem& targeting, 
                           const DamageNumberSystem& damageNumbers,
                           const glm::mat4& viewProj,
                           float dangerLevel,
                           float globalTime,
                           int nightCount,
                           bool isBloodMoon,
                           int dayCount,
                           bool isNightTime) 
{
    // =========================================================================
    // 1. WINDOWS 98 VRAM_DUNGEON_MONITOR.EXE (Top-Left in NDC)
    // =========================================================================
    float pX = -0.96f, pY = 0.50f;
    float pW = 0.60f, pH = 0.46f;

    DrawWin98Window(uiProgram, uiVAO, uiVBO, pX, pY, pW, pH, "VRAM_DUNGEON_MONITOR.EXE", true);

    // ECG Oscilloscope Screen
    float hpPct = std::clamp((float)stats.CurrentHP / (float)stats.MaxHP, 0.0f, 1.0f);
    drawECGWave(uiProgram, uiVAO, uiVBO, pX + 0.018f, pY + 0.19f, pW - 0.036f, 0.18f, hpPct, dangerLevel, globalTime);

    // VITALITY & MANA Numeric Indicators
    std::string hpText = "VITALIDAD: " + std::to_string(stats.CurrentHP) + "/" + std::to_string(stats.MaxHP);
    DrawString(hpText, pX + 0.020f, pY + 0.145f, 0.026f, (hpPct < 0.35f ? glm::vec3(0.85f, 0.15f, 0.10f) : glm::vec3(0.15f, 0.15f, 0.20f)), uiProgram, uiVAO, uiVBO);

    // Life Bar
    float barW = pW - 0.040f;
    drawColoredQuad(uiProgram, uiVAO, uiVBO, pX + 0.020f, pY + 0.115f, barW, 0.024f, glm::vec3(0.35f, 0.35f, 0.38f));
    if (hpPct > 0.005f) {
        glm::vec3 hpCol = (hpPct < 0.35f) ? glm::vec3(0.85f, 0.15f, 0.15f) : glm::vec3(0.15f, 0.65f, 0.25f);
        drawColoredQuad(uiProgram, uiVAO, uiVBO, pX + 0.022f, pY + 0.117f, (barW - 0.004f) * hpPct, 0.020f, hpCol);
    }

    // MANA / MENTE & NIVEL
    std::string mpText = "MANA: " + std::to_string(stats.CurrentMP) + "/" + std::to_string(stats.MaxMP) + "  NVL: " + std::to_string(stats.Level);
    DrawString(mpText, pX + 0.020f, pY + 0.078f, 0.025f, glm::vec3(0.15f, 0.15f, 0.20f), uiProgram, uiVAO, uiVBO);

    // DAY / NIGHT COUNTER & THREAT LEVEL
    std::string timeText;
    glm::vec3 timeCol;
    if (isNightTime) {
        timeText = "NOCHE " + std::to_string(nightCount) + (isBloodMoon ? " [LUNA DE SANGRE]" : " [AMENAZA NV." + std::to_string(nightCount) + "]");
        timeCol = isBloodMoon ? glm::vec3(0.88f, 0.08f, 0.08f) : glm::vec3(0.20f, 0.20f, 0.50f);
    } else {
        timeText = "DIA " + std::to_string(dayCount) + " [AMENAZA NV." + std::to_string(nightCount) + "]";
        timeCol = glm::vec3(0.70f, 0.45f, 0.10f); // Warm daytime sun gold
    }
    DrawString(timeText, pX + 0.020f, pY + 0.042f, 0.023f, timeCol, uiProgram, uiVAO, uiVBO);

    // Hotkey Hint [C] STATS
    std::string statHint = (stats.AvailableStatPoints > 0) ? "[C] PANEL (+PUNTOS!)" : "[C] PANEL ESTADISTICAS";
    glm::vec3 hintCol = (stats.AvailableStatPoints > 0) ? glm::vec3(0.85f, 0.50f, 0.05f) : glm::vec3(0.30f, 0.30f, 0.35f);
    DrawString(statHint, pX + 0.020f, pY + 0.010f, 0.022f, hintCol, uiProgram, uiVAO, uiVBO);

    // =========================================================================
    // 2. EXPERIENCE BAR (Bottom Taskbar)
    // =========================================================================
    drawColoredQuad(uiProgram, uiVAO, uiVBO, -0.99f, -0.99f, 1.98f, 0.038f, glm::vec3(0.76f, 0.76f, 0.78f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, -0.99f, -0.952f, 1.98f, 0.003f, glm::vec3(1.0f, 1.0f, 1.0f));

    float expPct = std::clamp((float)stats.CurrentExp / (float)stats.NextLevelExp, 0.0f, 1.0f);
    drawColoredQuad(uiProgram, uiVAO, uiVBO, -0.70f, -0.982f, 1.65f, 0.024f, glm::vec3(0.35f, 0.35f, 0.38f));
    if (expPct > 0.002f) {
        drawColoredQuad(uiProgram, uiVAO, uiVBO, -0.698f, -0.980f, 1.646f * expPct, 0.020f, glm::vec3(0.06f, 0.14f, 0.48f));
    }
    std::string taskbarText = "VRAM_DUNGEON.SYS | [Q] ATACAR  [F] ANTORCHA  [B] CONSTRUIR  [P] PALA  [I] INV";
    DrawString(taskbarText, -0.97f, -0.980f, 0.021f, glm::vec3(0.10f, 0.10f, 0.15f), uiProgram, uiVAO, uiVBO);

    // =========================================================================
    // 3. TARGET FRAME (Top-Center in NDC)
    // =========================================================================
    if (targeting.HasTarget() && targeting.IsTargetAlive()) {
        float tX = -0.34f, tY = 0.72f;
        float tW = 0.68f, tH = 0.22f;

        std::string targetTitle = "OBJETIVO: " + targeting.GetTargetName();
        DrawWin98Window(uiProgram, uiVAO, uiVBO, tX, tY, tW, tH, targetTitle, false);

        std::string tInfo = "NVL: " + std::to_string(targeting.GetTargetLevel()) + "  VIDA: " + std::to_string(targeting.GetTargetCurrentHP()) + "/" + std::to_string(targeting.GetTargetMaxHP());
        DrawString(tInfo, tX + 0.020f, tY + 0.085f, 0.026f, glm::vec3(0.10f, 0.10f, 0.15f), uiProgram, uiVAO, uiVBO);

        float targetHpPct = std::clamp((float)targeting.GetTargetCurrentHP() / (float)targeting.GetTargetMaxHP(), 0.0f, 1.0f);
        drawColoredQuad(uiProgram, uiVAO, uiVBO, tX + 0.020f, tY + 0.035f, tW - 0.040f, 0.035f, glm::vec3(0.35f, 0.35f, 0.38f));
        if (targetHpPct > 0.005f) {
            glm::vec3 tCol = (targeting.GetTargetType() == TargetType::PASSIVE_MOB) ? glm::vec3(0.85f, 0.65f, 0.15f) : glm::vec3(0.85f, 0.15f, 0.15f);
            drawColoredQuad(uiProgram, uiVAO, uiVBO, tX + 0.022f, tY + 0.037f, (tW - 0.044f) * targetHpPct, 0.031f, tCol);
        }
    }

    // =========================================================================
    // 4. FLOATING COMBAT DAMAGE & EXP NUMBERS
    // =========================================================================
    for (const auto& fn : damageNumbers.GetNumbers()) {
        glm::vec4 clip = viewProj * glm::vec4(fn.Pos, 1.0f);
        if (clip.w > 0.1f) {
            float ndcX = clip.x / clip.w;
            float ndcY = clip.y / clip.w;

            if (ndcX >= -1.2f && ndcX <= 1.2f && ndcY >= -1.2f && ndcY <= 1.2f) {
                if (fn.IsLevelUp) {
                    drawColoredQuad(uiProgram, uiVAO, uiVBO, ndcX - 0.16f, ndcY - 0.02f, 0.32f, 0.08f, glm::vec3(0.95f, 0.85f, 0.15f));
                    drawColoredQuad(uiProgram, uiVAO, uiVBO, ndcX - 0.15f, ndcY - 0.01f, 0.30f, 0.06f, glm::vec3(0.06f, 0.14f, 0.48f));
                    DrawString("LEVEL UP!", ndcX - 0.11f, ndcY + 0.005f, 0.028f, glm::vec3(1.0f, 1.0f, 1.0f), uiProgram, uiVAO, uiVBO);
                } else if (fn.IsExp) {
                    std::string expStr = "+" + std::to_string(fn.Value) + " EXP";
                    float dSize = 0.040f;
                    float textW = FontRenderer::GetTextWidth(expStr, dSize);
                    float textX = ndcX - textW * 0.5f;
                    DrawString(expStr, textX + 0.003f, ndcY - 0.003f, dSize, glm::vec3(0.0f, 0.0f, 0.0f), uiProgram, uiVAO, uiVBO);
                    DrawString(expStr, textX, ndcY, dSize, glm::vec3(0.25f, 0.90f, 1.0f), uiProgram, uiVAO, uiVBO);
                } else if (fn.IsHeal) {
                    // Curación de vida (Verde esmeralda luminoso con prefijo '+')
                    std::string healStr = "+" + std::to_string(fn.Value) + " HP";
                    float dSize = 0.058f;
                    float textW = FontRenderer::GetTextWidth(healStr, dSize);
                    float textX = ndcX - textW * 0.5f;
                    DrawString(healStr, textX + 0.003f, ndcY - 0.003f, dSize, glm::vec3(0.0f, 0.20f, 0.05f), uiProgram, uiVAO, uiVBO);
                    DrawString(healStr, textX, ndcY, dSize, glm::vec3(0.15f, 0.95f, 0.30f), uiProgram, uiVAO, uiVBO);
                } else if (fn.IsMana) {
                    // Restauración de maná (Azul cyan luminoso con prefijo '+')
                    std::string manaStr = "+" + std::to_string(fn.Value) + " MP";
                    float dSize = 0.058f;
                    float textW = FontRenderer::GetTextWidth(manaStr, dSize);
                    float textX = ndcX - textW * 0.5f;
                    DrawString(manaStr, textX + 0.003f, ndcY - 0.003f, dSize, glm::vec3(0.0f, 0.05f, 0.25f), uiProgram, uiVAO, uiVBO);
                    DrawString(manaStr, textX, ndcY, dSize, glm::vec3(0.20f, 0.65f, 1.0f), uiProgram, uiVAO, uiVBO);
                } else if (fn.IsPlayerDamage) {
                    // Daño recibido por el jugador (Rojo carmesí con signo '-')
                    std::string dmgStr = "-" + std::to_string(fn.Value);
                    float dSize = 0.056f;
                    float textW = FontRenderer::GetTextWidth(dmgStr, dSize);
                    float textX = ndcX - textW * 0.5f;
                    // Sombra de alto contraste
                    DrawString(dmgStr, textX + 0.003f, ndcY - 0.003f, dSize, glm::vec3(0.15f, 0.0f, 0.0f), uiProgram, uiVAO, uiVBO);
                    DrawString(dmgStr, textX, ndcY, dSize, glm::vec3(1.0f, 0.12f, 0.12f), uiProgram, uiVAO, uiVBO);
                } else {
                    // Daño saliente infligido a los mobs (Blanco pulido o Dorado Crítico)
                    std::string dmgStr = std::to_string(fn.Value);
                    float dSize = fn.IsCrit ? 0.076f : 0.052f;
                    float textW = FontRenderer::GetTextWidth(dmgStr, dSize);
                    float textX = ndcX - textW * 0.5f;

                    glm::vec3 col = fn.IsCrit ? glm::vec3(1.0f, 0.88f, 0.10f) : glm::vec3(1.0f, 1.0f, 0.95f);
                    // Sombra de alto contraste
                    DrawString(dmgStr, textX + 0.003f, ndcY - 0.003f, dSize, glm::vec3(0.08f, 0.08f, 0.08f), uiProgram, uiVAO, uiVBO);
                    DrawString(dmgStr, textX, ndcY, dSize, col, uiProgram, uiVAO, uiVBO);
                }
            }
        }
    }

    glUniform3f(glGetUniformLocation(uiProgram, "u_Color"), 1.0f, 1.0f, 1.0f);
}

void UIRenderer::RenderCharacterPanel(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, const PlayerStats& stats, float mouseNdcX, float mouseNdcY) {
    float pW = 0.96f, pH = 1.54f;
    float pX = -pW * 0.5f, pY = -pH * 0.5f;

    DrawWin98Window(uiProgram, uiVAO, uiVBO, pX, pY, pW, pH, "ESTADISTICAS_SISTEMA.EXE", true);

    float rowY = pY + pH - 0.14f;
    float lineStep = 0.082f;
    float fSize = 0.028f;

    // Row 1: Level & EXP
    std::string lvlStr = "NIVEL: " + std::to_string(stats.Level);
    DrawString(lvlStr, pX + 0.05f, rowY, fSize, glm::vec3(0.06f, 0.14f, 0.48f), uiProgram, uiVAO, uiVBO);
    std::string expStr = "EXP: " + std::to_string(stats.CurrentExp) + "/" + std::to_string(stats.NextLevelExp);
    DrawString(expStr, pX + 0.45f, rowY, fSize, glm::vec3(0.15f, 0.45f, 0.65f), uiProgram, uiVAO, uiVBO);
    rowY -= lineStep;

    // Row 2: HP & MP
    std::string hpStr = "VIDA: " + std::to_string(stats.CurrentHP) + "/" + std::to_string(stats.MaxHP);
    DrawString(hpStr, pX + 0.05f, rowY, fSize, glm::vec3(0.75f, 0.15f, 0.15f), uiProgram, uiVAO, uiVBO);
    std::string mpStr = "MANA: " + std::to_string(stats.CurrentMP) + "/" + std::to_string(stats.MaxMP);
    DrawString(mpStr, pX + 0.45f, rowY, fSize, glm::vec3(0.15f, 0.35f, 0.75f), uiProgram, uiVAO, uiVBO);
    rowY -= lineStep;

    // Separator line
    drawColoredQuad(uiProgram, uiVAO, uiVBO, pX + 0.03f, rowY + 0.02f, pW - 0.06f, 0.003f, glm::vec3(0.55f, 0.55f, 0.58f));
    rowY -= 0.04f;

    // Combat Stats Section Header
    DrawString("METRICAS DE COMBATE:", pX + 0.05f, rowY, fSize, glm::vec3(0.10f, 0.10f, 0.15f), uiProgram, uiVAO, uiVBO);
    rowY -= lineStep * 0.85f;

    // Row 3: Attack Damage & Defense
    std::string atqStr = "ATQ DANIO: " + std::to_string(stats.Attack);
    DrawString(atqStr, pX + 0.05f, rowY, fSize, glm::vec3(0.85f, 0.35f, 0.15f), uiProgram, uiVAO, uiVBO);
    std::string defStr = "DEFENSA: " + std::to_string(stats.Defense);
    DrawString(defStr, pX + 0.50f, rowY, fSize, glm::vec3(0.30f, 0.30f, 0.35f), uiProgram, uiVAO, uiVBO);
    rowY -= lineStep;

    // Row 4: Crit & Evasion
    std::string critStr = "CRITICO: " + std::to_string((int)stats.CritChance) + "%";
    DrawString(critStr, pX + 0.05f, rowY, fSize, glm::vec3(0.75f, 0.60f, 0.10f), uiProgram, uiVAO, uiVBO);
    std::string evaStr = "EVASION: " + std::to_string(stats.Evasion) + "%";
    DrawString(evaStr, pX + 0.50f, rowY, fSize, glm::vec3(0.15f, 0.65f, 0.55f), uiProgram, uiVAO, uiVBO);
    rowY -= lineStep;

    // Separator line
    drawColoredQuad(uiProgram, uiVAO, uiVBO, pX + 0.03f, rowY + 0.02f, pW - 0.06f, 0.003f, glm::vec3(0.55f, 0.55f, 0.58f));
    rowY -= 0.04f;

    // Available Points Banner Box
    float ptsBoxW = pW - 0.06f, ptsBoxH = 0.075f;
    float ptsBoxX = pX + 0.03f, ptsBoxY = rowY - 0.025f;

    if (stats.AvailableStatPoints > 0) {
        drawColoredQuad(uiProgram, uiVAO, uiVBO, ptsBoxX, ptsBoxY, ptsBoxW, ptsBoxH, glm::vec3(0.92f, 0.82f, 0.25f));
        drawColoredQuad(uiProgram, uiVAO, uiVBO, ptsBoxX + 0.003f, ptsBoxY + 0.003f, ptsBoxW - 0.006f, ptsBoxH - 0.006f, glm::vec3(0.18f, 0.14f, 0.04f));
        std::string ptsStr = "! PUNTOS DISPONIBLES: " + std::to_string(stats.AvailableStatPoints) + " (HAZ CLICK EN [+]) !";
        DrawString(ptsStr, ptsBoxX + 0.025f, ptsBoxY + 0.022f, 0.026f, glm::vec3(1.0f, 0.90f, 0.20f), uiProgram, uiVAO, uiVBO);
    } else {
        drawColoredQuad(uiProgram, uiVAO, uiVBO, ptsBoxX, ptsBoxY, ptsBoxW, ptsBoxH, glm::vec3(0.35f, 0.35f, 0.38f));
        drawColoredQuad(uiProgram, uiVAO, uiVBO, ptsBoxX + 0.003f, ptsBoxY + 0.003f, ptsBoxW - 0.006f, ptsBoxH - 0.006f, glm::vec3(0.70f, 0.70f, 0.72f));
        std::string ptsStr = "PUNTOS DISPONIBLES: 0 (SUBE DE NIVEL)";
        DrawString(ptsStr, ptsBoxX + 0.025f, ptsBoxY + 0.022f, 0.025f, glm::vec3(0.20f, 0.20f, 0.25f), uiProgram, uiVAO, uiVBO);
    }
    rowY -= 0.11f;

    // Attribute Rows with Interactive Clickable [ + ] Buttons
    float btnW = 0.080f, btnH = 0.065f;
    float btnX = pX + 0.045f;
    float attrStep = 0.095f;

    auto drawAttrButtonRow = [&](int index, const std::string& name, int value, const std::string& desc, glm::vec3 col) {
        float bY = rowY - 0.015f;
        bool hovered = (mouseNdcX >= btnX && mouseNdcX <= btnX + btnW && mouseNdcY >= bY && mouseNdcY <= bY + btnH);

        if (stats.AvailableStatPoints > 0) {
            DrawWin98Button(uiProgram, uiVAO, uiVBO, btnX, bY, btnW, btnH, "+", hovered, 0.030f);
        } else {
            // Disabled Gray Button
            drawColoredQuad(uiProgram, uiVAO, uiVBO, btnX, bY, btnW, btnH, glm::vec3(0.65f, 0.65f, 0.68f));
            DrawString("-", btnX + 0.028f, bY + 0.018f, 0.026f, glm::vec3(0.45f, 0.45f, 0.48f), uiProgram, uiVAO, uiVBO);
        }

        std::string textStr = name + ": " + std::to_string(value) + "  " + desc;
        DrawString(textStr, btnX + btnW + 0.030f, rowY, fSize, col, uiProgram, uiVAO, uiVBO);
        rowY -= attrStep;
    };

    drawAttrButtonRow(0, "FUERZA", stats.Strength, "(+2 ATQ)", glm::vec3(0.80f, 0.20f, 0.15f));
    drawAttrButtonRow(1, "AGILIDAD", stats.Agility, "(+VEL.ATQ +CRIT +EVA)", glm::vec3(0.15f, 0.65f, 0.30f));
    drawAttrButtonRow(2, "VITALIDAD", stats.Vitality, "(+5 VIDA)", glm::vec3(0.75f, 0.55f, 0.10f));
    drawAttrButtonRow(3, "INTELIGENCIA", stats.Intelligence, "(+5 MANA)", glm::vec3(0.15f, 0.35f, 0.80f));

    // Footer Close Button
    float closeBtnW = 0.42f, closeBtnH = 0.075f;
    float closeBtnX = pX + (pW - closeBtnW) * 0.5f;
    float closeBtnY = pY + 0.028f;
    bool closeHovered = (mouseNdcX >= closeBtnX && mouseNdcX <= closeBtnX + closeBtnW && mouseNdcY >= closeBtnY && mouseNdcY <= closeBtnY + closeBtnH);
    DrawWin98Button(uiProgram, uiVAO, uiVBO, closeBtnX, closeBtnY, closeBtnW, closeBtnH, "CERRAR PANEL (C)", closeHovered, 0.026f);

    glUniform3f(glGetUniformLocation(uiProgram, "u_Color"), 1.0f, 1.0f, 1.0f);
}

bool UIRenderer::HandleCharacterPanelClick(float mouseNdcX, float mouseNdcY, PlayerStats& stats, bool& closeRequested) {
    float pW = 0.96f, pH = 1.54f;
    float pX = -pW * 0.5f, pY = -pH * 0.5f;

    // Check Close Button [X] in Title Bar
    float xBtnW = 0.045f, xBtnH = 0.045f;
    float xBtnX = pX + pW - xBtnW - 0.014f;
    float xBtnY = pY + pH - 0.065f - 0.008f + 0.010f;
    if (mouseNdcX >= xBtnX && mouseNdcX <= xBtnX + xBtnW && mouseNdcY >= xBtnY && mouseNdcY <= xBtnY + xBtnH) {
        closeRequested = true;
        return true;
    }

    // Check Footer [CERRAR PANEL (C)] Button
    float closeBtnW = 0.42f, closeBtnH = 0.075f;
    float closeBtnX = pX + (pW - closeBtnW) * 0.5f;
    float closeBtnY = pY + 0.028f;
    if (mouseNdcX >= closeBtnX && mouseNdcX <= closeBtnX + closeBtnW && mouseNdcY >= closeBtnY && mouseNdcY <= closeBtnY + closeBtnH) {
        closeRequested = true;
        return true;
    }

    // Check [ + ] Attribute Allocation Buttons
    if (stats.AvailableStatPoints <= 0) return false;

    float rowY = pY + pH - 0.14f - 0.082f * 2.0f - 0.04f - 0.082f * 0.85f - 0.082f * 2.0f - 0.04f - 0.11f;
    float btnW = 0.080f, btnH = 0.065f;
    float btnX = pX + 0.045f;
    float attrStep = 0.095f;

    for (int i = 0; i < 4; ++i) {
        float bY = rowY - 0.015f;
        if (mouseNdcX >= btnX && mouseNdcX <= btnX + btnW && mouseNdcY >= bY && mouseNdcY <= bY + btnH) {
            if (i == 0) return stats.AllocateStrength();
            if (i == 1) return stats.AllocateAgility();
            if (i == 2) return stats.AllocateVitality();
            if (i == 3) return stats.AllocateIntelligence();
        }
        rowY -= attrStep;
    }

    return false;
}

void UIRenderer::RenderFatalErrorModal(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, const FatalErrorPopup& popup, float mouseNdcX, float mouseNdcY) {
    if (!popup.active) return;

    float mW = 0.82f, mH = 0.46f;
    float mX = -mW * 0.5f, mY = -mH * 0.5f;

    DrawWin98Window(uiProgram, uiVAO, uiVBO, mX, mY, mW, mH, "ERROR CRITICO DEL SISTEMA", true);

    // Red Circle with White Cross [X] Icon
    float iconX = mX + 0.05f, iconY = mY + mH - 0.18f;
    drawColoredQuad(uiProgram, uiVAO, uiVBO, iconX, iconY, 0.08f, 0.08f, glm::vec3(0.85f, 0.12f, 0.12f));
    DrawString("X", iconX + 0.024f, iconY + 0.015f, 0.050f, glm::vec3(1.0f, 1.0f, 1.0f), uiProgram, uiVAO, uiVBO);

    // Error Messages
    DrawString("SECTOR VITALIDAD COMPROMETIDO!", mX + 0.16f, mY + mH - 0.13f, 0.028f, glm::vec3(0.85f, 0.10f, 0.10f), uiProgram, uiVAO, uiVBO);
    DrawString(popup.message, mX + 0.16f, mY + mH - 0.18f, 0.026f, glm::vec3(0.15f, 0.15f, 0.20f), uiProgram, uiVAO, uiVBO);

    std::string dmgStr = "IMPACTO CRITICO: -" + std::to_string(popup.damageValue) + " HP";
    DrawString(dmgStr, mX + 0.16f, mY + mH - 0.23f, 0.026f, glm::vec3(0.75f, 0.15f, 0.15f), uiProgram, uiVAO, uiVBO);

    // 3D Beveled [ACEPTAR (ENTER)] Button
    float btnW = 0.36f, btnH = 0.075f;
    float btnX = mX + (mW - btnW) * 0.5f;
    float btnY = mY + 0.035f;
    bool hovered = (mouseNdcX >= btnX && mouseNdcX <= btnX + btnW && mouseNdcY >= btnY && mouseNdcY <= btnY + btnH);

    DrawWin98Button(uiProgram, uiVAO, uiVBO, btnX, btnY, btnW, btnH, "ACEPTAR (ENTER)", hovered, 0.026f);
}

bool UIRenderer::HandleFatalErrorClick(float mouseNdcX, float mouseNdcY, FatalErrorPopup& popup) {
    if (!popup.active) return false;

    float mW = 0.82f, mH = 0.46f;
    float mX = -mW * 0.5f, mY = -mH * 0.5f;

    // Check Close Button [X] in Title Bar
    float xBtnW = 0.045f, xBtnH = 0.045f;
    float xBtnX = mX + mW - xBtnW - 0.014f;
    float xBtnY = mY + mH - 0.065f - 0.008f + 0.010f;
    if (mouseNdcX >= xBtnX && mouseNdcX <= xBtnX + xBtnW && mouseNdcY >= xBtnY && mouseNdcY <= xBtnY + xBtnH) {
        popup.active = false;
        return true;
    }

    // Check [ACEPTAR] Button
    float btnW = 0.36f, btnH = 0.075f;
    float btnX = mX + (mW - btnW) * 0.5f;
    float btnY = mY + 0.035f;
    if (mouseNdcX >= btnX && mouseNdcX <= btnX + btnW && mouseNdcY >= btnY && mouseNdcY <= btnY + btnH) {
        popup.active = false;
        return true;
    }

    return false;
}

void UIRenderer::RenderLoreModal(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, const LoreDocumentModal& modal, float mouseNdcX, float mouseNdcY) {
    if (!modal.active) return;

    float mW = 0.88f, mH = 0.90f;
    float mX = -mW * 0.5f, mY = -mH * 0.5f;

    DrawWin98Window(uiProgram, uiVAO, uiVBO, mX, mY, mW, mH, modal.title, true);

    // Notepad White Paper Area
    float paperW = mW - 0.040f, paperH = mH - 0.18f;
    float paperX = mX + 0.020f, paperY = mY + 0.090f;
    drawColoredQuad(uiProgram, uiVAO, uiVBO, paperX, paperY, paperW, paperH, glm::vec3(0.96f, 0.96f, 0.94f));

    // Sunken Paper Border
    drawColoredQuad(uiProgram, uiVAO, uiVBO, paperX, paperY + paperH - 0.003f, paperW, 0.003f, glm::vec3(0.35f, 0.35f, 0.35f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, paperX, paperY, 0.003f, paperH, glm::vec3(0.35f, 0.35f, 0.35f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, paperX, paperY, paperW, 0.003f, glm::vec3(1.0f, 1.0f, 1.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, paperX + paperW - 0.003f, paperY, 0.003f, paperH, glm::vec3(1.0f, 1.0f, 1.0f));

    // Handwritten Lore Lines
    float textY = paperY + paperH - 0.08f;
    float lStep = 0.075f;
    glm::vec3 inkCol(0.10f, 0.12f, 0.25f);

    if (!modal.line1.empty()) { DrawString(modal.line1, paperX + 0.035f, textY, 0.026f, inkCol, uiProgram, uiVAO, uiVBO); textY -= lStep; }
    if (!modal.line2.empty()) { DrawString(modal.line2, paperX + 0.035f, textY, 0.026f, inkCol, uiProgram, uiVAO, uiVBO); textY -= lStep; }
    if (!modal.line3.empty()) { DrawString(modal.line3, paperX + 0.035f, textY, 0.026f, inkCol, uiProgram, uiVAO, uiVBO); textY -= lStep; }
    if (!modal.line4.empty()) { DrawString(modal.line4, paperX + 0.035f, textY, 0.026f, inkCol, uiProgram, uiVAO, uiVBO); textY -= lStep; }

    if (!modal.rewardText.empty()) {
        DrawString(modal.rewardText, paperX + 0.035f, paperY + 0.035f, 0.028f, glm::vec3(0.15f, 0.65f, 0.20f), uiProgram, uiVAO, uiVBO);
    }

    // Close button at bottom
    float btnW = 0.32f, btnH = 0.065f;
    float btnX = mX + (mW - btnW) * 0.5f;
    float btnY = mY + 0.015f;
    bool hovered = (mouseNdcX >= btnX && mouseNdcX <= btnX + btnW && mouseNdcY >= btnY && mouseNdcY <= btnY + btnH);

    DrawWin98Button(uiProgram, uiVAO, uiVBO, btnX, btnY, btnW, btnH, "CERRAR (E)", hovered, 0.026f);
}

bool UIRenderer::HandleLoreModalClick(float mouseNdcX, float mouseNdcY, LoreDocumentModal& modal) {
    if (!modal.active) return false;

    float mW = 0.88f, mH = 0.90f;
    float mX = -mW * 0.5f, mY = -mH * 0.5f;

    // Check Close Button [X] in Title Bar
    float xBtnW = 0.045f, xBtnH = 0.045f;
    float xBtnX = mX + mW - xBtnW - 0.014f;
    float xBtnY = mY + mH - 0.065f - 0.008f + 0.010f;
    if (mouseNdcX >= xBtnX && mouseNdcX <= xBtnX + xBtnW && mouseNdcY >= xBtnY && mouseNdcY <= xBtnY + xBtnH) {
        modal.active = false;
        return true;
    }

    // Check [CERRAR (E)] Button
    float btnW = 0.32f, btnH = 0.065f;
    float btnX = mX + (mW - btnW) * 0.5f;
    float btnY = mY + 0.015f;
    if (mouseNdcX >= btnX && mouseNdcX <= btnX + btnW && mouseNdcY >= btnY && mouseNdcY <= btnY + btnH) {
        modal.active = false;
        return true;
    }

    return false;
}

void UIRenderer::RenderInteractionPrompt(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, const std::string& prompt) {
    if (prompt.empty()) return;

    float pW = 0.58f, pH = 0.09f;
    float pX = -pW * 0.5f, pY = -0.35f;

    DrawWin98Window(uiProgram, uiVAO, uiVBO, pX, pY, pW, pH, "ACCION DISPONIBLE", false);
    DrawString(prompt, pX + 0.035f, pY + 0.020f, 0.028f, glm::vec3(0.06f, 0.14f, 0.48f), uiProgram, uiVAO, uiVBO);
}

void UIRenderer::RenderStunWarning(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float stunTimer) {
    if (stunTimer <= 0.0f) return;

    float pW = 0.58f, pH = 0.11f;
    float pX = -pW * 0.5f, pY = 0.15f;

    drawColoredQuad(uiProgram, uiVAO, uiVBO, pX, pY, pW, pH, glm::vec3(0.85f, 0.15f, 0.15f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, pX + 0.005f, pY + 0.005f, pW - 0.010f, pH - 0.010f, glm::vec3(0.15f, 0.05f, 0.05f));

    DrawString("! POSTURA QUEBRADA - ATURDIDO !", pX + 0.035f, pY + 0.040f, 0.028f, glm::vec3(1.0f, 0.85f, 0.20f), uiProgram, uiVAO, uiVBO);
}

void UIRenderer::RenderCursor(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float mouseNdcX, float mouseNdcY) {
    if (mouseNdcX < -1.1f || mouseNdcX > 1.1f || mouseNdcY < -1.1f || mouseNdcY > 1.1f) return;

    float x = mouseNdcX;
    float y = mouseNdcY;
    float pw = 0.0040f;
    float ph = 0.0070f;

    // Outer black border
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y - 12.0f * ph, 2.0f * pw, 13.0f * ph, glm::vec3(0.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x, y - ph, 9.0f * pw, 2.0f * ph, glm::vec3(0.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + pw, y - 3.0f * ph, 8.0f * pw, 2.0f * ph, glm::vec3(0.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 2.0f * pw, y - 5.0f * ph, 7.0f * pw, 2.0f * ph, glm::vec3(0.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 3.0f * pw, y - 7.0f * ph, 6.0f * pw, 2.0f * ph, glm::vec3(0.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 4.0f * pw, y - 9.0f * ph, 5.0f * pw, 2.0f * ph, glm::vec3(0.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 5.0f * pw, y - 14.0f * ph, 3.0f * pw, 6.0f * ph, glm::vec3(0.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 7.0f * pw, y - 15.0f * ph, 2.0f * pw, 3.0f * ph, glm::vec3(0.0f));

    // Inner white fill
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + pw, y - 10.0f * ph, pw, 10.0f * ph, glm::vec3(1.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 2.0f * pw, y - 9.0f * ph, pw, 8.0f * ph, glm::vec3(1.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 3.0f * pw, y - 8.0f * ph, pw, 6.0f * ph, glm::vec3(1.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 4.0f * pw, y - 7.0f * ph, pw, 4.0f * ph, glm::vec3(1.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 5.0f * pw, y - 12.0f * ph, pw, 7.0f * ph, glm::vec3(1.0f));
    drawColoredQuad(uiProgram, uiVAO, uiVBO, x + 6.0f * pw, y - 13.0f * ph, pw, 2.0f * ph, glm::vec3(1.0f));

    glUniform3f(glGetUniformLocation(uiProgram, "u_Color"), 1.0f, 1.0f, 1.0f);
}

void UIRenderer::RenderBuildingHUD(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, int currentType, float currentYaw) {
    float w = 1.32f, h = 0.16f;
    float x = -w * 0.5f, y = -0.74f;

    std::string title = "[ MODO CONSTRUCCION Y REFUGIO - ARPG ]";
    DrawWin98Window(uiProgram, uiVAO, uiVBO, x, y, w, h, title, false);

    std::string typeName = (currentType == 0) ? "[1] PARED (ACTIVA)" : "[1] PARED";
    std::string roofName = (currentType == 1) ? "[2] TECHO/BOVEDA (ACTIVA)" : "[2] TECHO/BOVEDA";
    std::string torchName = (currentType == 2) ? "[3] ANTORCHA (ACTIVA)" : "[3] ANTORCHA";

    std::string row1 = typeName + "  " + roofName + "  " + torchName;
    std::string row2 = "[R] ROTAR (" + std::to_string((int)currentYaw) + " DEG) | [CLICK IZQ] COLOCAR | [B] SALIR";

    DrawString(row1, x + 0.025f, y + 0.062f, 0.023f, glm::vec3(0.12f, 0.12f, 0.18f), uiProgram, uiVAO, uiVBO);
    DrawString(row2, x + 0.025f, y + 0.020f, 0.021f, glm::vec3(0.15f, 0.45f, 0.15f), uiProgram, uiVAO, uiVBO);
}

#include "inventory/Inventory.h"

void UIRenderer::RenderQuickbarHUD(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, const Inventory& inventory) {
    float slotW = 0.18f, slotH = 0.115f;
    float pad = 0.015f;
    int numSlots = 4;
    float totalW = numSlots * slotW + (numSlots - 1) * pad;
    float startX = -totalW * 0.5f;
    float startY = -0.96f;

    struct QuickSlotDef {
        std::string keyLabel;
        std::string nameLabel;
        std::string itemId;
        std::string altItemId;
        glm::vec3 textColor;
    };

    QuickSlotDef slots[4] = {
        { "[1]", "SALUD",  "potion_health", "",             glm::vec3(0.85f, 0.15f, 0.15f) },
        { "[2]", "MANA",   "potion_mana",   "",             glm::vec3(0.15f, 0.45f, 0.90f) },
        { "[3]", "SANGRE", "blood_vial",    "dragon_heart", glm::vec3(0.70f, 0.12f, 0.65f) },
        { "[4]", "CARNE",  "raw_meat",      "",             glm::vec3(0.75f, 0.45f, 0.15f) }
    };

    for (int i = 0; i < numSlots; ++i) {
        float sx = startX + i * (slotW + pad);
        DrawWin98Button(uiProgram, uiVAO, uiVBO, sx, startY, slotW, slotH, "", false, 0.022f);

        int count = inventory.CountItemByString(slots[i].itemId);
        if (!slots[i].altItemId.empty()) {
            count += inventory.CountItemByString(slots[i].altItemId);
        }

        // Key label [1]
        DrawString(slots[i].keyLabel, sx + 0.012f, startY + slotH - 0.040f, 0.022f, glm::vec3(0.10f, 0.12f, 0.20f), uiProgram, uiVAO, uiVBO);
        // Name label
        DrawString(slots[i].nameLabel, sx + 0.055f, startY + slotH - 0.040f, 0.019f, slots[i].textColor, uiProgram, uiVAO, uiVBO);

        // Count label
        std::string cntStr = "x" + std::to_string(count);
        glm::vec3 cntColor = (count > 0) ? glm::vec3(0.08f, 0.50f, 0.15f) : glm::vec3(0.65f, 0.65f, 0.70f);
        DrawString(cntStr, sx + 0.035f, startY + 0.018f, 0.022f, cntColor, uiProgram, uiVAO, uiVBO);
    }
}

void UIRenderer::RenderPauseMenu(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float mouseNdcX, float mouseNdcY) {
    // Oscurecer fondo translúcido
    drawColoredQuad(uiProgram, uiVAO, uiVBO, -1.0f, -1.0f, 2.0f, 2.0f, glm::vec3(0.02f, 0.02f, 0.05f));

    float mW = 0.90f, mH = 0.55f;
    float mX = -mW * 0.5f, mY = -mH * 0.5f;

    DrawWin98Window(uiProgram, uiVAO, uiVBO, mX, mY, mW, mH, "SISTEMA EN PAUSA - VRAM DUNGEON.EXE", false);

    DrawString("PARTIDA EN PAUSA", mX + 0.22f, mY + mH - 0.13f, 0.034f, glm::vec3(0.85f, 0.15f, 0.15f), uiProgram, uiVAO, uiVBO);
    DrawString("La simulacion del mundo ha sido congelada.", mX + 0.06f, mY + mH - 0.22f, 0.022f, glm::vec3(0.12f, 0.12f, 0.18f), uiProgram, uiVAO, uiVBO);
    DrawString("Pulsa [ESC] o haz clic en el boton para volver al juego.", mX + 0.06f, mY + mH - 0.28f, 0.021f, glm::vec3(0.35f, 0.35f, 0.40f), uiProgram, uiVAO, uiVBO);

    // Botón [REANUDAR PARTIDA]
    float btnW = 0.50f, btnH = 0.080f;
    float btnX = mX + (mW - btnW) * 0.5f;
    float btnY = mY + 0.040f;
    bool hovered = (mouseNdcX >= btnX && mouseNdcX <= btnX + btnW && mouseNdcY >= btnY && mouseNdcY <= btnY + btnH);

    DrawWin98Button(uiProgram, uiVAO, uiVBO, btnX, btnY, btnW, btnH, "REANUDAR PARTIDA (ESC)", hovered, 0.024f);
}

void UIRenderer::RenderGameOverScreen(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float deathTimer) {
    // 1. Red dark vignette / blood curtain
    drawColoredQuad(uiProgram, uiVAO, uiVBO, -1.0f, -1.0f, 2.0f, 2.0f, glm::vec3(0.18f, 0.02f, 0.02f));

    float mW = 1.05f, mH = 0.65f;
    float mX = -mW * 0.5f, mY = -mH * 0.5f;

    DrawWin98Window(uiProgram, uiVAO, uiVBO, mX, mY, mW, mH, "*** ERROR FATAL: COLAPSO VITAL EN 0x0000DEAD ***", false);

    // Inner dark terminal box
    float boxPad = 0.025f;
    drawColoredQuad(uiProgram, uiVAO, uiVBO, mX + boxPad, mY + 0.13f, mW - boxPad * 2.0f, mH - 0.22f, glm::vec3(0.04f, 0.02f, 0.02f));

    DrawString("HAS MUERTO", mX + 0.38f, mY + mH - 0.14f, 0.040f, glm::vec3(0.95f, 0.15f, 0.15f), uiProgram, uiVAO, uiVBO);
    DrawString("Tu fuerza vital ha descendido a 0. Las sombras te han consumido.", mX + 0.08f, mY + mH - 0.23f, 0.021f, glm::vec3(0.85f, 0.75f, 0.70f), uiProgram, uiVAO, uiVBO);
    DrawString("Causa de fallo: Daño letal recibido en combate.", mX + 0.08f, mY + mH - 0.30f, 0.020f, glm::vec3(0.65f, 0.55f, 0.50f), uiProgram, uiVAO, uiVBO);
    DrawString("Estado de memoria: Entidad biologica fuera de linea.", mX + 0.08f, mY + mH - 0.37f, 0.020f, glm::vec3(0.50f, 0.50f, 0.55f), uiProgram, uiVAO, uiVBO);

    // Pulsing respawn prompt
    float pulse = 0.5f + 0.5f * sin(deathTimer * 6.0f);
    glm::vec3 pulseColor = glm::mix(glm::vec3(0.20f, 0.90f, 0.35f), glm::vec3(1.0f, 1.0f, 0.40f), pulse);

    float btnW = 0.76f, btnH = 0.085f;
    float btnX = mX + (mW - btnW) * 0.5f;
    float btnY = mY + 0.025f;

    // Dibujar boton sin texto duplicado interno
    DrawWin98Button(uiProgram, uiVAO, uiVBO, btnX, btnY, btnW, btnH, "", true, 0.024f);
    // Unico texto centrado y pulsante
    DrawString("[ESPACIO / CLICK] RENACER EN EL CAMPAMENTO", btnX + 0.045f, btnY + 0.028f, 0.022f, pulseColor, uiProgram, uiVAO, uiVBO);
}

bool UIRenderer::HandlePauseMenuClick(float mouseNdcX, float mouseNdcY, bool& resumeRequested) {
    float mW = 0.90f, mH = 0.55f;
    float mX = -mW * 0.5f, mY = -mH * 0.5f;

    float btnW = 0.50f, btnH = 0.080f;
    float btnX = mX + (mW - btnW) * 0.5f;
    float btnY = mY + 0.040f;

    if (mouseNdcX >= btnX && mouseNdcX <= btnX + btnW && mouseNdcY >= btnY && mouseNdcY <= btnY + btnH) {
        resumeRequested = true;
        return true;
    }
    return false;
}

void UIRenderer::DrawCrosshair(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float size, float thickness, glm::vec3 color) {
    drawColoredQuad(uiProgram, uiVAO, uiVBO, -thickness * 0.5f, -size, thickness, size * 2.0f, color);
    drawColoredQuad(uiProgram, uiVAO, uiVBO, -size, -thickness * 0.5f, size * 2.0f, thickness, color);
}

