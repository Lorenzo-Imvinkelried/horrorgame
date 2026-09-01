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

void InputManager::HandleKeyPress(int key, Player& player, InventorySystem& inventory,
                                  BuildingSystem& buildingSystem, StructureSystem& structureSystem,
                                  ItemDropSystem& itemDropSystem, SkinningSystem& skinningSystem,
                                  SpellSystem& spellSystem, MobManager& mobManager,
                                  HorrorPropsSystem& horrorProps, DamageNumberSystem& damageNumbers,
                                  ParticleSystem& particles, ScentSystem& scentSystem)
{
#ifndef __EMSCRIPTEN__
    // SFML Key Codes
    if (key == sf::Keyboard::Escape) {
        if (m_fatalError.active) m_fatalError.active = false;
        else if (m_loreModal.active) m_loreModal.active = false;
        else if (inventory.IsOpen()) inventory.SetOpen(false);
        else if (m_isCharacterPanelOpen) m_isCharacterPanelOpen = false;
        else if (m_isBuildMode) m_isBuildMode = false;
        else if (m_isShovelMode) m_isShovelMode = false;
        else m_isGamePaused = !m_isGamePaused;
    }
    if (key == sf::Keyboard::R) {
        if (m_isBuildMode) {
            m_currentBuildYaw = fmod(m_currentBuildYaw + 90.0f, 360.0f);
        } else {
            // spellSystem.CastArcaneBeam is called in main loop / combat
        }
    }
    if (key == sf::Keyboard::F) {
        if (!itemDropSystem.TryCollectNearby(player.Position, inventory, damageNumbers, particles)) {
            player.ToggleTorch();
        }
    }
    if (key == sf::Keyboard::B) {
        m_isBuildMode = !m_isBuildMode;
        if (m_isBuildMode) m_isShovelMode = false;
    }
    if (key == sf::Keyboard::P) {
        m_isShovelMode = !m_isShovelMode;
        if (m_isShovelMode) m_isBuildMode = false;
    }
    if (m_isBuildMode) {
        if (key == sf::Keyboard::Num1 || key == sf::Keyboard::Numpad1) m_currentBuildType = BuildingType::WALL;
        if (key == sf::Keyboard::Num2 || key == sf::Keyboard::Numpad2) m_currentBuildType = BuildingType::ROOF;
        if (key == sf::Keyboard::Num3 || key == sf::Keyboard::Numpad3) m_currentBuildType = BuildingType::TORCH;
    }
    if (key == sf::Keyboard::F3) m_debugCam = !m_debugCam;
    if (m_debugCam && key == sf::Keyboard::G) m_showSpawnArea = !m_showSpawnArea;
    if (key == sf::Keyboard::H) m_showHitboxes = !m_showHitboxes;
    if (key == sf::Keyboard::O) m_showMonsterMarker = !m_showMonsterMarker;
    if (key == sf::Keyboard::V) player.ToggleCameraMode();
    if (key == sf::Keyboard::C) m_isCharacterPanelOpen = !m_isCharacterPanelOpen;

    if (!m_isBuildMode && m_isCharacterPanelOpen && player.Stats.AvailableStatPoints > 0) {
        if (key == sf::Keyboard::Num1 || key == sf::Keyboard::Numpad1) {
            if (player.Stats.AllocateStrength()) {
                for (int i = 0; i < 18; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                    particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.30f, 0.25f, 1.0f), 0.15f, 0.8f, -9.8f);
                }
            }
        } else if (key == sf::Keyboard::Num2 || key == sf::Keyboard::Numpad2) {
            if (player.Stats.AllocateAgility()) {
                for (int i = 0; i < 18; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                    particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.25f, 0.95f, 0.40f, 1.0f), 0.15f, 0.8f, -9.8f);
                }
            }
        } else if (key == sf::Keyboard::Num3 || key == sf::Keyboard::Numpad3) {
            if (player.Stats.AllocateVitality()) {
                for (int i = 0; i < 18; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                    particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.85f, 0.20f, 1.0f), 0.15f, 0.8f, -9.8f);
                }
            }
        } else if (key == sf::Keyboard::Num4 || key == sf::Keyboard::Numpad4) {
            if (player.Stats.AllocateIntelligence()) {
                for (int i = 0; i < 18; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                    particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.35f, 0.65f, 0.95f, 1.0f), 0.15f, 0.8f, -9.8f);
                }
            }
        }
    } else if (!m_isBuildMode && !m_isCharacterPanelOpen) {
        // Quickbar Potions [1] Health, [2] Mana, [3] Dragon Heart / Blood Vial, [4] Meat
        if (key == sf::Keyboard::Num1 || key == sf::Keyboard::Numpad1) {
            int s = inventory.GetInventory().FindItemByString("potion_health");
            if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
        } else if (key == sf::Keyboard::Num2 || key == sf::Keyboard::Numpad2) {
            int s = inventory.GetInventory().FindItemByString("potion_mana");
            if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
        } else if (key == sf::Keyboard::Num3 || key == sf::Keyboard::Numpad3) {
            int s = inventory.GetInventory().FindItemByString("dragon_heart");
            if (s == -1) s = inventory.GetInventory().FindItemByString("blood_vial");
            if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
        } else if (key == sf::Keyboard::Num4 || key == sf::Keyboard::Numpad4) {
            int s = inventory.GetInventory().FindItemByString("raw_meat");
            if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
        }
    }
    if (key == sf::Keyboard::I || key == sf::Keyboard::Tab) {
        inventory.ToggleOpen();
    }
    if (key == sf::Keyboard::T) {
        spellSystem.CastBloodBurst(player, mobManager.GetMonsters(), mobManager.GetPassiveMobs(), mobManager.GetEnemyMobs(), mobManager.GetWaterMonsters(), particles, damageNumbers, &mobManager.GetDragon());
    }
    if (key == sf::Keyboard::G && !m_isBuildMode) {
        skinningSystem.TrySkin(player.Position, mobManager.GetPassiveMobs(), inventory, itemDropSystem, player, damageNumbers, particles, scentSystem);
    }
    if (key == sf::Keyboard::Enter || key == sf::Keyboard::Space) {
        if (m_fatalError.active) m_fatalError.active = false;
    }
    if (key == sf::Keyboard::E) {
        if (m_loreModal.active) {
            m_loreModal.active = false;
        } else if (structureSystem.TryInteract(player.Position, player, inventory, itemDropSystem, damageNumbers, particles)) {
            // Opened ancestral chest / interacted
        } else if (!horrorProps.TryLootNearby(player.Position, &player, damageNumbers, m_loreModal)) {
            spellSystem.CastShadowAegis(player, particles);
        }
    }

