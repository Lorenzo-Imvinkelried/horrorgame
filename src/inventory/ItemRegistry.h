#pragma once

#include "ItemDefinition.h"
#include <unordered_map>
#include <vector>
#include <string>

/**
 * @brief ItemRegistry: Base de datos central inmutable de definiciones de objetos.
 * Permite indexación ultra rápida O(1) contigua en memoria por ItemId numérico
 * y búsqueda por string para carga de datos / deserialización.
 */
class ItemRegistry {
public:
    static ItemRegistry& Get();

    ItemRegistry();
    ~ItemRegistry() = default;

    // Prohibir copia para evitar duplicaciones
    ItemRegistry(const ItemRegistry&) = delete;
    ItemRegistry& operator=(const ItemRegistry&) = delete;

    /**
     * @brief Registra una nueva definición en el sistema y le asigna un ItemId numérico secuencial.
     */
    ItemId RegisterItem(ItemDefinition def);

    /**
     * @brief Obtiene la definición inmutable por ItemId en O(1).
     * Si el ID es inválido, retorna la definición nula/vacía de seguridad.
     */
    const ItemDefinition& Get(ItemId id) const;

    /**
     * @brief Busca un ItemId a partir de su identificador textual ("potion_health").
     * Retorna INVALID_ITEM_ID si no existe.
     */
    ItemId FindId(const std::string& stringId) const;

    /**
     * @brief Busca la definición directamente por stringId.
     */
    const ItemDefinition* FindByString(const std::string& stringId) const;

    bool HasItem(ItemId id) const noexcept;
    size_t GetTotalRegistered() const noexcept { return m_definitions.size() - 1; }

    /**
     * @brief Inicializa el catálogo completo de objetos por defecto del juego.
     */
    void InitDefaultItems();

private:
    std::vector<ItemDefinition> m_definitions;
    std::unordered_map<std::string, ItemId> m_stringToId;
    ItemDefinition m_dummyInvalid;
};
