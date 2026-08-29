#pragma once
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/Event.hpp>
#include <vector>
#include <map>

// Lista de todas las acciones posibles en tu juego
enum class Action {
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    
    Attack,             // Click Derecho / Espacio
    Interact,           // Click Izquierdo / E
    
    OpenInventory,      // B
    OpenCharacterPanel, // C
    OpenMap,            // M [NEW]
    ToggleFortify,      // F [NEW]
    OpenTitlesPanel,    // T [NEW]
    OpenSkillLevelUpPanel, // H [NEW]
    
    // Debug
    DebugXP,    // J
    DebugStats, // U
    DebugExplosion, // O [NEW]
    DebugRestock, // K [NEW]
    DebugItems, // N [NEW]
    OpenSkillDebugPanel, // V [NEW]
    ToggleFps,  // F3
    PickupLoot, // E [NEW]
    Guard,      // Q
    
    Skill1, // 1
    Skill2, // 2
    Skill3, // 3
    Skill4, // 4
    Skill5, // 5
    Skill6, // 6
    Skill7, // 7
    Skill8, // 8
    Skill9, // 9
    Skill10, // 10
    Skill11, // 11
    Skill12, // 12
    
    HoldModifier, // Alt / Ctrl [NEW]
    CycleTarget,  // TAB key targeting cycle
    Dash,         // Shift key dash [NEW]

    Exit        // Escape
};

class InputManager {
public:
    InputManager();

    // Actualiza el estado de las teclas mantenidas (WASD)
    void update();
    
    void clearState();  // Limpia los eventos de un toque
    void clearAllStates(); // Limpia todos los estados (continuos y de un toque)
    // Procesa eventos de "un solo toque" (Abrir inventario, Clicks)
    void processEvent(const sf::Event& event);

    // Consultas
    bool isActionActive(Action action) const;      // ¿Está mantenida apretada?
    bool isActionJustPressed(Action action) const; // ¿Se presionó en este frame?
    
    sf::Vector2i getMousePosition() const { return mMousePos; }
    void setMousePosition(sf::Vector2i pos) { mMousePos = pos; } // [NEW] Allow manual override

private:
    // Mapeo: Acción -> Teclas Físicas (permite WASD + Flechitas y Scancodes)
    std::map<Action, std::vector<sf::Keyboard::Key>> mKeyBindings;
    std::map<Action, std::vector<sf::Keyboard::Scancode>> mScancodeBindings;
    std::map<Action, sf::Mouse::Button> mMouseBindings;

    // Estado actual
    std::map<Action, bool> mActiveActions;      // Para movimiento continuo
    std::map<Action, bool> mJustPressedActions; // Para acciones de un toque

    sf::Vector2i mMousePos;
    int mStartupFrames = 0;
};