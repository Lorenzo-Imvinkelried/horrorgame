#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <functional>
#include "../core/graphics/BitmapText.h"

class AnimatorStudio;

struct UIButton {
    std::string id;
    std::string text;
    sf::FloatRect bounds;
    bool isHovered = false;
    bool isSelected = false;
    sf::Color baseColor = sf::Color(50, 51, 83, 240);
    sf::Color hoverColor = sf::Color(72, 74, 119, 255);
    sf::Color selectedColor = sf::Color(249, 194, 43, 255);
    sf::Color textColor = sf::Color::White;
};

class AnimatorUI {
public:
    AnimatorUI(sf::Texture* fontTexture);
    ~AnimatorUI() = default;

    void update(AnimatorStudio& studio, sf::Vector2f mousePosUi, bool isMouseDown);
    void draw(sf::RenderTarget& target, AnimatorStudio& studio);

    bool handleMousePress(sf::Vector2f mousePosUi, sf::Mouse::Button button, AnimatorStudio& studio);
    bool handleMouseRelease(sf::Vector2f mousePosUi, sf::Mouse::Button button, AnimatorStudio& studio);
    void handleMouseMove(sf::Vector2f mousePosUi, AnimatorStudio& studio);

    bool isInteractingWithUI() const { return mIsHoveringAnyUI || mIsDraggingTimeline || mShowCreateModal || mShowDeleteModal; }
    bool isModalOpen() const { return mShowCreateModal || mShowDeleteModal; }

    bool handleTextInput(char32_t unicode);
    bool handleModalKeyPress(sf::Keyboard::Key key, bool isCtrl, bool isShift, AnimatorStudio& studio);
    void openCreateModal(AnimatorStudio& studio);
    void closeCreateModal() { mShowCreateModal = false; }

    void setOnExitCallback(std::function<void()> callback) { mOnExitCallback = callback; }

private:
    void rebuildButtons(AnimatorStudio& studio);
    void rebuildModalButtons(AnimatorStudio& studio);
    void drawText(sf::RenderTarget& target, const std::string& text, sf::Vector2f pos, float scale = 1.0f, sf::Color color = sf::Color::White, bool center = false);
    void drawButton(sf::RenderTarget& target, const UIButton& btn);
    void drawCreateModal(sf::RenderTarget& target, AnimatorStudio& studio);
    void openDeleteModal() { mShowDeleteModal = true; }
    void closeDeleteModal() { mShowDeleteModal = false; }
    void drawDeleteModal(sf::RenderTarget& target, AnimatorStudio& studio);
    void rebuildDeleteModalButtons(AnimatorStudio& studio);

private:
    sf::Texture* mFontTexture = nullptr;
    std::function<void()> mOnExitCallback;

    std::vector<UIButton> mHeaderButtons;
    std::vector<UIButton> mTimelineButtons;
    std::vector<UIButton> mClipButtons;
    std::vector<UIButton> mWeaponButtons;
    std::vector<UIButton> mToggleButtons;
    std::vector<UIButton> mBoneButtons;
    std::vector<UIButton> mOffsetButtons;
    std::vector<UIButton> mKeyframeButtons;
    std::vector<UIButton> mRotationButtons;
    std::vector<UIButton> mNudgeButtons;
    std::vector<UIButton> mLayerButtons;
    std::vector<UIButton> mModalButtons;
    std::vector<UIButton> mDeleteModalButtons;

    sf::FloatRect mTimelineBarRect;
    sf::FloatRect mTimelineHandleRect;
    std::vector<sf::FloatRect> mKeyframeTickRects;
    std::vector<float> mKeyframeTickTimes;

    bool mIsDraggingTimeline = false;
    bool mIsHoveringAnyUI = false;

    // Creation Modal State
    bool mShowCreateModal = false;
    std::string mNewClipName = "";
    int mSelectedBaseClipIdx = 0;
    bool mNewClipIsTwoHanded = false;
    bool mNewClipIncludeWeapon = false;
    float mCursorBlinkTimer = 0.f;

    // Delete Modal State
    bool mShowDeleteModal = false;

    // Dynamic Section Layout Positions
    float mLeftWepTitleY = 0.f;
    float mRightTogglesTitleY = 0.f;
    float mRightInspectorBoxY = 0.f;
    float mRightBonesTitleY = 0.f;
    float mRightRotTitleY = 0.f;
    float mRightLayerTitleY = 0.f;
    float mRightLayerStackY = 0.f;

    float mUiScale = 1.0f;
};

