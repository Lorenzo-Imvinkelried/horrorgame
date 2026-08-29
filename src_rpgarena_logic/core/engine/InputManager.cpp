#include "InputManager.h"
#include <iostream>

InputManager::InputManager() {
    // --- CONFIGURACIÓN DE CONTROLES ---
    
    // Movimiento (Soporta WASD + Flechitas)
    mKeyBindings[Action::MoveUp]    = { sf::Keyboard::Key::W, sf::Keyboard::Key::Up };
    mKeyBindings[Action::MoveDown]  = { sf::Keyboard::Key::S, sf::Keyboard::Key::Down };
    mKeyBindings[Action::MoveLeft]  = { sf::Keyboard::Key::A, sf::Keyboard::Key::Left };
    mKeyBindings[Action::MoveRight] = { sf::Keyboard::Key::D, sf::Keyboard::Key::Right };
    mKeyBindings[Action::Dash]      = { sf::Keyboard::Key::LShift, sf::Keyboard::Key::RShift };

    // UI
    mKeyBindings[Action::OpenInventory]      = { sf::Keyboard::Key::B, sf::Keyboard::Key::I };
    mKeyBindings[Action::OpenCharacterPanel] = { sf::Keyboard::Key::C };
    mKeyBindings[Action::OpenMap]            = { sf::Keyboard::Key::M }; // [NEW]
    mKeyBindings[Action::ToggleFortify]      = { sf::Keyboard::Key::F }; // [NEW]
    mKeyBindings[Action::OpenTitlesPanel]    = { sf::Keyboard::Key::T }; // [NEW]
    mKeyBindings[Action::OpenSkillLevelUpPanel] = { sf::Keyboard::Key::H }; // [NEW]
    mKeyBindings[Action::OpenSkillDebugPanel] = { sf::Keyboard::Key::V }; // [NEW DEBUG]
    mKeyBindings[Action::PickupLoot]         = { sf::Keyboard::Key::E }; // [NEW]
    mKeyBindings[Action::Guard]              = { sf::Keyboard::Key::Q };
    mKeyBindings[Action::Exit]               = { sf::Keyboard::Key::Escape };
    
    // Debugs
    mKeyBindings[Action::DebugXP]        = { sf::Keyboard::Key::J };
    mKeyBindings[Action::DebugStats]     = { sf::Keyboard::Key::U };
    mKeyBindings[Action::DebugExplosion] = { sf::Keyboard::Key::O }; // [NEW]
    mKeyBindings[Action::DebugRestock]   = { sf::Keyboard::Key::K }; // [NEW]
    mKeyBindings[Action::DebugItems]     = { sf::Keyboard::Key::N }; // [NEW]
    mKeyBindings[Action::ToggleFps]      = { sf::Keyboard::Key::F3 }; // [NEW]
    
    // Skill Bindings (Defaults to Number Row)
    mKeyBindings[Action::Skill1] = { sf::Keyboard::Key::Num1 };
    mKeyBindings[Action::Skill2] = { sf::Keyboard::Key::Num2 };
    mKeyBindings[Action::Skill3] = { sf::Keyboard::Key::Num3 };
    mKeyBindings[Action::Skill4] = { sf::Keyboard::Key::Num4 };
    mKeyBindings[Action::Skill5] = { sf::Keyboard::Key::Num5 };
    mKeyBindings[Action::Skill6] = { sf::Keyboard::Key::Num6 };
    mKeyBindings[Action::Skill7] = { sf::Keyboard::Key::Num7 };
    mKeyBindings[Action::Skill8] = { sf::Keyboard::Key::Num8 };
    mKeyBindings[Action::Skill9] = { sf::Keyboard::Key::Num9 };
    mKeyBindings[Action::Skill10] = { sf::Keyboard::Key::Num0 };
    mKeyBindings[Action::Skill11] = { sf::Keyboard::Key::Hyphen, sf::Keyboard::Key::Apostrophe, sf::Keyboard::Key::Slash, sf::Keyboard::Key::Semicolon };
    mKeyBindings[Action::Skill12] = { sf::Keyboard::Key::Equal, sf::Keyboard::Key::LBracket, sf::Keyboard::Key::RBracket, sf::Keyboard::Key::Grave, sf::Keyboard::Key::Backslash };
    
    // [PHYSICAL KEYBOARD SCANCODES] Hardware physical position bindings (Works on all layouts Spanish/LatAm/US)
    mScancodeBindings[Action::Skill11] = { sf::Keyboard::Scancode::Hyphen };
    mScancodeBindings[Action::Skill12] = { sf::Keyboard::Scancode::Equal };

    mKeyBindings[Action::CycleTarget] = { sf::Keyboard::Key::Tab };

    // Mouse
    mMouseBindings[Action::Attack]   = sf::Mouse::Button::Right;
    mMouseBindings[Action::Interact] = sf::Mouse::Button::Left;

    /*
    Exactamente. No es necesario (ni recomendable) asignarla en el constructor (mKeyBindings) por una razón importante:

mKeyBindings es un mapa simple de "1 Acción = 1 Tecla". Si la asignamos ahí, tendríamos que elegir entre LAlt o RAlt, y la otra no funcionaría.

Como queremos que ambas teclas Alt funcionen, hicimos esa lógica especial en 
update()
 que verifica: bool mod = sf::Keyboard::isKeyPressed(LAlt) || sf::Keyboard::isKeyPressed(RAlt);

Así que, aunque no esté en la lista de arriba, el 
InputManager
 la está gestionando perfectamente abajo.
    */

}

