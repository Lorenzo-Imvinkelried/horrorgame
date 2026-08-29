#include "AnimatorState.h"
#include "Config.h"
#include "core/engine/Game.h"
#include "states/MenuState.h"
#include <cmath>
#include <iostream>

AnimatorState::AnimatorState(Game& game)
    : mStudio(game.getResources())
    , mUI(&game.getResources().getTexture("assets/fonts/font.png"))
{
    mUiView.setSize({ cfg::UI::LOGICAL_WIDTH, cfg::UI::LOGICAL_HEIGHT });
    mUiView.setCenter({ cfg::UI::LOGICAL_WIDTH * 0.5f, cfg::UI::LOGICAL_HEIGHT * 0.5f });

    resetCamera();

    mUI.setOnExitCallback([&game]() {
        game.changeState(std::make_unique<MenuState>(game));
    });

    try {
        sf::Texture& tex = game.getResources().getTexture("assets/ui/cursor.png");
        mCursorSprite.emplace(tex);
        mCursorSprite->setOrigin({ cfg::UI::CURSOR_HOTSPOT_X, cfg::UI::CURSOR_HOTSPOT_Y });
        mHasCursor = true;
    } catch (...) {
        mHasCursor = false;
    }
}

void AnimatorState::resetCamera() {
    mZoomLevel = 2.5f;
    mCameraCenter = { 0.f, -10.f };

    float viewW = cfg::UI::LOGICAL_WIDTH / mZoomLevel;
    float viewH = cfg::UI::LOGICAL_HEIGHT / mZoomLevel;
    mWorldView.setSize({ viewW, viewH });
    mWorldView.setCenter(mCameraCenter);
}

void AnimatorState::handleInput(Game& game, sf::Time dt) {
    if (mUI.isModalOpen()) {
        return;
    }
    if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
        game.changeState(std::make_unique<MenuState>(game));
        return;
    }
}

void AnimatorState::update(Game& game, sf::Time dt) {
    if (mHasCursor) {
        game.getWindow().setMouseCursorVisible(false);
    }

    sf::Vector2i pixelMouse = sf::Mouse::getPosition(game.getWindow());
    sf::Vector2f uiMouse = game.getWindow().mapPixelToCoords(pixelMouse, mUiView);
    sf::Vector2f worldMouse = game.getWindow().mapPixelToCoords(pixelMouse, mWorldView);

    bool isMouseDown = sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
    mUI.update(mStudio, uiMouse, isMouseDown);

    if (mUI.isModalOpen()) {
        mDragMode = StudioDragMode::None;
        mStudio.update(dt);
        return;
    }

    // Viewport drag interactions
    if (mDragMode == StudioDragMode::PanCamera) {
        sf::Vector2i mouseDelta = pixelMouse - mPanStartMouse;
        sf::Vector2f worldDelta = {
            (float)mouseDelta.x * (mWorldView.getSize().x / (float)game.getWindow().getSize().x),
            (float)mouseDelta.y * (mWorldView.getSize().y / (float)game.getWindow().getSize().y)
        };
        mCameraCenter = mPanStartCenter - worldDelta;
        mWorldView.setCenter(mCameraCenter);
    } else if (mDragMode == StudioDragMode::MoveBone) {
        sf::Vector2f worldDelta = worldMouse - mLastWorldMouse;
        mStudio.moveSelectedNode(worldDelta);
        mLastWorldMouse = worldMouse;
    } else if (mDragMode == StudioDragMode::RotateBone) {
        std::string selNode = mStudio.getSelectedNode();
        sf::Vector2f boneCenter = mStudio.getAnimation().getNodePosition(selNode);
        float currentAngle = std::atan2(worldMouse.y - boneCenter.y, worldMouse.x - boneCenter.x) * (180.f / 3.14159265f);
        float deltaAngle = currentAngle - mDragStartAngle;

        while (deltaAngle > 180.f) deltaAngle -= 360.f;
        while (deltaAngle < -180.f) deltaAngle += 360.f;

        mStudio.rotateSelectedNode(deltaAngle);
        mDragStartAngle = currentAngle;
    }

    mStudio.update(dt);
}

