#include "AnimatorStudio.h"
#include "Config.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <cstdint>
#include <set>
#include <sstream>
#include <iomanip>
#include <filesystem>

AnimatorStudio::AnimatorStudio(ResourceManager& res)
    : mRes(res)
{
    mAnim.setEnableProceduralIK(mEnableIK);
    mAnim.setEnableTwoHandGripIK(mEnableGripIK);
    mAnim.setEnableIdleLayer(false);
    initClips();
    loadCurrentEntity();
    setWeaponType(StudioWeaponType::TwoHandedSword);
}

void AnimatorStudio::initClips() {
    mClips.clear();

    if (mEntityType == "player") {
        mClips.push_back({ "Attack 2H (v3.2)", "assets/textures/player/attack_2h_v3_2.json", true });
        mClips.push_back({ "Idle",            "assets/textures/player/idle.json",            false });
        mClips.push_back({ "Walk",            "assets/textures/player/walk.json",            false });
        mClips.push_back({ "Idle 2H",         "assets/textures/player/idle_2h.json",         true });
        mClips.push_back({ "Walk 2H",         "assets/textures/player/walk_2h.json",         true });
        mClips.push_back({ "Attack Right",    "assets/textures/player/attack_r.json",        false });
        mClips.push_back({ "Attack Left",     "assets/textures/player/attack_l.json",        false });
        mClips.push_back({ "Attack Dual",     "assets/textures/player/attack_dual.json",     false });
        mClips.push_back({ "Attack 2H (v3)",  "assets/textures/player/attack_2h_v3.json",    true });
        mClips.push_back({ "Attack 2H (Base)","assets/textures/player/attack_2h.json",       true });
        mClips.push_back({ "Attack (Single)", "assets/textures/player/attack.json",          false });
    } else if (mEntityType == "goblin") {
        mClips.push_back({ "Idle",   "assets/textures/mobs/goblin/idle.json",   false });
        mClips.push_back({ "Walk",   "assets/textures/mobs/goblin/walk.json",   false });
        mClips.push_back({ "Attack", "assets/textures/mobs/goblin/attack.json", false });
    } else if (mEntityType == "mob_grande_1") {
        mClips.push_back({ "Idle",   "assets/textures/mobs/mob_grande_1/idle.json",   false });
        mClips.push_back({ "Walk",   "assets/textures/mobs/mob_grande_1/walk.json",   false });
        mClips.push_back({ "Attack", "assets/textures/mobs/mob_grande_1/attack.json", false });
    }

    // Dynamic scanning for any custom created JSON clips
    std::string dirPath = (mEntityType == "player") 
        ? "assets/textures/player" 
        : "assets/textures/mobs/" + mEntityType;

    std::error_code ec;
    if (std::filesystem::exists(dirPath, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(dirPath, ec)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string filename = entry.path().filename().string();
                if (filename == "esqueleto.json") continue;

                std::string fullPath = entry.path().generic_string();
                
                bool found = false;
                for (const auto& c : mClips) {
                    if (c.filePath == fullPath) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::string stem = entry.path().stem().string();
                    std::string dispName = stem;
                    for (size_t i = 0; i < dispName.size(); ++i) {
                        if (i == 0 || dispName[i - 1] == ' ') {
                            dispName[i] = std::toupper(dispName[i]);
                        } else if (dispName[i] == '_') {
                            dispName[i] = ' ';
                        }
                    }
                    bool is2H = (stem.find("2h") != std::string::npos);
                    mClips.push_back({ dispName, fullPath, is2H });
                }
            }
        }
    }

    if (!mClips.empty()) {
        selectClip(0);
    }
}

