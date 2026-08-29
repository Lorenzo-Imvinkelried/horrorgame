#pragma once
#include <SFML/Graphics.hpp>
#include <deque>
#include <string>
#include <mutex>
#include <functional>


struct ChatMessage {
    std::string text;
    sf::Color color;
};

class ChatBox { // : public sf::Drawable (optional, using explicit draw)
public:
    ChatBox() = default;

    void init(sf::Texture* fontTexture);
    void setBgTexture(sf::Texture* tex);
    
    // Event handling for typing
    void handleEvent(const sf::Event& event);
    
    // Call every frame for key-hold repeat (backspace, arrows, delete)
    void update(float dt);
    
    void setOnCommandSubmitted(std::function<void(const std::string&)> callback) {
        mOnCommandSubmitted = callback;
    }

    bool isFocused() const { return mIsFocused; }
    void setFocus(bool focused);
    void setPosition(sf::Vector2f pos);
    void setSize(sf::Vector2f size);
    
    bool hasBgSprite() const { return mBgSprite.has_value(); }
    sf::Vector2f getBgSpriteSize() const { return mBgSprite ? sf::Vector2f(mBgSprite->getLocalBounds().size.x, mBgSprite->getLocalBounds().size.y) : sf::Vector2f(); }
    
    // Thread-safe-ish adding of messages
    void addLine(const std::string& msg, sf::Color color = sf::Color::White);

    void draw(sf::RenderTarget& target);

    // Scroll Control
    void scroll(int delta);
    
    sf::FloatRect getBounds() const;
    bool contains(sf::Vector2f point) const;
    bool isInsideInputBox(sf::Vector2f point) const;

    // Input / Dragging (raw window drag)
    bool onMousePress(sf::Vector2f mousePos);
    void onMouseRelease();
    void onMouseMove(sf::Vector2f mousePos);

    // Text-input mouse interaction (coordinates already in UI/logical space)
    bool onTextMousePress(sf::Vector2f logicalPos);
    void onTextMouseMove(sf::Vector2f logicalPos);
    void onTextMouseRelease();

    // Visibility Control
    void setVisible(bool visible) { mIsVisible = visible; if (!visible) setFocus(false); }
    void toggleVisible() { setVisible(!mIsVisible); }
    bool isVisible() const { return mIsVisible; }

private:
    void updateVisuals();
    
    // --- Text editing helpers ---
    // Returns char width (logical, pre-scale) for BitmapText
    float charWidth() const;       // in pixels (post-scale)
    // Width of prefix [0, count) in the input buffer
    float textWidthUpTo(int count) const;
    // Hit-test: given pixel X relative to input line start, return char index
    int   charIndexAt(float relativeX) const;
    // Erase selection, adjust cursor. Returns true if anything was erased.
    bool  eraseSelection();
    // Ensure sel_start <= sel_end
    void  normalizeSelection(int& a, int& b) const;
    // Delete word to the left of cursor (Ctrl+Backspace)
    void  deleteWordLeft();
    // Copy selection to clipboard
    void  copySelection();
    // Paste from clipboard at cursor
    void  pasteClipboard();
    // Move cursor left/right, extending selection if shift held
    void  moveCursorLeft(bool shift);
    void  moveCursorRight(bool shift);
    // Select All
    void  selectAll();
    // Insert a char at cursor (replaces selection)
    void  insertChar(char c);
    // Backspace one char (or erase selection)
    void  doBackspace();

private:
    sf::RectangleShape mBackground;
    sf::Texture* mFontTexture = nullptr;
    
    // Input System
    std::string  mInputBuffer;
    bool         mIsFocused = false;
    std::function<void(const std::string&)> mOnCommandSubmitted;
    
    // --- Cursor & Selection ---
    int  mCursorPos    = 0;   // insertion point index in mInputBuffer
    int  mSelStart     = -1;  // -1 means no selection
    int  mSelEnd       = -1;
    // Track whether selection is being extended from the anchor
    int  mSelAnchor    = -1;  // anchor end when shift-selecting
    
    // BitmapText input scale (must match draw)
    float mInputScale = 2.f;
    // Cache: pixel X where the input text starts (set in draw, used for click)
    float mInputTextStartX = 0.f;
    float mInputTextStartY = 0.f;
    
    // Blinking cursor
    sf::Clock mCursorBlinkClock;
    
    // --- Key-hold repeat ---
    // Which repeatable key is currently held
    sf::Keyboard::Key mHeldKey      = sf::Keyboard::Key::Unknown;
    bool              mHeldShift    = false;
    float             mHeldTimer    = 0.f;      // time since key pressed
    static constexpr float REPEAT_DELAY = 0.40f; // seconds before first repeat
    static constexpr float REPEAT_RATE  = 0.04f; // seconds between repeats after delay
    float             mRepeatAccum  = 0.f;      // accumulator for repeat rate
    
    std::deque<ChatMessage> mMessages;
    static constexpr size_t MAX_HISTORY = 200;
    
    // Command History (Arrow Up/Down)
    std::vector<std::string> mCommandHistory;
    int mHistoryIndex = -1; // -1 = Typing new, >= 0 = Index in history

    // Scrolling properties
    // 0 = Bottom (newest), > 0 = Older
    int mScrollOffset = 0; 
    
    // Layout
    unsigned int mCharacterSize = 14;
    float mLineHeight = 18.f;
    int mVisibleLines = 0;
    
    // Thread safety for logging from redirector
    std::mutex mMutex;

    // Dragging
    bool mIsDragging = false;
    sf::Vector2f mDragOffset;
    
    // Mouse selection drag
    bool mMouseSelecting = false;

    bool mIsVisible = true;
    sf::FloatRect mScrollButtonBounds;
    std::optional<sf::Sprite> mBgSprite;
};
