#include "ChatBox.h"
#include "Config.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include "core/graphics/BitmapText.h"

// ──────────────────────────────────────────────
//  Constants & helpers
// ──────────────────────────────────────────────
// BitmapText: GLYPH_WIDTH = 3 px, plus 1 px spacing = 4 px per char (before scale)
static constexpr float GLYPH_LOGICAL_W = 4.f; // px per char at scale 1.0

// Prefix shown before user text
static const std::string INPUT_PREFIX = ": ";

// ──────────────────────────────────────────────
//  Init / Focus
// ──────────────────────────────────────────────
void ChatBox::init(sf::Texture* fontTexture) {
    mFontTexture = fontTexture;
    mBackground.setFillColor(sf::Color(0, 0, 0, 150));
    mBackground.setOutlineColor(sf::Color(50, 50, 50));
    mBackground.setOutlineThickness(1.f);
}

void ChatBox::setBgTexture(sf::Texture* tex) {
    if (tex) {
        mBgSprite.emplace(*tex);
        mBackground.setFillColor(sf::Color::Transparent);
        mBackground.setOutlineColor(sf::Color::Transparent);
    } else {
        mBgSprite.reset();
        mBackground.setFillColor(sf::Color(0, 0, 0, 150));
        mBackground.setOutlineColor(sf::Color(50, 50, 50));
        mBackground.setOutlineThickness(1.f);
    }
}

void ChatBox::setFocus(bool focused) {
    mIsFocused = focused;
    if (!focused) {
        mSelStart   = mSelEnd = mSelAnchor = -1;
        mHeldKey    = sf::Keyboard::Key::Unknown;
        mMouseSelecting = false;
    }
    if (mIsFocused) {
        mIsVisible = true; // Si recibe focus, forzar a visible para evitar congelamiento invisible
        mCursorPos = (int)mInputBuffer.size();
        mSelStart = mSelEnd = mSelAnchor = -1;
        if (!mBgSprite) {
            mBackground.setOutlineColor(sf::Color::Cyan);
            mBackground.setOutlineThickness(2.f);
        }
        mCursorBlinkClock.restart();
    } else {
        if (!mBgSprite) {
            mBackground.setOutlineColor(sf::Color(50, 50, 50));
            mBackground.setOutlineThickness(1.f);
        }
    }
}

// ──────────────────────────────────────────────
//  Text editing helpers
// ──────────────────────────────────────────────
float ChatBox::charWidth() const {
    return GLYPH_LOGICAL_W * mInputScale;
}

float ChatBox::textWidthUpTo(int count) const {
    // The prefix ">" + space is included in the rendered string but cursor is
    // relative to mInputBuffer only. We account for the prefix offset in
    // charIndexAt / draw, so here we just measure raw char count.
    return static_cast<float>(count) * charWidth();
}

int ChatBox::charIndexAt(float relativeX) const {
    // relativeX is relative to the START of the input buffer text (after prefix)
    if (relativeX <= 0.f) return 0;
    int idx = static_cast<int>(relativeX / charWidth() + 0.5f);
    return std::clamp(idx, 0, (int)mInputBuffer.size());
}

void ChatBox::normalizeSelection(int& a, int& b) const {
    if (a > b) std::swap(a, b);
}

bool ChatBox::eraseSelection() {
    if (mSelStart < 0 || mSelStart == mSelEnd) return false;
    int a = mSelStart, b = mSelEnd;
    normalizeSelection(a, b);
    a = std::clamp(a, 0, (int)mInputBuffer.size());
    b = std::clamp(b, 0, (int)mInputBuffer.size());
    mInputBuffer.erase(a, b - a);
    mCursorPos = a;
    mSelStart = mSelEnd = mSelAnchor = -1;
    return true;
}

void ChatBox::deleteWordLeft() {
    if (eraseSelection()) return;
    if (mCursorPos == 0) return;
    // Skip trailing spaces then skip a word
    int pos = mCursorPos;
    while (pos > 0 && mInputBuffer[pos-1] == ' ') pos--;
    while (pos > 0 && mInputBuffer[pos-1] != ' ') pos--;
    mInputBuffer.erase(pos, mCursorPos - pos);
    mCursorPos = pos;
}

void ChatBox::copySelection() {
    if (mSelStart < 0 || mSelStart == mSelEnd) return;
    int a = mSelStart, b = mSelEnd;
    normalizeSelection(a, b);
    a = std::clamp(a, 0, (int)mInputBuffer.size());
    b = std::clamp(b, 0, (int)mInputBuffer.size());
    sf::Clipboard::setString(mInputBuffer.substr(a, b - a));
}

