#include "Animation.h"
#include <map>

static std::string getTextureNameForPart(const std::string& part) {
    if (part == "hand_l" || part == "hand_r") return "hand";
    if (part == "foot_l" || part == "foot_r") return "foot";
    return part;
}

bool Animation::loadDynamicParts(ResourceManager& res, const std::string& mobType, const std::vector<std::string>& partNames) {
    std::string path = (mobType == "player") ? "assets/textures/player/parts/partes/" : "assets/textures/mobs/" + mobType + "/parts/";
    
    try {
        std::string atlasKey = "mob_atlas_dyn_" + mobType;
        auto it = res.mTextureCache.find(atlasKey);
        
        std::vector<sf::IntRect> atlasRects;
        
        if (it != res.mTextureCache.end()) {
            mAtlasTexture = &it->second;
            // Hacky fallback for rects if cached (needs a proper rect cache ideally)
            unsigned int xOff = 0;
            for (const auto& part : partNames) {
                std::string filename = getTextureNameForPart(part) + ".png";
                sf::Vector2u sz = res.getTexture(path + filename).getSize();
                atlasRects.push_back(sf::IntRect({(int)xOff, 0}, {(int)sz.x, (int)sz.y}));
                xOff += sz.x;
            }
        } else {
            std::vector<sf::Image> images;
            unsigned int totalW = 0;
            unsigned int totalH = 0;
            
            for (const auto& part : partNames) {
                std::string filename = getTextureNameForPart(part) + ".png";
                sf::Texture& tex = res.getTexture(path + filename);
                sf::Image img = tex.copyToImage();
                sf::Vector2u sz = img.getSize();
                totalW += sz.x;
                if (sz.y > totalH) totalH = sz.y;
                images.push_back(std::move(img));
            }

            sf::Image atlasImg;
            atlasImg.resize({totalW, totalH}, sf::Color::Transparent);

            unsigned int xOff = 0;
            for (size_t i = 0; i < images.size(); ++i) {
                (void)atlasImg.copy(images[i], {xOff, 0});
                sf::Vector2u sz = images[i].getSize();
                atlasRects.push_back(sf::IntRect({(int)xOff, 0}, {(int)sz.x, (int)sz.y}));
                xOff += sz.x;
            }

            sf::Texture atlasTex;
            if (!atlasTex.loadFromImage(atlasImg)) return false;
            atlasTex.setSmooth(false);
            res.mTextureCache[atlasKey] = std::move(atlasTex);
            mAtlasTexture = &res.mTextureCache[atlasKey];
        }

        mNodes.clear();
        mNodeMap.clear();

        for (size_t i = 0; i < partNames.size(); ++i) {
            SkeletonNode node;
            node.name = partNames[i];
            sf::IntRect rect = atlasRects[i];
            
            float hw = rect.size.x * 0.5f;
            float hh = rect.size.y * 0.5f;
            node.localBounds = sf::FloatRect({-hw, -hh}, {(float)rect.size.x, (float)rect.size.y});
            
            float u1 = (float)rect.position.x;
            float v1 = (float)rect.position.y;
            float u2 = (float)(rect.position.x + rect.size.x);
            float v2 = (float)(rect.position.y + rect.size.y);
            
            node.quad[0].texCoords = {u1, v1}; node.quad[0].color = sf::Color::White;
            node.quad[1].texCoords = {u2, v1}; node.quad[1].color = sf::Color::White;
            node.quad[2].texCoords = {u1, v2}; node.quad[2].color = sf::Color::White;
            node.quad[3].texCoords = {u2, v1}; node.quad[3].color = sf::Color::White;
            node.quad[4].texCoords = {u2, v2}; node.quad[4].color = sf::Color::White;
            node.quad[5].texCoords = {u1, v2}; node.quad[5].color = sf::Color::White;
            
            // Set default offsets matching animator.jsx to prevent overlap collapse (0,0)
            if (node.name == "head") node.currentPos = { -10.f, -40.f };
            else if (node.name == "body") node.currentPos = { 0.f, 0.f };
            else if (node.name == "hand_l") node.currentPos = { -12.f, -5.f };
            else if (node.name == "hand_r") node.currentPos = { 12.f, 5.f };
            else if (node.name == "foot_l") node.currentPos = { -6.f, 25.f };
            else if (node.name == "foot_r") node.currentPos = { 6.f, 35.f };
            
            node.defaultRestPos = node.currentPos;
            node.customRestPos = node.currentPos;
            node.basePos = node.currentPos;
            
            mNodes.push_back(node);
            mNodeMap[node.name] = i;
        }

        try {
            mFootprintTexture = &res.getTexture(path + "huella.png");
            static std::map<std::string, sf::Image> s_footprintImages;
            if (s_footprintImages.find(mobType) == s_footprintImages.end()) {
                s_footprintImages[mobType] = mFootprintTexture->copyToImage();
            }
            mFootprintImage = &s_footprintImages[mobType];
        } catch (...) {
            mFootprintTexture = nullptr;
            mFootprintImage = nullptr;
        }

        mIsLoaded = true;
        return true;
    } catch (...) {
        return false;
    }
}

