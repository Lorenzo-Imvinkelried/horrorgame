#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "CombatStats.h"
#include "TargetingSystem.h"
#include "DamageNumberSystem.h"

struct FatalErrorPopup {
    bool active = false;
    int damageValue = 0;
    float timer = 0.0f;
    std::string message = "VIOLACION EN 0x000000FF";
};

struct LoreDocumentModal {
    bool active = false;
    std::string title = "REGISTRO_FORENSE.TXT";
    std::string line1 = "";
    std::string line2 = "";
    std::string line3 = "";
    std::string line4 = "";
    std::string rewardText = "";
};

class UIRenderer {
public:
    UIRenderer();
    ~UIRenderer();

    void RenderHUD(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, 
                   const PlayerStats& stats, 
                   const TargetingSystem& targeting, 
                   const DamageNumberSystem& damageNumbers,
                   const glm::mat4& viewProj,
                   float dangerLevel,
                   float globalTime,
                   int nightCount = 1,
                   bool isBloodMoon = false);

    void RenderCharacterPanel(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, 
                              const PlayerStats& stats,
                              float mouseNdcX = -999.0f, float mouseNdcY = -999.0f);

    void RenderFatalErrorModal(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, 
                              const FatalErrorPopup& popup,
                              float mouseNdcX = -999.0f, float mouseNdcY = -999.0f);

    void RenderLoreModal(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, 
                         const LoreDocumentModal& modal,
                         float mouseNdcX = -999.0f, float mouseNdcY = -999.0f);

    void RenderInteractionPrompt(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, 
                                 const std::string& prompt);

    void RenderStunWarning(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float stunTimer);

    // Mouse Click Handlers
    static bool HandleCharacterPanelClick(float mouseNdcX, float mouseNdcY, PlayerStats& stats, bool& closeRequested);
    static bool HandleFatalErrorClick(float mouseNdcX, float mouseNdcY, FatalErrorPopup& popup);
    static bool HandleLoreModalClick(float mouseNdcX, float mouseNdcY, LoreDocumentModal& modal);

    static void DrawWin98Window(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, 
                                float x, float y, float w, float h, 
                                const std::string& title, bool hasCloseBtn);

    static void DrawWin98Button(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO,
                                float x, float y, float w, float h,
                                const std::string& text, bool isHovered,
                                float textSize = 0.026f);

    static void DrawString(const std::string& text, float x, float y, float size, glm::vec3 color, 
                           GLuint uiProgram, GLuint uiVAO, GLuint uiVBO);

private:
    static void pushQuad(std::vector<float>& data, float x, float y, float w, float h);
    static void drawColoredQuad(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, float x, float y, float w, float h, glm::vec3 color);
    static void drawDigit(int d, float x, float y, float size, glm::vec3 color, GLuint uiProgram, GLuint uiVAO, GLuint uiVBO);
    static void drawNumber(int num, float x, float y, float size, glm::vec3 color, GLuint uiProgram, GLuint uiVAO, GLuint uiVBO);
    static void drawGlyph(char c, float x, float y, float size, glm::vec3 color, GLuint uiProgram, GLuint uiVAO, GLuint uiVBO);
    static void drawECGWave(GLuint uiProgram, GLuint uiVAO, GLuint uiVBO, 
                            float x, float y, float w, float h, 
                            float healthPct, float dangerLevel, float globalTime);
};
