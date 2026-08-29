#include "AnimatorUI.h"
#include "AnimatorStudio.h"
#include "Config.h"
#include <iomanip>
#include <sstream>
#include <cmath>
#include <iostream>
#include <algorithm>

AnimatorUI::AnimatorUI(sf::Texture* fontTexture)
    : mFontTexture(fontTexture)
{
}

void AnimatorUI::drawText(sf::RenderTarget& target, const std::string& text, sf::Vector2f pos, float scale, sf::Color color, bool center) {
    if (!mFontTexture) return;
    BitmapText bt;
    bt.setTexture(mFontTexture);
    bt.setString(text);
    bt.setScale({ scale * cfg::UI::FONT_SCALE, scale * cfg::UI::FONT_SCALE });
    bt.setColor(color);

    if (center) {
        sf::FloatRect b = bt.getLocalBounds();
        bt.setOrigin({ std::round(b.size.x * 0.5f), std::round(b.size.y * 0.5f) });
    }
    bt.setPosition({ std::round(pos.x), std::round(pos.y) });
    target.draw(bt);
}

void AnimatorUI::drawButton(sf::RenderTarget& target, const UIButton& btn) {
    sf::RectangleShape rect({ btn.bounds.size.x, btn.bounds.size.y });
    rect.setPosition({ btn.bounds.position.x, btn.bounds.position.y });

    if (btn.isSelected) {
        rect.setFillColor(sf::Color(249, 194, 43, 220));
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(1.5f);
    } else if (btn.isHovered) {
        rect.setFillColor(btn.hoverColor);
        rect.setOutlineColor(sf::Color(249, 194, 43, 255));
        rect.setOutlineThickness(1.f);
    } else {
        rect.setFillColor(btn.baseColor);
        rect.setOutlineColor(sf::Color(72, 74, 119, 180));
        rect.setOutlineThickness(1.f);
    }

    target.draw(rect);

    sf::Color txtColor = btn.isSelected ? sf::Color(30, 30, 40) : (btn.isHovered ? sf::Color(249, 194, 43) : btn.textColor);
    sf::Vector2f centerPos = {
        btn.bounds.position.x + btn.bounds.size.x * 0.5f,
        btn.bounds.position.y + btn.bounds.size.y * 0.5f
    };
    float scale = (btn.bounds.size.x < 110.f || btn.bounds.size.y < 22.f) ? 0.85f : 0.95f;
    drawText(target, btn.text, centerPos, scale, txtColor, true);
}

