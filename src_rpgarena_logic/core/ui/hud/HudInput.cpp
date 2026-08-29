#include "Hud.h"
#include "Config.h"
#include "core/ui/UIPanel.h"
#include "core/systems/CommandManager.h"
#include "core/systems/InteractionSystem.h"
#include "entities/Entity.h"
#include "entities/player/Player.h"

void Hud::handleScroll(int delta) {
  // [CHAT]
  if (mChatBox.getBounds().contains(mCurrentMousePos)) {
    mChatBox.scroll(delta);
    return;
  }

  if (mItemDebugPanel.isVisible() &&
      mItemDebugPanel.getBounds().contains(mCurrentMousePos)) {
    mItemDebugPanel.onMouseWheel(delta);
    return;
  }

  if (mTitlesPanelOpen && mTitlesPanel.getBounds().contains(mCurrentMousePos)) {
    mTitlesPanel.onMouseWheel(delta);
    return;
  }

  if (mSkillDebugPanel.isVisible() && mSkillDebugPanel.getBounds().contains(mCurrentMousePos)) {
    mSkillDebugPanel.onMouseWheel(delta);
    return;
  }

  if (mSkillLevelUpPanel.isVisible() && mSkillLevelUpPanel.getBounds().contains(mCurrentMousePos)) {
    mSkillLevelUpPanel.onMouseWheel(delta);
    return;
  }

  // Future: Scroll Inventory?
}

bool Hud::handleChatTextPress(sf::Vector2f logicalPos) {
  return mChatBox.onTextMousePress(logicalPos);
}

void Hud::handleChatTextMove(sf::Vector2f logicalPos) {
  mChatBox.onTextMouseMove(logicalPos);
}

void Hud::handleChatTextRelease() { mChatBox.onTextMouseRelease(); }

bool Hud::checkCharacterClick(sf::Vector2f mousePos) const {
  return mCharacterIconBounds.contains(mousePos);
}

bool Hud::checkBagClick(sf::Vector2f mousePos) const {
  return mBagIconBounds.contains(mousePos);
}

// [Z-ORDER] Checks if a point is occluded by UI elements drawn ON TOP of panels
bool Hud::isPointOccludedByUpperUI(sf::Vector2f pos) const {
  // These elements are drawn AFTER inventory/character panels in draw(),
  // so they visually cover them and should block interactions.
  if (mMapPanel.isOpen() && mMapPanel.getBounds().contains(pos))
    return true;
  if (mPlayerFrame.getBounds().contains(pos))
    return true;
  if (mTargetFrame.getBounds().contains(pos))
    return true;
  return false;
}

