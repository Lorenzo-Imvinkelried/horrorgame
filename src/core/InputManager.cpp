#include "InputManager.h"
#include "Player.h"
#include "inventory/InventorySystem.h"
#include "world/BuildingSystem.h"
#include "world/StructureSystem.h"
#include "world/ItemDropSystem.h"
#include "world/SkinningSystem.h"
#include "combat/SpellSystem.h"
#include "entities/MobManager.h"
#include "world/HorrorPropsSystem.h"
#include "combat/DamageNumberSystem.h"
#include "ParticleSystem.h"
#include "ScentSystem.h"
#include "combat/TargetingSystem.h"
#include "ProjectileSystem.h"
#include "ChunkManager.h"
#include "ui/UIRenderer.h"
#include "core/PlatformInput.h"
#include <iostream>
#include <cmath>
#include <algorithm>

#ifdef __EMSCRIPTEN__
#include <GLFW/glfw3.h>
#else
#include <SFML/Window/Keyboard.hpp>
#endif

InputManager::InputManager() {}
InputManager::~InputManager() {}

enum class GameKey {
    Escape,
    Interact,       // E
    Collect,        // F
    Skin,           // G
    Skill1,         // R (Onda Sangre)
    Skill2,         // T (Escudo Sombra)
    Skill3,         // Y (Rayo Arcano)
    ToggleTorch,    // L
    Inventory,      // I, Tab
    Character,      // C
    BuildMode,      // B
    ShovelMode,     // P
    CameraMode,     // V
    DebugCam,       // F3
    DebugHitbox,    // H
    DebugMarker,    // O
    Confirm,        // Enter, Space
    Num1, Num2, Num3, Num4,
    Unknown
};

static GameKey TranslateRawKey(int key) {
#ifndef __EMSCRIPTEN__
    switch (key) {
        case sf::Keyboard::Escape: return GameKey::Escape;
        case sf::Keyboard::E: return GameKey::Interact;
        case sf::Keyboard::F: return GameKey::Collect;
        case sf::Keyboard::G: return GameKey::Skin;
        case sf::Keyboard::R: return GameKey::Skill1;
        case sf::Keyboard::T: return GameKey::Skill2;
        case sf::Keyboard::Y: return GameKey::Skill3;
        case sf::Keyboard::L: return GameKey::ToggleTorch;
        case sf::Keyboard::I:
        case sf::Keyboard::Tab: return GameKey::Inventory;
        case sf::Keyboard::C: return GameKey::Character;
        case sf::Keyboard::B: return GameKey::BuildMode;
        case sf::Keyboard::P: return GameKey::ShovelMode;
        case sf::Keyboard::V: return GameKey::CameraMode;
        case sf::Keyboard::F3: return GameKey::DebugCam;
        case sf::Keyboard::H: return GameKey::DebugHitbox;
        case sf::Keyboard::O: return GameKey::DebugMarker;
        case sf::Keyboard::Enter:
        case sf::Keyboard::Space: return GameKey::Confirm;
        case sf::Keyboard::Num1:
        case sf::Keyboard::Numpad1: return GameKey::Num1;
        case sf::Keyboard::Num2:
        case sf::Keyboard::Numpad2: return GameKey::Num2;
        case sf::Keyboard::Num3:
        case sf::Keyboard::Numpad3: return GameKey::Num3;
        case sf::Keyboard::Num4:
        case sf::Keyboard::Numpad4: return GameKey::Num4;
        default: return GameKey::Unknown;
    }
#else
    switch (key) {
        case GLFW_KEY_ESCAPE: return GameKey::Escape;
        case GLFW_KEY_E: return GameKey::Interact;
        case GLFW_KEY_F: return GameKey::Collect;
        case GLFW_KEY_G: return GameKey::Skin;
        case GLFW_KEY_R: return GameKey::Skill1;
        case GLFW_KEY_T: return GameKey::Skill2;
        case GLFW_KEY_Y: return GameKey::Skill3;
        case GLFW_KEY_L: return GameKey::ToggleTorch;
        case GLFW_KEY_I:
        case GLFW_KEY_TAB: return GameKey::Inventory;
        case GLFW_KEY_C: return GameKey::Character;
        case GLFW_KEY_B: return GameKey::BuildMode;
        case GLFW_KEY_P: return GameKey::ShovelMode;
        case GLFW_KEY_V: return GameKey::CameraMode;
        case GLFW_KEY_F3: return GameKey::DebugCam;
        case GLFW_KEY_H: return GameKey::DebugHitbox;
        case GLFW_KEY_O: return GameKey::DebugMarker;
        case GLFW_KEY_ENTER:
        case GLFW_KEY_SPACE: return GameKey::Confirm;
        case GLFW_KEY_1:
        case GLFW_KEY_KP_1: return GameKey::Num1;
        case GLFW_KEY_2:
        case GLFW_KEY_KP_2: return GameKey::Num2;
        case GLFW_KEY_3:
        case GLFW_KEY_KP_3: return GameKey::Num3;
        case GLFW_KEY_4:
        case GLFW_KEY_KP_4: return GameKey::Num4;
        default: return GameKey::Unknown;
    }
#endif
}