void AnimatorState::drawWorld(Game& game, sf::RenderTarget& target) {
    sf::View oldView = target.getView();
    target.setView(mWorldView);

    target.clear(sf::Color(26, 27, 40)); // Dark studio canvas

    mStudio.drawWorld(target);
    mStudio.drawOverlays(target, mZoomLevel);

    target.setView(oldView);
}

void AnimatorState::drawUI(Game& game, sf::RenderTarget& target) {
    sf::View oldView = target.getView();
    target.setView(mUiView);

    mUI.draw(target, mStudio);

    if (mHasCursor && mCursorSprite) {
        sf::Vector2i mousePos = sf::Mouse::getPosition(game.getWindow());
        sf::Vector2f uiMousePos = target.mapPixelToCoords(mousePos, mUiView);

        float zoom = cfg::Map::ZOOM_FACTOR;
        mCursorSprite->setScale({ zoom, zoom });
        mCursorSprite->setPosition(uiMousePos);
        target.draw(*mCursorSprite);
    }

    target.setView(oldView);
}

void AnimatorState::draw(Game& game, sf::RenderTarget& target) {
    drawWorld(game, target);
    drawUI(game, target);
}

void AnimatorState::onResize(Game& game, int w, int h) {
    mUiView.setSize({ cfg::UI::LOGICAL_WIDTH, cfg::UI::LOGICAL_HEIGHT });
    mUiView.setCenter({ cfg::UI::LOGICAL_WIDTH * 0.5f, cfg::UI::LOGICAL_HEIGHT * 0.5f });

    float viewW = cfg::UI::LOGICAL_WIDTH / mZoomLevel;
    float viewH = cfg::UI::LOGICAL_HEIGHT / mZoomLevel;
    mWorldView.setSize({ viewW, viewH });
    mWorldView.setCenter(mCameraCenter);
}