// --- HANDLE MOUSE MOVE ---
void Hud::handleMouseMove(sf::Vector2f mousePos) {
  mChatBox.onMouseMove(mousePos); // [CHAT]
  mCurrentMousePos = mousePos;

  if (mMapPanel.isOpen())
    mMapPanel.onMouseMove(mousePos);

  // Propagate mouse move to all visible panels
  for (auto *panel : mPanels) {
    if (panel && panel->isVisible()) {
      panel->onMouseMove(mousePos);
    }
  }

  // Check Drag Threshold
  if (mDragCandidateSlot != -1 && !mDragSystem.isDragging()) {
    sf::Vector2f diff = mousePos - mDragStartPos;
    float distSq = diff.x * diff.x + diff.y * diff.y;
    if (distSq >= DRAG_THRESHOLD * DRAG_THRESHOLD) {
      // Start Drag Now!
      std::shared_ptr<Item> item = nullptr;

      if (mDragCandidateSource == DragSource::Inventory) {
        item = mInventoryPanel.getItem(mDragCandidateSlot);
        if (item) {
          if (mDragCandidateIsSplit && item->stackCount > 1) {
            int half = item->stackCount / 2;
            int remaining = item->stackCount - half;

            auto draggedHalf = std::make_shared<Item>(*item);
            draggedHalf->stackCount = half;

            item->stackCount = remaining;

            mDragSystem.startDrag(mDragCandidateSlot, DragSource::Inventory,
                                  draggedHalf, true);
          } else {
            // Start Inventory Drag
            if (mDragSystem.startDrag(mDragCandidateSlot, DragSource::Inventory,
                                      item, false)) {
              mInventoryPanel.setItem(mDragCandidateSlot, nullptr);
            }
          }
        }
      } else if (mDragCandidateSource == DragSource::Fortify) { // [NEW]
        item = mFortifyPanel.getItem();
        if (item) {
          if (mDragSystem.startDrag(0, DragSource::Fortify, item)) {
            mFortifyPanel.setItem(nullptr);
          }
        }
      } else if (mDragCandidateSource ==
                 DragSource::Cultivo) { // [CULTIVO SYSTEM]
        item = mCultivoPanel.getItem();
        if (item) {
          if (mDragSystem.startDrag(0, DragSource::Cultivo, item)) {
            mCultivoPanel.setItem(nullptr);
          }
        }
      } else if (mDragCandidateSource == DragSource::ItemSpawner) { // [NEW]
        auto originalItem = mItemDebugPanel.getItemAt(mDragStartPos);
        if (originalItem) {
          // Genera una copia profunda al instante de arrastrar
          item = std::make_shared<Item>(*originalItem);
          mDragSystem.startDrag(0, DragSource::ItemSpawner, item);
        }
      } else if (mDragCandidateSource == DragSource::InspectPanel) {
        if (mInspectionPanelPtr && mInspectionPanelPtr->getEntity()) {
          item = mInspectionPanelPtr->getEntity()->getEquippedItem(
              static_cast<EquipmentSlot>(mDragCandidateSlot));
          if (item) {
            if (mDragSystem.startDrag(mDragCandidateSlot,
                                      DragSource::InspectPanel, item)) {
              mInspectionPanelPtr->setHiddenSlot(mDragCandidateSlot);
            }
          }
        }
      } else {
        // Character Drag Logic
        std::string slotName = mCharacterPanel.getSlotName(mDragCandidateSlot);
        int weaponIdx = -1;
        if (slotName == "Arma 1")
          weaponIdx = 0;
        else if (slotName == "Arma 2")
          weaponIdx = 1;

        if (weaponIdx != -1) {
          item = mPlayerFrame.getPlayer()->getWeapon(weaponIdx);
          if (!item && weaponIdx == 1) {
            auto mh = mPlayerFrame.getPlayer()->getWeapon(0);
            if (mh && mh->gripType == GripType::TwoHanded) {
              item = mh;
              mDragCandidateSlot = static_cast<int>(EquipmentSlot::MainHand);
            }
          }
        } else {
          EquipmentSlot slotType =
              static_cast<EquipmentSlot>(mDragCandidateSlot);
          item = mPlayerFrame.getPlayer()->getEquippedItem(slotType);
        }

        if (item) {
          if (mDragSystem.startDrag(mDragCandidateSlot, DragSource::Character,
                                    item)) {
            mCharacterPanel.setHiddenSlot(mDragCandidateSlot);
          }
        }
      }
      // Clear candidate
      mDragCandidateSlot = -1;
      mDragCandidateIsSplit = false;
    }
  }
}