bool AnimatorStudio::createNewClip(const std::string& clipName, int baseClipIndex, bool isTwoHanded, bool includeWeapon) {
    if (clipName.empty()) {
        mStatusMessage = "[ERROR] El nombre de la animacion no puede estar vacio.";
        return false;
    }

    // Sanitize filename
    std::string safeName = clipName;
    for (char& c : safeName) {
        if (c == ' ' || c == '-' || c == '.') c = '_';
        else c = std::tolower(c);
    }
    safeName.erase(std::remove_if(safeName.begin(), safeName.end(), [](char c) {
        return !(std::isalnum(c) || c == '_');
    }), safeName.end());

    if (safeName.empty()) {
        safeName = "nueva_animacion";
    }

    std::string dirPath = (mEntityType == "player") 
        ? "assets/textures/player/" 
        : "assets/textures/mobs/" + mEntityType + "/";

    std::string targetPath = dirPath + safeName + ".json";

    AnimationClip newClip;
    newClip.name = clipName;

    if (baseClipIndex >= 0 && baseClipIndex < static_cast<int>(mClips.size())) {
        const auto& baseInfo = mClips[baseClipIndex];
        const AnimationClip* base = mRes.getAnimationClip(baseInfo.filePath);
        if (base) {
            newClip = *base; // Copy all keyframes & tracks
            newClip.name = clipName;
        } else {
            newClip.loadFromFile(baseInfo.filePath);
            newClip.name = clipName;
        }
    } else {
        newClip.duration = 1.0f;
        newClip.isLoop = true;

        // Populate frame 0 (neutral pose) from the base Idle clip so all parts match the visual character rest pose perfectly
        std::string idlePath = (mEntityType == "player") ? "assets/textures/player/idle.json" : "assets/textures/mobs/" + mEntityType + "/idle.json";
        const AnimationClip* idleClip = mRes.getAnimationClip(idlePath);
        if (idleClip) {
            for (const auto& [nodeName, track] : idleClip->positionTracks) {
                sf::Vector2f pos0 = track.evaluate(0.0f);
                newClip.positionTracks[nodeName].insertOrUpdateKeyframe(0.0f, pos0);
            }
            for (const auto& [nodeName, track] : idleClip->rotationTracks) {
                float rot0 = track.evaluate(0.0f);
                newClip.rotationTracks[nodeName].insertOrUpdateKeyframe(0.0f, rot0);
            }
        } else {
            newClip.positionTracks["head"].insertOrUpdateKeyframe(0.0f, { -10.0f, -28.0f });
            newClip.positionTracks["body"].insertOrUpdateKeyframe(0.0f, { 0.0f, 0.0f });
            newClip.positionTracks["hand_l"].insertOrUpdateKeyframe(0.0f, { -12.0f, -5.0f });
            newClip.positionTracks["hand_r"].insertOrUpdateKeyframe(0.0f, { 12.0f, 5.0f });
            newClip.positionTracks["foot_l"].insertOrUpdateKeyframe(0.0f, { -6.0f, 25.0f });
            newClip.positionTracks["foot_r"].insertOrUpdateKeyframe(0.0f, { 6.0f, 35.0f });
            for (const auto& node : mAnim.getNodes()) {
                newClip.rotationTracks[node.name].insertOrUpdateKeyframe(0.0f, 0.0f);
            }
        }
    }

    // If user explicitly chose clean animation without weapons
    if (!includeWeapon) {
        newClip.positionTracks.erase("weapon");
        newClip.rotationTracks.erase("weapon");
        newClip.scaleTracks.erase("weapon");
    }

    if (!newClip.saveToFile(targetPath)) {
        mStatusMessage = "[ERROR] Fallo al crear archivo JSON en: " + targetPath;
        std::cerr << "[AnimatorStudio] " << mStatusMessage << "\n";
        return false;
    }

    std::cout << "[AnimatorStudio] Nueva animacion creada: " << targetPath << " (Base: " 
              << (baseClipIndex >= 0 && baseClipIndex < static_cast<int>(mClips.size()) ? mClips[baseClipIndex].displayName : "En blanco") << ")\n";

    // Reload cache & clips list
    mRes.clearAnimationCache();
    initClips();

    if (!includeWeapon) {
        setWeaponType(StudioWeaponType::None);
    }

    // Find and select the newly created clip
    for (size_t i = 0; i < mClips.size(); ++i) {
        if (mClips[i].filePath == targetPath) {
            selectClip(i);
            if (!includeWeapon) {
                setWeaponType(StudioWeaponType::None);
            }
            mStatusMessage = "[NUEVA ANIMACION] Creada con exito: " + mClips[i].displayName;
            return true;
        }
    }

    return true;
}

bool AnimatorStudio::deleteCurrentClip() {
    if (mActiveClipIndex >= mClips.size()) return false;
    const std::string& path = mClips[mActiveClipIndex].filePath;
    std::string clipName = mClips[mActiveClipIndex].displayName;

    std::error_code ec;
    if (std::filesystem::exists(path, ec)) {
        std::filesystem::remove(path, ec);
        std::cout << "[AnimatorStudio] Archivo eliminado de disco: " << path << "\n";
    }

    mRes.clearAnimationCache();
    initClips();
    if (!mClips.empty()) {
        selectClip(0);
    }
    mStatusMessage = "[ELIMINADO] Clip '" + clipName + "' eliminado con exito.";
    return true;
}

void AnimatorStudio::loadCurrentEntity() {
    std::vector<std::string> partNames = { "foot_l", "hand_l", "body", "head", "foot_r", "hand_r" };
    mAnim.loadDynamicParts(mRes, mEntityType, partNames);

    std::string skelPath = (mEntityType == "player") 
        ? "assets/textures/player/esqueleto.json"
        : "assets/textures/mobs/" + mEntityType + "/parts/esqueleto.json";
    
    mAnim.loadSkeleton(mRes, skelPath);

    std::string idlePath = (mEntityType == "player") 
        ? "assets/textures/player/idle.json"
        : "assets/textures/mobs/" + mEntityType + "/idle.json";
    mAnim.setBaseIdleClip(mRes.getAnimationClip(idlePath));
    mAnim.setEnableIdleLayer(false);
    
    float scale = (mEntityType == "player") ? cfg::Player::SCALE_Y : 2.5f;
    mAnim.setScale({ scale, scale });

    if (mEntityType == "player") {
        updateWeaponVisuals();
    }
}

void AnimatorStudio::selectClip(size_t index) {
    if (index >= mClips.size()) return;
    mActiveClipIndex = index;
    const auto& info = mClips[index];

    // Const cast allows editing keyframes directly in the studio
    mActiveClip = const_cast<AnimationClip*>(mRes.getAnimationClip(info.filePath));
    if (mActiveClip) {
        if (info.isTwoHanded && mWeaponType != StudioWeaponType::TwoHandedSword && mWeaponType != StudioWeaponType::None) {
            setWeaponType(StudioWeaponType::TwoHandedSword);
        } else if (!info.isTwoHanded && mWeaponType == StudioWeaponType::TwoHandedSword) {
            setWeaponType(StudioWeaponType::None);
        }
        mAnim.playAnimation(mActiveClip);
    }
    mCurrentTime = 0.f;
    mAnim.setAnimTimer(0.f);
    mTrailPoints.clear();
    mStatusMessage = "Clip seleccionado: " + info.displayName;
}