void ChatBox::pasteClipboard() {
    eraseSelection();
    std::string clip = sf::Clipboard::getString();
    // Remove control characters except newlines/returns
    std::string clean;
    clean.reserve(clip.size());
    for (char c : clip) {
        if (c == '\n' || c == '\r' || (c >= 32 && c != 127)) {
            clean += c;
        }
    }
    mInputBuffer.insert(mCursorPos, clean);
    mCursorPos = std::clamp(mCursorPos + (int)clean.size(), 0, (int)mInputBuffer.size());
    mCursorBlinkClock.restart();
}

void ChatBox::moveCursorLeft(bool shift) {
    int newPos = std::max(0, mCursorPos - 1);
    if (shift) {
        if (mSelAnchor < 0) mSelAnchor = mCursorPos; // anchor at old pos
        mCursorPos = newPos;
        mSelStart = std::min(mSelAnchor, mCursorPos);
        mSelEnd   = std::max(mSelAnchor, mCursorPos);
        if (mSelStart == mSelEnd) mSelStart = mSelEnd = mSelAnchor = -1;
    } else {
        // If selection exists, jump to left end
        if (mSelStart >= 0 && mSelStart != mSelEnd) {
            int a = mSelStart, b = mSelEnd;
            normalizeSelection(a, b);
            mCursorPos = a;
        } else {
            mCursorPos = newPos;
        }
        mSelStart = mSelEnd = mSelAnchor = -1;
    }
    mCursorBlinkClock.restart();
}

void ChatBox::moveCursorRight(bool shift) {
    int newPos = std::min((int)mInputBuffer.size(), mCursorPos + 1);
    if (shift) {
        if (mSelAnchor < 0) mSelAnchor = mCursorPos;
        mCursorPos = newPos;
        mSelStart = std::min(mSelAnchor, mCursorPos);
        mSelEnd   = std::max(mSelAnchor, mCursorPos);
        if (mSelStart == mSelEnd) mSelStart = mSelEnd = mSelAnchor = -1;
    } else {
        if (mSelStart >= 0 && mSelStart != mSelEnd) {
            int a = mSelStart, b = mSelEnd;
            normalizeSelection(a, b);
            mCursorPos = b;
        } else {
            mCursorPos = newPos;
        }
        mSelStart = mSelEnd = mSelAnchor = -1;
    }
    mCursorBlinkClock.restart();
}

void ChatBox::selectAll() {
    mSelStart  = 0;
    mSelEnd    = (int)mInputBuffer.size();
    mSelAnchor = 0;
    mCursorPos = mSelEnd;
    mCursorBlinkClock.restart();
}

void ChatBox::insertChar(char c) {
    eraseSelection();
    mInputBuffer.insert(mInputBuffer.begin() + mCursorPos, c);
    mCursorPos++;
    mCursorBlinkClock.restart();
}

void ChatBox::doBackspace() {
    if (eraseSelection()) return;
    if (mCursorPos > 0) {
        mInputBuffer.erase(mCursorPos - 1, 1);
        mCursorPos--;
        mCursorBlinkClock.restart();
    }
}