bool Hud::handleMousePress(sf::Vector2f mousePos) {
  mCurrentMousePos = mousePos;

  // [BUG 5 FIX] Si ya estamos arrastrando algo, ignoramos el inicio de nuevos
  // clics
  if (mDragSystem.isDragging())
    return true;

  // [BUG 3 - Z-ORDER] Determinamos qué panel tiene prioridad visual.
  // Prioridad: Map > PlayerFrame > TargetFrame > Panel Superior > Otros > Chat
  if (mMapPanel.isOpen() && mMapPanel.getBounds().contains(mousePos)) {
    mMapPanel.onMousePress(mousePos);
    return true;
  }

  // [Z-ORDER] Player Frame and Target Frame are drawn ON TOP of panels
  if (mPlayerFrame.getBounds().contains(mousePos))
    return true;
  if (mTargetFrame.getBounds().contains(mousePos))
    return true;

  // Check panels in reverse Z-order (front to back)
  for (auto it = mPanels.rbegin(); it != mPanels.rend(); ++it) {
    UIPanel *panel = *it;
    if (panel && panel->isVisible()) {
      if (panel->onMousePress(mousePos)) {
        // Focus: bring clicked panel to front
        bringToFront(panel);

        Player *p = mPlayerFrame.getPlayer();
        bool isStunned = (p && p->isStunned());

        if (panel == &mInventoryPanel) {
          if (!isStunned) {
            int slot = mInventoryPanel.getSlotAt(mousePos);
            if (slot != -1) {
              auto clickedItem = mInventoryPanel.getItem(slot);
              if (InteractionSystem::getInstance().isActive()) {
                if (InteractionSystem::getInstance().onSlotClicked(
                        slot, clickedItem, &mInventoryPanel, p)) {
                  return true;
                }
              } else if (clickedItem) {
                mDragCandidateSlot = slot;
                mDragCandidateSource = DragSource::Inventory;
                mDragStartPos = mousePos;
                mDragCandidateIsSplit =
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                    sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
              }
            }
          }
        } else if (panel == &mCharacterPanel) {
          if (!isStunned) {
            int slot = mCharacterPanel.getSlotAt(mousePos);
            if (slot != -1) {
              std::string slotName = mCharacterPanel.getSlotName(slot);
              if (slotName == "Cultivo") {
                toggleCultivoPanel();
                return true;
              }
              int weaponIdx =
                  (slotName == "Arma 2") ? 1 : (slotName == "Arma 1" ? 0 : -1);
              std::shared_ptr<Item> targetItem = nullptr;
              if (weaponIdx != -1) {
                targetItem = p->getWeapon(weaponIdx);
              } else {
                targetItem =
                    p->getEquippedItem(static_cast<EquipmentSlot>(slot));
              }

              if (InteractionSystem::getInstance().isActive()) {
                if (InteractionSystem::getInstance().onSlotClicked(
                        -1, targetItem, &mInventoryPanel, p)) {
                  return true;
                }
              } else if (targetItem) {
                mDragCandidateSlot = slot;
                mDragCandidateSource = DragSource::Character;
                mDragStartPos = mousePos;
              }
            }
          }
        } else if (panel == mInspectionPanelPtr) {
          int slot = mInspectionPanelPtr->getSlotAt(mousePos);
          if (slot != -1 && mInspectionPanelPtr->getEntity()) {
            bool hasItem = (mInspectionPanelPtr->getEntity()->getEquippedItem(
                                static_cast<EquipmentSlot>(slot)) != nullptr);
            if (hasItem) {
              mDragCandidateSlot = slot;
              mDragCandidateSource = DragSource::InspectPanel;
              mDragStartPos = mousePos;
            }
          }
        } else if (panel == &mFortifyPanel) {
          if (mFortifyPanel.isMouseOverSlot(mousePos) &&
              mFortifyPanel.getItem()) {
            mDragCandidateSlot = 0;
            mDragCandidateSource = DragSource::Fortify;
            mDragStartPos = mousePos;
          }
        } else if (panel == &mItemDebugPanel) {
          if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
            auto item = mItemDebugPanel.getItemAt(mousePos);
            if (item) {
              mDragCandidateSlot = 0;
              mDragCandidateSource = DragSource::ItemSpawner;
              mDragStartPos = mousePos;
            }
          }
        }
        return true;
      }
    }
  }

  // [CHAT] Ahora el chat tiene la prioridad más BAJA (está al fondo)
  if (mChatBox.onMousePress(mousePos))
    return true;

  // Hotbar / Bag / Character Icon checks
  if (checkBagClick(mousePos))
    return true;
  if (checkCharacterClick(mousePos))
    return true;

  // Check if clicked anywhere else inside opened panels (for safety)
  if (checkPanelClick(mousePos))
    return true;

  return false;
}