void AnimatorStudio::selectClipByName(const std::string& name) {
    for (size_t i = 0; i < mClips.size(); ++i) {
        if (mClips[i].displayName == name || mClips[i].filePath == name) {
            selectClip(i);
            return;
        }
    }
}

void AnimatorStudio::reloadAllClips() {
    mRes.clearAnimationCache();
    initClips();
    loadCurrentEntity();
    if (mActiveClipIndex < mClips.size()) {
        selectClip(mActiveClipIndex);
    }
    mStatusMessage = "Recargados todos los archivos JSON desde disco.";
}

bool AnimatorStudio::saveCurrentClip() {
    if (!mActiveClip || mActiveClipIndex >= mClips.size()) return false;
    const std::string& path = mClips[mActiveClipIndex].filePath;
    if (mActiveClip->saveToFile(path)) {
        mStatusMessage = "[GUARDADO] Clip guardado con exito en: " + path;
        std::cout << "[AnimatorStudio] " << mStatusMessage << "\n";
        return true;
    } else {
        mStatusMessage = "[ERROR] No se pudo guardar el archivo: " + path;
        std::cerr << "[AnimatorStudio] " << mStatusMessage << "\n";
        return false;
    }
}

void AnimatorStudio::setTime(float t) {
    float dur = getDuration();
    if (dur <= 0.f) dur = 1.0f;
    mCurrentTime = std::clamp(t, 0.f, dur);
    mAnim.setAnimTimer(mCurrentTime);
    mAnim.update(sf::Time::Zero, false, { 0.f, 0.f }, mFacingDir, mSpeedMultiplier, nullptr);
}

float AnimatorStudio::getDuration() const {
    if (mActiveClip) return mActiveClip->duration;
    return 1.0f;
}

void AnimatorStudio::stepForward(float dt) {
    float dur = getDuration();
    mCurrentTime += dt;
    if (mCurrentTime > dur) {
        mCurrentTime = mIsLooping ? std::fmod(mCurrentTime, dur) : dur;
    }
    setTime(mCurrentTime);
}

void AnimatorStudio::stepBackward(float dt) {
    float dur = getDuration();
    mCurrentTime -= dt;
    if (mCurrentTime < 0.f) {
        mCurrentTime = mIsLooping ? (dur + std::fmod(mCurrentTime, dur)) : 0.f;
    }
    setTime(mCurrentTime);
}

std::string AnimatorStudio::getHoveredNode(sf::Vector2f worldMousePos) const {
    // 1. Check weapon bounds first (so clicking on sword selects "weapon")
    if (mWeaponType != StudioWeaponType::None) {
        sf::FloatRect wepBounds = mAnim.getNodeGlobalBounds("weapon");
        float pad = 4.f;
        if (worldMousePos.x >= wepBounds.position.x - pad && worldMousePos.x <= wepBounds.position.x + wepBounds.size.x + pad &&
            worldMousePos.y >= wepBounds.position.y - pad && worldMousePos.y <= wepBounds.position.y + wepBounds.size.y + pad) {
            return "weapon";
        }
    }

    // 2. Check body nodes
    const auto& nodes = mAnim.getNodes();
    for (int i = static_cast<int>(nodes.size()) - 1; i >= 0; --i) {
        const auto& node = nodes[i];
        float minX = std::min({ node.quad[0].position.x, node.quad[1].position.x, node.quad[2].position.x, node.quad[4].position.x });
        float maxX = std::max({ node.quad[0].position.x, node.quad[1].position.x, node.quad[2].position.x, node.quad[4].position.x });
        float minY = std::min({ node.quad[0].position.y, node.quad[1].position.y, node.quad[2].position.y, node.quad[4].position.y });
        float maxY = std::max({ node.quad[0].position.y, node.quad[1].position.y, node.quad[2].position.y, node.quad[4].position.y });

        // Add small padding for easier clicking
        float pad = 4.f;
        if (worldMousePos.x >= minX - pad && worldMousePos.x <= maxX + pad &&
            worldMousePos.y >= minY - pad && worldMousePos.y <= maxY + pad) {
            return node.name;
        }
    }
    return "";
}

void AnimatorStudio::moveSelectedNode(sf::Vector2f worldDelta) {
    if (mSelectedNode.empty() || !mActiveClip) return;

    pause(); // Auto-pause when editing

    sf::Vector2f scale = mAnim.getBaseScale();
    float scaleX = (scale.x != 0.f) ? (scale.x * -mFacingDir) : 1.f;
    float scaleY = (scale.y != 0.f) ? scale.y : 1.f;

    sf::Vector2f localDelta = { worldDelta.x / scaleX, worldDelta.y / scaleY };

    auto& posTrack = mActiveClip->positionTracks[mSelectedNode];
    sf::Vector2f currentVal;
    if (posTrack.frames.empty()) {
        if (mSelectedNode == "weapon") {
            sf::Vector2f wepPos = mAnim.getNodePosition("weapon");
            currentVal = { wepPos.x / scaleX, wepPos.y / scaleY };
        } else {
            auto itNode = mAnim.getNodeMap().find(mSelectedNode);
            if (itNode != mAnim.getNodeMap().end() && itNode->second < mAnim.getNodes().size()) {
                currentVal = mAnim.getNodes()[itNode->second].customRestPos;
            } else {
                currentVal = posTrack.evaluate(mCurrentTime);
            }
        }
    } else {
        currentVal = posTrack.evaluate(mCurrentTime);
    }
    sf::Vector2f newVal = currentVal + localDelta;

    posTrack.insertOrUpdateKeyframe(mCurrentTime, newVal);

    setTime(mCurrentTime);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "[Keyframe] " << mSelectedNode << " Pos -> (" << newVal.x << ", " << newVal.y << ") en t=" << mCurrentTime << "s";
    mStatusMessage = ss.str();
}

