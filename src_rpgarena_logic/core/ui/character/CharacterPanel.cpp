#include "CharacterPanel.h"
#include "Config.h"
#include <cmath>

std::vector<CharacterPanel::SlotConfig> CharacterPanel::sSlotLayouts;
bool CharacterPanel::sLayoutInitialized = false;

CharacterPanel::CharacterPanel(sf::Texture* fontTexture, std::optional<sf::Sprite>& slotSprite)
    : mFontTexture(fontTexture), mSlotSprite(slotSprite)
{
    mPosition = { UI_MARGIN, UI_MARGIN + 120.f }; 
    mSize = { cfg::UI::CharacterPanel::WIDTH, cfg::UI::CharacterPanel::HEIGHT }; 
    mIsBeingDragged = false;
    
    loadLayout();
}

void CharacterPanel::setEntity(Entity* entity) {
    if (mEntity == entity) return;
    
    if (mEntity && mObserverId != -1) {
        mEntity->removeStatsObserver(mObserverId);
        mObserverId = -1;
    }
    
    mEntity = entity;
    
    if (mEntity) {
        mObserverId = mEntity->addStatsObserver([this](){
             this->updateTexts();
        });
        updateTexts();
    }
}

void CharacterPanel::notifyEntityDeath(Entity* entity) {
    if (mEntity == entity) {
        setEntity(nullptr);
    }
}

void CharacterPanel::setOnCloseCallback(std::function<void()> callback) {
    mOnCloseCallback = callback;
}

void CharacterPanel::setPosition(sf::Vector2f pos) {
    mPosition = pos;
}

sf::FloatRect CharacterPanel::getBounds() const {
    return sf::FloatRect(mPosition, mSize);
}

void CharacterPanel::setVisible(bool visible) {
    UIPanel::setVisible(visible);
    if (!visible) {
        mIsBeingDragged = false;
    }
}

bool CharacterPanel::onMousePress(sf::Vector2f mousePos) {
    float zoom = cfg::Map::ZOOM_FACTOR;
    
    float btnX = mPosition.x + cfg::UI::CharacterPanel::CLOSE_BTN_X * zoom;
    float btnY = mPosition.y + cfg::UI::CharacterPanel::CLOSE_BTN_Y * zoom;
    float btnSize = cfg::UI::CharacterPanel::CLOSE_BTN_SIZE * zoom;
    
    if (mousePos.x >= btnX && mousePos.x <= btnX + btnSize &&
        mousePos.y >= btnY && mousePos.y <= btnY + btnSize) {
        
        if (mOnCloseCallback) mOnCloseCallback();
        return true;
    }

    if (mDebugLayoutMode) {
        int slotIdx = getSlotAt(mousePos);
        if (slotIdx != -1) {
            mSelectedDebugSlotIndex = slotIdx;
            mDebugDragStartOffset = sSlotLayouts[slotIdx].offset;
            mDebugDragStartMousePos = mousePos;
            return true;
        }
    }

    sf::FloatRect titleBar(mPosition, { mSize.x, cfg::UI::CharacterPanel::TITLE_HEIGHT * zoom }); 
    if (titleBar.contains(mousePos)) {
        mIsBeingDragged = true;
        mDragOffset = mousePos - mPosition;
        return true; 
    }

    if (getBounds().contains(mousePos)) {
        return true;
    }

    return false;
}

void CharacterPanel::onMouseRelease() {
    mIsBeingDragged = false;
    mSelectedDebugSlotIndex = -1;
}

void CharacterPanel::onMouseMove(sf::Vector2f mousePos) {
    if (mDebugLayoutMode && mSelectedDebugSlotIndex != -1) {
        float zoom = cfg::Map::ZOOM_FACTOR;
        sf::Vector2f mouseDelta = mousePos - mDebugDragStartMousePos;
        float deltaX_unscaled = std::round(mouseDelta.x / zoom);
        float deltaY_unscaled = std::round(mouseDelta.y / zoom);
        sSlotLayouts[mSelectedDebugSlotIndex].offset.x = mDebugDragStartOffset.x + deltaX_unscaled;
        sSlotLayouts[mSelectedDebugSlotIndex].offset.y = mDebugDragStartOffset.y + deltaY_unscaled;
        return;
    }

    if (mIsBeingDragged) {
        mPosition.x = std::floor(mousePos.x - mDragOffset.x);
        mPosition.y = std::floor(mousePos.y - mDragOffset.y);
    }
}

void CharacterPanel::drawSlotFallback(sf::RenderTarget& target, sf::Vector2f pos) {
    sf::RectangleShape r({cfg::UI::UNIFIED_SLOT_SIZE, cfg::UI::UNIFIED_SLOT_SIZE});
    r.setPosition(pos);
    r.setFillColor(sf::Color(30,30,30,220));
    r.setOutlineThickness(1.f);
    r.setOutlineColor(sf::Color(90,90,90));
    target.draw(r);
}

int CharacterPanel::getSlotAt(sf::Vector2f mousePos) const {
    if (!sLayoutInitialized) return -1;
    const float panelX = std::floor(mPosition.x);
    const float panelY = std::floor(mPosition.y);
    float zoom = cfg::Map::ZOOM_FACTOR;
    float slotSize = cfg::UI::BASE_SLOT_SIZE * zoom;
    
    for (size_t i = 0; i < sSlotLayouts.size(); ++i) {
        const auto& slot = sSlotLayouts[i];
        sf::Vector2f slotPos = {
            panelX + slot.offset.x * zoom,
            panelY + slot.offset.y * zoom
        };
        
        sf::FloatRect slotRect(slotPos, {slotSize, slotSize});
        if (slotRect.contains(mousePos)) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string CharacterPanel::getSlotName(int index) const {
    if (!sLayoutInitialized) return "";
    if (index >= 0 && index < (int)sSlotLayouts.size()) {
        return sSlotLayouts[index].name;
    }
    return "";
}

std::shared_ptr<Item> CharacterPanel::getItemAt(sf::Vector2f mousePos, Entity* entity) const {
    int slotIndex = getSlotAt(mousePos);
    if (slotIndex == -1 || !entity) return nullptr;
    if (slotIndex == mHiddenSlotIndex) return nullptr;

    return entity->getEquippedItem(static_cast<EquipmentSlot>(slotIndex));
}

void CharacterPanel::setHiddenSlot(int slotIndex) {
    mHiddenSlotIndex = slotIndex;
}