void AnimatorUI::rebuildButtons(AnimatorStudio& studio) {
    mHeaderButtons.clear();
    mTimelineButtons.clear();
    mClipButtons.clear();
    mWeaponButtons.clear();
    mToggleButtons.clear();
    mBoneButtons.clear();
    mOffsetButtons.clear();
    mKeyframeButtons.clear();
    mRotationButtons.clear();
    mNudgeButtons.clear();
    mLayerButtons.clear();

    float w = cfg::UI::LOGICAL_WIDTH;
    float h = cfg::UI::LOGICAL_HEIGHT;

    // --- Header Buttons ---
    mHeaderButtons.push_back({ "btn_menu", "VOLVER AL MENU (ESC)", sf::FloatRect({12.f, 9.f}, {165.f, 26.f}) });

    UIButton btnNew;
    btnNew.id = "btn_new_anim";
    btnNew.text = "+ CREAR CLIP";
    btnNew.bounds = sf::FloatRect({w - 685.f, 9.f}, {160.f, 26.f});
    btnNew.baseColor = sf::Color(35, 80, 55, 240);
    btnNew.hoverColor = sf::Color(45, 120, 75, 255);
    btnNew.textColor = sf::Color(140, 255, 170);
    mHeaderButtons.push_back(btnNew);

    UIButton btnFlip;
    btnFlip.id = "btn_flip_view";
    btnFlip.text = (studio.getFacingDir() == 1) ? "MIRAR: DER [D]" : "MIRAR: IZQ [A]";
    btnFlip.bounds = sf::FloatRect({w - 515.f, 9.f}, {165.f, 26.f});
    if (studio.getFacingDir() == 1) {
        btnFlip.baseColor = sf::Color(30, 60, 95, 240);
        btnFlip.hoverColor = sf::Color(45, 90, 145, 255);
        btnFlip.textColor = sf::Color(110, 220, 255);
    } else {
        btnFlip.baseColor = sf::Color(75, 45, 95, 240);
        btnFlip.hoverColor = sf::Color(115, 65, 145, 255);
        btnFlip.textColor = sf::Color(230, 160, 255);
    }
    mHeaderButtons.push_back(btnFlip);

    mHeaderButtons.push_back({ "btn_save", "GUARDAR (CTRL+S)", sf::FloatRect({w - 340.f, 9.f}, {152.f, 26.f}) });
    mHeaderButtons.push_back({ "btn_reload", "RECARGAR (F5)", sf::FloatRect({w - 180.f, 9.f}, {168.f, 26.f}) });

    // --- Left Sidebar: Clips & Weapons ---
    float leftX = 16.f;
    float clipY = 72.f;
    const auto& clips = studio.getAvailableClips();
    
    // Arrange clips in 2 columns of 104px width if > 6 clips, or 1 column if <= 6
    bool twoColClips = (clips.size() > 6);
    for (size_t i = 0; i < clips.size(); ++i) {
        UIButton b;
        b.id = "clip_" + std::to_string(i);
        b.text = clips[i].displayName;
        if (twoColClips) {
            float cx = (i % 2 == 0) ? leftX : (leftX + 110.f);
            float cy = clipY + (i / 2) * 22.f;
            b.bounds = sf::FloatRect({cx, cy}, {104.f, 20.f});
        } else {
            b.bounds = sf::FloatRect({leftX, clipY + i * 22.f}, {214.f, 20.f});
        }
        b.isSelected = (i == studio.getActiveClipIndex());
        mClipButtons.push_back(b);
    }
    
    if (twoColClips) {
        clipY += ((clips.size() + 1) / 2) * 22.f + 4.f;
    } else {
        clipY += clips.size() * 22.f + 4.f;
    }

    // Delete Current Clip Button
    UIButton btnDelClip;
    btnDelClip.id = "btn_delete_clip";
    btnDelClip.text = "- ELIMINAR CLIP (JSON)";
    btnDelClip.bounds = sf::FloatRect({leftX, clipY}, {214.f, 20.f});
    btnDelClip.baseColor = sf::Color(80, 35, 40, 240);
    btnDelClip.hoverColor = sf::Color(130, 45, 55, 255);
    btnDelClip.textColor = sf::Color(255, 180, 190);
    mClipButtons.push_back(btnDelClip);
    clipY += 26.f;

    // Left Sidebar: Weapons section title position
    mLeftWepTitleY = clipY + 8.f;
    float wepY = mLeftWepTitleY + 18.f;

    auto addWepBtn = [&](const std::string& id, const std::string& name, bool isSel, float x, float y, float bw) {
        UIButton b;
        b.id = id;
        b.text = name;
        b.bounds = sf::FloatRect({x, y}, {bw, 20.f});
        b.isSelected = isSel;
        mWeaponButtons.push_back(b);
    };

    StudioWeaponType currWep = studio.getWeaponType();
    addWepBtn("wep_none", "Sin Arma", currWep == StudioWeaponType::None, leftX, wepY, 104.f);
    addWepBtn("wep_1h", "Espada 1M", currWep == StudioWeaponType::OneHandedSword, leftX + 110.f, wepY, 104.f);
    wepY += 22.f;

    addWepBtn("wep_2h", "Espadon 2M", currWep == StudioWeaponType::TwoHandedSword, leftX, wepY, 104.f);
    addWepBtn("wep_dual", "Doble Arma", currWep == StudioWeaponType::DualWield, leftX + 110.f, wepY, 104.f);
    wepY += 22.f;

    addWepBtn("wep_shield", "Espada+Esc", currWep == StudioWeaponType::SwordAndShield, leftX, wepY, 104.f);
    addWepBtn("wep_shield_main", "Escudo+Esp", currWep == StudioWeaponType::ShieldAndSword, leftX + 110.f, wepY, 104.f);
    wepY += 22.f;

    addWepBtn("wep_shield_only_r", "Solo Esc(D)", currWep == StudioWeaponType::ShieldRightOnly, leftX, wepY, 104.f);
    addWepBtn("wep_shield_only_l", "Solo Esc(I)", currWep == StudioWeaponType::ShieldLeftOnly, leftX + 110.f, wepY, 104.f);
    wepY += 22.f;


    // --- Right Sidebar ---
    float rightX = w - 238.f;
    float rightY = 54.f;

    // Section 1: Diagnóstico & IK
    mRightTogglesTitleY = rightY;
    rightY += 18.f;

    auto addToggle = [&](const std::string& id, const std::string& label, bool state, float x, float y, float bw) {
        UIButton b;
        b.id = id;
        b.text = (state ? "[X] " : "[ ] ") + label;
        b.bounds = sf::FloatRect({x, y}, {bw, 20.f});
        b.isSelected = state;
        mToggleButtons.push_back(b);
    };

    addToggle("toggle_ik", "Proc. IK", studio.isIKEnabled(), rightX, rightY, 107.f);
    addToggle("toggle_curves", "Curvas Vel.", studio.isSpeedCurvesEnabled(), rightX + 113.f, rightY, 107.f);
    rightY += 22.f;

    addToggle("toggle_bones", "Pivotes", studio.isShowBones(), rightX, rightY, 107.f);
    addToggle("toggle_trail", "Motion Trail", studio.isShowMotionTrail(), rightX + 113.f, rightY, 107.f);
    rightY += 26.f;

    // Section 2: Inspector Box
    mRightInspectorBoxY = rightY;
    rightY += 46.f;

    // Section 3: Bone Selector
    mRightBonesTitleY = rightY;
    rightY += 18.f;

    const auto& nodes = studio.getNodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        float bx = (i % 2 == 0) ? rightX : (rightX + 113.f);
        float by = rightY + (i / 2) * 21.f;
        UIButton b;
        b.id = "bone_" + nodes[i].name;
        b.text = nodes[i].name;
        b.bounds = sf::FloatRect({bx, by}, {107.f, 19.f});
        b.isSelected = (nodes[i].name == studio.getSelectedNode());
        mBoneButtons.push_back(b);
    }
    rightY += ((nodes.size() + 1) / 2) * 21.f + 3.f;

    // Weapon node button & Clear bone tracks
    bool hasWeapon = (studio.getWeaponType() != StudioWeaponType::None);
    std::string selNode = studio.getSelectedNode();

    if (hasWeapon && !selNode.empty()) {
        UIButton bWep;
        bWep.id = "bone_weapon";
        bWep.text = "[!] WEAPON";
        bWep.bounds = sf::FloatRect({rightX, rightY}, {107.f, 20.f});
        bWep.isSelected = ("weapon" == selNode);
        bWep.baseColor = sf::Color(80, 45, 60, 240);
        bWep.hoverColor = sf::Color(140, 60, 85, 255);
        mBoneButtons.push_back(bWep);

        UIButton btnClearBone;
        btnClearBone.id = "btn_clear_bone_tracks";
        btnClearBone.text = "Borrar Pistas";
        btnClearBone.bounds = sf::FloatRect({rightX + 113.f, rightY}, {107.f, 20.f});
        btnClearBone.baseColor = sf::Color(85, 40, 45, 240);
        btnClearBone.hoverColor = sf::Color(140, 50, 60, 255);
        btnClearBone.textColor = sf::Color(255, 190, 200);
        mBoneButtons.push_back(btnClearBone);
        rightY += 24.f;
    } else if (hasWeapon) {
        UIButton bWep;
        bWep.id = "bone_weapon";
        bWep.text = "[!] WEAPON";
        bWep.bounds = sf::FloatRect({rightX, rightY}, {220.f, 20.f});
        bWep.isSelected = ("weapon" == selNode);
        bWep.baseColor = sf::Color(80, 45, 60, 240);
        bWep.hoverColor = sf::Color(140, 60, 85, 255);
        mBoneButtons.push_back(bWep);
        rightY += 24.f;
    } else if (!selNode.empty()) {
        UIButton btnClearBone;
        btnClearBone.id = "btn_clear_bone_tracks";
        btnClearBone.text = "Borrar Pistas: " + selNode;
        btnClearBone.bounds = sf::FloatRect({rightX, rightY}, {220.f, 20.f});
        btnClearBone.baseColor = sf::Color(85, 40, 45, 240);
        btnClearBone.hoverColor = sf::Color(140, 50, 60, 255);
        btnClearBone.textColor = sf::Color(255, 190, 200);
        mBoneButtons.push_back(btnClearBone);
        rightY += 24.f;
    }

    // Section 4: Quick Transform
    mRightRotTitleY = rightY + 4.f;
    rightY = mRightRotTitleY + 18.f;

    auto addRotBtn = [&](const std::string& id, const std::string& label, float x, float y, float bw = 52.f) {
        UIButton b;
        b.id = id;
        b.text = label;
        b.bounds = sf::FloatRect({x, y}, {bw, 19.f});
        mRotationButtons.push_back(b);
    };

    addRotBtn("rot_sub45", "-45", rightX, rightY, 52.f);
    addRotBtn("rot_sub15", "-15", rightX + 56.f, rightY, 52.f);
    addRotBtn("rot_add15", "+15", rightX + 112.f, rightY, 52.f);
    addRotBtn("rot_add45", "+45", rightX + 168.f, rightY, 52.f);
    rightY += 22.f;

    addRotBtn("rot_set0", "0 deg", rightX, rightY, 52.f);
    addRotBtn("rot_set90", "90 deg", rightX + 56.f, rightY, 52.f);
    addRotBtn("rot_set180", "180", rightX + 112.f, rightY, 52.f);
    addRotBtn("rot_set270", "270", rightX + 168.f, rightY, 52.f);
    rightY += 23.f;

    // Nudge Controls
    auto addNudgeBtn = [&](const std::string& id, const std::string& label, float x, float y, float bw = 52.f) {
        UIButton b;
        b.id = id;
        b.text = label;
        b.bounds = sf::FloatRect({x, y}, {bw, 19.f});
        mNudgeButtons.push_back(b);
    };

    addNudgeBtn("nudge_left", "<-", rightX, rightY, 52.f);
    addNudgeBtn("nudge_up", "UP", rightX + 56.f, rightY, 52.f);
    addNudgeBtn("nudge_down", "DN", rightX + 112.f, rightY, 52.f);
    addNudgeBtn("nudge_right", "->", rightX + 168.f, rightY, 52.f);
    rightY += 23.f;

    // Offset Calibration Buttons
    auto addOffsetBtn = [&](const std::string& id, const std::string& label, float x, float y, float bw = 52.f) {
        UIButton b;
        b.id = id;
        b.text = label;
        b.bounds = sf::FloatRect({x, y}, {bw, 19.f});
        mOffsetButtons.push_back(b);
    };

    addOffsetBtn("off_x_sub", "Off -X", rightX, rightY, 52.f);
    addOffsetBtn("off_x_add", "Off +X", rightX + 56.f, rightY, 52.f);
    addOffsetBtn("off_y_sub", "Off -Y", rightX + 112.f, rightY, 52.f);
    addOffsetBtn("off_y_add", "Off +Y", rightX + 168.f, rightY, 52.f);
    rightY += 26.f;

    // Section 5: Layer / Z-Order Controls
    mRightLayerTitleY = rightY;
    rightY += 18.f;

    UIButton bLayerDown;
    bLayerDown.id = "btn_layer_down";
    bLayerDown.text = "▼ AL FONDO";
    bLayerDown.bounds = sf::FloatRect({rightX, rightY}, {107.f, 20.f});
    bLayerDown.baseColor = sf::Color(45, 55, 75, 240);
    bLayerDown.hoverColor = sf::Color(65, 80, 110, 255);
    bLayerDown.textColor = sf::Color(200, 220, 255);
    mLayerButtons.push_back(bLayerDown);

    UIButton bLayerUp;
    bLayerUp.id = "btn_layer_up";
    bLayerUp.text = "▲ AL FRENTE";
    bLayerUp.bounds = sf::FloatRect({rightX + 113.f, rightY}, {107.f, 20.f});
    bLayerUp.baseColor = sf::Color(45, 55, 75, 240);
    bLayerUp.hoverColor = sf::Color(65, 80, 110, 255);
    bLayerUp.textColor = sf::Color(200, 220, 255);
    mLayerButtons.push_back(bLayerUp);
    rightY += 23.f;

    UIButton bLayerReset;
    bLayerReset.id = "btn_layer_reset";
    bLayerReset.text = "Reset Capas 2.5D";
    bLayerReset.bounds = sf::FloatRect({rightX, rightY}, {220.f, 18.f});
    bLayerReset.baseColor = sf::Color(40, 42, 55, 220);
    bLayerReset.textColor = sf::Color(160, 170, 190);
    mLayerButtons.push_back(bLayerReset);
    rightY += 21.f;

    mRightLayerStackY = rightY;


    // --- Bottom Timeline Controls ---
    float tlCenterX = w * 0.5f;

    // Timeline Bar
    mTimelineBarRect = sf::FloatRect({tlCenterX - 180.f, h - 61.f}, {410.f, 14.f});

    float dur = studio.getDuration();
    float progress = (dur > 0.f) ? (studio.getTime() / dur) : 0.f;
    float handleX = mTimelineBarRect.position.x + progress * mTimelineBarRect.size.x;
    mTimelineHandleRect = sf::FloatRect({handleX - 6.f, h - 65.f}, {12.f, 22.f});

    // Keyframe Ticks for Selected Bone (Generous 18px x 24px clickable hitbox)
    mKeyframeTickRects.clear();
    mKeyframeTickTimes.clear();
    std::vector<float> kfTimes = studio.getKeyframeTimes(studio.getSelectedNode());
    if (dur > 0.f) {
        for (float kt : kfTimes) {
            float kfX = mTimelineBarRect.position.x + (kt / dur) * mTimelineBarRect.size.x;
            mKeyframeTickRects.push_back(sf::FloatRect({kfX - 9.f, h - 66.f}, {18.f, 24.f}));
            mKeyframeTickTimes.push_back(kt);
        }
    }

    // Keyframe Navigation & Editing Buttons
    mKeyframeButtons.clear();
    mKeyframeButtons.push_back({ "btn_prev_key", "< KEY", sf::FloatRect({tlCenterX - 390.f, h - 65.f}, {48.f, 22.f}) });
    mKeyframeButtons.push_back({ "btn_next_key", "KEY >", sf::FloatRect({tlCenterX - 338.f, h - 65.f}, {48.f, 22.f}) });
    mKeyframeButtons.push_back({ "btn_add_key", "+ KEY",  sf::FloatRect({tlCenterX - 286.f, h - 65.f}, {48.f, 22.f}) });
    mKeyframeButtons.push_back({ "btn_del_key", "- KEY",  sf::FloatRect({tlCenterX - 234.f, h - 65.f}, {48.f, 22.f}) });

    // Loop Range Management Buttons
    UIButton bLoopStart;
    bLoopStart.id = "btn_loop_start";
    bLoopStart.text = "[ INI LOOP";
    bLoopStart.bounds = sf::FloatRect({tlCenterX + 234.f, h - 65.f}, {72.f, 22.f});
    bLoopStart.baseColor = sf::Color(30, 60, 50, 240);
    bLoopStart.hoverColor = sf::Color(45, 100, 80, 255);
    bLoopStart.textColor = sf::Color(120, 255, 180);
    mKeyframeButtons.push_back(bLoopStart);

    UIButton bLoopEnd;
    bLoopEnd.id = "btn_loop_end";
    bLoopEnd.text = "FIN LOOP ]";
    bLoopEnd.bounds = sf::FloatRect({tlCenterX + 310.f, h - 65.f}, {72.f, 22.f});
    bLoopEnd.baseColor = sf::Color(70, 35, 35, 240);
    bLoopEnd.hoverColor = sf::Color(115, 50, 50, 255);
    bLoopEnd.textColor = sf::Color(255, 160, 160);
    mKeyframeButtons.push_back(bLoopEnd);

    UIButton bLoopReset;
    bLoopReset.id = "btn_loop_reset";
    bLoopReset.text = "RESET";
    bLoopReset.bounds = sf::FloatRect({tlCenterX + 386.f, h - 65.f}, {48.f, 22.f});
    bLoopReset.baseColor = sf::Color(45, 48, 65, 220);
    bLoopReset.hoverColor = sf::Color(65, 70, 95, 255);
    bLoopReset.textColor = sf::Color(180, 190, 210);
    mKeyframeButtons.push_back(bLoopReset);

    // Playback Buttons
    mTimelineButtons.push_back({ "btn_flip", (studio.getFacingDir() == 1 ? "FLIP: DER" : "FLIP: IZQ"), sf::FloatRect({tlCenterX - 240.f, h - 34.f}, {84.f, 24.f}) });
    mTimelineButtons.push_back({ "btn_step_back", "<", sf::FloatRect({tlCenterX - 150.f, h - 34.f}, {32.f, 24.f}) });
    mTimelineButtons.push_back({ "btn_play", studio.isPlaying() ? "PAUSA" : "PLAY", sf::FloatRect({tlCenterX - 112.f, h - 34.f}, {76.f, 24.f}) });
    mTimelineButtons.push_back({ "btn_step_fwd", ">", sf::FloatRect({tlCenterX - 30.f, h - 34.f}, {32.f, 24.f}) });

    // Speed Buttons
    float spd = studio.getSpeedMultiplier();
    mTimelineButtons.push_back({ "spd_0.25", "0.25x", sf::FloatRect({tlCenterX + 12.f, h - 34.f}, {44.f, 24.f}), false, (std::abs(spd - 0.25f) < 0.05f) });
    mTimelineButtons.push_back({ "spd_0.5", "0.5x",   sf::FloatRect({tlCenterX + 60.f, h - 34.f}, {44.f, 24.f}), false, (std::abs(spd - 0.5f) < 0.05f) });
    mTimelineButtons.push_back({ "spd_1.0", "1.0x",   sf::FloatRect({tlCenterX + 108.f, h - 34.f}, {44.f, 24.f}), false, (std::abs(spd - 1.0f) < 0.05f) });
    mTimelineButtons.push_back({ "spd_2.0", "2.0x",   sf::FloatRect({tlCenterX + 156.f, h - 34.f}, {44.f, 24.f}), false, (std::abs(spd - 2.0f) < 0.05f) });

    // Loop
    mTimelineButtons.push_back({ "btn_loop", studio.isLooping() ? "LOOP: ON" : "LOOP: OFF", sf::FloatRect({tlCenterX + 210.f, h - 34.f}, {84.f, 24.f}), false, studio.isLooping() });

    if (mShowCreateModal) {
        rebuildModalButtons(studio);
    }
}

