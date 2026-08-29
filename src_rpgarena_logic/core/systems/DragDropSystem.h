#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include "../items/Item.h"

// Forward declarations
class Player;
class InventoryPanel;
class CharacterPanel;
class FortifyPanel; // [NEW]
class CultivoPanel; // [CULTIVO SYSTEM]
class ResourceManager;
class UIPanel;

enum class DragSource {
    Inventory,
    Character,
    Fortify, // [NEW]
    ItemSpawner, // [NEW]
    InspectPanel,
    Cultivo // [CULTIVO SYSTEM]
};

struct DragState {
    bool active = false;
    std::shared_ptr<Item> item = nullptr;
    int sourceSlotIndex = -1;
    DragSource sourceType = DragSource::Inventory;
    bool isSplit = false;
};

class DragDropSystem {
public:
    DragDropSystem();

    // Input Handling
    // Retorna true si inició un drag
    bool startDrag(int slotIndex, DragSource source, std::shared_ptr<Item> item, bool isSplit = false);
    
    bool handleDrop(sf::Vector2f uiMousePos, Player* player, InventoryPanel& inventory, CharacterPanel& character, FortifyPanel& fortify, ResourceManager& res, const std::vector<UIPanel*>& panels, CharacterPanel* inspectPanel = nullptr, CultivoPanel* cultivo = nullptr);
    
    // Termina el drag inmediatamente y lo elimina del origen (útil para el drop en el mundo)
    void consumeDrag(Player* player, InventoryPanel& inventory, CharacterPanel& character, FortifyPanel& fortify, CharacterPanel* inspectPanel = nullptr, CultivoPanel* cultivo = nullptr);
    
    // Si el drop falló o se canceló, devolver item al origen
    void cancelDrag(Player* player, InventoryPanel& inventory, CharacterPanel& character, FortifyPanel& fortify, ResourceManager& res, CharacterPanel* inspectPanel = nullptr, CultivoPanel* cultivo = nullptr); // [MODIFIED]

    // Getters
    bool isDragging() const { return mState.active; }
    const std::shared_ptr<Item>& getDraggedItem() const { return mState.item; }
    DragSource getSource() const { return mState.sourceType; }
    
    void render(sf::RenderTarget& target, sf::Vector2f mousePos, ResourceManager& res);

private:
    DragState mState;
};