#else
    // GLFW Key Codes (WebAssembly)
    if (key == GLFW_KEY_ESCAPE) {
        if (m_fatalError.active) m_fatalError.active = false;
        else if (m_loreModal.active) m_loreModal.active = false;
        else if (inventory.IsOpen()) inventory.SetOpen(false);
        else if (m_isCharacterPanelOpen) m_isCharacterPanelOpen = false;
        else if (m_isBuildMode) m_isBuildMode = false;
        else if (m_isShovelMode) m_isShovelMode = false;
        else m_isGamePaused = !m_isGamePaused;
    }
    if (key == GLFW_KEY_R) {
        if (m_isBuildMode) {
            m_currentBuildYaw = fmod(m_currentBuildYaw + 90.0f, 360.0f);
        }
    }
    if (key == GLFW_KEY_F) {
        if (!itemDropSystem.TryCollectNearby(player.Position, inventory, damageNumbers, particles)) {
            player.ToggleTorch();
        }
    }
    if (key == GLFW_KEY_B) {
        m_isBuildMode = !m_isBuildMode;
        if (m_isBuildMode) m_isShovelMode = false;
    }
    if (key == GLFW_KEY_P) {
        m_isShovelMode = !m_isShovelMode;
        if (m_isShovelMode) m_isBuildMode = false;
    }
    if (m_isBuildMode) {
        if (key == GLFW_KEY_1 || key == GLFW_KEY_KP_1) m_currentBuildType = BuildingType::WALL;
        if (key == GLFW_KEY_2 || key == GLFW_KEY_KP_2) m_currentBuildType = BuildingType::ROOF;
        if (key == GLFW_KEY_3 || key == GLFW_KEY_KP_3) m_currentBuildType = BuildingType::TORCH;
    }
    if (key == GLFW_KEY_F3) m_debugCam = !m_debugCam;
    if (m_debugCam && key == GLFW_KEY_G) m_showSpawnArea = !m_showSpawnArea;
    if (key == GLFW_KEY_H) m_showHitboxes = !m_showHitboxes;
    if (key == GLFW_KEY_O) m_showMonsterMarker = !m_showMonsterMarker;
    if (key == GLFW_KEY_V) player.ToggleCameraMode();
    if (key == GLFW_KEY_C) m_isCharacterPanelOpen = !m_isCharacterPanelOpen;

    if (!m_isBuildMode && m_isCharacterPanelOpen && player.Stats.AvailableStatPoints > 0) {
        if (key == GLFW_KEY_1 || key == GLFW_KEY_KP_1) {
            if (player.Stats.AllocateStrength()) {
                for (int i = 0; i < 18; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                    particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.30f, 0.25f, 1.0f), 0.15f, 0.8f, -9.8f);
                }
            }
        } else if (key == GLFW_KEY_2 || key == GLFW_KEY_KP_2) {
            if (player.Stats.AllocateAgility()) {
                for (int i = 0; i < 18; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                    particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.25f, 0.95f, 0.40f, 1.0f), 0.15f, 0.8f, -9.8f);
                }
            }
        } else if (key == GLFW_KEY_3 || key == GLFW_KEY_KP_3) {
            if (player.Stats.AllocateVitality()) {
                for (int i = 0; i < 18; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                    particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.95f, 0.85f, 0.20f, 1.0f), 0.15f, 0.8f, -9.8f);
                }
            }
        } else if (key == GLFW_KEY_4 || key == GLFW_KEY_KP_4) {
            if (player.Stats.AllocateIntelligence()) {
                for (int i = 0; i < 18; ++i) {
                    glm::vec3 pVel((rand()%100/50.0f - 1.0f)*2.5f, (rand()%100/50.0f + 0.3f)*3.0f, (rand()%100/50.0f - 1.0f)*2.5f);
                    particles.SpawnParticle(player.Position + glm::vec3(0, 1.2f, 0), pVel, glm::vec4(0.35f, 0.65f, 0.95f, 1.0f), 0.15f, 0.8f, -9.8f);
                }
            }
        }
    } else if (!m_isBuildMode && !m_isCharacterPanelOpen) {
        if (key == GLFW_KEY_1 || key == GLFW_KEY_KP_1) {
            int s = inventory.GetInventory().FindItemByString("potion_health");
            if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
        } else if (key == GLFW_KEY_2 || key == GLFW_KEY_KP_2) {
            int s = inventory.GetInventory().FindItemByString("potion_mana");
            if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
        } else if (key == GLFW_KEY_3 || key == GLFW_KEY_KP_3) {
            int s = inventory.GetInventory().FindItemByString("dragon_heart");
            if (s == -1) s = inventory.GetInventory().FindItemByString("blood_vial");
            if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
        } else if (key == GLFW_KEY_4 || key == GLFW_KEY_KP_4) {
            int s = inventory.GetInventory().FindItemByString("raw_meat");
            if (s != -1) inventory.UseOrEquipSlot(s, &player, &particles, &damageNumbers);
        }
    }
    if (key == GLFW_KEY_I || key == GLFW_KEY_TAB) {
        inventory.ToggleOpen();
    }
    if (key == GLFW_KEY_T) {
        spellSystem.CastBloodBurst(player, mobManager.GetMonsters(), mobManager.GetPassiveMobs(), mobManager.GetEnemyMobs(), mobManager.GetWaterMonsters(), particles, damageNumbers, &mobManager.GetDragon());
    }
    if (key == GLFW_KEY_G && !m_isBuildMode) {
        skinningSystem.TrySkin(player.Position, mobManager.GetPassiveMobs(), inventory, itemDropSystem, player, damageNumbers, particles, scentSystem);
    }
    if (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE) {
        if (m_fatalError.active) m_fatalError.active = false;
    }
    if (key == GLFW_KEY_E) {
        if (m_loreModal.active) {
            m_loreModal.active = false;
        } else if (structureSystem.TryInteract(player.Position, player, inventory, itemDropSystem, damageNumbers, particles)) {
            // Opened
        } else if (!horrorProps.TryLootNearby(player.Position, &player, damageNumbers, m_loreModal)) {
            spellSystem.CastShadowAegis(player, particles);
        }
    }