void AnimatorUI::openCreateModal(AnimatorStudio& studio) {
    mShowCreateModal = true;
    mNewClipName = "";
    mSelectedBaseClipIdx = static_cast<int>(studio.getActiveClipIndex());
    mNewClipIncludeWeapon = false; // Default clean animation without unwanted sword
    const auto& clips = studio.getAvailableClips();
    if (mSelectedBaseClipIdx >= 0 && mSelectedBaseClipIdx < static_cast<int>(clips.size())) {
        mNewClipIsTwoHanded = clips[mSelectedBaseClipIdx].isTwoHanded;
    } else {
        mNewClipIsTwoHanded = false;
    }
}

bool AnimatorUI::handleTextInput(char32_t unicode) {
    if (!mShowCreateModal) return false;
    if (unicode >= 32 && unicode != 127 && mNewClipName.size() < 28) {
        char c = static_cast<char>(unicode);
        if (std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '_' || c == '-') {
            mNewClipName += c;
            return true;
        }
    }
    return false;
}

bool AnimatorUI::handleModalKeyPress(sf::Keyboard::Key key, bool isCtrl, bool isShift, AnimatorStudio& studio) {
    if (mShowDeleteModal) {
        if (key == sf::Keyboard::Key::Escape) {
            closeDeleteModal();
            return true;
        } else if (key == sf::Keyboard::Key::Enter) {
            studio.deleteCurrentClip();
            closeDeleteModal();
            return true;
        }
        return true;
    }

    if (!mShowCreateModal) return false;

    if (key == sf::Keyboard::Key::Escape) {
        closeCreateModal();
        return true;
    } else if (key == sf::Keyboard::Key::Enter) {
        if (!mNewClipName.empty()) {
            if (studio.createNewClip(mNewClipName, mSelectedBaseClipIdx, mNewClipIsTwoHanded, mNewClipIncludeWeapon)) {
                closeCreateModal();
            }
        }
        return true;
    } else if (key == sf::Keyboard::Key::Backspace) {
        if (!mNewClipName.empty()) {
            if (isCtrl) {
                mNewClipName.clear();
            } else {
                mNewClipName.pop_back();
            }
        }
        return true;
    } else if (key == sf::Keyboard::Key::Left) {
        const auto& clips = studio.getAvailableClips();
        mSelectedBaseClipIdx--;
        if (mSelectedBaseClipIdx < -1) mSelectedBaseClipIdx = static_cast<int>(clips.size()) - 1;
        if (mSelectedBaseClipIdx >= 0 && mSelectedBaseClipIdx < static_cast<int>(clips.size())) {
            mNewClipIsTwoHanded = clips[mSelectedBaseClipIdx].isTwoHanded;
        }
        return true;
    } else if (key == sf::Keyboard::Key::Right) {
        const auto& clips = studio.getAvailableClips();
        mSelectedBaseClipIdx++;
        if (mSelectedBaseClipIdx >= static_cast<int>(clips.size())) mSelectedBaseClipIdx = -1;
        if (mSelectedBaseClipIdx >= 0 && mSelectedBaseClipIdx < static_cast<int>(clips.size())) {
            mNewClipIsTwoHanded = clips[mSelectedBaseClipIdx].isTwoHanded;
        }
        return true;
    }
    return true;
}

