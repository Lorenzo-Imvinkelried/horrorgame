#include "Hud.h"
#include "Config.h"
#include "core/ui/UIPanel.h"
#include "core/engine/InputManager.h"
#include "core/items/ConsumablesSystem.h"
#include "core/systems/InteractionSystem.h"
#include "entities/player/Player.h"
#include "entities/Entity.h"

// --- IMPLEMENTACIÓN CLICK DERECHO (EQUIPAR / DESEQUIPAR) ---
bool Hud::handleRightClick(sf::Vector2f mousePos, Player *player,
                           ResourceManager &res, const InputManager &input) {
  if (InteractionSystem::getInstance().isActive()) {
    InteractionSystem::getInstance().cancel();
    return true;
  }
  // [CLICK DERECHO] Gestiona equipar/desequipar ítems sin arrastrar.
  if (mDragSystem.isDragging())
    return true; // [FIX] Bloquear click derecho durante el arrastre (Bug 5)
  if (player && player->isStunned())
    return false;

  // [Z-ORDER] Block right-clicks on panels underneath upper UI elements
  if (isPointOccludedByUpperUI(mousePos))
    return true;

  auto handleCharRightClick = [&]() -> bool {
    if (mCharacterPanelOpen && mCharacterPanel.getBounds().contains(mousePos)) {
      int slotIndex = mCharacterPanel.getSlotAt(mousePos);
      if (slotIndex != -1) {
        std::string slotName = mCharacterPanel.getSlotName(slotIndex);
        if (slotName == "Arma 1" || slotName == "Arma 2") {
          EquipmentSlot slotType = (slotName == "Arma 2") ? EquipmentSlot::OffHand : EquipmentSlot::MainHand;
          auto weapon = player->getEquippedItem(slotType);
          if (!weapon && slotType == EquipmentSlot::OffHand) {
            auto mh = player->getEquippedItem(EquipmentSlot::MainHand);
            if (mh && mh->gripType == GripType::TwoHanded) {
              weapon = mh;
              slotType = EquipmentSlot::MainHand;
            }
          }
          if (weapon) {
            int emptySlot = findEmptyInventorySlot();
            if (emptySlot != -1) {
              mInventoryPanel.setItem(emptySlot, weapon);
              player->unequipItem(slotType);
              handleMouseMove(mousePos);
            }
          }
        } else {
          EquipmentSlot slotType = static_cast<EquipmentSlot>(slotIndex);
          auto item = player->getEquippedItem(slotType);
          if (item) {
            int emptySlot = findEmptyInventorySlot();
            if (emptySlot != -1) {
              mInventoryPanel.setItem(emptySlot, item);
              player->unequipItem(slotType);
              handleMouseMove(mousePos);
            }
          }
        }
      }
      return true;
    }
    return false;
  };

  auto handleInvRightClick = [&]() -> bool {
    if (mInventoryOpen && mInventoryPanel.getBounds().contains(mousePos)) {
      int slotIndex = mInventoryPanel.getSlotAt(mousePos);
      if (slotIndex != -1) {
        auto item = mInventoryPanel.getItem(slotIndex);
        if (item) {
          if (item->type == ItemType::Stone) {
            InteractionSystem::getInstance().start(InteractionMode::SocketStone,
                                                   item, slotIndex);
            return true;
          } else if (item->type == ItemType::Weapon || (item->type == ItemType::Armor && item->isShield())) {
            bool modifierPressed = input.isActionActive(Action::HoldModifier) ||
                                  sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) ||
                                  sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt);
            EquipmentSlot targetSlot = EquipmentSlot::MainHand;

            if (item->type == ItemType::Weapon) {
              if (item->gripType == GripType::TwoHanded) {
                targetSlot = modifierPressed ? EquipmentSlot::OffHand : EquipmentSlot::MainHand;
              } else {
                if (modifierPressed) {
                  targetSlot = EquipmentSlot::OffHand;
                } else if (player->getEquippedItem(EquipmentSlot::MainHand) == nullptr) {
                  targetSlot = EquipmentSlot::MainHand;
                } else if (player->getEquippedItem(EquipmentSlot::OffHand) == nullptr) {
                  targetSlot = EquipmentSlot::OffHand;
                } else {
                  targetSlot = EquipmentSlot::MainHand;
                }
              }
            } else { // Shield
              if (modifierPressed) {
                targetSlot = EquipmentSlot::MainHand;
              } else if (player->getEquippedItem(EquipmentSlot::OffHand) == nullptr) {
                targetSlot = EquipmentSlot::OffHand;
              } else if (player->getEquippedItem(EquipmentSlot::MainHand) == nullptr) {
                targetSlot = EquipmentSlot::MainHand;
              } else {
                targetSlot = EquipmentSlot::OffHand;
              }
            }

            EquipmentSlot otherSlot = (targetSlot == EquipmentSlot::MainHand) ? EquipmentSlot::OffHand : EquipmentSlot::MainHand;
            auto existingTarget = player->getEquippedItem(targetSlot);
            auto existingOther = player->getEquippedItem(otherSlot);

            bool unequipOther = (item->gripType == GripType::TwoHanded) ||
                                (existingOther && existingOther->gripType == GripType::TwoHanded);

            auto dispTarget = existingTarget;
            auto dispOther = unequipOther ? existingOther : nullptr;

            std::shared_ptr<Item> primaryDisp = dispTarget ? dispTarget : dispOther;
            std::shared_ptr<Item> secondaryDisp = (dispTarget && dispOther) ? dispOther : nullptr;

            if (secondaryDisp != nullptr) {
              int emptyIdx = findEmptyInventorySlot();
              if (emptyIdx == -1) {
                return true; // Inventory full, abort to prevent item loss
              }
            }

            if (dispOther) player->unequipItem(otherSlot);
            if (dispTarget) player->unequipItem(targetSlot);

            mInventoryPanel.setItem(slotIndex, primaryDisp);
            if (secondaryDisp) {
              int emptyIdx = findEmptyInventorySlot();
              if (emptyIdx != -1) {
                mInventoryPanel.setItem(emptyIdx, secondaryDisp);
              }
            }

            player->equipItem(item, targetSlot, res);
            handleMouseMove(mousePos);
          } else if (item->type == ItemType::Armor) {
            EquipmentSlot targetSlot = item->slotType;
            auto existingArmor = player->getEquippedItem(targetSlot);
            player->equipItem(item, targetSlot, res);
            mInventoryPanel.setItem(slotIndex, existingArmor);
            handleMouseMove(mousePos);
          } else if (item->type == ItemType::Ring) {
            EquipmentSlot targetSlot = EquipmentSlot::Ring1;
            if (player->getEquippedItem(EquipmentSlot::Ring1) == nullptr) {
              targetSlot = EquipmentSlot::Ring1;
            } else if (player->getEquippedItem(EquipmentSlot::Ring2) ==
                       nullptr) {
              targetSlot = EquipmentSlot::Ring2;
            } else {
              targetSlot = EquipmentSlot::Ring1;
            }
            auto existingRing = player->getEquippedItem(targetSlot);
            player->equipItem(item, targetSlot, res);
            mInventoryPanel.setItem(slotIndex, existingRing);
            handleMouseMove(mousePos);
          } else if (item->type == ItemType::Potion) {
            if (ConsumablesSystem::use(player, item)) {
              item->stackCount--;
              if (item->stackCount <= 0) {
                mInventoryPanel.setItem(slotIndex, nullptr);
              }
              handleMouseMove(mousePos);
            }
          }
        }
      }
      return true;
    }
    return false;
  };

  // Check panels in visual Z-order (top panel first)
  for (auto it = mPanels.rbegin(); it != mPanels.rend(); ++it) {
    UIPanel *panel = *it;
    if (panel && panel->isVisible() && panel->getBounds().contains(mousePos)) {
      if (panel == &mInventoryPanel) {
        if (handleInvRightClick())
          return true;
      } else if (panel == &mCharacterPanel) {
        if (handleCharRightClick())
          return true;
      } else if (panel == &mFortifyPanel) {
        if (mFortifyPanel.isMouseOverSlot(mousePos)) {
          auto item = mFortifyPanel.getItem();
          if (item) {
            int emptySlot = findEmptyInventorySlot();
            if (emptySlot != -1) {
              mInventoryPanel.setItem(emptySlot, item);
              mFortifyPanel.setItem(nullptr);
              handleMouseMove(mousePos);
            }
          }
        }
        return true;
      } else if (panel == &mCultivoPanel) {
        if (mCultivoPanel.isMouseOverSlot(mousePos))
          mCultivoPanel.setItem(nullptr);
      } else if (panel == mInspectionPanelPtr) {
        int slotIndex = mInspectionPanelPtr->getSlotAt(mousePos);
        if (slotIndex != -1 && mInspectionPanelPtr->getEntity()) {
          EquipmentSlot slotType = static_cast<EquipmentSlot>(slotIndex);
          auto item =
              mInspectionPanelPtr->getEntity()->getEquippedItem(slotType);
          if (item) {
            int emptySlot = findEmptyInventorySlot();
            if (emptySlot != -1) {
              mInventoryPanel.setItem(emptySlot, item);
              mInspectionPanelPtr->getEntity()->unequipItem(slotType);
              handleMouseMove(mousePos);
            }
          }
        }
        return true;
      } else if (panel == &mItemDebugPanel) {
        return true;
      }
    }
  }

  return false;
}
