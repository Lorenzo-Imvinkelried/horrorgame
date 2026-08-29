#include "DragDropSystem.h"
#include "../engine/ResourceManager.h"
#include "../graphics/BitmapText.h"
#include "core/ui/character/CharacterPanel.h"
#include "../ui/CultivoPanel.h"
#include "../ui/FortifyPanel.h"
#include "../ui/InventoryPanel.h"
#include "../ui/UIPanel.h"
#include "Config.h"
#include "entities/player/Player.h"
#include <cmath>
#include <iostream>

DragDropSystem::DragDropSystem() {}

bool DragDropSystem::startDrag(int slotIndex, DragSource source,
                               std::shared_ptr<Item> item, bool isSplit) {
  if (!item)
    return false;

  std::cout << "[DragDrop] startDrag from "
            << (source == DragSource::Inventory ? "Inventory" : "CharPanel")
            << " slot " << slotIndex << "\n";

  mState.active = true;
  mState.item = item;
  mState.sourceSlotIndex = slotIndex;
  mState.sourceType = source;
  mState.isSplit = isSplit;
  return true;
}

bool DragDropSystem::handleDrop(sf::Vector2f uiMousePos, Player *player,
                                InventoryPanel &inventory,
                                CharacterPanel &character,
                                FortifyPanel &fortify, ResourceManager &res,
                                const std::vector<UIPanel *> &panels,
                                CharacterPanel *inspectPanel,
                                CultivoPanel *cultivo) {
  if (!mState.active || !mState.item)
    return false;

  // Helper to try dropping in Inspect Panel
  auto dropInspect = [&]() -> bool {
    if (inspectPanel && inspectPanel->isVisible() &&
        inspectPanel->getBounds().contains(uiMousePos)) {
      int targetSlot = inspectPanel->getSlotAt(uiMousePos);

      if (targetSlot == -1 || (mState.sourceType == DragSource::InspectPanel &&
                               mState.sourceSlotIndex == targetSlot)) {
        cancelDrag(player, inventory, character, fortify, res, inspectPanel);
        return true;
      }

      bool dropped = false;
      std::string slotName = inspectPanel->getSlotName(targetSlot);
      Entity *inspectedEntity = inspectPanel->getEntity();

      if (inspectedEntity) {
        EquipmentSlot targetEquipSlot = static_cast<EquipmentSlot>(targetSlot);
        auto isShieldItem = [](const std::shared_ptr<Item> &item) {
          return item && item->isShield();
        };

        bool isCompatible = false;
        if (mState.item->type == ItemType::Weapon) {
          isCompatible = (targetEquipSlot == EquipmentSlot::MainHand ||
                          targetEquipSlot == EquipmentSlot::OffHand ||
                          targetEquipSlot == EquipmentSlot::SubWeapon1 ||
                          targetEquipSlot == EquipmentSlot::SubWeapon2);
        } else if (mState.item->type == ItemType::Armor) {
          if (isShieldItem(mState.item)) {
            isCompatible = (targetEquipSlot == EquipmentSlot::MainHand ||
                            targetEquipSlot == EquipmentSlot::OffHand);
          } else {
            isCompatible = (mState.item->slotType == targetEquipSlot);
          }
        } else if (mState.item->type == ItemType::Ring) {
          isCompatible = (targetEquipSlot == EquipmentSlot::Ring1 ||
                          targetEquipSlot == EquipmentSlot::Ring2);
        }

        if (isCompatible) {
          bool isHandSlot = (targetEquipSlot == EquipmentSlot::MainHand || targetEquipSlot == EquipmentSlot::OffHand);
          EquipmentSlot otherSlot = (targetEquipSlot == EquipmentSlot::MainHand) ? EquipmentSlot::OffHand : EquipmentSlot::MainHand;

          auto existingTarget = inspectedEntity->getEquippedItem(targetEquipSlot);
          auto existingOther = isHandSlot ? inspectedEntity->getEquippedItem(otherSlot) : nullptr;

          bool unequipOther = isHandSlot && (
              (mState.item->gripType == GripType::TwoHanded) ||
              (existingOther && existingOther->gripType == GripType::TwoHanded)
          );

          auto dispTarget = existingTarget;
          auto dispOther = unequipOther ? existingOther : nullptr;

          std::shared_ptr<Item> primaryDisp = dispTarget ? dispTarget : dispOther;
          std::shared_ptr<Item> secondaryDisp = (dispTarget && dispOther) ? dispOther : nullptr;

          if (secondaryDisp != nullptr && mState.sourceType == DragSource::Inventory) {
            if (inventory.findEmptySlot() == -1) {
              cancelDrag(player, inventory, character, fortify, res, inspectPanel);
              return true;
            }
          }

          if (mState.sourceType == DragSource::Character) {
            player->unequipItem(static_cast<EquipmentSlot>(mState.sourceSlotIndex));
          } else if (mState.sourceType == DragSource::InspectPanel) {
            inspectedEntity->unequipItem(static_cast<EquipmentSlot>(mState.sourceSlotIndex));
          }

          if (dispOther) {
            inspectedEntity->unequipItem(otherSlot);
          }

          inspectedEntity->equipItem(mState.item, targetEquipSlot, res);
          dropped = true;

          if (mState.sourceType == DragSource::Inventory) {
            inventory.setItem(mState.sourceSlotIndex, primaryDisp);
            if (secondaryDisp) {
              int emptyIdx = inventory.findEmptySlot();
              if (emptyIdx != -1) {
                inventory.setItem(emptyIdx, secondaryDisp);
              }
            }
          } else if (mState.sourceType == DragSource::Character) {
            EquipmentSlot srcSlot = static_cast<EquipmentSlot>(mState.sourceSlotIndex);
            if (primaryDisp) {
              player->equipItem(primaryDisp, srcSlot, res);
            }
            if (secondaryDisp) {
              int emptyIdx = inventory.findEmptySlot();
              if (emptyIdx != -1) {
                inventory.setItem(emptyIdx, secondaryDisp);
              }
            }
          } else if (mState.sourceType == DragSource::InspectPanel) {
            EquipmentSlot srcSlot = static_cast<EquipmentSlot>(mState.sourceSlotIndex);
            if (primaryDisp) {
              inspectedEntity->equipItem(primaryDisp, srcSlot, res);
            }
            if (secondaryDisp) {
              int emptyIdx = inventory.findEmptySlot();
              if (emptyIdx != -1) {
                inventory.setItem(emptyIdx, secondaryDisp);
              }
            }
          }
        }
      }

      if (dropped) {
        mState.active = false;
        mState.item = nullptr;
        return true;
      }

      cancelDrag(player, inventory, character, fortify, res, inspectPanel);
      return true;
    }
    return false;
  };

  // Helper to try dropping in Character Panel
  auto dropCharacter = [&]() -> bool {
    if (character.isVisible() && character.getBounds().contains(uiMousePos)) {
      int targetSlot = character.getSlotAt(uiMousePos);

      if ((targetSlot == -1 && mState.sourceType != DragSource::Cultivo) ||
          (mState.sourceType == DragSource::Character &&
           mState.sourceSlotIndex == targetSlot)) {
        cancelDrag(player, inventory, character, fortify, res, inspectPanel,
                   cultivo);
        return true;
      }

      bool dropped = false;
      EquipmentSlot targetEquipSlot =
          (targetSlot != -1) ? static_cast<EquipmentSlot>(targetSlot)
                             : EquipmentSlot::None;

      auto isShieldItem = [](const std::shared_ptr<Item> &item) {
        return item && item->isShield();
      };

      bool isCompatible = false;
      if (mState.item->type == ItemType::Weapon) {
        isCompatible = (targetEquipSlot == EquipmentSlot::MainHand ||
                        targetEquipSlot == EquipmentSlot::OffHand ||
                        targetEquipSlot == EquipmentSlot::SubWeapon1 ||
                        targetEquipSlot == EquipmentSlot::SubWeapon2);
        if (!isCompatible && mState.sourceType == DragSource::Cultivo) {
          targetEquipSlot =
              EquipmentSlot::MainHand; // Default weapon slot: Arma 1
          isCompatible = true;
        }
      } else if (mState.item->type == ItemType::Armor) {
        if (isShieldItem(mState.item)) {
          isCompatible = (targetEquipSlot == EquipmentSlot::MainHand ||
                          targetEquipSlot == EquipmentSlot::OffHand);
          if (!isCompatible && mState.sourceType == DragSource::Cultivo) {
            targetEquipSlot = EquipmentSlot::OffHand;
            isCompatible = true;
          }
        } else {
          if (mState.sourceType == DragSource::Cultivo &&
              targetEquipSlot != mState.item->slotType) {
            targetEquipSlot = mState.item->slotType;
          }
          isCompatible = (mState.item->slotType == targetEquipSlot);
        }
      } else if (mState.item->type == ItemType::Ring) {
        isCompatible = (targetEquipSlot == EquipmentSlot::Ring1 ||
                        targetEquipSlot == EquipmentSlot::Ring2);
        if (!isCompatible && mState.sourceType == DragSource::Cultivo) {
          targetEquipSlot = EquipmentSlot::Ring1;
          isCompatible = true;
        }
      }

      if (isCompatible) {
        bool isHandSlot = (targetEquipSlot == EquipmentSlot::MainHand || targetEquipSlot == EquipmentSlot::OffHand);
        EquipmentSlot otherSlot = (targetEquipSlot == EquipmentSlot::MainHand) ? EquipmentSlot::OffHand : EquipmentSlot::MainHand;

        auto existingTarget = player->getEquippedItem(targetEquipSlot);
        auto existingOther = isHandSlot ? player->getEquippedItem(otherSlot) : nullptr;

        bool unequipOther = isHandSlot && (
            (mState.item->gripType == GripType::TwoHanded) ||
            (existingOther && existingOther->gripType == GripType::TwoHanded)
        );

        auto dispTarget = existingTarget;
        auto dispOther = unequipOther ? existingOther : nullptr;

        std::shared_ptr<Item> primaryDisp = dispTarget ? dispTarget : dispOther;
        std::shared_ptr<Item> secondaryDisp = (dispTarget && dispOther) ? dispOther : nullptr;

        if (secondaryDisp != nullptr && mState.sourceType == DragSource::Inventory) {
          if (inventory.findEmptySlot() == -1) {
            cancelDrag(player, inventory, character, fortify, res, inspectPanel, cultivo);
            return true;
          }
        }

        if (mState.sourceType == DragSource::Character) {
          player->unequipItem(static_cast<EquipmentSlot>(mState.sourceSlotIndex));
        } else if (mState.sourceType == DragSource::InspectPanel) {
          if (inspectPanel && inspectPanel->getEntity()) {
            inspectPanel->getEntity()->unequipItem(static_cast<EquipmentSlot>(mState.sourceSlotIndex));
          }
        }

        if (dispOther) {
          player->unequipItem(otherSlot);
        }

        player->equipItem(mState.item, targetEquipSlot, res);
        dropped = true;

        if (mState.sourceType == DragSource::Inventory) {
          inventory.setItem(mState.sourceSlotIndex, primaryDisp);
          if (secondaryDisp) {
            int emptyIdx = inventory.findEmptySlot();
            if (emptyIdx != -1) {
              inventory.setItem(emptyIdx, secondaryDisp);
            }
          }
        } else if (mState.sourceType == DragSource::Character) {
          EquipmentSlot srcSlot = static_cast<EquipmentSlot>(mState.sourceSlotIndex);
          if (primaryDisp) {
            player->equipItem(primaryDisp, srcSlot, res);
          }
          if (secondaryDisp) {
            int emptyIdx = inventory.findEmptySlot();
            if (emptyIdx != -1) {
              inventory.setItem(emptyIdx, secondaryDisp);
            }
          }
        } else if (mState.sourceType == DragSource::InspectPanel) {
          if (inspectPanel && inspectPanel->getEntity()) {
            EquipmentSlot srcSlot = static_cast<EquipmentSlot>(mState.sourceSlotIndex);
            if (primaryDisp) {
              inspectPanel->getEntity()->equipItem(primaryDisp, srcSlot, res);
            }
            if (secondaryDisp) {
              int emptyIdx = inventory.findEmptySlot();
              if (emptyIdx != -1) {
                inventory.setItem(emptyIdx, secondaryDisp);
              }
            }
          }
        } else if (mState.sourceType == DragSource::Cultivo) {
          if (primaryDisp) {
            int emptyIdx = inventory.findEmptySlot();
            if (emptyIdx != -1) inventory.setItem(emptyIdx, primaryDisp);
          }
          if (secondaryDisp) {
            int emptyIdx = inventory.findEmptySlot();
            if (emptyIdx != -1) inventory.setItem(emptyIdx, secondaryDisp);
          }
        }
      }

      if (dropped) {
        mState.active = false;
        mState.item = nullptr;
        return true;
      }

      cancelDrag(player, inventory, character, fortify, res, inspectPanel,
                 cultivo);
      return true;
    }
    return false;
  };

  // Helper to try dropping in Inventory Panel
  auto dropInventory = [&]() -> bool {
    if (inventory.isVisible() && inventory.getBounds().contains(uiMousePos)) {
      int targetSlot = inventory.getSlotAt(uiMousePos);

      if (targetSlot == -1 || (mState.sourceType == DragSource::Inventory &&
                               mState.sourceSlotIndex == targetSlot)) {
        cancelDrag(player, inventory, character, fortify, res, inspectPanel);
        return true;
      }

      auto existingItem = inventory.getItem(targetSlot);
      bool dropped = false;

      if (!existingItem) {
        inventory.setItem(targetSlot, mState.item);
        if (mState.sourceType == DragSource::Character) {
          player->unequipItem(
              static_cast<EquipmentSlot>(mState.sourceSlotIndex));
        } else if (mState.sourceType == DragSource::InspectPanel) {
          if (inspectPanel && inspectPanel->getEntity()) {
            inspectPanel->getEntity()->unequipItem(
                static_cast<EquipmentSlot>(mState.sourceSlotIndex));
          }
        }
        dropped = true;
      } else {
        if (mState.sourceType == DragSource::Inventory) {
          if (existingItem->id == mState.item->id &&
              existingItem->type == ItemType::Potion) {
            int spaceLeft = 99 - existingItem->stackCount;
            if (spaceLeft > 0) {
              if (mState.item->stackCount <= spaceLeft) {
                existingItem->stackCount += mState.item->stackCount;
                if (!mState.isSplit) {
                  inventory.setItem(mState.sourceSlotIndex, nullptr);
                }
              } else {
                existingItem->stackCount = 99;
                mState.item->stackCount -= spaceLeft;
                if (mState.isSplit) {
                  auto srcItem = inventory.getItem(mState.sourceSlotIndex);
                  if (srcItem && srcItem->id == mState.item->id) {
                    srcItem->stackCount += mState.item->stackCount;
                  } else {
                    inventory.setItem(mState.sourceSlotIndex, mState.item);
                  }
                } else {
                  inventory.setItem(mState.sourceSlotIndex, mState.item);
                }
              }
              dropped = true;
            }
          }
          if (!dropped) {
            if (mState.isSplit) {
              cancelDrag(player, inventory, character, fortify, res,
                         inspectPanel);
              return true;
            } else {
              inventory.setItem(targetSlot, mState.item);
              inventory.setItem(mState.sourceSlotIndex, existingItem);
              dropped = true;
            }
          }
        } else if (mState.sourceType == DragSource::Character) {
          EquipmentSlot srcSlot =
              static_cast<EquipmentSlot>(mState.sourceSlotIndex);
          bool swapCompatible = false;
          if (existingItem->type == ItemType::Weapon) {
            swapCompatible = (srcSlot == EquipmentSlot::MainHand ||
                              srcSlot == EquipmentSlot::OffHand ||
                              srcSlot == EquipmentSlot::SubWeapon1 ||
                              srcSlot == EquipmentSlot::SubWeapon2);
          } else if (existingItem->type == ItemType::Armor) {
            swapCompatible = (existingItem->slotType == srcSlot);
          } else if (existingItem->type == ItemType::Ring) {
            swapCompatible = (srcSlot == EquipmentSlot::Ring1 ||
                              srcSlot == EquipmentSlot::Ring2);
          }

          if (swapCompatible) {
            player->equipItem(existingItem, srcSlot, res);
            inventory.setItem(targetSlot, mState.item);
            dropped = true;
          }
        } else if (mState.sourceType == DragSource::InspectPanel) {
          if (inspectPanel && inspectPanel->getEntity()) {
            Entity *inspectEntity = inspectPanel->getEntity();
            EquipmentSlot srcSlot =
                static_cast<EquipmentSlot>(mState.sourceSlotIndex);
            bool swapCompatible = false;
            if (existingItem->type == ItemType::Weapon) {
              swapCompatible = (srcSlot == EquipmentSlot::MainHand ||
                                srcSlot == EquipmentSlot::OffHand ||
                                srcSlot == EquipmentSlot::SubWeapon1 ||
                                srcSlot == EquipmentSlot::SubWeapon2);
            } else if (existingItem->type == ItemType::Armor) {
              swapCompatible = (existingItem->slotType == srcSlot);
            } else if (existingItem->type == ItemType::Ring) {
              swapCompatible = (srcSlot == EquipmentSlot::Ring1 ||
                                srcSlot == EquipmentSlot::Ring2);
            }

            if (swapCompatible) {
              inspectEntity->equipItem(existingItem, srcSlot, res);
              inventory.setItem(targetSlot, mState.item);
              dropped = true;
            }
          }
        } else if (mState.sourceType == DragSource::Fortify) {
          fortify.setItem(existingItem);
          inventory.setItem(targetSlot, mState.item);
          dropped = true;
        }
      }

      if (dropped) {
        mState.active = false;
        mState.item = nullptr;
        return true;
      }

      cancelDrag(player, inventory, character, fortify, res, inspectPanel);
      return true;
    }
    return false;
  };

  // Helper to try dropping in Fortify Panel
  auto dropFortify = [&]() -> bool {
    if (fortify.isVisible() && fortify.getBounds().contains(uiMousePos)) {
      bool overSlot = fortify.isMouseOverSlot(uiMousePos);

      if (!overSlot || (mState.sourceType == DragSource::Fortify)) {
        cancelDrag(player, inventory, character, fortify, res, inspectPanel);
        return true;
      }

      if (mState.item->type == ItemType::Weapon ||
          mState.item->type == ItemType::Armor) {
        auto currentInSlot = fortify.getItem();
        if (currentInSlot) {
          if (mState.sourceType == DragSource::Inventory) {
            inventory.setItem(mState.sourceSlotIndex, currentInSlot);
          } else if (mState.sourceType == DragSource::Character) {
            int emptySlot = inventory.findEmptySlot();
            if (emptySlot != -1)
              inventory.setItem(emptySlot, currentInSlot);
            else {
              cancelDrag(player, inventory, character, fortify, res,
                         inspectPanel);
              return true;
            }
          } else if (mState.sourceType == DragSource::InspectPanel) {
            int emptySlot = inventory.findEmptySlot();
            if (emptySlot != -1)
              inventory.setItem(emptySlot, currentInSlot);
            else {
              cancelDrag(player, inventory, character, fortify, res,
                         inspectPanel);
              return true;
            }
          }
        }

        fortify.setItem(mState.item);
        if (mState.sourceType == DragSource::Character) {
          player->unequipItem(
              static_cast<EquipmentSlot>(mState.sourceSlotIndex));
        } else if (mState.sourceType == DragSource::InspectPanel) {
          if (inspectPanel && inspectPanel->getEntity()) {
            inspectPanel->getEntity()->unequipItem(
                static_cast<EquipmentSlot>(mState.sourceSlotIndex));
          }
        }
        mState.active = false;
        mState.item = nullptr;
        return true;
      }

      cancelDrag(player, inventory, character, fortify, res, inspectPanel,
                 cultivo);
      return true;
    }
    return false;
  };

  // Helper to try dropping in Cultivo Panel
  auto dropCultivo = [&]() -> bool {
    if (cultivo && cultivo->isVisible() &&
        cultivo->getBounds().contains(uiMousePos)) {
      bool overSlot = cultivo->isMouseOverSlot(uiMousePos);
      if (overSlot && mState.sourceType == DragSource::Inventory) {
        std::cout << "NO PODES CULTIVAR ALGO QUE NO TENES PUESTO!!!"
                  << std::endl;
      } else if (overSlot && mState.sourceType == DragSource::Character) {
        cultivo->setItem(mState.item);
      }
      cancelDrag(player, inventory, character, fortify, res, inspectPanel,
                 cultivo);
      return true;
    }
    return false;
  };

  // Try dropping on panels in Z-order priority (front to back)
  for (auto it = panels.rbegin(); it != panels.rend(); ++it) {
    UIPanel *panel = *it;
    if (panel == &inventory) {
      if (dropInventory())
        return true;
    } else if (panel == &character) {
      if (dropCharacter())
        return true;
    } else if (panel == inspectPanel) {
      if (dropInspect())
        return true;
    } else if (panel == &fortify) {
      if (dropFortify())
        return true;
    } else if (panel == (UIPanel *)cultivo) {
      if (dropCultivo())
        return true;
    }
  }

  return false; // Suelto en el mundo real
}

