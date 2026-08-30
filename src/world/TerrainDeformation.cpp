#include "TerrainDeformation.h"
#include "Config.h"
#include <cmath>
#include <algorithm>

std::unordered_map<int64_t, float> TerrainDeformation::s_deltas;

float TerrainDeformation::GetGridDeformation(int gridX, int gridZ) {
    int64_t key = MakeKey(gridX, gridZ);
    auto it = s_deltas.find(key);
    if (it != s_deltas.end()) {
        return it->second;
    }
    return 0.0f;
}

float TerrainDeformation::GetDeformation(float worldX, float worldZ) {
    if (s_deltas.empty()) return 0.0f;

    float scale = Config::World::ChunkScale;
    int gx = (int)std::floor(worldX / scale);
    int gz = (int)std::floor(worldZ / scale);

    float localX = (worldX - (float)gx * scale) / scale;
    float localZ = (worldZ - (float)gz * scale) / scale;

    float d00 = GetGridDeformation(gx, gz);
    float d10 = GetGridDeformation(gx + 1, gz);
    float d01 = GetGridDeformation(gx, gz + 1);
    float d11 = GetGridDeformation(gx + 1, gz + 1);

    if (d00 == 0.0f && d10 == 0.0f && d01 == 0.0f && d11 == 0.0f) {
        return 0.0f;
    }

    if (localX + localZ <= 1.0f) {
        return d00 + (d10 - d00) * localX + (d01 - d00) * localZ;
    } else {
        return d11 + (d01 - d11) * (1.0f - localX) + (d10 - d11) * (1.0f - localZ);
    }
}

bool TerrainDeformation::Deform(float centerX, float centerZ, float radius, float deltaHeight, float minLimit, float maxLimit) {
    if (radius <= 0.001f || std::abs(deltaHeight) <= 0.0001f) return false;

    float scale = Config::World::ChunkScale;
    int minGX = (int)std::floor((centerX - radius) / scale) - 1;
    int maxGX = (int)std::ceil((centerX + radius) / scale) + 1;
    int minGZ = (int)std::floor((centerZ - radius) / scale) - 1;
    int maxGZ = (int)std::ceil((centerZ + radius) / scale) + 1;

    bool modified = false;

    for (int gz = minGZ; gz <= maxGZ; ++gz) {
        for (int gx = minGX; gx <= maxGX; ++gx) {
            float vx = (float)gx * scale;
            float vz = (float)gz * scale;

            float dist = glm::distance(glm::vec2(vx, vz), glm::vec2(centerX, centerZ));
            if (dist <= radius) {
                float t = 1.0f - (dist / radius);
                // Smoothstep radial curve: 3t^2 - 2t^3
                float falloff = t * t * (3.0f - 2.0f * t);
                float change = deltaHeight * falloff;

                int64_t key = MakeKey(gx, gz);
                float currentDelta = 0.0f;
                auto it = s_deltas.find(key);
                if (it != s_deltas.end()) {
                    currentDelta = it->second;
                }

                float newDelta = currentDelta + change;
                // Clamp total deformation delta to avoid runaway geometry spikes
                newDelta = glm::clamp(newDelta, minLimit, maxLimit);

                if (std::abs(newDelta) < 0.001f) {
                    if (it != s_deltas.end()) s_deltas.erase(it);
                } else {
                    s_deltas[key] = newDelta;
                }
                modified = true;
            }
        }
    }

    return modified;
}

void TerrainDeformation::Reset() {
    s_deltas.clear();
}

bool TerrainDeformation::HasModificationsInRange(float worldX, float worldZ, float range) {
    if (s_deltas.empty()) return false;
    float rangeSq = range * range;
    float scale = Config::World::ChunkScale;

    for (const auto& pair : s_deltas) {
        int gx, gz;
        UnpackKey(pair.first, gx, gz);
        float vx = (float)gx * scale;
        float vz = (float)gz * scale;
        float dx = vx - worldX;
        float dz = vz - worldZ;
        if (dx * dx + dz * dz <= rangeSq) {
            return true;
        }
    }
    return false;
}
