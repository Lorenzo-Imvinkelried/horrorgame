#include "AnimCore.h"
#include "utils/TinyJson.h"
#include <iostream>
#include <fstream>
#include <set>
#include <unordered_set>

bool AnimationClip::loadFromFile(const std::string& path) {
    static std::unordered_set<std::string> failedPaths;
    if (failedPaths.count(path)) {
        return false;
    }

    json::Value root = json::parseFile(path);
    if (root.type != json::Type::Object) {
        failedPaths.insert(path);
        std::cerr << "[AnimCore] Failed to load JSON (not an object): " << path << " (future warnings silenced)\n";
        return false;
    }
    
    const auto& rootObj = root.asObject();
    
    if (rootObj.count("name")) name = rootObj.at("name").asString();
    if (rootObj.count("duration")) duration = (float)rootObj.at("duration").asDouble();
    if (rootObj.count("isLoop")) isLoop = rootObj.at("isLoop").asBool();
    loopStart = rootObj.count("loopStart") ? (float)rootObj.at("loopStart").asDouble() : 0.0f;
    loopEnd = rootObj.count("loopEnd") ? (float)rootObj.at("loopEnd").asDouble() : -1.0f;

    // Load custom layer order (Z-Index) if specified
    layerOrder.clear();
    if (rootObj.count("layerOrder") && rootObj.at("layerOrder").type == json::Type::Array) {
        for (const auto& item : rootObj.at("layerOrder").asArray()) {
            if (item.type == json::Type::String) {
                layerOrder.push_back(item.asString());
            }
        }
    }
    
    // Load events
    if (rootObj.count("events") && rootObj.at("events").type == json::Type::Array) {
        for (const auto& evVal : rootObj.at("events").asArray()) {
            if (evVal.type == json::Type::Object) {
                const auto& evObj = evVal.asObject();
                AnimEvent ev;
                if (evObj.count("time")) ev.time = (float)evObj.at("time").asDouble();
                if (evObj.count("name")) ev.name = evObj.at("name").asString();
                events.push_back(ev);
            }
        }
    }
    
    // Inject fallback events for walk clip variants if none loaded
    if ((name == "walk" || name == "walk_2h" || name.find("walk") != std::string::npos) && events.empty()) {
        events.push_back({0.01f * duration, "footstep_l"});
        events.push_back({0.51f * duration, "footstep_r"});
    }
    
    // Load tracks
    if (rootObj.count("tracks") && rootObj.at("tracks").type == json::Type::Object) {
        const auto& tracksObj = rootObj.at("tracks").asObject();
        for (const auto& [partName, partVal] : tracksObj) {
            if (partVal.type != json::Type::Object) continue;
            const auto& partObj = partVal.asObject();
            
            // Positions
            if (partObj.count("position") && partObj.at("position").type == json::Type::Array) {
                Track<sf::Vector2f> t;
                for (const auto& kfVal : partObj.at("position").asArray()) {
                    if (kfVal.type == json::Type::Object) {
                        const auto& kfObj = kfVal.asObject();
                        Keyframe<sf::Vector2f> kf;
                        if (kfObj.count("time")) kf.time = (float)kfObj.at("time").asDouble();
                        float x = kfObj.count("x") ? (float)kfObj.at("x").asDouble() : 0.f;
                        float y = kfObj.count("y") ? (float)kfObj.at("y").asDouble() : 0.f;
                        kf.value = {x, y};
                        t.frames.push_back(kf);
                    }
                }
                if (!t.frames.empty()) positionTracks[partName] = t;
            }
            
            // Rotations
            if (partObj.count("rotation") && partObj.at("rotation").type == json::Type::Array) {
                Track<float> t;
                for (const auto& kfVal : partObj.at("rotation").asArray()) {
                    if (kfVal.type == json::Type::Object) {
                        const auto& kfObj = kfVal.asObject();
                        Keyframe<float> kf;
                        if (kfObj.count("time")) kf.time = (float)kfObj.at("time").asDouble();
                        if (kfObj.count("value")) kf.value = (float)kfObj.at("value").asDouble();
                        t.frames.push_back(kf);
                    }
                }
                if (!t.frames.empty()) rotationTracks[partName] = t;
            }
            
            // Scales
            if (partObj.count("scale") && partObj.at("scale").type == json::Type::Array) {
                Track<sf::Vector2f> t;
                for (const auto& kfVal : partObj.at("scale").asArray()) {
                    if (kfVal.type == json::Type::Object) {
                        const auto& kfObj = kfVal.asObject();
                        Keyframe<sf::Vector2f> kf;
                        if (kfObj.count("time")) kf.time = (float)kfObj.at("time").asDouble();
                        float x = kfObj.count("x") ? (float)kfObj.at("x").asDouble() : 1.f;
                        float y = kfObj.count("y") ? (float)kfObj.at("y").asDouble() : 1.f;
                        kf.value = {x, y};
                        t.frames.push_back(kf);
                    }
                }
                if (!t.frames.empty()) scaleTracks[partName] = t;
            }
        }
    }
    
    return true;
}

