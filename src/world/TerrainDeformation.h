#pragma once

#include <unordered_map>
#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

class TerrainDeformation {
public:
    // Retorna el delta de altura acumulado en una coordenada de mundo (x, z) con interpolación suave
    static float GetDeformation(float worldX, float worldZ);

    // Retorna el delta exacto almacenado en un vértice del grid
    static float GetGridDeformation(int gridX, int gridZ);

    // Aplica una deformación radial suave (deltaHeight positivo = elevar/construir, negativo = cavar/destruir)
    // Retorna true si al menos un vértice fue modificado
    static bool Deform(float centerX, float centerZ, float radius, float deltaHeight, float minLimit = -15.0f, float maxLimit = 65.0f);

    // Reinicia todas las deformaciones
    static void Reset();

    // Helper para empaquetar coordenadas de grid en una clave única de 64 bits
    static inline int64_t MakeKey(int gx, int gz) {
        return ((int64_t)gx << 32) | ((int64_t)gz & 0xFFFFFFFFLL);
    }

    static inline void UnpackKey(int64_t key, int& gx, int& gz) {
        gx = (int)(key >> 32);
        gz = (int)(key & 0xFFFFFFFFLL);
    }

    // Consulta si existen modificaciones en un radio dado
    static bool HasModificationsInRange(float worldX, float worldZ, float range);

    static const std::unordered_map<int64_t, float>& GetAllDeltas() { return s_deltas; }

private:
    static std::unordered_map<int64_t, float> s_deltas;
};