void AnimatorUI::rebuildModalButtons(AnimatorStudio& studio) {
    mModalButtons.clear();
    if (!mShowCreateModal) return;

    float w = cfg::UI::LOGICAL_WIDTH;
    float h = cfg::UI::LOGICAL_HEIGHT;
    float mw = 490.f;
    float mh = 330.f;
    float mx = std::round((w - mw) * 0.5f);
    float my = std::round((h - mh) * 0.5f);

    const auto& clips = studio.getAvailableClips();
    std::string baseName = "Ninguno (En Blanco)";
    if (mSelectedBaseClipIdx >= 0 && mSelectedBaseClipIdx < static_cast<int>(clips.size())) {
        baseName = clips[mSelectedBaseClipIdx].displayName + (clips[mSelectedBaseClipIdx].isTwoHanded ? " [2H]" : "");
    }

    // Base Selector Buttons
    UIButton bPrev;
    bPrev.id = "btn_base_prev";
    bPrev.text = "<";
    bPrev.bounds = sf::FloatRect({mx + 20.f, my + 130.f}, {30.f, 26.f});
    mModalButtons.push_back(bPrev);

    UIButton bCycle;
    bCycle.id = "btn_base_cycle";
    bCycle.text = baseName;
    bCycle.bounds = sf::FloatRect({mx + 55.f, my + 130.f}, {380.f, 26.f});
    bCycle.textColor = sf::Color(249, 194, 43);
    mModalButtons.push_back(bCycle);

    UIButton bNext;
    bNext.id = "btn_base_next";
    bNext.text = ">";
    bNext.bounds = sf::FloatRect({mx + 440.f, my + 130.f}, {30.f, 26.f});
    mModalButtons.push_back(bNext);

    // 2-Handed Toggle
    UIButton b2h;
    b2h.id = "btn_modal_2h";
    b2h.text = mNewClipIsTwoHanded ? "[X] Animacion para Arma a 2 Manos (2H)" : "[ ] Animacion Normal (1 Mano / Escudo / Dual)";
    b2h.bounds = sf::FloatRect({mx + 20.f, my + 168.f}, {450.f, 24.f});
    b2h.textColor = mNewClipIsTwoHanded ? sf::Color(249, 194, 43) : sf::Color::White;
    mModalButtons.push_back(b2h);

    // Include Weapon Toggle
    UIButton bWep;
    bWep.id = "btn_modal_wep";
    bWep.text = mNewClipIncludeWeapon ? "[X] Incluir Pistas de Arma en la Animacion" : "[ ] Sin Arma (Animacion Limpia sin Espada)";
    bWep.bounds = sf::FloatRect({mx + 20.f, my + 198.f}, {450.f, 24.f});
    bWep.textColor = mNewClipIncludeWeapon ? sf::Color(249, 194, 43) : sf::Color(180, 210, 240);
    bWep.baseColor = mNewClipIncludeWeapon ? sf::Color(50, 51, 83, 240) : sf::Color(35, 45, 65, 240);
    mModalButtons.push_back(bWep);

    // Action Buttons
    UIButton bCreate;
    bCreate.id = "btn_modal_create";
    bCreate.text = "CREAR Y EDITAR";
    bCreate.bounds = sf::FloatRect({mx + 20.f, my + 245.f}, {218.f, 36.f});
    bCreate.baseColor = sf::Color(35, 90, 55, 240);
    bCreate.hoverColor = sf::Color(45, 130, 80, 255);
    bCreate.textColor = sf::Color(140, 255, 170);
    mModalButtons.push_back(bCreate);

    UIButton bCancel;
    bCancel.id = "btn_modal_cancel";
    bCancel.text = "CANCELAR (ESC)";
    bCancel.bounds = sf::FloatRect({mx + 252.f, my + 245.f}, {218.f, 36.f});
    bCancel.baseColor = sf::Color(80, 40, 45, 240);
    bCancel.hoverColor = sf::Color(120, 50, 60, 255);
    bCancel.textColor = sf::Color(255, 170, 180);
    mModalButtons.push_back(bCancel);
}

void AnimatorUI::rebuildDeleteModalButtons(AnimatorStudio& studio) {
    mDeleteModalButtons.clear();
    if (!mShowDeleteModal) return;

    float w = cfg::UI::LOGICAL_WIDTH;
    float h = cfg::UI::LOGICAL_HEIGHT;
    float mw = 440.f;
    float mh = 200.f;
    float mx = std::round((w - mw) * 0.5f);
    float my = std::round((h - mh) * 0.5f);

    UIButton bConfirm;
    bConfirm.id = "btn_delete_confirm";
    bConfirm.text = "SI, ELIMINAR ARCHIVO";
    bConfirm.bounds = sf::FloatRect({mx + 20.f, my + 135.f}, {190.f, 36.f});
    bConfirm.baseColor = sf::Color(140, 30, 40, 240);
    bConfirm.hoverColor = sf::Color(190, 40, 50, 255);
    bConfirm.textColor = sf::Color(255, 220, 220);
    mDeleteModalButtons.push_back(bConfirm);

    UIButton bCancel;
    bCancel.id = "btn_delete_cancel";
    bCancel.text = "CANCELAR (ESC)";
    bCancel.bounds = sf::FloatRect({mx + 230.f, my + 135.f}, {190.f, 36.f});
    bCancel.baseColor = sf::Color(55, 60, 80, 240);
    bCancel.hoverColor = sf::Color(75, 85, 115, 255);
    bCancel.textColor = sf::Color(200, 210, 230);
    mDeleteModalButtons.push_back(bCancel);
}