void AnimatorState::handleEvent(Game& game, const sf::Event& ev) {
    sf::Vector2i pixelMouse = sf::Mouse::getPosition(game.getWindow());
    sf::Vector2f uiMouse = game.getWindow().mapPixelToCoords(pixelMouse, mUiView);
    sf::Vector2f worldMouse = game.getWindow().mapPixelToCoords(pixelMouse, mWorldView);

    if (const auto* te = ev.getIf<sf::Event::TextEntered>()) {
        if (mUI.handleTextInput(te->unicode)) {
            return;
        }
    }

    if (const auto* mb = ev.getIf<sf::Event::MouseButtonPressed>()) {
        if (mUI.handleMousePress(uiMouse, mb->button, mStudio)) {
            return;
        }

        // Viewport click
        std::string hitNode = mStudio.getHoveredNode(worldMouse);

        if (mb->button == sf::Mouse::Button::Left) {
            if (!hitNode.empty()) {
                mStudio.setSelectedNode(hitNode);
                bool isShift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
                if (isShift) {
                    mDragMode = StudioDragMode::RotateBone;
                    sf::Vector2f boneCenter = mStudio.getAnimation().getNodePosition(hitNode);
                    mDragStartAngle = std::atan2(worldMouse.y - boneCenter.y, worldMouse.x - boneCenter.x) * (180.f / 3.14159265f);
                } else {
                    mDragMode = StudioDragMode::MoveBone;
                    mLastWorldMouse = worldMouse;
                }
            } else {
                mDragMode = StudioDragMode::None;
            }
        } else if (mb->button == sf::Mouse::Button::Right) {
            if (!hitNode.empty()) {
                mStudio.setSelectedNode(hitNode);
                mDragMode = StudioDragMode::RotateBone;
                sf::Vector2f boneCenter = mStudio.getAnimation().getNodePosition(hitNode);
                mDragStartAngle = std::atan2(worldMouse.y - boneCenter.y, worldMouse.x - boneCenter.x) * (180.f / 3.14159265f);
            } else {
                mDragMode = StudioDragMode::PanCamera;
                mPanStartMouse = pixelMouse;
                mPanStartCenter = mCameraCenter;
            }
        } else if (mb->button == sf::Mouse::Button::Middle) {
            mDragMode = StudioDragMode::PanCamera;
            mPanStartMouse = pixelMouse;
            mPanStartCenter = mCameraCenter;
        }
    } else if (const auto* mr = ev.getIf<sf::Event::MouseButtonReleased>()) {
        mUI.handleMouseRelease(uiMouse, mr->button, mStudio);
        mDragMode = StudioDragMode::None;
    } else if (const auto* mw = ev.getIf<sf::Event::MouseWheelScrolled>()) {
        if (!mUI.isInteractingWithUI()) {
            bool isShift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
            if (isShift) {
                // Shift + Wheel = Rotate selected part / weapon
                float rotStep = (mw->delta > 0) ? 5.f : -5.f;
                mStudio.rotateSelectedNode(rotStep);
            } else {
                // Normal Wheel = Zoom
                if (mw->delta > 0) {
                    mZoomLevel = std::min(mZoomLevel * 1.15f, 10.0f);
                } else if (mw->delta < 0) {
                    mZoomLevel = std::max(mZoomLevel * 0.85f, 0.5f);
                }
                float viewW = cfg::UI::LOGICAL_WIDTH / mZoomLevel;
                float viewH = cfg::UI::LOGICAL_HEIGHT / mZoomLevel;
                mWorldView.setSize({ viewW, viewH });
                mWorldView.setCenter(mCameraCenter);
            }
        }
    } else if (const auto* kp = ev.getIf<sf::Event::KeyPressed>()) {
        bool isCtrl = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
        bool isShift = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);

        if (mUI.isModalOpen()) {
            mUI.handleModalKeyPress(kp->code, isCtrl, isShift, mStudio);
            return;
        }

        if (isCtrl && kp->code == sf::Keyboard::Key::S) {
            mStudio.saveCurrentClip();
        } else if (kp->code == sf::Keyboard::Key::Space) {
            mStudio.togglePlay();
        } else if (kp->code == sf::Keyboard::Key::R) {
            resetCamera();
        } else if (kp->code == sf::Keyboard::Key::F5) {
            mStudio.reloadAllClips();
        } else if (kp->code == sf::Keyboard::Key::Left) {
            mStudio.stepBackward();
        } else if (kp->code == sf::Keyboard::Key::Right) {
            mStudio.stepForward();
        } else if (kp->code == sf::Keyboard::Key::LBracket || kp->code == sf::Keyboard::Key::PageUp || kp->code == sf::Keyboard::Key::J) {
            mStudio.jumpToPrevKeyframe();
        } else if (kp->code == sf::Keyboard::Key::RBracket || kp->code == sf::Keyboard::Key::PageDown || kp->code == sf::Keyboard::Key::L) {
            mStudio.jumpToNextKeyframe();
        } else if (kp->code == sf::Keyboard::Key::K) {
            mStudio.insertKeyframeAtCurrentTime(mStudio.getSelectedNode());
        } else if (kp->code == sf::Keyboard::Key::Delete) {
            mStudio.removeKeyframeAtCurrentTime(mStudio.getSelectedNode());
        } else if (kp->code == sf::Keyboard::Key::Q) {
            mStudio.rotateSelectedNode(isShift ? -15.f : -5.f);
        } else if (kp->code == sf::Keyboard::Key::E) {
            mStudio.rotateSelectedNode(isShift ? 15.f : 5.f);
        } else if (kp->code == sf::Keyboard::Key::W) {
            mStudio.moveSelectedNode({ 0.f, -1.f });
        } else if (kp->code == sf::Keyboard::Key::S && !isCtrl) {
            mStudio.moveSelectedNode({ 0.f, 1.f });
        } else if (kp->code == sf::Keyboard::Key::A) {
            mStudio.moveSelectedNode({ -1.f, 0.f });
        } else if (kp->code == sf::Keyboard::Key::D) {
            mStudio.moveSelectedNode({ 1.f, 0.f });
        }
    }
}