void DragDropSystem::consumeDrag(Player *player, InventoryPanel &inventory,
                                 CharacterPanel &character,
                                 FortifyPanel &fortify,
                                 CharacterPanel *inspectPanel,
                                 CultivoPanel *cultivo) {
  if (!mState.active || !mState.item)
    return;

  if (mState.sourceType == DragSource::Character && player) {
    player->unequipItem(static_cast<EquipmentSlot>(mState.sourceSlotIndex));
  } else if (mState.sourceType == DragSource::InspectPanel && inspectPanel &&
             inspectPanel->getEntity()) {
    inspectPanel->getEntity()->unequipItem(
        static_cast<EquipmentSlot>(mState.sourceSlotIndex));
  } else if (mState.sourceType == DragSource::Inventory) {
    if (!mState.isSplit) {
      inventory.setItem(mState.sourceSlotIndex, nullptr);
    }
  } else if (mState.sourceType == DragSource::Fortify) {
    fortify.setItem(nullptr);
  } else if (mState.sourceType == DragSource::Cultivo && cultivo) {
    cultivo->setItem(nullptr);
  }

  mState.active = false;
  mState.item = nullptr;
}

void DragDropSystem::cancelDrag(Player *player, InventoryPanel &inventory,
                                CharacterPanel &character,
                                FortifyPanel &fortify, ResourceManager &res,
                                CharacterPanel *inspectPanel,
                                CultivoPanel *cultivo) {
  if (!mState.active || !mState.item)
    return;

  // Devolver al origen
  if (mState.sourceType == DragSource::Inventory) {
    if (mState.isSplit) {
      auto existing = inventory.getItem(mState.sourceSlotIndex);
      if (existing && existing->id == mState.item->id) {
        existing->stackCount += mState.item->stackCount;
      } else {
        inventory.setItem(mState.sourceSlotIndex, mState.item);
      }
    } else {
      inventory.setItem(mState.sourceSlotIndex, mState.item);
    }
  } else if (mState.sourceType == DragSource::Fortify) {
    fortify.setItem(mState.item);
  } else if (mState.sourceType == DragSource::Cultivo && cultivo) {
    cultivo->setItem(mState.item);

  } else if (mState.sourceType == DragSource::ItemSpawner) {
    // Drop it into the void, it's just a generated debug item
  } else if (mState.sourceType == DragSource::InspectPanel) {
    if (inspectPanel && inspectPanel->getEntity()) {
      inspectPanel->getEntity()->equipItem(
          mState.item, static_cast<EquipmentSlot>(mState.sourceSlotIndex), res);
    }
  } else if (mState.sourceType == DragSource::Character) {
    // Devolver al cuerpo (equipar de nuevo)
    if (player) {
      player->equipItem(
          mState.item, static_cast<EquipmentSlot>(mState.sourceSlotIndex), res);
    }
  }

  mState.active = false;
  mState.item = nullptr;
}