void AnimatorUI::update(AnimatorStudio& studio, sf::Vector2f mousePosUi, bool isMouseDown) {
    mCursorBlinkTimer += 0.016f;
    rebuildButtons(studio);

    auto updateBtnList = [&](std::vector<UIButton>& list) {
        for (auto& b : list) {
            b.isHovered = b.bounds.contains(mousePosUi);
        }
    };

    if (mShowDeleteModal) {
        rebuildDeleteModalButtons(studio);
        updateBtnList(mDeleteModalButtons);
        mIsHoveringAnyUI = true;
        return;
    }

    if (mShowCreateModal) {
        updateBtnList(mModalButtons);
        mIsHoveringAnyUI = true;
        return;
    }

    updateBtnList(mHeaderButtons);
    updateBtnList(mTimelineButtons);
    updateBtnList(mClipButtons);
    updateBtnList(mWeaponButtons);
    updateBtnList(mToggleButtons);
    updateBtnList(mBoneButtons);
    updateBtnList(mOffsetButtons);
    updateBtnList(mKeyframeButtons);
    updateBtnList(mRotationButtons);
    updateBtnList(mNudgeButtons);
    updateBtnList(mLayerButtons);

    if (mIsDraggingTimeline && isMouseDown) {
        float relX = mousePosUi.x - mTimelineBarRect.position.x;
        float factor = std::clamp(relX / mTimelineBarRect.size.x, 0.f, 1.f);
        float rawTime = factor * studio.getDuration();
        float snapDist = (10.f / mTimelineBarRect.size.x) * studio.getDuration();
        float snappedTime = studio.findNearestKeyframeTime(rawTime, snapDist);
        studio.setTime(snappedTime);
    }

    mIsHoveringAnyUI = false;
    auto checkHover = [&](const std::vector<UIButton>& list) {
        for (const auto& b : list) {
            if (b.isHovered) mIsHoveringAnyUI = true;
        }
    };
    checkHover(mHeaderButtons);
    checkHover(mTimelineButtons);
    checkHover(mClipButtons);
    checkHover(mWeaponButtons);
    checkHover(mToggleButtons);
    checkHover(mBoneButtons);
    checkHover(mOffsetButtons);
    checkHover(mKeyframeButtons);
    checkHover(mRotationButtons);
    checkHover(mNudgeButtons);
    checkHover(mLayerButtons);
    if (mTimelineBarRect.contains(mousePosUi)) mIsHoveringAnyUI = true;
}