void AnimatorStudio::rotateSelectedNode(float deltaDegrees) {
    if (mSelectedNode.empty() || !mActiveClip) return;

    pause(); // Auto-pause when editing

    auto& rotTrack = mActiveClip->rotationTracks[mSelectedNode];
    float currentVal;
    if (rotTrack.frames.empty()) {
        currentVal = mAnim.getNodeRotation(mSelectedNode);
    } else {
        currentVal = rotTrack.evaluate(mCurrentTime);
    }
    float newVal = currentVal + deltaDegrees * -mFacingDir;

    while (newVal > 180.f) newVal -= 360.f;
    while (newVal < -180.f) newVal += 360.f;

    rotTrack.insertOrUpdateKeyframe(mCurrentTime, newVal);

    setTime(mCurrentTime);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "[Keyframe] " << mSelectedNode << " Rot -> " << newVal << " deg en t=" << mCurrentTime << "s";
    mStatusMessage = ss.str();
}

void AnimatorStudio::setNodeAbsoluteRotation(float absoluteDegrees) {
    if (mSelectedNode.empty() || !mActiveClip) return;

    pause();

    while (absoluteDegrees > 180.f) absoluteDegrees -= 360.f;
    while (absoluteDegrees < -180.f) absoluteDegrees += 360.f;

    auto& rotTrack = mActiveClip->rotationTracks[mSelectedNode];
    rotTrack.insertOrUpdateKeyframe(mCurrentTime, absoluteDegrees);

    setTime(mCurrentTime);

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1);
    ss << "[Keyframe] " << mSelectedNode << " Rot -> " << absoluteDegrees << " deg en t=" << mCurrentTime << "s";
    mStatusMessage = ss.str();
}

void AnimatorStudio::insertKeyframeAtCurrentTime(const std::string& nodeName) {
    if (nodeName.empty() || !mActiveClip) return;

    auto& posTrack = mActiveClip->positionTracks[nodeName];
    sf::Vector2f currentPos;
    if (posTrack.frames.empty()) {
        if (nodeName == "weapon") {
            sf::Vector2f wepPos = mAnim.getNodePosition("weapon");
            sf::Vector2f scale = mAnim.getBaseScale();
            float scaleX = (scale.x != 0.f) ? (scale.x * -mFacingDir) : 1.f;
            float scaleY = (scale.y != 0.f) ? scale.y : 1.f;
            currentPos = { wepPos.x / scaleX, wepPos.y / scaleY };
        } else {
            auto itNode = mAnim.getNodeMap().find(nodeName);
            if (itNode != mAnim.getNodeMap().end() && itNode->second < mAnim.getNodes().size()) {
                currentPos = mAnim.getNodes()[itNode->second].customRestPos;
            } else {
                currentPos = posTrack.evaluate(mCurrentTime);
            }
        }
    } else {
        currentPos = posTrack.evaluate(mCurrentTime);
    }
    posTrack.insertOrUpdateKeyframe(mCurrentTime, currentPos);

    auto& rotTrack = mActiveClip->rotationTracks[nodeName];
    float currentRot;
    if (rotTrack.frames.empty()) {
        currentRot = mAnim.getNodeRotation(nodeName);
    } else {
        currentRot = rotTrack.evaluate(mCurrentTime);
    }
    rotTrack.insertOrUpdateKeyframe(mCurrentTime, currentRot);

    mStatusMessage = "Insertado Keyframe para '" + nodeName + "' en t=" + std::to_string(mCurrentTime) + "s";
}

void AnimatorStudio::removeKeyframeAtCurrentTime(const std::string& nodeName) {
    if (nodeName.empty() || !mActiveClip) return;

    bool removedPos = false;
    auto itPos = mActiveClip->positionTracks.find(nodeName);
    if (itPos != mActiveClip->positionTracks.end()) {
        removedPos = itPos->second.removeKeyframeNear(mCurrentTime, 0.03f);
    }

    bool removedRot = false;
    auto itRot = mActiveClip->rotationTracks.find(nodeName);
    if (itRot != mActiveClip->rotationTracks.end()) {
        removedRot = itRot->second.removeKeyframeNear(mCurrentTime, 0.03f);
    }

    if (removedPos || removedRot) {
        setTime(mCurrentTime);
        mStatusMessage = "Eliminado Keyframe de '" + nodeName + "' en t=" + std::to_string(mCurrentTime) + "s";
    } else {
        mStatusMessage = "No habia Keyframe cercano para eliminar en t=" + std::to_string(mCurrentTime) + "s";
    }
}