// ──────────────────────────────────────────────
//  handleEvent
// ──────────────────────────────────────────────
void ChatBox::handleEvent(const sf::Event& event) {
    if (!mIsFocused) return;

    // ── Text Typing ──────────────────────────
    if (const auto* te = event.getIf<sf::Event::TextEntered>()) {
        // Printable including extended ASCII (e.g. Ñ), skip control chars
        if (te->unicode >= 32 && te->unicode != 127) {
            insertChar(static_cast<char>(te->unicode));
        }
        return;
    }

    // ── Key Pressed ──────────────────────────
    if (const auto* kp = event.getIf<sf::Event::KeyPressed>()) {
        bool ctrl  = kp->control;
        bool shift = kp->shift;

        // Start tracking this key for hold-repeat
        // (only repeatable keys)
        switch (kp->code) {
            case sf::Keyboard::Key::Backspace:
            case sf::Keyboard::Key::Delete:
            case sf::Keyboard::Key::Left:
            case sf::Keyboard::Key::Right:
            case sf::Keyboard::Key::Home:
            case sf::Keyboard::Key::End:
                mHeldKey    = kp->code;
                mHeldShift  = shift;
                mHeldTimer  = 0.f;
                mRepeatAccum = 0.f;
                break;
            default:
                mHeldKey = sf::Keyboard::Key::Unknown;
                break;
        }

        // ── Ctrl combos ──
        if (ctrl) {
            if (kp->code == sf::Keyboard::Key::A) {
                selectAll(); return;
            }
            if (kp->code == sf::Keyboard::Key::C) {
                copySelection(); return;
            }
            if (kp->code == sf::Keyboard::Key::X) {
                copySelection(); eraseSelection(); return;
            }
            if (kp->code == sf::Keyboard::Key::V) {
                pasteClipboard(); return;
            }
            if (kp->code == sf::Keyboard::Key::Backspace) {
                deleteWordLeft(); return;
            }
        }

        // ── Movement ──
        if (kp->code == sf::Keyboard::Key::Left) {
            moveCursorLeft(shift); return;
        }
        if (kp->code == sf::Keyboard::Key::Right) {
            moveCursorRight(shift); return;
        }
        if (kp->code == sf::Keyboard::Key::Home) {
            if (shift) {
                if (mSelAnchor < 0) mSelAnchor = mCursorPos;
                mCursorPos = 0;
                mSelStart = 0; mSelEnd = mSelAnchor;
                if (mSelStart == mSelEnd) mSelStart = mSelEnd = mSelAnchor = -1;
            } else {
                mCursorPos = 0;
                mSelStart = mSelEnd = mSelAnchor = -1;
            }
            mCursorBlinkClock.restart();
            return;
        }
        if (kp->code == sf::Keyboard::Key::End) {
            int endPos = (int)mInputBuffer.size();
            if (shift) {
                if (mSelAnchor < 0) mSelAnchor = mCursorPos;
                mCursorPos = endPos;
                mSelStart = mSelAnchor; mSelEnd = endPos;
                if (mSelStart == mSelEnd) mSelStart = mSelEnd = mSelAnchor = -1;
            } else {
                mCursorPos = endPos;
                mSelStart = mSelEnd = mSelAnchor = -1;
            }
            mCursorBlinkClock.restart();
            return;
        }

        // ── Backspace / Delete ──
        if (kp->code == sf::Keyboard::Key::Backspace) {
            doBackspace(); return;
        }
        if (kp->code == sf::Keyboard::Key::Delete) {
            if (eraseSelection()) return;
            if (mCursorPos < (int)mInputBuffer.size()) {
                mInputBuffer.erase(mCursorPos, 1);
                mCursorBlinkClock.restart();
            }
            return;
        }

        // ── Submit ──
        if (kp->code == sf::Keyboard::Key::Enter) {
            if (!mInputBuffer.empty()) {
                if (mCommandHistory.empty() || mCommandHistory.back() != mInputBuffer)
                    mCommandHistory.push_back(mInputBuffer);
                mHistoryIndex = (int)mCommandHistory.size();
                if (mOnCommandSubmitted) mOnCommandSubmitted(mInputBuffer);
            }
            mInputBuffer.clear();
            mCursorPos = 0;
            mSelStart = mSelEnd = mSelAnchor = -1;
            setFocus(false);
            return;
        }
        if (kp->code == sf::Keyboard::Key::Escape) {
            setFocus(false);
            return;
        }

        // ── History navigation ──
        if (kp->code == sf::Keyboard::Key::Up) {
            if (!mCommandHistory.empty()) {
                if (mHistoryIndex <= 0) mHistoryIndex = 0;
                else mHistoryIndex--;
                mInputBuffer = mCommandHistory[mHistoryIndex];
                mCursorPos   = (int)mInputBuffer.size();
                mSelStart = mSelEnd = mSelAnchor = -1;
            }
            return;
        }
        if (kp->code == sf::Keyboard::Key::Down) {
            if (!mCommandHistory.empty() && mHistoryIndex >= 0) {
                if (mHistoryIndex < (int)mCommandHistory.size() - 1) {
                    mHistoryIndex++;
                    mInputBuffer = mCommandHistory[mHistoryIndex];
                } else {
                    mHistoryIndex = (int)mCommandHistory.size();
                    mInputBuffer.clear();
                }
                mCursorPos = (int)mInputBuffer.size();
                mSelStart = mSelEnd = mSelAnchor = -1;
            }
            return;
        }
    }

    // ── Key Released → stop repeat ────────────
    if (const auto* kr = event.getIf<sf::Event::KeyReleased>()) {
        if (kr->code == mHeldKey) {
            mHeldKey = sf::Keyboard::Key::Unknown;
        }
    }

    // ── Mouse Release → end selection drag ──
    if (const auto* mr = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (mr->button == sf::Mouse::Button::Left) {
            mMouseSelecting = false;
        }
    }
}