void DragDropSystem::render(sf::RenderTarget &target, sf::Vector2f mousePos,
                            ResourceManager &res) {
  if (mState.active && mState.item) {
    try {
      sf::Texture &tex = res.getTexture(mState.item->texturePath);
      sf::Sprite dragSprite(tex);

      // [NUEVO] Soporte Texture Rect
      if (mState.item->textureRect.size.x > 0 &&
          mState.item->textureRect.size.y > 0) {
        dragSprite.setTextureRect(mState.item->textureRect);
      }

      float zoom = cfg::Map::ZOOM_FACTOR;
      dragSprite.setScale(sf::Vector2f(zoom, zoom));

      sf::FloatRect lb = dragSprite.getLocalBounds();
      // Origin at center, but pixel-aligned
      dragSprite.setOrigin(sf::Vector2f(std::floor(lb.size.x / 2.f),
                                        std::floor(lb.size.y / 2.f)));

      // Snap position to nearest integer for pixel perfection
      sf::Vector2f snappedPos(std::floor(mousePos.x), std::floor(mousePos.y));
      dragSprite.setPosition(snappedPos);

      dragSprite.setColor(sf::Color(255, 255, 255, 220));

      // Draw aura if fortified >= 6
      if (mState.item->fortificationLevel >= 6) {
        ItemAuraRenderer::drawAura(target, dragSprite,
                                   mState.item->fortificationLevel, zoom);
      }

      target.draw(dragSprite);

      // Draw stack count when dragging
      if (mState.item->type == ItemType::Potion &&
          mState.item->stackCount > 1) {
        try {
          sf::Texture &fontTex = res.getTexture("assets/fonts/font.png");
          BitmapText countText;
          countText.setTexture(&fontTex);
          countText.setString(std::to_string(mState.item->stackCount));
          countText.setColor(sf::Color::White);
          countText.setShadowOffset({1.f, 1.f});
          countText.setShadowColor(sf::Color(0, 0, 0, 200));
          countText.setScale({zoom, zoom});

          float tw = countText.getWidth() * zoom;
          float th = 5.f * zoom;

          float posX =
              snappedPos.x + (lb.size.x * 0.5f) * zoom - tw - 1.f * zoom;
          float posY =
              snappedPos.y + (lb.size.y * 0.5f) * zoom - th - 1.f * zoom;

          countText.setPosition({posX, posY});
          target.draw(countText);
        } catch (...) {
        }
      }

      // [NUEVO] Dibujar Overlay de Calidad si es Arma
      if (mState.item->quality != ItemQuality::Common &&
          mState.item->type == ItemType::Weapon) {
        try {
          sf::Texture &qTex =
              res.getTexture("assets/items/weapons/weapons_layout.png");
          sf::Sprite qIcon(qTex);

          if (mState.item->textureRect.size.x > 0 &&
              mState.item->textureRect.size.y > 0) {
            qIcon.setTextureRect(mState.item->textureRect);
          }

          // Copiar transformación del sprite base (ya alineada)
          qIcon.setScale(dragSprite.getScale());
          qIcon.setOrigin(dragSprite.getOrigin());
          qIcon.setPosition(dragSprite.getPosition());

          sf::Color qualityColor = getQualityColor(mState.item->quality);
          // Mantener un poco de transparencia global si quieres (220) o full
          // color qualityColor.a = 220;
          qIcon.setColor(qualityColor);

          target.draw(qIcon);
        } catch (...) {
          // Ignore
        }
      }
    } catch (...) {
      sf::RectangleShape r(sf::Vector2f(30.f, 30.f));
      r.setOrigin(sf::Vector2f(15.f, 15.f));
      r.setPosition(mousePos);
      r.setFillColor(sf::Color::Yellow);
      target.draw(r);
    }
  }
}