bool AnimationClip::saveToFile(const std::string& path) const {
    std::ofstream out(path);
    if (!out.is_open()) return false;

    out << "{\n";
    out << "  \"name\": \"" << name << "\",\n";
    out << "  \"duration\": " << duration << ",\n";
    out << "  \"isLoop\": " << (isLoop ? "true" : "false") << ",\n";
    if (loopStart > 0.001f) {
        out << "  \"loopStart\": " << loopStart << ",\n";
    }
    if (loopEnd > 0.001f && loopEnd < duration - 0.001f) {
        out << "  \"loopEnd\": " << loopEnd << ",\n";
    }
    out << "  \"events\": [\n";
    for (size_t i = 0; i < events.size(); ++i) {
        out << "    { \"time\": " << events[i].time << ", \"name\": \"" << events[i].name << "\" }"
            << (i + 1 < events.size() ? ",\n" : "\n");
    }
    out << "  ],\n";

    if (!layerOrder.empty()) {
        out << "  \"layerOrder\": [\n";
        for (size_t i = 0; i < layerOrder.size(); ++i) {
            out << "    \"" << layerOrder[i] << "\"" << (i + 1 < layerOrder.size() ? ",\n" : "\n");
        }
        out << "  ],\n";
    }

    out << "  \"tracks\": {\n";

    std::set<std::string> allParts;
    for (const auto& kv : positionTracks) allParts.insert(kv.first);
    for (const auto& kv : rotationTracks) allParts.insert(kv.first);
    for (const auto& kv : scaleTracks) allParts.insert(kv.first);

    size_t pIdx = 0;
    for (const auto& part : allParts) {
        out << "    \"" << part << "\": {\n";

        // Position
        out << "      \"position\": [\n";
        auto itPos = positionTracks.find(part);
        if (itPos != positionTracks.end()) {
            for (size_t k = 0; k < itPos->second.frames.size(); ++k) {
                const auto& kf = itPos->second.frames[k];
                out << "        { \"time\": " << kf.time << ", \"x\": " << kf.value.x << ", \"y\": " << kf.value.y << " }"
                    << (k + 1 < itPos->second.frames.size() ? ",\n" : "\n");
            }
        }
        out << "      ],\n";

        // Rotation
        out << "      \"rotation\": [\n";
        auto itRot = rotationTracks.find(part);
        if (itRot != rotationTracks.end()) {
            for (size_t k = 0; k < itRot->second.frames.size(); ++k) {
                const auto& kf = itRot->second.frames[k];
                out << "        { \"time\": " << kf.time << ", \"value\": " << kf.value << " }"
                    << (k + 1 < itRot->second.frames.size() ? ",\n" : "\n");
            }
        }
        out << "      ],\n";

        // Scale
        out << "      \"scale\": [\n";
        auto itScl = scaleTracks.find(part);
        if (itScl != scaleTracks.end()) {
            for (size_t k = 0; k < itScl->second.frames.size(); ++k) {
                const auto& kf = itScl->second.frames[k];
                out << "        { \"time\": " << kf.time << ", \"x\": " << kf.value.x << ", \"y\": " << kf.value.y << " }"
                    << (k + 1 < itScl->second.frames.size() ? ",\n" : "\n");
            }
        }
        out << "      ]\n";

        pIdx++;
        out << "    }" << (pIdx < allParts.size() ? ",\n" : "\n");
    }

    out << "  }\n";
    out << "}\n";
    return true;
}