// ──────────────────────────────────────────────
//  update — called every frame for key-hold repeat
// ──────────────────────────────────────────────
void ChatBox::update(float dt) {
    if (!mIsFocused) return;
    if (mHeldKey == sf::Keyboard::Key::Unknown) return;

    mHeldTimer += dt;
    if (mHeldTimer < REPEAT_DELAY) return;

    mRepeatAccum += dt;
    while (mRepeatAccum >= REPEAT_RATE) {
        mRepeatAccum -= REPEAT_RATE;

        // Fire the action again
        switch (mHeldKey) {
            case sf::Keyboard::Key::Backspace:
                doBackspace();
                break;
            case sf::Keyboard::Key::Delete:
                if (eraseSelection()) break;
                if (mCursorPos < (int)mInputBuffer.size())
                    mInputBuffer.erase(mCursorPos, 1);
                break;
            case sf::Keyboard::Key::Left:
                moveCursorLeft(mHeldShift);
                break;
            case sf::Keyboard::Key::Right:
                moveCursorRight(mHeldShift);
                break;
            case sf::Keyboard::Key::Home:
                mCursorPos = 0;
                if (!mHeldShift) mSelStart = mSelEnd = mSelAnchor = -1;
                break;
            case sf::Keyboard::Key::End:
                mCursorPos = (int)mInputBuffer.size();
                if (!mHeldShift) mSelStart = mSelEnd = mSelAnchor = -1;
                break;
            default: break;
        }
    }
}

// ──────────────────────────────────────────────
//  Layout
// ──────────────────────────────────────────────
void ChatBox::setPosition(sf::Vector2f pos) {
    mBackground.setPosition(pos);
}

void ChatBox::setSize(sf::Vector2f size) {
    mBackground.setSize(size);
    if (mLineHeight > 0) {
        float zoom = cfg::Map::ZOOM_FACTOR;
        mVisibleLines = static_cast<int>((86.f * zoom - mLineHeight - cfg::UI::Chat::PADDING_TOP) / mLineHeight);
    }
}

// ──────────────────────────────────────────────
//  addLine / scroll
// ──────────────────────────────────────────────
void ChatBox::addLine(const std::string& msg, sf::Color color) {
    std::lock_guard<std::mutex> lock(mMutex);
    
    // Calcular cuántos caracteres caben en el ancho actual del ChatBox
    float boxW = mBackground.getSize().x;
    float charW = GLYPH_LOGICAL_W * cfg::UI::FONT_SCALE;
    float availW = boxW - cfg::UI::Chat::PADDING_LEFT - cfg::UI::Chat::PADDING_RIGHT;
    int maxChars = (charW > 0.f) ? static_cast<int>(availW / charW) : 50;
    if (maxChars < 10) maxChars = 10;

    auto wrapAndPush = [&](const std::string& s) {
        if (s.empty()) {
            mMessages.push_back({" ", color});
            if (mMessages.size() > MAX_HISTORY) mMessages.pop_front();
            return;
        }

        size_t pos = 0;
        while (pos < s.size()) {
            size_t len = maxChars;
            if (pos + len >= s.size()) {
                mMessages.push_back({s.substr(pos), color});
                if (mMessages.size() > MAX_HISTORY) mMessages.pop_front();
                break;
            }

            size_t lastSpace = s.rfind(' ', pos + len);
            if (lastSpace != std::string::npos && lastSpace > pos) {
                size_t chunkLen = lastSpace - pos;
                mMessages.push_back({s.substr(pos, chunkLen), color});
                if (mMessages.size() > MAX_HISTORY) mMessages.pop_front();
                pos = lastSpace + 1; // saltar el espacio
            } else {
                // Palabra larga sin espacios, cortar en maxChars
                mMessages.push_back({s.substr(pos, len), color});
                if (mMessages.size() > MAX_HISTORY) mMessages.pop_front();
                pos += len;
            }
        }
    };

    std::stringstream ss(msg);
    std::string line;
    while (std::getline(ss, line)) {
        wrapAndPush(line);
    }
}