void InputManager::update() {
    if (mStartupFrames < 15) {
        mStartupFrames++;
    }
   
    // Verificamos teclas mantenidas (Continuous Input)
    for (auto const& [action, keys] : mKeyBindings) {
        if (action == Action::HoldModifier) continue; 
        
        bool active = false;
        for (auto key : keys) {
            // Poll the key to flush any OS state, but ignore it during initial frames
            bool pressed = sf::Keyboard::isKeyPressed(key);
            if (pressed && mStartupFrames >= 15) {
                active = true;
                break;
            }
        }
        mActiveActions[action] = active;
    }

    for (auto const& [action, scancodes] : mScancodeBindings) {
        if (mActiveActions[action]) continue;
        for (auto scancode : scancodes) {
            bool pressed = sf::Keyboard::isKeyPressed(scancode);
            if (pressed && mStartupFrames >= 15) {
                mActiveActions[action] = true;
                break;
            }
        }
    }
    
    // Explicit Modifier Check (Robust)
    bool mod = sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LAlt) || 
               sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RAlt);
    mActiveActions[Action::HoldModifier] = mod;

    // Verificamos botones del mouse mantenidos
    for (auto const& [action, btn] : mMouseBindings) {
        mActiveActions[action] = sf::Mouse::isButtonPressed(btn);
    }
}

void InputManager::clearState() {
    mJustPressedActions.clear();
}

void InputManager::clearAllStates() {
    mActiveActions.clear();
    mJustPressedActions.clear();
    mStartupFrames = 0; // Reset counter when window focus is lost or states are cleared
}

void InputManager::processEvent(const sf::Event& event) {
    // Detectar pulsaciones únicas (Event Input)
    
    // Teclado
    if (const auto* k = event.getIf<sf::Event::KeyPressed>()) {
        for (auto const& [action, keys] : mKeyBindings) {
            for (auto key : keys) {
                if (k->code == key) {
                    mJustPressedActions[action] = true;
                    break;
                }
            }
        }
        for (auto const& [action, scancodes] : mScancodeBindings) {
            for (auto scancode : scancodes) {
                if (k->scancode == scancode) {
                    mJustPressedActions[action] = true;
                    break;
                }
            }
        }
    }
    
    // Mouse (Botones)
    if (const auto* m = event.getIf<sf::Event::MouseButtonPressed>()) {
        for (auto const& [action, btn] : mMouseBindings) {
            if (m->button == btn) {
                mJustPressedActions[action] = true;
            }
        }
        mMousePos = m->position; // Guardar posición al hacer click
    }
    
    // Mouse (Movimiento)
    if (const auto* m = event.getIf<sf::Event::MouseMoved>()) {
        mMousePos = m->position;
    }
}

bool InputManager::isActionActive(Action action) const {
    auto it = mActiveActions.find(action);
    if (it != mActiveActions.end()) return it->second;
    return false;
}

bool InputManager::isActionJustPressed(Action action) const {
    auto it = mJustPressedActions.find(action);
    if (it != mJustPressedActions.end() && it->second) {
        return true;
    }
    return false;
}