bool AnimatorUI::handleMousePress(sf::Vector2f mousePosUi, sf::Mouse::Button button, AnimatorStudio& studio) {
    if (button != sf::Mouse::Button::Left) return false;

    // If delete modal is open, modal captures all mouse input
    if (mShowDeleteModal) {
        for (const auto& b : mDeleteModalButtons) {
            if (b.bounds.contains(mousePosUi)) {
                if (b.id == "btn_delete_confirm") {
                    studio.deleteCurrentClip();
                    closeDeleteModal();
                } else if (b.id == "btn_delete_cancel") {
                    closeDeleteModal();
                }
                return true;
            }
        }
        return true;
    }

    // If modal is open, modal captures all mouse input
    if (mShowCreateModal) {
        const auto& clips = studio.getAvailableClips();
        for (const auto& b : mModalButtons) {
            if (b.bounds.contains(mousePosUi)) {
                if (b.id == "btn_modal_cancel") {
                    closeCreateModal();
                    return true;
                } else if (b.id == "btn_modal_create") {
                    if (!mNewClipName.empty()) {
                        if (studio.createNewClip(mNewClipName, mSelectedBaseClipIdx, mNewClipIsTwoHanded, mNewClipIncludeWeapon)) {
                            closeCreateModal();
                        }
                    }
                    return true;
                } else if (b.id == "btn_base_prev") {
                    mSelectedBaseClipIdx--;
                    if (mSelectedBaseClipIdx < -1) mSelectedBaseClipIdx = static_cast<int>(clips.size()) - 1;
                    if (mSelectedBaseClipIdx >= 0 && mSelectedBaseClipIdx < static_cast<int>(clips.size())) {
                        mNewClipIsTwoHanded = clips[mSelectedBaseClipIdx].isTwoHanded;
                    }
                    return true;
                } else if (b.id == "btn_base_next" || b.id == "btn_base_cycle") {
                    mSelectedBaseClipIdx++;
                    if (mSelectedBaseClipIdx >= static_cast<int>(clips.size())) mSelectedBaseClipIdx = -1;
                    if (mSelectedBaseClipIdx >= 0 && mSelectedBaseClipIdx < static_cast<int>(clips.size())) {
                        mNewClipIsTwoHanded = clips[mSelectedBaseClipIdx].isTwoHanded;
                    }
                    return true;
                } else if (b.id == "btn_modal_2h") {
                    mNewClipIsTwoHanded = !mNewClipIsTwoHanded;
                    return true;
                } else if (b.id == "btn_modal_wep") {
                    mNewClipIncludeWeapon = !mNewClipIncludeWeapon;
                    return true;
                }
            }
        }
        return true; // Absorb click so nothing outside modal triggers
    }

    // 1. Check click on Keyframe Ticks with highest priority (generous 18px hitbox)
    for (size_t i = 0; i < mKeyframeTickRects.size(); ++i) {
        if (mKeyframeTickRects[i].contains(mousePosUi)) {
            studio.selectKeyframe(mKeyframeTickTimes[i]);
            return true;
        }
    }

    // 2. Timeline Drag / Click start with magnetic snap
    if (mTimelineBarRect.contains(mousePosUi) || mTimelineHandleRect.contains(mousePosUi)) {
        mIsDraggingTimeline = true;
        float relX = mousePosUi.x - mTimelineBarRect.position.x;
        float factor = std::clamp(relX / mTimelineBarRect.size.x, 0.f, 1.f);
        float rawTime = factor * studio.getDuration();
        float snapDist = (12.f / mTimelineBarRect.size.x) * studio.getDuration();
        float snappedTime = studio.findNearestKeyframeTime(rawTime, snapDist);
        if (std::abs(snappedTime - rawTime) > 0.001f) {
            studio.selectKeyframe(snappedTime);
        } else {
            studio.setTime(rawTime);
        }
        return true;
    }

    // Header buttons
    for (const auto& b : mHeaderButtons) {
        if (b.bounds.contains(mousePosUi)) {
            if (b.id == "btn_menu" && mOnExitCallback) mOnExitCallback();
            else if (b.id == "btn_new_anim") openCreateModal(studio);
            else if (b.id == "btn_flip_view") studio.toggleFacingDir();
            else if (b.id == "btn_save") studio.saveCurrentClip();
            else if (b.id == "btn_reload") studio.reloadAllClips();
            return true;
        }
    }

    // Keyframe & Loop buttons
    for (const auto& b : mKeyframeButtons) {
        if (b.bounds.contains(mousePosUi)) {
            if (b.id == "btn_prev_key") studio.jumpToPrevKeyframe();
            else if (b.id == "btn_next_key") studio.jumpToNextKeyframe();
            else if (b.id == "btn_add_key") studio.insertKeyframeAtCurrentTime(studio.getSelectedNode());
            else if (b.id == "btn_del_key") studio.removeKeyframeAtCurrentTime(studio.getSelectedNode());
            else if (b.id == "btn_loop_start") studio.setLoopStartAtCurrentTime();
            else if (b.id == "btn_loop_end") studio.setLoopEndAtCurrentTime();
            else if (b.id == "btn_loop_reset") studio.resetLoopRange();
            return true;
        }
    }

    // Rotation buttons
    for (const auto& b : mRotationButtons) {
        if (b.bounds.contains(mousePosUi)) {
            if (b.id == "rot_sub45") studio.rotateSelectedNode(-45.f);
            else if (b.id == "rot_sub15") studio.rotateSelectedNode(-15.f);
            else if (b.id == "rot_add15") studio.rotateSelectedNode(15.f);
            else if (b.id == "rot_add45") studio.rotateSelectedNode(45.f);
            else if (b.id == "rot_set0") studio.setNodeAbsoluteRotation(0.f);
            else if (b.id == "rot_set90") studio.setNodeAbsoluteRotation(90.f);
            else if (b.id == "rot_set180") studio.setNodeAbsoluteRotation(180.f);
            else if (b.id == "rot_set270") studio.setNodeAbsoluteRotation(-90.f);
            return true;
        }
    }

    // Nudge buttons
    for (const auto& b : mNudgeButtons) {
        if (b.bounds.contains(mousePosUi)) {
            if (b.id == "nudge_left") studio.moveSelectedNode({ -2.f, 0.f });
            else if (b.id == "nudge_right") studio.moveSelectedNode({ 2.f, 0.f });
            else if (b.id == "nudge_up") studio.moveSelectedNode({ 0.f, -2.f });
            else if (b.id == "nudge_down") studio.moveSelectedNode({ 0.f, 2.f });
            return true;
        }
    }

    // Timeline buttons
    for (const auto& b : mTimelineButtons) {
        if (b.bounds.contains(mousePosUi)) {
            if (b.id == "btn_play") studio.togglePlay();
            else if (b.id == "btn_step_back") studio.stepBackward();
            else if (b.id == "btn_step_fwd") studio.stepForward();
            else if (b.id == "spd_0.25") studio.setSpeedMultiplier(0.25f);
            else if (b.id == "spd_0.5") studio.setSpeedMultiplier(0.5f);
            else if (b.id == "spd_1.0") studio.setSpeedMultiplier(1.0f);
            else if (b.id == "spd_2.0") studio.setSpeedMultiplier(2.0f);
            else if (b.id == "btn_loop") studio.toggleLoop();
            else if (b.id == "btn_flip") studio.toggleFacingDir();
            return true;
        }
    }

    // Clip buttons
    for (const auto& b : mClipButtons) {
        if (b.bounds.contains(mousePosUi)) {
            if (b.id == "btn_delete_clip") {
                openDeleteModal();
            } else if (b.id.rfind("clip_", 0) == 0) {
                size_t idx = std::stoul(b.id.substr(5));
                studio.selectClip(idx);
            }
            return true;
        }
    }

    // Weapon buttons
    for (const auto& b : mWeaponButtons) {
        if (b.bounds.contains(mousePosUi)) {
            if (b.id == "wep_none") studio.setWeaponType(StudioWeaponType::None);
            else if (b.id == "wep_1h") studio.setWeaponType(StudioWeaponType::OneHandedSword);
            else if (b.id == "wep_2h") studio.setWeaponType(StudioWeaponType::TwoHandedSword);
            else if (b.id == "wep_dual") studio.setWeaponType(StudioWeaponType::DualWield);
            else if (b.id == "wep_shield") studio.setWeaponType(StudioWeaponType::SwordAndShield);
            else if (b.id == "wep_shield_main") studio.setWeaponType(StudioWeaponType::ShieldAndSword);
            else if (b.id == "wep_shield_only_r") studio.setWeaponType(StudioWeaponType::ShieldRightOnly);
            else if (b.id == "wep_shield_only_l") studio.setWeaponType(StudioWeaponType::ShieldLeftOnly);
            return true;
        }
    }

    // Diagnostic Toggles
    for (const auto& b : mToggleButtons) {
        if (b.bounds.contains(mousePosUi)) {
            if (b.id == "toggle_ik") studio.toggleIK();
            else if (b.id == "toggle_curves") studio.toggleSpeedCurves();
            else if (b.id == "toggle_grip") studio.toggleGripIK();
            else if (b.id == "toggle_bones") studio.toggleShowBones();
            else if (b.id == "toggle_trail") studio.toggleShowMotionTrail();
            return true;
        }
    }

    // Bone selection
    for (const auto& b : mBoneButtons) {
        if (b.bounds.contains(mousePosUi)) {
            if (b.id == "btn_clear_bone_tracks") {
                studio.clearNodeTracks(studio.getSelectedNode());
            } else if (b.id == "bone_weapon") {
                studio.setSelectedNode("weapon");
            } else if (b.id.rfind("bone_", 0) == 0) {
                studio.setSelectedNode(b.id.substr(5)); // Strip "bone_"
            }
            return true;
        }
    }

    // Offset adjustments
    for (const auto& b : mOffsetButtons) {
        if (b.bounds.contains(mousePosUi)) {
            bool is2H = (studio.getWeaponType() == StudioWeaponType::TwoHandedSword);
            if (b.id == "off_x_sub") is2H ? studio.adjustWeapon2HOffset({ -1.f, 0.f }) : studio.adjustWeaponOffset({ -1.f, 0.f });
            else if (b.id == "off_x_add") is2H ? studio.adjustWeapon2HOffset({ 1.f, 0.f }) : studio.adjustWeaponOffset({ 1.f, 0.f });
            else if (b.id == "off_y_sub") is2H ? studio.adjustWeapon2HOffset({ 0.f, -1.f }) : studio.adjustWeaponOffset({ 0.f, -1.f });
            else if (b.id == "off_y_add") is2H ? studio.adjustWeapon2HOffset({ 0.f, 1.f }) : studio.adjustWeaponOffset({ 0.f, 1.f });
            return true;
        }
    }

    // Layer / Z-Order adjustments
    for (const auto& b : mLayerButtons) {
        if (b.bounds.contains(mousePosUi)) {
            if (b.id == "btn_layer_up") studio.moveSelectedNodeLayerUp();
            else if (b.id == "btn_layer_down") studio.moveSelectedNodeLayerDown();
            else if (b.id == "btn_layer_reset") studio.resetLayerOrder();
            return true;
        }
    }

    return false;
}

bool AnimatorUI::handleMouseRelease(sf::Vector2f mousePosUi, sf::Mouse::Button button, AnimatorStudio& studio) {
    if (button == sf::Mouse::Button::Left) {
        mIsDraggingTimeline = false;
    }
    return false;
}

void AnimatorUI::handleMouseMove(sf::Vector2f mousePosUi, AnimatorStudio& studio) {
}

