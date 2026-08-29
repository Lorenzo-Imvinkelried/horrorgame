#include "Hud.h"
#include "Config.h"
#include "core/ui/UIPanel.h"
#include "core/managers/TitleManager.h"
#include "core/skills/SkillManager.h"
#include "entities/player/Player.h"
#include "core/items/ItemManager.h"

void Hud::updateTooltip(sf::Vector2f mousePos, const SkillManager *skillMgr) {
  // [TOOLTIP] Decide qué descripción mostrar bajo el ratón.
  // Usamos variables estáticas para no recalcular todo si el ratón no se ha
  // movido de ítem.
  static const Item *lastHovered = nullptr;
  static const Item *lastCompared = nullptr;
  static const Skill *lastSkill = nullptr;
  static const Title *lastTitle = nullptr;
  static int lastHoveredFortLevel = -1; // [NEW]
  static int lastStonesCount = -1;      // [NEW]
  static int lastStackCount = -1;       // [NEW]

  // [FIX] No mostrar tooltips si estamos arrastrando un ítem
  if (mDragSystem.isDragging()) {
    mTooltip.hide();
    // [FIX] Reset static cache so tooltip regenerates properly after drop
    lastHovered = nullptr;
    lastCompared = nullptr;
    lastSkill = nullptr;
    lastTitle = nullptr;
    lastHoveredFortLevel = -1;
    lastStonesCount = -1;
    lastStackCount = -1;
    return;
  }

  if (mHoveredStatusEffect) {
    mTooltip.show(mHoveredStatusEffect->name, mHoveredStatusEffect->description,
                  mHoveredStatusEffectDuration, mousePos, mWindowSize);
    lastHovered = nullptr;
    lastCompared = nullptr;
    lastSkill = nullptr;
    lastTitle = nullptr;
    lastHoveredFortLevel = -1;
    lastStonesCount = -1;
    lastStackCount = -1;
    return;
  }

  std::shared_ptr<Item> hoveredItem = nullptr;
  const Skill *hoveredSkill = nullptr;
  const Title *hoveredTitle = nullptr;
  bool isFromInventory = false;

  // [Z-ORDER] Block tooltips from panels occluded by upper UI
  if (isPointOccludedByUpperUI(mousePos)) {
    // Nothing to show - upper UI covers everything below
  } else {
    // Check panels in Z-order priority (front to back)
    for (auto it = mPanels.rbegin(); it != mPanels.rend(); ++it) {
      UIPanel *panel = *it;
      if (panel && panel->isVisible() &&
          panel->getBounds().contains(mousePos)) {
        if (panel == &mInventoryPanel) {
          int slot = mInventoryPanel.getSlotAt(mousePos);
          if (slot != -1) {
            hoveredItem = mInventoryPanel.getItem(slot);
            isFromInventory = true;
          }
        } else if (panel == &mCharacterPanel) {
          Player *p = mPlayerFrame.getPlayer();
          if (p)
            hoveredItem = mCharacterPanel.getItemAt(mousePos, p);
        } else if (panel == &mFortifyPanel) {
          if (mFortifyPanel.isMouseOverSlot(mousePos)) {
            hoveredItem = mFortifyPanel.getItem();
            isFromInventory = true;
          }
        } else if (panel == &mItemDebugPanel) {
          hoveredItem = mItemDebugPanel.getItemAt(mousePos);
        } else if (panel == mInspectionPanelPtr) {
          Entity *ent = mInspectionPanelPtr->getEntity();
          if (ent)
            hoveredItem = mInspectionPanelPtr->getItemAt(mousePos, ent);
        } else if (panel == &mTitlesPanel) {
          hoveredTitle = mTitlesPanel.getHoveredTitle();
        } else if (panel == &mSkillDebugPanel) {
          int sId = mSkillDebugPanel.getHoveredSkillId();
          if (sId != -1 && skillMgr) {
            hoveredSkill = skillMgr->getSkill(sId);
          }
        } else if (panel == &mSkillLevelUpPanel) {
          int sId = mSkillLevelUpPanel.getHoveredSkillId();
          if (sId != -1 && skillMgr) {
            hoveredSkill = skillMgr->getSkill(sId);
          }
        }
        break; // Found a matching panel, stop checking others below it
      }
    }

    if (!hoveredItem) {
      if (mChatBox.getBounds().contains(mousePos) || checkBagClick(mousePos) ||
          checkCharacterClick(mousePos)) {
        hoveredItem = nullptr;
      } else if (mHoveredWorldItem) {
        hoveredItem = mHoveredWorldItem;
        isFromInventory = false;
      }
    }
  }

  // Hotbar (Habilidades) - checked independently since it doesn't overlap with panels
  if (!hoveredItem && !isPointOccludedByUpperUI(mousePos) && skillMgr) {
    const sf::Vector2f win((float)mWindowSize.x, (float)mWindowSize.y);
    const float totalW = cfg::UI::HOTBAR_SLOTS * mSlotSize +
                         (cfg::UI::HOTBAR_SLOTS - 1) * cfg::UI::SLOT_MARGIN;
    const sf::Vector2f origin((win.x - totalW) * 0.5f,
                              win.y - UI_MARGIN - mSlotSize);

    sf::FloatRect hotbarRect(origin, sf::Vector2f(totalW, mSlotSize));
    if (hotbarRect.contains(mousePos)) {
      float relX = mousePos.x - origin.x;
      int slotIndex = (int)(relX / (mSlotSize + cfg::UI::SLOT_MARGIN));
      float slotStart = slotIndex * (mSlotSize + cfg::UI::SLOT_MARGIN);

      if (relX >= slotStart && relX <= slotStart + mSlotSize) {
        if (slotIndex >= 0 && slotIndex < cfg::UI::HOTBAR_SLOTS) {
          Player *p = mPlayerFrame.getPlayer();
          if (p) {
            int sId = p->getEquippedSkill(slotIndex);
            if (sId != -1) {
              hoveredSkill = skillMgr->getSkill(sId);
            }
          }
        }
      }
    }
  }

  // --- LÓGICA DE VISUALIZACIÓN ---

  // A. Mostrar Tooltip de Habilidad
  if (hoveredSkill) {
    mTooltip.show(*hoveredSkill, mousePos, mWindowSize,
                  mPlayerFrame.getPlayer());
    lastSkill = hoveredSkill;
    lastHovered = nullptr;
    lastCompared = nullptr;
    lastTitle = nullptr;
    lastStackCount = -1;
    return;
  }
  lastSkill = nullptr;

  // B. Mostrar Tooltip de Título
  if (hoveredTitle) {
    if (hoveredTitle != lastTitle) {
      mTooltip.show(*hoveredTitle, mousePos, mWindowSize);
      lastTitle = hoveredTitle;
    } else {
      mTooltip.setPosition(mousePos, mWindowSize);
    }
    lastHovered = nullptr;
    lastCompared = nullptr;
    lastSkill = nullptr;
    lastHoveredFortLevel = -1;
    lastStackCount = -1;
    return;
  }
  lastTitle = nullptr;

  // C. Mostrar Tooltip de Ítem
  if (hoveredItem) {
    // [COMPARACIÓN] Si el ítem es un arma y está en el inventario, comparamos
    // con la equipada.
    const Item *comparisonItem = nullptr;

    auto isShieldItem = [](const std::shared_ptr<Item> &item) {
      return item && item->isShield();
    };

    if (isFromInventory &&
        (hoveredItem->type == ItemType::Weapon || isShieldItem(hoveredItem))) {
      Player *p = mPlayerFrame.getPlayer();
      bool altPressed = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) ||
                        sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt);

      int slotToCompare = 0;
      if (isShieldItem(hoveredItem)) {
        // Para escudos: sin ALT compara con la mano secundaria (slot 1 / OffHand).
        // Con ALT compara con la mano principal (slot 0 / MainHand).
        slotToCompare = altPressed ? 0 : 1;
      } else {
        // Para armas: sin ALT compara con la mano principal (slot 0 / MainHand).
        // Con ALT compara con la mano secundaria (slot 1 / OffHand).
        slotToCompare = altPressed ? 1 : 0;
      }

      auto equipped = p->getWeapon(slotToCompare);
      if (equipped && equipped.get() != hoveredItem.get()) {
        comparisonItem = equipped.get();
      }
    }

    int currentStonesCount = 0;
    for (const auto &stone : hoveredItem->socketedStones) {
      if (stone)
        currentStonesCount++;
    }

    // Si el ítem cambió, el contexto de comparación, la cantidad de gemas o el
    // stack cambió, regeneramos
    bool dirty = (hoveredItem.get() != lastHovered) ||
                 (comparisonItem != lastCompared) ||
                 (hoveredItem->fortificationLevel != lastHoveredFortLevel) ||
                 (currentStonesCount != lastStonesCount) ||
                 (hoveredItem->stackCount != lastStackCount);
    if (dirty) {
      mTooltip.show(*hoveredItem, mousePos, mWindowSize, comparisonItem);
      lastHovered = hoveredItem.get();
      lastCompared = comparisonItem;
      lastHoveredFortLevel = hoveredItem->fortificationLevel;
      lastStonesCount = currentStonesCount;
      lastStackCount = hoveredItem->stackCount;
    } else {
      mTooltip.setPosition(mousePos, mWindowSize);
    }
  } else {
    // Si no hay nada bajo el ratón, ocultamos el tooltip
    mTooltip.hide();
    lastHovered = nullptr;
    lastCompared = nullptr;
    lastSkill = nullptr;
    lastTitle = nullptr;
    lastHoveredFortLevel = -1;
    lastStonesCount = -1;
    lastStackCount = -1;
  }
}