void Hud::handleMouseRelease() {
  mChatBox.onMouseRelease(); // [CHAT]

  if (mMapPanel.isOpen())
    mMapPanel.onMouseRelease();

  mCharacterPanel.onMouseRelease();
  mInventoryPanel.onMouseRelease();
  mFortifyPanel.onMouseRelease();   // [NEW]
  mItemDebugPanel.onMouseRelease(); // [NEW]
  mTitlesPanel.onMouseRelease();    // [NEW]
  mCultivoPanel.onMouseRelease();   // [CULTIVO SYSTEM]
  mSkillLevelUpPanel.onMouseRelease(); // [NEW]
  mSkillDebugPanel.onMouseRelease(); // [NEW DEBUG]
  mDragCandidateSlot = -1; // Reset candidate if we released before dragging
  mDragCandidateIsSplit = false;

  if (!mDragSystem.isDragging())
    return;

  // Delegate drop logic to system
  bool handled =
      mDragSystem.handleDrop(mCurrentMousePos, mPlayerFrame.getPlayer(),
                             mInventoryPanel, mCharacterPanel,
                             mFortifyPanel, // [NEW]
                             *mRes,         // Necesitamos res
                             mPanels, mInspectionPanelPtr, &mCultivoPanel);

  if (!handled) {
    if (mDragSystem.getSource() != DragSource::ItemSpawner) {
      if (mWorldDropCallback) {
        mWorldDropCallback(mDragSystem.getDraggedItem());
      }
      mDragSystem.consumeDrag(mPlayerFrame.getPlayer(), mInventoryPanel,
                              mCharacterPanel, mFortifyPanel,
                              mInspectionPanelPtr, &mCultivoPanel);
    } else {
      mDragSystem.cancelDrag(mPlayerFrame.getPlayer(), mInventoryPanel,
                              mCharacterPanel, mFortifyPanel, *mRes,
                              mInspectionPanelPtr, &mCultivoPanel);
    }
  }

  // [FIX] Always reset hidden slot after drag ends
  mCharacterPanel.setHiddenSlot(-1);
  if (mInspectionPanelPtr) {
    mInspectionPanelPtr->setHiddenSlot(-1);
  }
}

void Hud::handleEvent(const sf::Event &event, Player *player) {
  if (mSkillDebugPanel.isVisible()) {
    if (const auto *kp = event.getIf<sf::Event::KeyPressed>()) {
      if (mSkillDebugPanel.onKeyPress(kp->code)) {
        return;
      }
    }
  }

  // [CHAT] Captura el Enter para empezar a escribir
  if (const auto *kp = event.getIf<sf::Event::KeyPressed>()) {
    if (kp->code == sf::Keyboard::Key::Enter && !mChatBox.isFocused()) {
      mChatBox.setFocus(true); // Bloquea el teclado para el chat
      return;
    }

    // [BUG 4 FIX] Tecla 'Y' para toggle del chat
    if (kp->code == sf::Keyboard::Key::Y && !mChatBox.isFocused()) {
      mChatBox.toggleVisible();
      return;
    }

    // Tecla 'Q' para el modo debug del char panel
    if (kp->code == sf::Keyboard::Key::Q && !mChatBox.isFocused() &&
        cfg::Debug::ENABLE_CHAR_PANEL_SLOTS_DEBUG) {
      if (!mCharacterPanelOpen) {
        toggleCharacterPanel();
      }
      bool nextMode = !mCharacterPanel.isDebugLayoutMode();
      mCharacterPanel.setDebugLayoutMode(nextMode);
      if (!nextMode) {
        CharacterPanel::saveLayout();
      }
      return;
    }
  }

  // Si el chat tiene el foco, los eventos del teclado van directos al cuadro de texto
  if (mChatBox.isFocused()) {
    mChatBox.setOnCommandSubmitted([this, player](const std::string &cmd) {
      if (mCommandManager && player) {
        // El CommandManager procesa comandos como "/heal", "/level 100", etc.
        std::string feedback =
            mCommandManager->processCommand(cmd, player, mTargetedEntity);
        if (!feedback.empty()) {
          if (feedback.find("[YO]:") == 0) {
            mChatBox.addLine(feedback, sf::Color::White); // Mensaje del usuario
          } else {
            mChatBox.addLine(feedback,
                             sf::Color::Yellow); // Respuesta del sistema
          }
        }
      }
    });

    mChatBox.handleEvent(event);
  }
}