void AnimatorUI::draw(sf::RenderTarget& target, AnimatorStudio& studio) {
    float w = cfg::UI::LOGICAL_WIDTH;
    float h = cfg::UI::LOGICAL_HEIGHT;

    // 1. Header Bar Background
    sf::RectangleShape headerBg({ w, 44.f });
    headerBg.setFillColor(sf::Color(30, 31, 50, 245));
    headerBg.setOutlineColor(sf::Color(72, 74, 119, 200));
    headerBg.setOutlineThickness(1.f);
    target.draw(headerBg);

    // Title left-aligned cleanly beside menu button
    drawText(target, "RPG ARENA - ANIMATOR STUDIO", { 190.f, 15.f }, 1.15f, sf::Color(249, 194, 43), false);

    for (const auto& b : mHeaderButtons) drawButton(target, b);

    // 2. Left Panel Background (Clips & Weapons)
    sf::RectangleShape leftBg({ 230.f, h - 145.f });
    leftBg.setPosition({ 8.f, 50.f });
    leftBg.setFillColor(sf::Color(30, 31, 50, 210));
    leftBg.setOutlineColor(sf::Color(72, 74, 119, 150));
    leftBg.setOutlineThickness(1.f);
    target.draw(leftBg);

    drawText(target, "ANIMACIONES (CLIPS)", { 16.f, 54.f }, 1.0f, sf::Color(249, 194, 43));
    for (const auto& b : mClipButtons) drawButton(target, b);

    drawText(target, "ARMAS VISUALES", { 16.f, mLeftWepTitleY }, 1.0f, sf::Color(249, 194, 43));
    for (const auto& b : mWeaponButtons) drawButton(target, b);

    // 3. Right Panel Background (Toggles & Inspector)
    sf::RectangleShape rightBg({ 240.f, h - 145.f });
    rightBg.setPosition({ w - 248.f, 50.f });
    rightBg.setFillColor(sf::Color(30, 31, 50, 210));
    rightBg.setOutlineColor(sf::Color(72, 74, 119, 150));
    rightBg.setOutlineThickness(1.f);
    target.draw(rightBg);

    float rightX = w - 238.f;

    // Diagnóstico & IK
    drawText(target, "DIAGNOSTICO & IK", { rightX, mRightTogglesTitleY }, 1.0f, sf::Color(249, 194, 43));
    for (const auto& b : mToggleButtons) drawButton(target, b);

    // Inspector Info Box
    std::string selNode = studio.getSelectedNode();
    sf::Vector2f nodePos = studio.getAnimation().getNodePosition(selNode);
    float nodeRot = studio.getAnimation().getNodeRotation(selNode);

    sf::RectangleShape inspBg({ 220.f, 42.f });
    inspBg.setPosition({ rightX, mRightInspectorBoxY });
    inspBg.setFillColor(sf::Color(20, 21, 35, 230));
    inspBg.setOutlineColor(sf::Color(72, 74, 119, 180));
    inspBg.setOutlineThickness(1.f);
    target.draw(inspBg);

    float exactKfTime = 0.f;
    bool isAtKf = studio.isAtKeyframe(selNode, &exactKfTime);

    std::ostringstream ss1;
    ss1 << std::fixed << std::setprecision(1);
    ss1 << "Pieza: " << selNode << " | Pos: (" << nodePos.x << ", " << nodePos.y << ")";
    drawText(target, ss1.str(), { rightX + 6.f, mRightInspectorBoxY + 5.f }, 0.85f, sf::Color(200, 220, 255));

    std::ostringstream ss2;
    ss2 << std::fixed << std::setprecision(1);
    ss2 << "Rot: " << nodeRot << " deg  ";
    if (isAtKf) {
        ss2 << "[KEYFRAME t=" << std::setprecision(2) << exactKfTime << "s]";
    } else {
        ss2 << "[t=" << std::setprecision(2) << studio.getTime() << "s Interm.]";
    }
    drawText(target, ss2.str(), { rightX + 6.f, mRightInspectorBoxY + 22.f }, 0.85f, isAtKf ? sf::Color(0, 240, 255) : sf::Color(160, 180, 210));

    // Bone Selector
    drawText(target, "PIEZAS DEL ESQUELETO", { rightX, mRightBonesTitleY }, 1.0f, sf::Color(249, 194, 43));
    for (const auto& b : mBoneButtons) drawButton(target, b);

    // Quick Transform
    drawText(target, "TRANSFORMACION RAPIDA", { rightX, mRightRotTitleY }, 1.0f, sf::Color(249, 194, 43));
    for (const auto& b : mRotationButtons) drawButton(target, b);
    for (const auto& b : mNudgeButtons) drawButton(target, b);
    for (const auto& b : mOffsetButtons) drawButton(target, b);

    // Layers
    drawText(target, "ORDEN DE CAPAS (2.5D)", { rightX, mRightLayerTitleY }, 1.0f, sf::Color(249, 194, 43));
    for (const auto& b : mLayerButtons) drawButton(target, b);

    // Visual layer stack representation
    const auto& order = studio.getLayerOrder();
    if (!order.empty()) {
        std::string stackStr = "Fondo: ";
        for (size_t i = 0; i < order.size(); ++i) {
            stackStr += (order[i] == selNode ? ("[" + order[i] + "]") : order[i]);
            if (i + 1 < order.size()) stackStr += " < ";
        }
        stackStr += " :Frente";
        drawText(target, stackStr, { rightX, mRightLayerStackY }, 0.72f, sf::Color(170, 205, 240));
    }

    // 4. Bottom Timeline Background
    sf::RectangleShape tlBg({ w, 90.f });
    tlBg.setPosition({ 0.f, h - 90.f });
    tlBg.setFillColor(sf::Color(25, 26, 42, 250));
    tlBg.setOutlineColor(sf::Color(72, 74, 119, 200));
    tlBg.setOutlineThickness(1.f);
    target.draw(tlBg);

    // Row 1: Status Message (Top of bottom bar)
    drawText(target, studio.getStatusMessage(), { 20.f, h - 84.f }, 0.9f, sf::Color(100, 220, 255), false);

    // Row 2: Timeline Track
    sf::RectangleShape tlTrack({ mTimelineBarRect.size.x, mTimelineBarRect.size.y });
    tlTrack.setPosition({ mTimelineBarRect.position.x, mTimelineBarRect.position.y });
    tlTrack.setFillColor(sf::Color(40, 42, 65));
    tlTrack.setOutlineColor(sf::Color(100, 110, 150));
    tlTrack.setOutlineThickness(1.f);
    target.draw(tlTrack);

    // Loop Range Highlight Region on Timeline Track
    float dur = studio.getDuration();
    float lStart = studio.getLoopStart();
    float lEnd = studio.getLoopEnd();
    if (dur > 0.f && lEnd > lStart) {
        float startX = mTimelineBarRect.position.x + (lStart / dur) * mTimelineBarRect.size.x;
        float endX = mTimelineBarRect.position.x + (lEnd / dur) * mTimelineBarRect.size.x;
        
        sf::RectangleShape loopZone({ endX - startX, mTimelineBarRect.size.y });
        loopZone.setPosition({ startX, mTimelineBarRect.position.y });
        loopZone.setFillColor(sf::Color(34, 197, 94, 70)); // Translucent emerald
        loopZone.setOutlineColor(sf::Color(34, 197, 94, 200));
        loopZone.setOutlineThickness(1.f);
        target.draw(loopZone);

        // Loop Start Indicator (Green Bracket |)
        sf::Vertex bStart[] = {
            sf::Vertex{ { startX, mTimelineBarRect.position.y - 5.f }, sf::Color(34, 230, 110, 255) },
            sf::Vertex{ { startX, mTimelineBarRect.position.y + mTimelineBarRect.size.y + 5.f }, sf::Color(34, 230, 110, 255) }
        };
        target.draw(bStart, 2, sf::PrimitiveType::Lines);

        // Loop End Indicator (Red Bracket |)
        sf::Vertex bEnd[] = {
            sf::Vertex{ { endX, mTimelineBarRect.position.y - 5.f }, sf::Color(255, 100, 100, 255) },
            sf::Vertex{ { endX, mTimelineBarRect.position.y + mTimelineBarRect.size.y + 5.f }, sf::Color(255, 100, 100, 255) }
        };
        target.draw(bEnd, 2, sf::PrimitiveType::Lines);
    }

    // Timeline Filled Progress
    float progress = (dur > 0.f) ? std::clamp(studio.getTime() / dur, 0.f, 1.f) : 0.f;
    sf::RectangleShape tlFill({ mTimelineBarRect.size.x * progress, mTimelineBarRect.size.y });
    tlFill.setPosition({ mTimelineBarRect.position.x, mTimelineBarRect.position.y });
    tlFill.setFillColor(sf::Color(249, 194, 43, 160));
    target.draw(tlFill);

    // Keyframe Ticks (Diamonds)
    float curTime = studio.getTime();
    for (size_t i = 0; i < mKeyframeTickRects.size(); ++i) {
        float kt = mKeyframeTickTimes[i];
        bool isSelected = std::abs(curTime - kt) <= 0.015f;

        float diamondRadius = isSelected ? 6.f : 4.f;
        sf::CircleShape tick(diamondRadius, 4);
        tick.setOrigin({ diamondRadius, diamondRadius });
        float centerX = mKeyframeTickRects[i].position.x + mKeyframeTickRects[i].size.x * 0.5f;
        float centerY = mTimelineBarRect.position.y + mTimelineBarRect.size.y * 0.5f;
        tick.setPosition({ centerX, centerY });

        if (isSelected) {
            tick.setFillColor(sf::Color(0, 240, 255));
            tick.setOutlineColor(sf::Color::White);
            tick.setOutlineThickness(1.5f);

            // Vertical indicator needle above the timeline
            sf::Vertex needle[] = {
                sf::Vertex{ { centerX, centerY - 14.f }, sf::Color(0, 240, 255, 220) },
                sf::Vertex{ { centerX, centerY }, sf::Color(0, 240, 255, 220) }
            };
            target.draw(needle, 2, sf::PrimitiveType::Lines);
        } else {
            tick.setFillColor(sf::Color(249, 194, 43));
            tick.setOutlineColor(sf::Color(30, 31, 50));
            tick.setOutlineThickness(1.f);
        }
        target.draw(tick);
    }

    // Timeline Handle
    sf::RectangleShape tlHandle({ mTimelineHandleRect.size.x, mTimelineHandleRect.size.y });
    tlHandle.setPosition({ mTimelineHandleRect.position.x, mTimelineHandleRect.position.y });
    tlHandle.setFillColor(sf::Color::White);
    tlHandle.setOutlineColor(sf::Color(249, 194, 43));
    tlHandle.setOutlineThickness(1.5f);
    target.draw(tlHandle);

    // Time Label & Loop Info
    std::ostringstream timeSs;
    timeSs << std::fixed << std::setprecision(2) << studio.getTime() << "s / " << dur << "s";
    if (lStart > 0.001f || (lEnd < dur - 0.001f && lEnd > 0.f)) {
        timeSs << "  [Loop: " << lStart << "s - " << lEnd << "s]";
    }
    drawText(target, timeSs.str(), { mTimelineBarRect.position.x + 2.f, mTimelineBarRect.position.y - 13.f }, 0.82f, sf::Color(249, 194, 43), false);

    // Buttons
    for (const auto& b : mKeyframeButtons) drawButton(target, b);
    for (const auto& b : mTimelineButtons) drawButton(target, b);

    // 5. Creation / Delete Modals (if active)
    if (mShowCreateModal) {
        drawCreateModal(target, studio);
    } else if (mShowDeleteModal) {
        drawDeleteModal(target, studio);
    }
}