void AnimatorStudio::clearNodeTracks(const std::string& nodeName) {
    if (nodeName.empty() || !mActiveClip) return;

    mActiveClip->positionTracks.erase(nodeName);
    mActiveClip->rotationTracks.erase(nodeName);
    mActiveClip->scaleTracks.erase(nodeName);

    if (nodeName == "weapon") {
        setWeaponType(StudioWeaponType::None);
    }

    setTime(mCurrentTime);

    mStatusMessage = "[PISTAS BORRADAS] Se eliminaron todas las pistas de '" + nodeName + "' en este clip.";
    std::cout << "[AnimatorStudio] " << mStatusMessage << "\n";
}

const std::vector<std::string>& AnimatorStudio::getLayerOrder() {
    static std::vector<std::string> emptyOrder;
    if (!mActiveClip) return emptyOrder;
    if (mActiveClip->layerOrder.empty()) {
        // Initialize default layer order from nodes
        for (const auto& node : mAnim.getNodes()) {
            mActiveClip->layerOrder.push_back(node.name);
        }
    }
    return mActiveClip->layerOrder;
}

void AnimatorStudio::moveSelectedNodeLayerUp() {
    if (!mActiveClip || mSelectedNode.empty()) return;
    auto& order = mActiveClip->layerOrder;
    if (order.empty()) getLayerOrder();
    auto it = std::find(order.begin(), order.end(), mSelectedNode);
    if (it != order.end() && it + 1 != order.end()) {
        std::iter_swap(it, it + 1);
        mStatusMessage = "[CAPAS] '" + mSelectedNode + "' movido hacia el FRENTE.";
        std::cout << "[AnimatorStudio] " << mStatusMessage << "\n";
    }
}

void AnimatorStudio::moveSelectedNodeLayerDown() {
    if (!mActiveClip || mSelectedNode.empty()) return;
    auto& order = mActiveClip->layerOrder;
    if (order.empty()) getLayerOrder();
    auto it = std::find(order.begin(), order.end(), mSelectedNode);
    if (it != order.end() && it != order.begin()) {
        std::iter_swap(it, it - 1);
        mStatusMessage = "[CAPAS] '" + mSelectedNode + "' movido hacia el FONDO.";
        std::cout << "[AnimatorStudio] " << mStatusMessage << "\n";
    }
}

void AnimatorStudio::resetLayerOrder() {
    if (!mActiveClip) return;
    mActiveClip->layerOrder.clear();
    mStatusMessage = "[CAPAS] Orden de capas reseteado al estandar 2.5D.";
    std::cout << "[AnimatorStudio] " << mStatusMessage << "\n";
}

std::vector<float> AnimatorStudio::getKeyframeTimes(const std::string& nodeName) const {
    std::set<float> times;
    if (!mActiveClip || nodeName.empty()) return {};

    auto itPos = mActiveClip->positionTracks.find(nodeName);
    if (itPos != mActiveClip->positionTracks.end()) {
        for (const auto& kf : itPos->second.frames) times.insert(kf.time);
    }

    auto itRot = mActiveClip->rotationTracks.find(nodeName);
    if (itRot != mActiveClip->rotationTracks.end()) {
        for (const auto& kf : itRot->second.frames) times.insert(kf.time);
    }

    return std::vector<float>(times.begin(), times.end());
}

void AnimatorStudio::selectKeyframe(float time) {
    pause();
    setTime(time);
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "[Keyframe] '" << mSelectedNode << "' seleccionado en t=" << time << "s";
    mStatusMessage = ss.str();
}

void AnimatorStudio::jumpToPrevKeyframe() {
    std::vector<float> times = getKeyframeTimes(mSelectedNode);
    if (times.empty()) return;

    float targetTime = times.front();
    bool found = false;
    for (int i = static_cast<int>(times.size()) - 1; i >= 0; --i) {
        if (times[i] < mCurrentTime - 0.005f) {
            targetTime = times[i];
            found = true;
            break;
        }
    }
    if (!found) {
        targetTime = times.back();
    }
    selectKeyframe(targetTime);
}

void AnimatorStudio::jumpToNextKeyframe() {
    std::vector<float> times = getKeyframeTimes(mSelectedNode);
    if (times.empty()) return;

    float targetTime = times.back();
    bool found = false;
    for (size_t i = 0; i < times.size(); ++i) {
        if (times[i] > mCurrentTime + 0.005f) {
            targetTime = times[i];
            found = true;
            break;
        }
    }
    if (!found) {
        targetTime = times.front();
    }
    selectKeyframe(targetTime);
}

bool AnimatorStudio::isAtKeyframe(const std::string& nodeName, float* outExactTime, float tolerance) const {
    std::vector<float> times = getKeyframeTimes(nodeName);
    for (float kt : times) {
        if (std::abs(mCurrentTime - kt) <= tolerance) {
            if (outExactTime) *outExactTime = kt;
            return true;
        }
    }
    return false;
}

float AnimatorStudio::findNearestKeyframeTime(float queryTime, float maxDistance) const {
    std::vector<float> times = getKeyframeTimes(mSelectedNode);
    float bestTime = queryTime;
    float bestDist = maxDistance;
    for (float kt : times) {
        float d = std::abs(kt - queryTime);
        if (d < bestDist) {
            bestDist = d;
            bestTime = kt;
        }
    }
    return bestTime;
}

