#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iostream>
#include "Config.h"
#include "utils/TinyJson.h"

struct SpeedPhase {
    float startProgress = 0.f;
    float endProgress = 1.f;
    float speedMultiplier = 1.0f;
};

struct AnimationSpeedCurve {
    float hitProgress = 0.55f;
    float totalRealTime = 1.0f;
    std::vector<SpeedPhase> phases;
};

inline std::unordered_map<std::string, AnimationSpeedCurve>& getAnimationSpeedCurves() {
    static std::unordered_map<std::string, AnimationSpeedCurve> sSpeedCurves;
    return sSpeedCurves;
}

inline void ensureSpeedCurvesLoaded() {
    static bool sSpeedCurvesLoaded = false;
    if (sSpeedCurvesLoaded) return;
    sSpeedCurvesLoaded = true;

    json::Value root = json::parseFile("assets/data/animation_curves.json");
    if (root.type != json::Type::Object) return;

    auto& curves = getAnimationSpeedCurves();

    for (const auto& kv : root.asObject()) {
        const std::string& clipName = kv.first;
        if (kv.second.type != json::Type::Object) continue;
        const auto& clipObj = kv.second.asObject();

        AnimationSpeedCurve curve;
        if (clipObj.count("hit_progress")) {
            curve.hitProgress = (float)clipObj.at("hit_progress").asDouble();
        }

        if (clipObj.count("speed_phases") && clipObj.at("speed_phases").type == json::Type::Array) {
            float totalRT = 0.f;
            for (const auto& elem : clipObj.at("speed_phases").asArray()) {
                if (elem.type != json::Type::Object) continue;
                const auto& phaseObj = elem.asObject();

                SpeedPhase phase;
                if (phaseObj.count("start")) phase.startProgress = (float)phaseObj.at("start").asDouble();
                if (phaseObj.count("end")) phase.endProgress = (float)phaseObj.at("end").asDouble();
                if (phaseObj.count("speed")) phase.speedMultiplier = (float)phaseObj.at("speed").asDouble();

                float dP = phase.endProgress - phase.startProgress;
                float spd = (phase.speedMultiplier > 0.001f) ? phase.speedMultiplier : 1.0f;
                if (dP > 0.f) totalRT += dP / spd;

                curve.phases.push_back(phase);
            }
            curve.totalRealTime = (totalRT > 0.0001f) ? totalRT : 1.0f;
            curves[clipName] = curve;
            std::cout << "[Animation] Loaded speed curve for clip '" << clipName << "' (hit_progress=" << curve.hitProgress << ", totalRT=" << curve.totalRealTime << ") with " << curve.phases.size() << " phase(s).\n";
        }
    }
}