#endif
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

    // Left Click Actions
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

        // World Targeting & Weapon Attack
        glm::vec3 activeCamPos = player.GetCameraPosition();
        glm::vec3 camForward = player.Front;
        float bestDot = 0.80f;
        PassiveMob* bestDeer = nullptr;
        Monster* bestMonster = nullptr;
        EnemyMob* bestEnemy = nullptr;
        WaterMonster* bestWater = nullptr;
        Dragon* bestDragon = nullptr;
        float closestDist = 45.0f;

        if (mobManager.GetDragon().IsAlive() && !mobManager.GetDragon().IsDying()) {
            glm::vec3 dPos = mobManager.GetDragon().GetPosition() + glm::vec3(0, 1.8f, 0);
            glm::vec3 toD = glm::normalize(dPos - activeCamPos);
            float dot = glm::dot(camForward, toD);
            float dist = glm::distance(player.Position, mobManager.GetDragon().GetPosition());
            if (dot > 0.72f && dist < 160.0f) {
                bestDragon = &mobManager.GetDragon();
            }
        }

        for (auto& deer : mobManager.GetPassiveMobs()) {
            if (!deer->IsAlive()) continue;
            glm::vec3 mPos = deer->GetPosition() + glm::vec3(0, 1.2f, 0);
            glm::vec3 toM = glm::normalize(mPos - activeCamPos);
            float dot = glm::dot(camForward, toM);
            float dist = glm::distance(player.Position, deer->GetPosition());
            if (dot > bestDot && dist < closestDist) {
                bestDot = dot; closestDist = dist; bestDeer = deer.get();
            }
        }

        for (auto& enemy : mobManager.GetEnemyMobs()) {
            if (!enemy->IsAlive()) continue;
            glm::vec3 ePos = enemy->GetPosition() + glm::vec3(0, 1.4f, 0);
            glm::vec3 toE = glm::normalize(ePos - activeCamPos);
            float dot = glm::dot(camForward, toE);
            float dist = glm::distance(player.Position, enemy->GetPosition());
            if (dot > bestDot && dist < closestDist) {
                bestDot = dot; closestDist = dist; bestEnemy = enemy.get();
            }
        }

        for (auto& wm : mobManager.GetWaterMonsters()) {
            if (!wm->IsAlive()) continue;
            glm::vec3 wPos = wm->GetPosition() + glm::vec3(0, 1.2f, 0);
            glm::vec3 toW = glm::normalize(wPos - activeCamPos);
            float dot = glm::dot(camForward, toW);
            float dist = glm::distance(player.Position, wm->GetPosition());
            if (dot > bestDot && dist < closestDist) {
                bestDot = dot; closestDist = dist; bestWater = wm.get();
            }
        }

        for (auto& mPtr : mobManager.GetMonsters()) {
            if (mPtr->IsDead()) continue;
            glm::vec3 mPos = mPtr->GetPosition() + glm::vec3(0, 1.5f, 0);
            glm::vec3 toM = glm::normalize(mPos - activeCamPos);
            float dot = glm::dot(camForward, toM);
            float dist = glm::distance(player.Position, mPtr->GetPosition());
            if (dot > bestDot && dist < closestDist) {
                bestDot = dot; closestDist = dist; bestMonster = mPtr.get();
            }
        }

        if (bestDragon) targeting.SelectDragon(bestDragon);
        else if (bestWater) targeting.SelectWaterMonster(bestWater);
        else if (bestEnemy) targeting.SelectEnemy(bestEnemy);
        else if (bestDeer) targeting.SelectPassive(bestDeer);
        else if (bestMonster) targeting.SelectMonster(bestMonster);

        // Execute Attack
        if (!m_isGamePaused) {
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
                    glm::vec3 spawnPos = player.Position + glm::vec3(0, 1.3f, 0);
                    glm::vec3 aimTarget = spawnPos + player.Front * 45.0f;
                    if (targeting.HasTarget()) {
                        aimTarget = targeting.GetTargetPosition() + glm::vec3(0, 1.2f, 0);
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
    }
    m_leftWasPressed = leftIsPressed;
}