void ChatBox::scroll(int delta) {
    std::lock_guard<std::mutex> lock(mMutex);
    mScrollOffset += delta;
    int maxOffset = 0;
    if ((int)mMessages.size() > mVisibleLines)
        maxOffset = (int)mMessages.size() - mVisibleLines;
    mScrollOffset = std::clamp(mScrollOffset, 0, maxOffset);
}

// ──────────────────────────────────────────────
//  draw
// ──────────────────────────────────────────────
void ChatBox::draw(sf::RenderTarget& target) {
    if (!mIsVisible) return;
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mFontTexture) return;

    sf::Vector2f pos  = mBackground.getPosition();
    sf::Vector2f size = mBackground.getSize();

    if (mBgSprite) {
        float zoom = cfg::Map::ZOOM_FACTOR;
        mBgSprite->setPosition(pos);
        mBgSprite->setScale({zoom, zoom});
        target.draw(*mBgSprite);
    } else {
        target.draw(mBackground);
    }

    // ── Input line ──────────────────────────
    float zoom = cfg::Map::ZOOM_FACTOR;
    float inputX = pos.x + 7.f * zoom;
    float inputY = pos.y + 88.f * zoom;

    // Cache for click hit-testing
    mInputTextStartX = inputX;
    mInputTextStartY = inputY;

    // Pre-compute char pixel width
    float cw = charWidth();
    float prefixW = (float)INPUT_PREFIX.size() * cw;

    // ── Draw selection highlight ──
    if (mIsFocused && mSelStart >= 0 && mSelEnd > mSelStart) {
        int a = std::min(mSelStart, mSelEnd);
        int b = std::max(mSelStart, mSelEnd);
        a = std::clamp(a, 0, (int)mInputBuffer.size());
        b = std::clamp(b, 0, (int)mInputBuffer.size());
        float selX = inputX + prefixW + (float)a * cw;
        float selW = (float)(b - a) * cw;
        sf::RectangleShape selRect({selW, mLineHeight});
        selRect.setPosition({selX, inputY});
        selRect.setFillColor(sf::Color(80, 160, 255, 100));
        target.draw(selRect);
    }

    // ── Draw text ──
    BitmapText inputTxt;
    inputTxt.setTexture(mFontTexture);
    inputTxt.setScale({mInputScale, mInputScale});
    inputTxt.setColor(sf::Color::Cyan);
    inputTxt.setString(INPUT_PREFIX + mInputBuffer);
    inputTxt.setPosition({inputX, inputY});
    target.draw(inputTxt);

    // ── Draw blinking cursor ──
    if (mIsFocused) {
        bool cursorVisible = (int)(mCursorBlinkClock.getElapsedTime().asSeconds() * 2) % 2 == 0;
        if (cursorVisible) {
            float cursorX = inputX + prefixW + (float)mCursorPos * cw;
            BitmapText cursor;
            cursor.setTexture(mFontTexture);
            cursor.setScale({mInputScale, mInputScale});
            cursor.setColor(sf::Color::Cyan);
            cursor.setString("|");
            cursor.setPosition({cursorX, inputY});
            target.draw(cursor);
        }
    }

    // ── Chat messages ────────────────────────
    int total = (int)mMessages.size();
    if (total == 0) return;

    int count   = std::min(total, mVisibleLines);
    int lastIdx = total - 1 - mScrollOffset;
    int firstIdx = lastIdx - count + 1;
    if (lastIdx >= total) lastIdx = total - 1;
    if (firstIdx < 0) firstIdx = 0;

    float x = std::floor(pos.x + cfg::UI::Chat::PADDING_LEFT);
    float y = std::floor(pos.y + 88.f * zoom - mLineHeight);

    BitmapText text;
    text.setTexture(mFontTexture);
    text.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
    text.setColor(sf::Color::White);

    for (int i = 0; i < count; ++i) {
        int idx = lastIdx - i;
        if (idx < 0) break;
        text.setString(mMessages[idx].text);
        text.setColor(mMessages[idx].color);
        text.setPosition({x, std::floor(y)});
        target.draw(text);
        y -= mLineHeight;
    }

    // Scroll indicator
    if (mScrollOffset > 0) {
        BitmapText ind;
        ind.setTexture(mFontTexture);
        ind.setScale({cfg::UI::FONT_SCALE, cfg::UI::FONT_SCALE});
        ind.setString("BAJAR");
        ind.setPosition({pos.x + size.x - 60.f - cfg::UI::Chat::PADDING_RIGHT, pos.y + cfg::UI::Chat::PADDING_TOP});
        ind.setColor(sf::Color::Yellow);
        target.draw(ind);
        
        mScrollButtonBounds = sf::FloatRect({pos.x + size.x - 65.f - cfg::UI::Chat::PADDING_RIGHT, pos.y + cfg::UI::Chat::PADDING_TOP - 4.f}, {65.f, 25.f});
    } else {
        mScrollButtonBounds = sf::FloatRect();
    }
}