bool SkeletonData::loadFromFile(const std::string& path) {
    json::Value root = json::parseFile(path);
    if (root.type != json::Type::Object) {
        return false;
    }
    
    const auto& skObj = root.asObject();
    
    auto parseOffset = [](const json::Value& val) -> std::optional<sf::Vector2f> {
        if (val.type == json::Type::Array) {
            const auto& arr = val.asArray();
            if (arr.size() >= 2) {
                return sf::Vector2f{(float)arr[0].asDouble(), (float)arr[1].asDouble()};
            }
        }
        return std::nullopt;
    };
    
    const json::Object* targetObj = &skObj;
    if (skObj.count("animConfig") && skObj.at("animConfig").type == json::Type::Object) {
        targetObj = &skObj.at("animConfig").asObject();
    }
    
    if (targetObj->count("headOffset")) headOffset = parseOffset(targetObj->at("headOffset"));
    if (targetObj->count("handLOffset")) handLOffset = parseOffset(targetObj->at("handLOffset"));
    if (targetObj->count("handROffset")) handROffset = parseOffset(targetObj->at("handROffset"));
    if (targetObj->count("footLOffset")) footLOffset = parseOffset(targetObj->at("footLOffset"));
    if (targetObj->count("footROffset")) footROffset = parseOffset(targetObj->at("footROffset"));
    if (targetObj->count("weaponOffset")) weaponOffset = parseOffset(targetObj->at("weaponOffset"));
    if (targetObj->count("weaponSecondaryOffset")) weaponSecondaryOffset = parseOffset(targetObj->at("weaponSecondaryOffset"));
    if (targetObj->count("weaponTwoHandedOffset")) weaponTwoHandedOffset = parseOffset(targetObj->at("weaponTwoHandedOffset")); // [NEW]
    
    if (targetObj->count("groundOffsetY")) {
        groundOffsetY = (float)targetObj->at("groundOffsetY").asDouble();
    }
    if (targetObj->count("stride")) {
        stride = (float)targetObj->at("stride").asDouble();
    }

    // Parse parts array if it exists
    if (targetObj->count("parts") && targetObj->at("parts").type == json::Type::Array) {
        for (const auto& partVal : targetObj->at("parts").asArray()) {
            if (partVal.type == json::Type::String) {
                parts.push_back(partVal.asString());
            }
        }
    }

    // Parse offsets object if it exists
    if (targetObj->count("offsets") && targetObj->at("offsets").type == json::Type::Object) {
        const auto& offsetsObj = targetObj->at("offsets").asObject();
        for (const auto& [partName, partVal] : offsetsObj) {
            auto parsedOff = parseOffset(partVal);
            if (parsedOff.has_value()) {
                offsets[partName] = parsedOff.value();
            }
        }
    }

    // Populate offsets map from legacy fields if not already populated
    if (headOffset.has_value() && !offsets.count("head")) offsets["head"] = headOffset.value();
    if (handLOffset.has_value() && !offsets.count("hand_l")) offsets["hand_l"] = handLOffset.value();
    if (handROffset.has_value() && !offsets.count("hand_r")) offsets["hand_r"] = handROffset.value();
    if (footLOffset.has_value() && !offsets.count("foot_l")) offsets["foot_l"] = footLOffset.value();
    if (footROffset.has_value() && !offsets.count("foot_r")) offsets["foot_r"] = footROffset.value();
    if (!offsets.count("body")) offsets["body"] = {0.f, 0.f};
    
    return true;
}