void AnimatorStudio::setLoopStartAtCurrentTime() {
    if (!mActiveClip) return;
    float curT = std::round(mCurrentTime * 100.f) / 100.f;
    float dur = getDuration();
    if (curT < 0.f) curT = 0.f;
    if (curT >= dur) curT = std::max(0.f, dur - 0.05f);
    mActiveClip->loopStart = curT;
    if (mActiveClip->loopEnd > 0.f && mActiveClip->loopEnd <= curT) {
        mActiveClip->loopEnd = dur;
    }
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "[LOOP] Inicio de Loop fijado en t=" << curT << "s (Fin: " << mActiveClip->getEffectiveLoopEnd() << "s)";
    mStatusMessage = ss.str();
    std::cout << "[AnimatorStudio] " << mStatusMessage << "\n";
}

void AnimatorStudio::setLoopEndAtCurrentTime() {
    if (!mActiveClip) return;
    float curT = std::round(mCurrentTime * 100.f) / 100.f;
    float dur = getDuration();
    float lStart = mActiveClip->getEffectiveLoopStart();
    if (curT <= lStart) curT = std::min(dur, lStart + 0.05f);
    if (curT > dur) curT = dur;
    mActiveClip->loopEnd = curT;
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "[LOOP] Fin de Loop fijado en t=" << curT << "s (Inicio: " << lStart << "s)";
    mStatusMessage = ss.str();
    std::cout << "[AnimatorStudio] " << mStatusMessage << "\n";
}

void AnimatorStudio::resetLoopRange() {
    if (!mActiveClip) return;
    mActiveClip->loopStart = 0.f;
    mActiveClip->loopEnd = -1.f;
    mStatusMessage = "[LOOP] Rango de loop reseteado a toda la animacion (0.0s -> " + std::to_string(getDuration()) + "s)";
    std::cout << "[AnimatorStudio] " << mStatusMessage << "\n";
}

float AnimatorStudio::getLoopStart() const {
    if (!mActiveClip) return 0.f;
    return mActiveClip->getEffectiveLoopStart();
}

float AnimatorStudio::getLoopEnd() const {
    if (!mActiveClip) return getDuration();
    return mActiveClip->getEffectiveLoopEnd();
}

void AnimatorStudio::setWeaponType(StudioWeaponType type) {
    mWeaponType = type;
    updateWeaponVisuals();
}

void AnimatorStudio::setEntityType(const std::string& mobType) {
    if (mEntityType == mobType) return;
    mEntityType = mobType;
    initClips();
    loadCurrentEntity();
}

void AnimatorStudio::setWeaponOffset(sf::Vector2f offset) {
    mWeaponOffset = offset;
    updateWeaponVisuals();
}

void AnimatorStudio::setWeapon2HOffset(sf::Vector2f offset) {
    mWeapon2HOffset = offset;
    updateWeaponVisuals();
}

void AnimatorStudio::updateWeaponVisuals() {
    if (mEntityType != "player") {
        mAnim.setWeaponVisuals(nullptr, nullptr, {}, {}, ItemQuality::Common, {0.f, 0.f});
        mAnim.setSecondaryWeaponVisuals(nullptr, nullptr, {}, {}, ItemQuality::Common, {0.f, 0.f});
        return;
    }

    try {
        const sf::Texture& weaponBase = mRes.getTexture("assets/items/weapons/weapons-base.png");
        const sf::Texture& weaponLayout = mRes.getTexture("assets/items/weapons/weapons_layout.png");
        const sf::Texture& sword32 = mRes.getTexture("assets/items/weapons/32x32_sword.png");
        const sf::Texture& shieldTex = mRes.getTexture("assets/items/shields/16x16x10_escudos.png");

        switch (mWeaponType) {
            case StudioWeaponType::None:
                mAnim.setWeaponVisuals(nullptr, nullptr, {}, {}, ItemQuality::Common, {0.f, 0.f});
                mAnim.setSecondaryWeaponVisuals(nullptr, nullptr, {}, {}, ItemQuality::Common, {0.f, 0.f});
                break;

            case StudioWeaponType::OneHandedSword:
                mAnim.setWeaponVisuals(&weaponBase, &weaponLayout, 
                                       sf::IntRect({0, 0}, {16, 16}), sf::IntRect({0, 0}, {16, 16}), 
                                       ItemQuality::Rare, mWeaponOffset, false, 0, false);
                mAnim.setSecondaryWeaponVisuals(nullptr, nullptr, {}, {}, ItemQuality::Common, {0.f, 0.f});
                break;

            case StudioWeaponType::TwoHandedSword:
                mAnim.setWeaponVisuals(&sword32, &sword32, 
                                       sf::IntRect({0, 0}, {32, 32}), sf::IntRect({0, 0}, {0, 0}), 
                                       ItemQuality::Legendary, mWeapon2HOffset, true, 0, false);
                mAnim.setSecondaryWeaponVisuals(nullptr, nullptr, {}, {}, ItemQuality::Common, {0.f, 0.f});
                break;

            case StudioWeaponType::DualWield:
                mAnim.setWeaponVisuals(&weaponBase, &weaponLayout, 
                                       sf::IntRect({0, 0}, {16, 16}), sf::IntRect({0, 0}, {16, 16}), 
                                       ItemQuality::Rare, mWeaponOffset, false, 0, false);
                mAnim.setSecondaryWeaponVisuals(&weaponBase, &weaponLayout, 
                                               sf::IntRect({16, 0}, {16, 16}), sf::IntRect({16, 0}, {16, 16}), 
                                               ItemQuality::Epic, mWeaponOffset, 0, false);
                break;

            case StudioWeaponType::SwordAndShield:
                mAnim.setWeaponVisuals(&weaponBase, &weaponLayout, 
                                       sf::IntRect({0, 0}, {16, 16}), sf::IntRect({0, 0}, {16, 16}), 
                                       ItemQuality::Rare, mWeaponOffset, false, 0, false);
                mAnim.setSecondaryWeaponVisuals(&shieldTex, &shieldTex, 
                                               sf::IntRect({0, 0}, {16, 16}), sf::IntRect({0, 0}, {0, 0}), 
                                               ItemQuality::Common, {0.f, 0.f}, 0, true);
                break;

            case StudioWeaponType::ShieldAndSword:
                mAnim.setWeaponVisuals(&shieldTex, &shieldTex, 
                                       sf::IntRect({0, 0}, {16, 16}), sf::IntRect({0, 0}, {0, 0}), 
                                       ItemQuality::Common, {0.f, 0.f}, false, 0, true);
                mAnim.setSecondaryWeaponVisuals(&weaponBase, &weaponLayout, 
                                               sf::IntRect({0, 0}, {16, 16}), sf::IntRect({0, 0}, {16, 16}), 
                                               ItemQuality::Rare, mWeaponOffset, 0, false);
                break;

            case StudioWeaponType::ShieldRightOnly:
                mAnim.setWeaponVisuals(&shieldTex, &shieldTex, 
                                       sf::IntRect({0, 0}, {16, 16}), sf::IntRect({0, 0}, {0, 0}), 
                                       ItemQuality::Common, {0.f, 0.f}, false, 0, true);
                mAnim.setSecondaryWeaponVisuals(nullptr, nullptr, {}, {}, ItemQuality::Common, {0.f, 0.f});
                break;

            case StudioWeaponType::ShieldLeftOnly:
                mAnim.setWeaponVisuals(nullptr, nullptr, {}, {}, ItemQuality::Common, {0.f, 0.f});
                mAnim.setSecondaryWeaponVisuals(&shieldTex, &shieldTex, 
                                               sf::IntRect({0, 0}, {16, 16}), sf::IntRect({0, 0}, {0, 0}), 
                                               ItemQuality::Common, {0.f, 0.f}, 0, true);
                break;
        }
    } catch (...) {
        // Texturas fallback
    }
}