void Animation::setCustomRestOffsets(sf::Vector2f head, sf::Vector2f handL, sf::Vector2f handR, sf::Vector2f footL, sf::Vector2f footR) {
    for (auto& node : mNodes) {
        if (node.name == "head") node.customRestPos = head;
        else if (node.name == "hand_l") node.customRestPos = handL;
        else if (node.name == "hand_r") node.customRestPos = handR;
        else if (node.name == "foot_l") node.customRestPos = footL;
        else if (node.name == "foot_r") node.customRestPos = footR;
        else if (node.name == "body") node.customRestPos = {0.f, 0.f};
        
        if (!mCurrentClip) {
            node.basePos = node.customRestPos;
            node.currentPos = node.customRestPos;
        }
    }
}

bool Animation::loadSkeleton(ResourceManager& res, const std::string& path) {
    const SkeletonData* sk = res.getSkeleton(path);
    if (!sk) {
        return false;
    }
    
    sf::Vector2f head = { -10.f, -40.f };
    sf::Vector2f handL = { -12.f, -5.f };
    sf::Vector2f handR = { 12.f, 5.f };
    sf::Vector2f footL = { -6.f, 25.f };
    sf::Vector2f footR = { 6.f, 35.f };
    
    for (const auto& node : mNodes) {
        if (node.name == "head") head = node.defaultRestPos;
        else if (node.name == "hand_l") handL = node.defaultRestPos;
        else if (node.name == "hand_r") handR = node.defaultRestPos;
        else if (node.name == "foot_l") footL = node.defaultRestPos;
        else if (node.name == "foot_r") footR = node.defaultRestPos;
    }
    
    if (sk->headOffset.has_value()) head = sk->headOffset.value();
    if (sk->handLOffset.has_value()) handL = sk->handLOffset.value();
    if (sk->handROffset.has_value()) handR = sk->handROffset.value();
    if (sk->footLOffset.has_value()) footL = sk->footLOffset.value();
    if (sk->footROffset.has_value()) footR = sk->footROffset.value();
    
    if (sk->groundOffsetY.has_value()) {
        mGroundOffsetY = sk->groundOffsetY.value();
    }
    if (sk->stride.has_value()) {
        mStride = sk->stride.value();
    }
    if (sk->weaponOffset.has_value()) {
        mBaseWeaponOffsetMain = sk->weaponOffset.value();
    }
    if (sk->weaponSecondaryOffset.has_value()) {
        mBaseWeaponOffsetSec = sk->weaponSecondaryOffset.value();
    }
    if (sk->weaponTwoHandedOffset.has_value()) {
        mBaseWeaponOffsetTwoHanded = sk->weaponTwoHandedOffset.value();
    }
    
    setCustomRestOffsets(head, handL, handR, footL, footR);

    // Apply custom offsets from the offsets map for any custom parts
    for (auto& node : mNodes) {
        auto it = sk->offsets.find(node.name);
        if (it != sk->offsets.end()) {
            node.customRestPos = it->second;
            if (!mCurrentClip) {
                node.currentPos = node.customRestPos;
            }
        }
    }

    return true;
}