// ──────────────────────────────────────────────
//  Bounds / Mouse (dragging)
// ──────────────────────────────────────────────
sf::FloatRect ChatBox::getBounds() const {
    if (!mIsVisible) return sf::FloatRect();
    return mBackground.getGlobalBounds();
}

bool ChatBox::contains(sf::Vector2f point) const {
    if (!mIsVisible) return false;
    return mBackground.getGlobalBounds().contains(point);
}

bool ChatBox::isInsideInputBox(sf::Vector2f mousePos) const {
    if (!mIsVisible) return false;
    sf::Vector2f pos = mBackground.getPosition();
    float zoom = cfg::Map::ZOOM_FACTOR;
    
    float boxLeft = pos.x + 5.f * zoom;
    float boxTop = pos.y + 86.f * zoom;
    float boxRight = pos.x + 194.f * zoom;
    float boxBottom = pos.y + 94.f * zoom;
    
    return (mousePos.x >= boxLeft && mousePos.x <= boxRight &&
            mousePos.y >= boxTop && mousePos.y <= boxBottom);
}

bool ChatBox::onMousePress(sf::Vector2f mousePos) {
    if (!mIsVisible) return false;
    if (mScrollOffset > 0 && mScrollButtonBounds.contains(mousePos)) {
        std::lock_guard<std::mutex> lock(mMutex);
        mScrollOffset = 0;
        return true;
    }

    if (isInsideInputBox(mousePos)) {
        if (!mIsFocused) {
            setFocus(true);
        }
        onTextMousePress(mousePos);
        return true;
    }

    if (contains(mousePos)) {
        mIsDragging  = true;
        mDragOffset  = mBackground.getPosition() - mousePos;
        return true;
    }
    return false;
}

void ChatBox::onMouseRelease() {
    mIsDragging     = false;
    mMouseSelecting = false;
}

void ChatBox::onMouseMove(sf::Vector2f mousePos) {
    if (mIsDragging) {
        sf::Vector2f newPos = mousePos + mDragOffset;
        setPosition({std::floor(newPos.x), std::floor(newPos.y)});
    }
}

// ──────────────────────────────────────────────
//  Text-input mouse: logical-space coordinates
// ──────────────────────────────────────────────
bool ChatBox::onTextMousePress(sf::Vector2f logicalPos) {
    if (!mIsFocused) return false;
    sf::Vector2f pos = mBackground.getPosition();
    float zoom = cfg::Map::ZOOM_FACTOR;
    float boxTop = pos.y + 86.f * zoom;
    float boxBottom = pos.y + 94.f * zoom;
    if (logicalPos.y >= boxTop && logicalPos.y <= boxBottom) {
        float prefixW = (float)INPUT_PREFIX.size() * charWidth();
        float relX    = logicalPos.x - mInputTextStartX - prefixW;
        int idx = charIndexAt(relX);
        mCursorPos = idx;
        mSelAnchor = idx;
        mSelStart = mSelEnd = -1; // clear old selection; anchor set
        mMouseSelecting = true;
        mCursorBlinkClock.restart();
        return true;
    }
    return false;
}

void ChatBox::onTextMouseMove(sf::Vector2f logicalPos) {
    if (!mIsFocused || !mMouseSelecting) return;
    float prefixW = (float)INPUT_PREFIX.size() * charWidth();
    float relX    = logicalPos.x - mInputTextStartX - prefixW;
    int idx = charIndexAt(relX);
    mCursorPos = idx;
    int anchor = (mSelAnchor >= 0) ? mSelAnchor : idx;
    mSelStart = std::min(anchor, idx);
    mSelEnd   = std::max(anchor, idx);
    if (mSelStart == mSelEnd) mSelStart = mSelEnd = -1;
    mCursorBlinkClock.restart();
}

void ChatBox::onTextMouseRelease() {
    mMouseSelecting = false;
}

void ChatBox::updateVisuals() {}