void InputManager::HandleKeyPress(int key, Player& player, InventorySystem& inventory,
                                  BuildingSystem& buildingSystem, StructureSystem& structureSystem,
                                  ItemDropSystem& itemDropSystem, SkinningSystem& skinningSystem,
                                  SpellSystem& spellSystem, MobManager& mobManager,
                                  HorrorPropsSystem& horrorProps, DamageNumberSystem& damageNumbers,
                                  ParticleSystem& particles, ScentSystem& scentSystem,
                                  TargetingSystem& targeting, ProjectileSystem& projectiles)
{
    GameKey gKey = TranslateRawKey(key);

    if (player.IsDead()) {
        if (gKey == GameKey::Confirm) {
            player.Respawn(glm::vec3(0.0f, 15.0f, 0.0f));
        }
        return;
    }

    switch (gKey) {
        case GameKey::Escape:
            if (m_fatalError.active) m_fatalError.active = false;
            else if (m_loreModal.active) m_loreModal.active = false;
            else if (inventory.IsOpen()) inventory.SetOpen(false);
            else if (m_isCharacterPanelOpen) m_isCharacterPanelOpen = false;
            else if (m_isBuildMode) m_isBuildMode = false;
            else if (m_isShovelMode) m_isShovelMode = false;
            else m_isGamePaused = !m_isGamePaused;
            break;

        case GameKey::Interact: // E
            if (m_loreModal.active) {
                m_loreModal.active = false;
            } else if (structureSystem.TryInteract(player.Position, player, inventory, itemDropSystem, damageNumbers, particles)) {
                // Opened ancestral chest or offered blood
            } else if (itemDropSystem.TryCollectNearby(player.Position, inventory, damageNumbers, particles)) {
                // Picked up ground loot with E
            } else if (horrorProps.TryLootNearby(player.Position, &player, damageNumbers, m_loreModal)) {
                // Looted corpse or inspected prop / opened lore
            } else if (skinningSystem.TrySkin(player.Position, mobManager.GetPassiveMobs(), inventory, itemDropSystem, player, damageNumbers, particles, scentSystem)) {
                // Skinned dead deer
            }
            break;

        case GameKey::Collect: // F (also collects)
            itemDropSystem.TryCollectNearby(player.Position, inventory, damageNumbers, particles);
            break;

        case GameKey::Skin: // G
            if (m_debugCam) {
                m_showSpawnArea = !m_showSpawnArea;
            } else if (!m_isBuildMode) {
                skinningSystem.TrySkin(player.Position, mobManager.GetPassiveMobs(), inventory, itemDropSystem, player, damageNumbers, particles, scentSystem);
            }
            break;

        case GameKey::ToggleTorch: // L
            player.ToggleTorch();
            break;

        case GameKey::Skill1: // R
            if (m_isBuildMode) {
                m_currentBuildYaw = fmod(m_currentBuildYaw + 90.0f, 360.0f);
            } else {
                spellSystem.CastBloodBurst(player, mobManager.GetMonsters(), mobManager.GetPassiveMobs(), mobManager.GetEnemyMobs(), mobManager.GetWaterMonsters(), particles, damageNumbers, &mobManager.GetDragon());
            }
            break;

        case GameKey::Skill2: // T
            spellSystem.CastShadowAegis(player, particles);
            break;

        case GameKey::Skill3: // Y
            spellSystem.CastArcaneBeam(player, targeting, projectiles, particles);
            break;

        case GameKey::BuildMode: // B
            m_isBuildMode = !m_isBuildMode;
            if (m_isBuildMode) m_isShovelMode = false;
            break;

        case GameKey::ShovelMode: // P
            m_isShovelMode = !m_isShovelMode;
            if (m_isShovelMode) m_isBuildMode = false;
            break;

        case GameKey::CameraMode: // V
            player.ToggleCameraMode();
            break;

        case GameKey::Inventory: // I, Tab
            inventory.ToggleOpen();
            break;

        case GameKey::Character: // C
            m_isCharacterPanelOpen = !m_isCharacterPanelOpen;
            break;

        case GameKey::DebugCam: // F3
            m_debugCam = !m_debugCam;
            break;

        case GameKey::DebugHitbox: // H
            m_showHitboxes = !m_showHitboxes;
            break;

        case GameKey::DebugMarker: // O
            m_showMonsterMarker = !m_showMonsterMarker;
            break;

        case GameKey::Confirm: // Enter, Space
            if (m_fatalError.active) m_fatalError.active = false;
            break;

        case GameKey::Num1:
            if (m_isBuildMode) {
                m_currentBuildType = BuildingType::WALL;
            } else if (m_isCharacterPanelOpen && player.Stats.AvailableStatPoints > 0) {
                if (player.Stats.AllocateStrength()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.30f, 0.25f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            } else if (!m_isCharacterPanelOpen) {
                int s = inventory.GetInventory().FindItemByString("potion_health");
                if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
            }
            break;

        case GameKey::Num2:
            if (m_isBuildMode) {
                m_currentBuildType = BuildingType::ROOF;
            } else if (m_isCharacterPanelOpen && player.Stats.AvailableStatPoints > 0) {
                if (player.Stats.AllocateAgility()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.25f, 0.95f, 0.40f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            } else if (!m_isCharacterPanelOpen) {
                int s = inventory.GetInventory().FindItemByString("potion_mana");
                if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
            }
            break;

        case GameKey::Num3:
            if (m_isBuildMode) {
                m_currentBuildType = BuildingType::TORCH;
            } else if (m_isCharacterPanelOpen && player.Stats.AvailableStatPoints > 0) {
                if (player.Stats.AllocateVitality()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.85f, 0.20f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            } else if (!m_isCharacterPanelOpen) {
                int s = inventory.GetInventory().FindItemByString("dragon_heart");
                if (s == -1) s = inventory.GetInventory().FindItemByString("blood_vial");
                if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
            }
            break;

        case GameKey::Num4:
            if (m_isCharacterPanelOpen && player.Stats.AvailableStatPoints > 0) {
                if (player.Stats.AllocateIntelligence()) {
                    for (int i = 0; i < 18; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.35f, 0.65f, 0.95f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
            } else if (!m_isBuildMode && !m_isCharacterPanelOpen) {
                int s = inventory.GetInventory().FindItemByString("raw_meat");
                if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
            }
            break;

        default:
            break;
    }
}

void InputManager::UpdateMouseAndLook(float deltaTime, float curMouseX, float curMouseY,
                                      bool leftIsPressed, bool rightIsPressed,
                                      int screenW, int screenH, bool hasFocus,
                                      Player& player, InventorySystem& inventory,
                                      BuildingSystem& buildingSystem, StructureSystem& structureSystem,
                                      ItemDropSystem& itemDropSystem, TargetingSystem& targeting,
                                      ProjectileSystem& projectiles, MobManager& mobManager,
                                      HorrorPropsSystem& horrorProps, DamageNumberSystem& damageNumbers,
                                      ParticleSystem& particles, ChunkManager& chunkManager,
                                      float dayCycleTime, const glm::vec3& terraTarget,
                                      bool hasTerraTarget, const glm::vec3& buildPos)
{
    if (!hasFocus) return;

    if (m_firstMouse) {
        m_lastMouseX = curMouseX;
        m_lastMouseY = curMouseY;
        m_firstMouse = false;
    }

    float xoff = curMouseX - m_lastMouseX;
    float yoff = m_lastMouseY - curMouseY;
    m_lastMouseX = curMouseX;
    m_lastMouseY = curMouseY;

#ifdef __EMSCRIPTEN__
    static float virtualMouseX = (float)screenW * 0.5f;
    static float virtualMouseY = (float)screenH * 0.5f;
    static bool s_firstVirtual = true;
    if (s_firstVirtual) {
        virtualMouseX = (float)screenW * 0.5f;
        virtualMouseY = (float)screenH * 0.5f;
        s_firstVirtual = false;
    }
    virtualMouseX += xoff;
    virtualMouseY -= yoff;
    virtualMouseX = std::clamp(virtualMouseX, 0.0f, (float)screenW);
    virtualMouseY = std::clamp(virtualMouseY, 0.0f, (float)screenH);

    m_mouseNdcX = (virtualMouseX / (float)screenW) * 2.0f - 1.0f;
    m_mouseNdcY = 1.0f - (virtualMouseY / (float)screenH) * 2.0f;
#else
    m_mouseNdcX = (curMouseX / (float)screenW) * 2.0f - 1.0f;
    m_mouseNdcY = 1.0f - (curMouseY / (float)screenH) * 2.0f;
#endif

    if (player.IsDead()) {
        m_leftWasPressed = leftIsPressed;
        return;
    }

    if (inventory.IsOpen()) {
        inventory.UpdateDrag(m_mouseNdcX, m_mouseNdcY, leftIsPressed);
    }

    bool isUiModalActive = m_isCharacterPanelOpen || m_loreModal.active || m_fatalError.active || inventory.IsOpen();
    player.IsFreeOrbiting = rightIsPressed;

    bool shouldRotateCam = (!isUiModalActive && !player.IsThirdPerson) || rightIsPressed;

    if (shouldRotateCam) {
        if (std::abs(xoff) < 250.0f && std::abs(yoff) < 250.0f && (std::abs(xoff) > 0.001f || std::abs(yoff) > 0.001f)) {
            if (!m_debugCam) {
                player.ProcessMouseMovement(xoff, yoff);
            } else {
                m_freeCamYaw += xoff * 0.1f;
                m_freeCamPitch += yoff * 0.1f;
                m_freeCamPitch = std::clamp(m_freeCamPitch, -89.0f, 89.0f);
                glm::vec3 f;
                f.x = cos(glm::radians(m_freeCamYaw)) * cos(glm::radians(m_freeCamPitch));
                f.y = sin(glm::radians(m_freeCamPitch));
                f.z = sin(glm::radians(m_freeCamYaw)) * cos(glm::radians(m_freeCamPitch));
                m_freeCamFront = glm::normalize(f);
            }
        }
    }

    // 1. Single Click Actions (UI, Modals, Build Mode placement)
    if (leftIsPressed && !m_leftWasPressed) {
        if (m_isGamePaused) {
            bool resumeReq = false;
            if (UIRenderer::HandlePauseMenuClick(m_mouseNdcX, m_mouseNdcY, resumeReq)) {
                if (resumeReq) m_isGamePaused = false;
                m_leftWasPressed = leftIsPressed;
                return;
            }
        }

        if (m_isBuildMode) {
            buildingSystem.PlacePiece(m_currentBuildType, buildPos, m_currentBuildYaw, &particles);
            m_leftWasPressed = leftIsPressed;
            return;
        }

        if (m_isShovelMode && hasTerraTarget) {
            chunkManager.ModifyTerrain(terraTarget.x, terraTarget.z, 3.6f, -1.2f, &particles);
            m_leftWasPressed = leftIsPressed;
            return;
        }

        if (inventory.IsOpen()) {
            bool closeReq = false;
            if (inventory.HandleMouseClick(m_mouseNdcX, m_mouseNdcY, &player, &particles, &damageNumbers, &itemDropSystem, closeReq)) {
                if (closeReq) inventory.SetOpen(false);
                m_leftWasPressed = leftIsPressed;
                return;
            }
        }

        if (m_fatalError.active) {
            if (UIRenderer::HandleFatalErrorClick(m_mouseNdcX, m_mouseNdcY, m_fatalError)) {
                m_leftWasPressed = leftIsPressed;
                return;
            }
        }

        if (m_loreModal.active) {
            if (UIRenderer::HandleLoreModalClick(m_mouseNdcX, m_mouseNdcY, m_loreModal)) {
                m_leftWasPressed = leftIsPressed;
                return;
            }
        }

        if (m_isCharacterPanelOpen) {
            bool closeReq = false;
            if (UIRenderer::HandleCharacterPanelClick(m_mouseNdcX, m_mouseNdcY, player.Stats, closeReq)) {
                if (closeReq) {
                    m_isCharacterPanelOpen = false;
                } else {
                    for (int i = 0; i < 22; ++i) {
                        glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                        particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.85f, 0.20f, 1.0f), 0.15f, 0.8f, -9.8f);
                    }
                }
                m_leftWasPressed = leftIsPressed;
                return;
            }
        }

        // 1.B World Cursor Target Picking: Seleccionar objetivo con clic del cursor
        if (!isUiModalActive && !m_isGamePaused && !m_isBuildMode && !m_isShovelMode) {
            glm::mat4 projection = glm::perspective(glm::radians(70.0f), (float)screenW / (float)screenH, 0.1f, 300.0f);
            glm::mat4 view = player.GetViewMatrix();
            glm::mat4 invVP = glm::inverse(projection * view);

            glm::vec3 rayOrigin, rayDir;
            if (player.IsThirdPerson && !rightIsPressed) {
                glm::vec4 nearPoint = invVP * glm::vec4(m_mouseNdcX, m_mouseNdcY, -1.0f, 1.0f);
                nearPoint /= nearPoint.w;
                glm::vec4 farPoint = invVP * glm::vec4(m_mouseNdcX, m_mouseNdcY, 1.0f, 1.0f);
                farPoint /= farPoint.w;
                rayOrigin = glm::vec3(nearPoint);
                rayDir = glm::normalize(glm::vec3(farPoint - nearPoint));
            } else {
                rayOrigin = player.GetCameraPosition();
                rayDir = player.Front;
            }

            float closestHitDist = 65.0f;
            enum class ClickedType { NONE, DRAGON, ENEMY, MONSTER, WATER, PASSIVE } clickedType = ClickedType::NONE;
            void* clickedPtr = nullptr;

            auto testRaySphere = [&](const glm::vec3& center, float radius, ClickedType type, void* ptr) {
                glm::vec3 m = rayOrigin - center;
                float b = glm::dot(m, rayDir);
                float c = glm::dot(m, m) - (radius * radius);
                if (c > 0.0f && b > 0.0f) return;
                float discr = b * b - c;
                if (discr < 0.0f) return;
                float t = -b - sqrt(discr);
                if (t < 0.0f) t = 0.0f;
                if (t < closestHitDist) {
                    closestHitDist = t;
                    clickedType = type;
                    clickedPtr = ptr;
                }
            };

            if (mobManager.GetDragon().IsAlive() && !mobManager.GetDragon().IsDying()) {
                testRaySphere(mobManager.GetDragon().GetPosition() + glm::vec3(0, 1.8f, 0), mobManager.GetDragon().GetRadius() + 1.2f, ClickedType::DRAGON, &mobManager.GetDragon());
            }
            for (auto& enemy : mobManager.GetEnemyMobs()) {
                if (enemy->IsAlive()) {
                    testRaySphere(enemy->GetPosition() + glm::vec3(0, 1.2f, 0), enemy->GetRadius() + 0.6f, ClickedType::ENEMY, enemy.get());
                }
            }
            for (auto& mPtr : mobManager.GetMonsters()) {
                if (!mPtr->IsDead()) {
                    testRaySphere(mPtr->GetPosition() + glm::vec3(0, 1.2f, 0), 1.5f, ClickedType::MONSTER, mPtr.get());
                }
            }
            for (auto& wm : mobManager.GetWaterMonsters()) {
                if (wm->IsAlive()) {
                    testRaySphere(wm->GetPosition() + glm::vec3(0, 1.0f, 0), wm->GetRadius() + 0.6f, ClickedType::WATER, wm.get());
                }
            }
            for (auto& deer : mobManager.GetPassiveMobs()) {
                if (deer->IsAlive()) {
                    testRaySphere(deer->GetPosition() + glm::vec3(0, 1.0f, 0), deer->GetRadius() + 0.55f, ClickedType::PASSIVE, deer.get());
                }
            }

            if (clickedType == ClickedType::DRAGON) targeting.SelectDragon((Dragon*)clickedPtr);
            else if (clickedType == ClickedType::ENEMY) targeting.SelectEnemy((EnemyMob*)clickedPtr);
            else if (clickedType == ClickedType::MONSTER) targeting.SelectMonster((Monster*)clickedPtr);
            else if (clickedType == ClickedType::WATER) targeting.SelectWaterMonster((WaterMonster*)clickedPtr);
            else if (clickedType == ClickedType::PASSIVE) targeting.SelectPassive((PassiveMob*)clickedPtr);
            else {
                // Si pulsamos en el aire o terreno sin tocar ningún mob, deseleccionamos el target
                targeting.ClearTarget();
            }
        }
    }

    // 2. Continuous Combat Attack on Hold (Left Mouse Button)
    if (leftIsPressed && !isUiModalActive && !m_isGamePaused && !m_isBuildMode && !m_isShovelMode) {
        // Execute Attack (continuous while holding; player.TryAttack handles cooldown & attack speed)
        const ItemInstance& mhItem = inventory.GetEquipment().GetEquipped(EquipSlot::MAIN_HAND);
        bool isHoldingBow = false;
        if (mhItem.IsValid()) {
            const ItemDefinition& def = ItemRegistry::Get().Get(mhItem.id);
            if (def.iconId == "bow" || def.stringId == "hunting_bow" || def.stringId == "dragon_bone_bow") {
                isHoldingBow = true;
            }
        }

        if (isHoldingBow) {
            if (player.TryAttack()) {
                // Salida de flecha directamente desde el arco en mano (no arriba de la cabeza)
                glm::vec3 spawnPos;
                if (player.IsThirdPerson) {
                    glm::vec3 fwd = glm::length(glm::vec2(player.Front.x, player.Front.z)) > 0.001f 
                        ? glm::normalize(glm::vec3(player.Front.x, 0.0f, player.Front.z)) 
                        : glm::vec3(0, 0, 1);
                    glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0)));
                    // En 3ª persona: posición del arco en la mano izquierda hacia adelante
                    spawnPos = player.Position + glm::vec3(0.0f, 1.15f, 0.0f) - right * 0.28f + fwd * 0.42f;
                } else {
                    // En 1ª persona: desde el arco a la izquierda de la cámara
                    glm::vec3 right = glm::normalize(glm::cross(player.Front, glm::vec3(0, 1, 0)));
                    spawnPos = player.GetCameraPosition() - glm::vec3(0.0f, 0.20f, 0.0f) - right * 0.20f + player.Front * 0.45f;
                }

                // Dirección natural de disparo hacia donde apunta la mira / cursor
                glm::vec3 aimTarget = spawnPos + player.Front * 45.0f;
                if (player.IsThirdPerson && !rightIsPressed) {
                    glm::mat4 projection = glm::perspective(glm::radians(70.0f), (float)screenW / (float)screenH, 0.1f, 300.0f);
                    glm::mat4 view = player.GetViewMatrix();
                    glm::mat4 invVP = glm::inverse(projection * view);
                    glm::vec4 nearPoint = invVP * glm::vec4(m_mouseNdcX, m_mouseNdcY, -1.0f, 1.0f);
                    nearPoint /= nearPoint.w;
                    glm::vec4 farPoint = invVP * glm::vec4(m_mouseNdcX, m_mouseNdcY, 1.0f, 1.0f);
                    farPoint /= farPoint.w;
                    glm::vec3 cursorDir = glm::normalize(glm::vec3(farPoint - nearPoint));
                    aimTarget = spawnPos + cursorDir * 45.0f;
                }

                // AIMBOT: Solo se usa si YO tengo seleccionado/apretado el objetivo
                if (targeting.HasTarget() && targeting.IsTargetAlive()) {
                    aimTarget = targeting.GetTargetPosition() + glm::vec3(0.0f, 1.1f, 0.0f);
                }

                int bonusDmg = 0;
                int arrowSlot = inventory.GetInventory().FindItemByString("hunting_arrow");
                if (arrowSlot != -1) {
                    inventory.GetInventory().RemoveItemAt(arrowSlot, 1);
                    bonusDmg = 12;
                }

                int shotDmg = player.Stats.Attack + bonusDmg;
                glm::vec4 arrowCol = (mhItem.id == ItemRegistry::Get().FindId("dragon_bone_bow"))
                    ? glm::vec4(1.0f, 0.45f, 0.1f, 1.0f)
                    : glm::vec4(0.95f, 0.88f, 0.55f, 1.0f);

                projectiles.Spawn(spawnPos, aimTarget, 38.0f, shotDmg, arrowCol, true, ProjectileType::ARROW);
            }
        } else {
            if (player.TryAttack()) {
                horrorProps.CheckSwordCut(player.Position, 3.2f, particles);
            }
        }
    }

    m_leftWasPressed = leftIsPressed;
}