void AnimatorStudio::update(sf::Time dt) {
    if (mIsPlaying) {
        float effectiveDt = dt.asSeconds() * mSpeedMultiplier;
        mCurrentTime += effectiveDt;
        float dur = getDuration();
        if (dur > 0.f) {
            float lStart = getLoopStart();
            float lEnd = getLoopEnd();
            if (mCurrentTime >= lEnd) {
                if (mIsLooping) {
                    float loopLen = lEnd - lStart;
                    if (loopLen > 0.001f) {
                        float excess = mCurrentTime - lEnd;
                        mCurrentTime = lStart + std::fmod(excess, loopLen);
                    } else {
                        mCurrentTime = lStart;
                    }
                } else {
                    mCurrentTime = dur;
                    mIsPlaying = false;
                }
            }
        }
    }

    mAnim.setAnimTimer(mCurrentTime);

    sf::Vector2f charOrigin = { 0.f, 0.f };
    bool isMoving = (mActiveClip && mActiveClip->name.find("walk") != std::string::npos);

    mAnim.update(mIsPlaying ? dt : sf::Time::Zero, isMoving, charOrigin, mFacingDir, mSpeedMultiplier, nullptr);
    mAnim.setAnimTimer(mCurrentTime);

    updateTrail();
}

void AnimatorStudio::updateTrail() {
    if (!mShowMotionTrail) {
        mTrailPoints.clear();
        return;
    }

    sf::Vector2f tipPos = mAnim.getNodePosition("hand_r");
    if (mWeaponType == StudioWeaponType::TwoHandedSword) {
        tipPos.y -= 24.f;
    } else if (mWeaponType == StudioWeaponType::OneHandedSword) {
        tipPos.y -= 14.f;
    }

    mTrailPoints.push_back({ tipPos, mCurrentTime, 1.0f });

    while (mTrailPoints.size() > 40) {
        mTrailPoints.erase(mTrailPoints.begin());
    }

    for (size_t i = 0; i < mTrailPoints.size(); ++i) {
        mTrailPoints[i].alpha = (float)(i + 1) / (float)mTrailPoints.size();
    }
}

void AnimatorStudio::drawWorld(sf::RenderTarget& target) {
    // 1. Grilla y Ejes
    sf::Vertex lineX[] = {
        sf::Vertex{ { -600.f, 0.f }, sf::Color(59, 130, 246, 120) },
        sf::Vertex{ { 600.f, 0.f }, sf::Color(59, 130, 246, 120) }
    };
    sf::Vertex lineY[] = {
        sf::Vertex{ { 0.f, -400.f }, sf::Color(59, 130, 246, 120) },
        sf::Vertex{ { 0.f, 400.f }, sf::Color(59, 130, 246, 120) }
    };
    target.draw(lineX, 2, sf::PrimitiveType::Lines);
    target.draw(lineY, 2, sf::PrimitiveType::Lines);

    // Suelo de referencia (GroundOffsetY)
    float groundY = mAnim.getGroundOffsetY() != 0.f ? mAnim.getGroundOffsetY() : 35.f;
    sf::Vertex groundLine[] = {
        sf::Vertex{ { -600.f, groundY }, sf::Color(244, 63, 94, 180) },
        sf::Vertex{ { 600.f, groundY }, sf::Color(244, 63, 94, 180) }
    };
    target.draw(groundLine, 2, sf::PrimitiveType::Lines);

    // Sombra proyectada
    std::vector<sf::Vertex> shadowVerts;
    const sf::Texture* shadowTex = nullptr;
    mAnim.getShadowRenderData(shadowVerts, shadowTex);
    if (!shadowVerts.empty() && shadowTex) {
        sf::RenderStates shStates;
        shStates.texture = shadowTex;
        target.draw(shadowVerts.data(), shadowVerts.size(), sf::PrimitiveType::Triangles, shStates);
    }

    // Personaje completo
    mAnim.draw(target);
}