void AnimatorUI::drawCreateModal(sf::RenderTarget& target, AnimatorStudio& studio) {
    float w = cfg::UI::LOGICAL_WIDTH;
    float h = cfg::UI::LOGICAL_HEIGHT;

    // 1. Dark Backdrop
    sf::RectangleShape backdrop({ w, h });
    backdrop.setFillColor(sf::Color(10, 12, 20, 215));
    target.draw(backdrop);

    // 2. Modal Box
    float mw = 490.f;
    float mh = 330.f;
    float mx = std::round((w - mw) * 0.5f);
    float my = std::round((h - mh) * 0.5f);

    sf::RectangleShape box({ mw, mh });
    box.setPosition({ mx, my });
    box.setFillColor(sf::Color(24, 25, 38, 250));
    box.setOutlineColor(sf::Color(249, 194, 43, 230));
    box.setOutlineThickness(2.f);
    target.draw(box);

    // 3. Header title
    drawText(target, "--- CREAR NUEVA ANIMACION (JSON) ---", { mx + mw * 0.5f, my + 18.f }, 1.15f, sf::Color(249, 194, 43), true);

    // 4. Input Box Label
    drawText(target, "Nombre de la animacion / Archivo JSON:", { mx + 20.f, my + 44.f }, 0.95f, sf::Color(180, 190, 220), false);

    // Input Box Rectangle
    sf::RectangleShape inputRect({ 450.f, 30.f });
    inputRect.setPosition({ mx + 20.f, my + 60.f });
    inputRect.setFillColor(sf::Color(14, 15, 24));
    inputRect.setOutlineColor(sf::Color(72, 74, 119));
    inputRect.setOutlineThickness(1.5f);
    target.draw(inputRect);

    // Input Text or Placeholder
    if (mNewClipName.empty()) {
        drawText(target, "Escribe aqui (ej: bloqueo, guard, salto)...", { mx + 30.f, my + 69.f }, 1.0f, sf::Color(100, 105, 130), false);
    } else {
        bool blink = (static_cast<int>(mCursorBlinkTimer * 2.5f) % 2 == 0);
        drawText(target, mNewClipName + (blink ? "_" : ""), { mx + 30.f, my + 69.f }, 1.05f, sf::Color::White, false);
    }

    // Path preview note
    std::string safePreview = mNewClipName;
    for (char& c : safePreview) {
        if (c == ' ' || c == '-' || c == '.') c = '_';
        else c = std::tolower(c);
    }
    if (safePreview.empty()) safePreview = "<nombre>";
    std::string dir = (studio.getEntityType() == "player") ? "assets/textures/player/" : "assets/textures/mobs/" + studio.getEntityType() + "/";
    drawText(target, "Archivo: " + dir + safePreview + ".json", { mx + 20.f, my + 94.f }, 0.85f, sf::Color(130, 140, 170), false);

    // 5. Base Animation Label
    drawText(target, "Tomar como base de (copia keyframes existentes):", { mx + 20.f, my + 114.f }, 0.95f, sf::Color(180, 190, 220), false);

    // 6. Draw Modal Buttons
    for (const auto& b : mModalButtons) {
        drawButton(target, b);
    }

    // 7. Hint at bottom
    drawText(target, "[ENTER] Crear y Abrir   [ESC] Cancelar", { mx + mw * 0.5f, my + mh - 14.f }, 0.85f, sf::Color(130, 140, 160), true);
}

void AnimatorUI::drawDeleteModal(sf::RenderTarget& target, AnimatorStudio& studio) {
    float w = cfg::UI::LOGICAL_WIDTH;
    float h = cfg::UI::LOGICAL_HEIGHT;

    // 1. Dark Backdrop
    sf::RectangleShape backdrop({ w, h });
    backdrop.setFillColor(sf::Color(10, 12, 20, 225));
    target.draw(backdrop);

    // 2. Modal Box
    float mw = 440.f;
    float mh = 200.f;
    float mx = std::round((w - mw) * 0.5f);
    float my = std::round((h - mh) * 0.5f);

    sf::RectangleShape box({ mw, mh });
    box.setPosition({ mx, my });
    box.setFillColor(sf::Color(28, 18, 22, 250));
    box.setOutlineColor(sf::Color(235, 60, 75, 240));
    box.setOutlineThickness(2.f);
    target.draw(box);

    // 3. Header title
    drawText(target, "--- ELIMINAR ANIMACION (JSON) ---", { mx + mw * 0.5f, my + 18.f }, 1.15f, sf::Color(255, 80, 95), true);

    // 4. Clip Name & Warning
    const auto& clips = studio.getAvailableClips();
    std::string clipName = (studio.getActiveClipIndex() < clips.size()) ? clips[studio.getActiveClipIndex()].displayName : "Desconocido";
    std::string clipPath = (studio.getActiveClipIndex() < clips.size()) ? clips[studio.getActiveClipIndex()].filePath : "";

    drawText(target, "¿Estas seguro de eliminar este archivo de disco?", { mx + mw * 0.5f, my + 50.f }, 1.0f, sf::Color::White, true);
    drawText(target, "Clip: " + clipName, { mx + mw * 0.5f, my + 72.f }, 1.05f, sf::Color(249, 194, 43), true);
    drawText(target, "Archivo: " + clipPath, { mx + mw * 0.5f, my + 94.f }, 0.85f, sf::Color(190, 160, 170), true);

    // 5. Draw Buttons
    for (const auto& b : mDeleteModalButtons) {
        drawButton(target, b);
    }
}