void AnimatorStudio::drawOverlays(sf::RenderTarget& target, float zoomFactor) {
    // 1. Motion Trail (Arco de ataque)
    if (mShowMotionTrail && mTrailPoints.size() >= 2) {
        for (size_t i = 0; i < mTrailPoints.size() - 1; ++i) {
            std::uint8_t alpha1 = static_cast<std::uint8_t>(mTrailPoints[i].alpha * 220);
            std::uint8_t alpha2 = static_cast<std::uint8_t>(mTrailPoints[i + 1].alpha * 220);
            sf::Vertex segment[] = {
                sf::Vertex{ mTrailPoints[i].pos, sf::Color(249, 194, 43, alpha1) },
                sf::Vertex{ mTrailPoints[i + 1].pos, sf::Color(249, 194, 43, alpha2) }
            };
            target.draw(segment, 2, sf::PrimitiveType::Lines);
        }
    }

    // 2. Bone Pivots / Boxes / Gizmos
    if (mShowBones) {
        for (const auto& node : mAnim.getNodes()) {
            bool isSelected = (node.name == mSelectedNode);
            sf::Color boxColor = isSelected ? sf::Color(249, 194, 43, 240) : sf::Color(100, 200, 255, 100);

            // Dibujar quad exterior
            sf::Vertex box[] = {
                sf::Vertex{ node.quad[0].position, boxColor },
                sf::Vertex{ node.quad[1].position, boxColor },
                sf::Vertex{ node.quad[4].position, boxColor },
                sf::Vertex{ node.quad[2].position, boxColor },
                sf::Vertex{ node.quad[0].position, boxColor }
            };
            target.draw(box, 5, sf::PrimitiveType::LineStrip);

            // Centro / Pivote (Crosshair)
            sf::Vector2f center = (node.quad[0].position + node.quad[4].position) * 0.5f;
            float crossSize = isSelected ? (5.f / zoomFactor) : (3.f / zoomFactor);
            sf::Vertex cross[] = {
                sf::Vertex{ { center.x - crossSize, center.y }, boxColor },
                sf::Vertex{ { center.x + crossSize, center.y }, boxColor },
                sf::Vertex{ { center.x, center.y - crossSize }, boxColor },
                sf::Vertex{ { center.x, center.y + crossSize }, boxColor }
            };
            target.draw(cross, 4, sf::PrimitiveType::Lines);

            // Si está seleccionado, dibujar anillo/gizmo de rotación
            if (isSelected) {
                sf::CircleShape ring(12.f / zoomFactor);
                ring.setOrigin({ 12.f / zoomFactor, 12.f / zoomFactor });
                ring.setPosition(center);
                ring.setFillColor(sf::Color::Transparent);
                ring.setOutlineColor(sf::Color(249, 194, 43, 180));
                ring.setOutlineThickness(1.5f / zoomFactor);
                target.draw(ring);
            }
        }

        // Overlays para "weapon"
        if (mWeaponType != StudioWeaponType::None) {
            bool isSelected = (mSelectedNode == "weapon");
            sf::Color boxColor = isSelected ? sf::Color(249, 194, 43, 240) : sf::Color(255, 120, 120, 140);
            sf::FloatRect wepBounds = mAnim.getNodeGlobalBounds("weapon");

            if (wepBounds.size.x > 0.f && wepBounds.size.y > 0.f) {
                sf::RectangleShape wepBox({ wepBounds.size.x, wepBounds.size.y });
                wepBox.setPosition(wepBounds.position);
                wepBox.setFillColor(sf::Color::Transparent);
                wepBox.setOutlineColor(boxColor);
                wepBox.setOutlineThickness(1.f / zoomFactor);
                target.draw(wepBox);

                sf::Vector2f center = mAnim.getNodePosition("weapon");
                float crossSize = isSelected ? (5.f / zoomFactor) : (3.f / zoomFactor);
                sf::Vertex cross[] = {
                    sf::Vertex{ { center.x - crossSize, center.y }, boxColor },
                    sf::Vertex{ { center.x + crossSize, center.y }, boxColor },
                    sf::Vertex{ { center.x, center.y - crossSize }, boxColor },
                    sf::Vertex{ { center.x, center.y + crossSize }, boxColor }
                };
                target.draw(cross, 4, sf::PrimitiveType::Lines);

                if (isSelected) {
                    sf::CircleShape ring(14.f / zoomFactor);
                    ring.setOrigin({ 14.f / zoomFactor, 14.f / zoomFactor });
                    ring.setPosition(center);
                    ring.setFillColor(sf::Color::Transparent);
                    ring.setOutlineColor(sf::Color(249, 194, 43, 180));
                    ring.setOutlineThickness(1.5f / zoomFactor);
                    target.draw(ring);
                }
            }
        }
    }
}
