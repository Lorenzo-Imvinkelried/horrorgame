#include "core/managers/EntityManager.h"
#include "core/systems/combat/CombatSystem.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include "Config.h"
#include "utils/TinyJson.h" 
#include "entities/mob/Mob.h" // [FIX] Required for dynamic_cast logging 
#include "entities/Goblin.h"
#include "entities/MobGrande1.h"
#include <tracy/Tracy.hpp>

EntityManager::EntityManager(ResourceManager& res, ItemManager& itemMgr) 
    : mRespawnSystem() 
    , mResourceManager(res)
    , mItemManager(itemMgr)
{
    mActiveEntities.reserve(1000); 
    // [SPATIAL GRID] Init
    mGrid.cellSize = cfg::Optimization::GRID_CELL_SIZE;
}

EntityManager::~EntityManager() {
    clear();
}

void EntityManager::clear() {
    // [DESTRUCTOR CRASH FIX] Null out all aggro targets before deleting entities
    // to prevent Mobs from dereferencing a deleted Player in their destructors.
    for (auto& entity : mActiveEntities) {
        if (auto* mob = dynamic_cast<Mob*>(entity.get())) {
            mob->resetAggro(mob->getAggroTarget());
        }
    }
    for (auto& entity : mSleepingEntities) {
        if (auto* mob = dynamic_cast<Mob*>(entity.get())) {
            mob->resetAggro(mob->getAggroTarget());
        }
    }
    for (auto& entity : mMobPool) {
        if (auto* mob = dynamic_cast<Mob*>(entity.get())) {
            mob->resetAggro(mob->getAggroTarget());
        }
    }

    mActiveEntities.clear();
    mSleepingEntities.clear();
    mRespawnSystem.clear();
    mEntityRegistry.clear(); // [OPTIMIZATION]
    mPlayerPtr = nullptr;
    mMobPool.clear(); // Ensure object pool is cleared to release memory
}

bool EntityManager::isValid(Entity* entity) const {
    return mEntityRegistry.count(entity) > 0;
}

Player* EntityManager::getPlayer() const {
    return mPlayerPtr;
}

std::vector<std::unique_ptr<Entity>>& EntityManager::getActiveEntities() {
    return mActiveEntities;
}

const std::vector<std::unique_ptr<Entity>>& EntityManager::getActiveEntities() const {
    return mActiveEntities;
}

const std::vector<std::unique_ptr<Entity>>& EntityManager::getSleepingEntities() const {
    return mSleepingEntities;
}

void EntityManager::setCombatSystem(CombatSystem* combatSystem) {
    mCombatSystem = combatSystem;
}

void EntityManager::spawnPlayer(std::unique_ptr<Player> player) {
    if (!player) return;
    mPlayerPtr = player.get();
    mEntityRegistry.insert(mPlayerPtr); // [OPTIMIZATION]
    // Player is always active
    mActiveEntities.push_back(std::move(player));
}

void EntityManager::loadMobBlueprints() {
    std::string path = "assets/data/mobs.json"; 
    auto root = json::parseFile(path);
    
    if (root.type == json::Type::Null) {
        std::cerr << "[EntityManager] ERROR: No se pudo abrir " << path << " o JSON invalido.\n";
        return;
    }
    
    // TinyJson: Root should be an object containing "mobs": [...] or just array? 
    // Looking at PlayingState legacy, it did `root.asArray()`, implying the file IS an array.
    // BUT the previous iteration I wrote checked `root.hasKey("mobs")`.
    // Let's assume standard format: verify if root is array or object.
    // If we look at the error log "root.asArray()" was used in PlayingState. 
    // Wait, PlayingState code I removed used `root.asArray()`.
    
    const auto& mobsArr = root.asArray(); // Assuming root IS the array of mobs
    for (const auto& val : mobsArr) {
        const auto& mobObj = val.asObject();
        MobBlueprint bp;
        
        // Defaults
        if (mobObj.count("type")) bp.type = mobObj.at("type").asString();
        else bp.type = "unknown";

        if (mobObj.count("name")) bp.name = mobObj.at("name").asString();
        else bp.name = "Mob";

        // Stats Logic matching MobBlueprint members
        if (mobObj.count("stats")) {
             const auto& stats = mobObj.at("stats").asObject();
             
             if(stats.count("hp")) bp.maxHp = stats.at("hp").asInt();
             if(stats.count("mp")) bp.maxMp = stats.at("mp").asInt();
             
             if(stats.count("strength")) bp.strength = stats.at("strength").asInt();
             if(stats.count("dexterity")) bp.agility = stats.at("dexterity").asInt();
             else if(stats.count("agility")) bp.agility = stats.at("agility").asInt();
             if(stats.count("intelligence")) bp.intelligence = stats.at("intelligence").asInt();
             if(stats.count("vitality")) bp.vitality = stats.at("vitality").asInt();
             
             if(stats.count("attack")) bp.attack = stats.at("attack").asInt();
             if(stats.count("defense")) bp.defense = stats.at("defense").asInt();
             
             // Speed / AtkSpeed
             if(stats.count("speed")) bp.speed = (float)stats.at("speed").asDouble();
             if(stats.count("atkSpeed")) bp.atkSpeed = (float)stats.at("atkSpeed").asDouble();
             if(stats.count("attackDelayFactor")) bp.attackDelayFactor = (float)stats.at("attackDelayFactor").asDouble();
             if(stats.count("attackRange")) bp.attackRange = (float)stats.at("attackRange").asDouble();
             if(stats.count("leashRadius")) bp.leashRadius = (float)stats.at("leashRadius").asDouble();
             if(stats.count("rangeViolent")) bp.rangeViolent = (float)stats.at("rangeViolent").asDouble();
             if(stats.count("malice")) bp.malice = (float)stats.at("malice").asDouble();
             if(stats.count("stance")) {
                 std::string st = stats.at("stance").asString();
                 if (st == "p" || st == "passive" || st == "Passive") bp.stance = MobStance::Passive;
                 else if (st == "v" || st == "violent" || st == "Violent") bp.stance = MobStance::Violent;
                 else bp.stance = MobStance::Neutral;
             }
             
             // Base Crit / Pen
             if(stats.count("critChance")) bp.critChance = (float)stats.at("critChance").asDouble();
             else if(stats.count("criticalChance")) bp.critChance = (float)stats.at("criticalChance").asDouble();
             if(stats.count("critDamage")) bp.critDamage = (float)stats.at("critDamage").asDouble();
             if(stats.count("armorPenetration")) bp.armorPenetration = stats.at("armorPenetration").asInt();
             
             if(stats.count("xp")) bp.xp = stats.at("xp").asInt();
             if(stats.count("level")) bp.level = stats.at("level").asInt();

             // Visual Scale
             if (stats.count("scale")) {
                 const auto& scaleArr = stats.at("scale").asArray();
                 if (scaleArr.size() >= 2) {
                     bp.scale.x = (float)scaleArr[0].asDouble();
                     bp.scale.y = (float)scaleArr[1].asDouble();
                 }
             }

             // Advanced Stats
             if(stats.count("armorPenetrationPercent")) bp.armorPenetrationPercent = (float)stats.at("armorPenetrationPercent").asDouble();
             if(stats.count("physicalDamageBonus")) bp.physicalDamageBonus = (float)stats.at("physicalDamageBonus").asDouble();
             else bp.physicalDamageBonus = 100.0f; // Default

             if(stats.count("lifestealPercent")) bp.lifestealPercent = (float)stats.at("lifestealPercent").asDouble();
             if(stats.count("cooldownReductionPercent")) bp.cooldownReductionPercent = (float)stats.at("cooldownReductionPercent").asDouble();
             if(stats.count("doubleStrikeChance")) bp.doubleStrikeChance = (float)stats.at("doubleStrikeChance").asDouble();
             if(stats.count("tripleStrikeChance")) bp.tripleStrikeChance = (float)stats.at("tripleStrikeChance").asDouble();
             if(stats.count("blockChance")) bp.blockChance = (float)stats.at("blockChance").asDouble();
             if(stats.count("blockValuePercent")) bp.blockValuePercent = (float)stats.at("blockValuePercent").asDouble(); // [FIX]
             if(stats.count("thornsPercent")) bp.thornsPercent = (float)stats.at("thornsPercent").asDouble(); // [FIX]
             if(stats.count("hpRegenPercent")) bp.hpRegenPercent = (float)stats.at("hpRegenPercent").asDouble(); // [FIX]
             if(stats.count("mpRegenPercent")) bp.mpRegenPercent = (float)stats.at("mpRegenPercent").asDouble(); // [FIX]
             if(stats.count("weightKg")) bp.weightKg = (float)stats.at("weightKg").asDouble(); // [WEIGHT]

             // [STUN]
             if(stats.count("stunChance")) bp.stunChance = (float)stats.at("stunChance").asDouble();
             if(stats.count("stunDuration")) bp.stunDuration = (float)stats.at("stunDuration").asDouble();

             // [DEBUFFS]
             if(stats.count("slowMovePercent")) bp.slowMovePercent = (float)stats.at("slowMovePercent").asDouble();
             if(stats.count("slowMoveDuration")) bp.slowMoveDuration = (float)stats.at("slowMoveDuration").asDouble();
             if(stats.count("slowAttackPercent")) bp.slowAttackPercent = (float)stats.at("slowAttackPercent").asDouble();
             if(stats.count("slowAttackDuration")) bp.slowAttackDuration = (float)stats.at("slowAttackDuration").asDouble();

             // [NEW STATS - SYNC]
             if(stats.count("tenacityPercent")) bp.tenacityPercent = (float)stats.at("tenacityPercent").asDouble();
             if(stats.count("damageReductionPercent")) bp.damageReductionPercent = (float)stats.at("damageReductionPercent").asDouble();
             if(stats.count("critAvoidancePercent")) bp.critAvoidancePercent = (float)stats.at("critAvoidancePercent").asDouble();
             if(stats.count("antiArmorPenPercent")) bp.antiArmorPenPercent = (float)stats.at("antiArmorPenPercent").asDouble();
             if(stats.count("antiArmorPenFlat")) bp.antiArmorPenFlat = stats.at("antiArmorPenFlat").asInt();
             if(stats.count("manaStealPercent")) bp.manaStealPercent = (float)stats.at("manaStealPercent").asDouble();
             if(stats.count("xpBonusPercent")) bp.xpBonusPercent = (float)stats.at("xpBonusPercent").asDouble();

             // Bleed
             if(stats.count("bleedDurationFlat")) bp.bleedDurationFlat = (float)stats.at("bleedDurationFlat").asDouble();
             if(stats.count("bleedDurationPercent")) bp.bleedDurationPercent = (float)stats.at("bleedDurationPercent").asDouble();
             if(stats.count("bleedFlat")) bp.bleedFlat = stats.at("bleedFlat").asInt();
             if(stats.count("bleedPercent")) bp.bleedPercent = (float)stats.at("bleedPercent").asDouble();

             if(stats.count("aoeRadius")) bp.aoeRadius = (float)stats.at("aoeRadius").asDouble();
             if(stats.count("aoeDamagePercent")) bp.aoeDamagePercent = (float)stats.at("aoeDamagePercent").asDouble();
             if(stats.count("trueDamagePercent")) bp.trueDamagePercent = stats.at("trueDamagePercent").asInt();
             
             if(stats.count("executeDamagePercent")) bp.executeDamagePercent = stats.at("executeDamagePercent").asInt();
             if(stats.count("executeHealthThresholdPercent")) bp.executeHealthThresholdPercent = stats.at("executeHealthThresholdPercent").asInt();

             // Accuracy / Erasion
             if(stats.count("accuracy")) bp.accuracy = stats.at("accuracy").asInt();
             else bp.accuracy = 100;
             if(stats.count("evasion")) bp.evasion = stats.at("evasion").asInt();

             // [WEIGHT SYSTEM]
             if(stats.count("weightKg")) bp.weightKg = (float)stats.at("weightKg").asDouble();
        }

        if (mobObj.count("animConfig")) {
            const auto& animConfig = mobObj.at("animConfig").asObject();
            if (animConfig.count("groundOffsetY")) {
                bp.groundOffsetY = (float)animConfig.at("groundOffsetY").asDouble();
            }
            if (animConfig.count("baseAnimSpeed")) {
                bp.baseAnimSpeed = (float)animConfig.at("baseAnimSpeed").asDouble();
            }
            if (animConfig.count("headOffset")) {
                const auto& arr = animConfig.at("headOffset").asArray();
                if (arr.size() >= 2) {
                    bp.headOffset = { (float)arr[0].asDouble(), (float)arr[1].asDouble() };
                }
            }
            if (animConfig.count("handLOffset")) {
                const auto& arr = animConfig.at("handLOffset").asArray();
                if (arr.size() >= 2) {
                    bp.handLOffset = { (float)arr[0].asDouble(), (float)arr[1].asDouble() };
                }
            }
            if (animConfig.count("handROffset")) {
                const auto& arr = animConfig.at("handROffset").asArray();
                if (arr.size() >= 2) {
                    bp.handROffset = { (float)arr[0].asDouble(), (float)arr[1].asDouble() };
                }
            }
            if (animConfig.count("footLOffset")) {
                const auto& arr = animConfig.at("footLOffset").asArray();
                if (arr.size() >= 2) {
                    bp.footLOffset = { (float)arr[0].asDouble(), (float)arr[1].asDouble() };
                }
            }
            if (animConfig.count("footROffset")) {
                const auto& arr = animConfig.at("footROffset").asArray();
                if (arr.size() >= 2) {
                    bp.footROffset = { (float)arr[0].asDouble(), (float)arr[1].asDouble() };
                }
            }
        }

        // Parse skills overrides
        if (mobObj.count("skills")) {
            const auto& skillsArr = mobObj.at("skills").asArray();
            for (const auto& skillVal : skillsArr) {
                MobSkillBlueprint sBp;
                if (skillVal.type == json::Type::Object) {
                    const auto& skillObj = skillVal.asObject();
                    if (skillObj.count("id")) sBp.id = skillObj.at("id").asInt();
                    if (skillObj.count("cooldown")) sBp.cooldown = (float)skillObj.at("cooldown").asDouble();
                    if (skillObj.count("manaCost")) sBp.manaCost = skillObj.at("manaCost").asInt();
                    if (skillObj.count("damageFlat")) sBp.damageFlat = skillObj.at("damageFlat").asInt();
                    if (skillObj.count("damagePercent")) sBp.damagePercent = (float)skillObj.at("damagePercent").asDouble();
                    if (skillObj.count("range")) sBp.range = skillObj.at("range").asInt();
                    if (skillObj.count("buffDuration")) sBp.buffDuration = (float)skillObj.at("buffDuration").asDouble();
                    if (skillObj.count("castTime")) sBp.castTime = (float)skillObj.at("castTime").asDouble();
                    if (skillObj.count("stunDuration")) sBp.stunDuration = (float)skillObj.at("stunDuration").asDouble();
                } else if (skillVal.type == json::Type::Number) {
                    sBp.id = skillVal.asInt();
                }
                bp.skills.push_back(sBp);
            }
        }

        // Parse equipment pools
        if (mobObj.count("equipment_pool") && mobObj.at("equipment_pool").type == json::Type::Object) {
            const auto& eqObj = mobObj.at("equipment_pool").asObject();
            for (const auto& pair : eqObj) {
                std::string slotStr = pair.first;
                EquipmentSlot slot = EquipmentSlot::None;
                
                if (slotStr == "Head") slot = EquipmentSlot::Head;
                else if (slotStr == "Cape") slot = EquipmentSlot::Cape;
                else if (slotStr == "Chest") slot = EquipmentSlot::Chest;
                else if (slotStr == "Hands") slot = EquipmentSlot::Hands;
                else if (slotStr == "MainHand" || slotStr == "WeaponMain") slot = EquipmentSlot::MainHand;
                else if (slotStr == "Legs") slot = EquipmentSlot::Legs;
                else if (slotStr == "OffHand" || slotStr == "WeaponSecondary") slot = EquipmentSlot::OffHand;
                else if (slotStr == "Ring1") slot = EquipmentSlot::Ring1;
                else if (slotStr == "Feet") slot = EquipmentSlot::Feet;
                else if (slotStr == "Ring2") slot = EquipmentSlot::Ring2;
                else if (slotStr == "SubWeapon1") slot = EquipmentSlot::SubWeapon1;
                else if (slotStr == "SubWeapon2") slot = EquipmentSlot::SubWeapon2;
                
                if (slot != EquipmentSlot::None && pair.second.type == json::Type::Array) {
                    for (const auto& optVal : pair.second.asArray()) {
                        if (optVal.type == json::Type::Object) {
                            const auto& optObj = optVal.asObject();
                            MobEquipmentOption option;
                            if (optObj.count("id")) option.itemId = optObj.at("id").asString();
                            if (optObj.count("chance")) option.chance = (float)optObj.at("chance").asDouble();
                            bp.equipmentPool[slot].push_back(option);
                        }
                    }
                }
            }
        }

         mMobBlueprints[bp.type] = bp;
    }
    std::cout << "[EntityManager] Loaded " << mMobBlueprints.size() << " mob blueprints.\n";
}

void EntityManager::loadEntitiesFromFile(ResourceManager& res, const std::string& filename, const WorldData& worldData) {
    // Clear existing (preserving Player)
    std::unique_ptr<Entity> playerPtrOwner;
    if (mPlayerPtr) {
        for (auto it = mActiveEntities.begin(); it != mActiveEntities.end(); ) {
            if (it->get() == mPlayerPtr) {
                playerPtrOwner = std::move(*it);
                it = mActiveEntities.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    
    // [FIX] Clear Registry as well, but handle Player preservation
    mActiveEntities.clear();
    mSleepingEntities.clear();
    mRespawnSystem.clear();
    mEntityRegistry.clear(); // [CRITICAL FIX]
    
    
    if (playerPtrOwner) {
        mPlayerPtr = static_cast<Player*>(playerPtrOwner.get()); // Reset ptr just in case
        mEntityRegistry.insert(mPlayerPtr); // Re-insert player
        mActiveEntities.push_back(std::move(playerPtrOwner));
    }

    std::string entityFile = worldData.entities;
    if (entityFile.empty()) entityFile = filename; 
    
    if (entityFile.find("assets/") == std::string::npos) {
         entityFile = "assets/maps/" + entityFile;
    }

    if (entityFile.size() >= 4 && entityFile.substr(entityFile.size() - 4) == ".txt") {
        entityFile = entityFile.substr(0, entityFile.size() - 4) + ".json";
    }

    json::Value root = json::parseFile(entityFile);
    if (root.type != json::Type::Array) {
        std::cerr << "[EntityManager] WARN: Could not open entities JSON file: " << entityFile << "\n";
        return;
    }

    for (const auto& val : root.asArray()) {
        if (val.type != json::Type::Object) continue;
        const auto& obj = val.asObject();

        std::string type = obj.count("type") ? obj.at("type").asString() : "";
        if (type.empty()) continue;

        float col = 0.f;
        if (obj.count("col")) col = static_cast<float>(obj.at("col").asDouble());
        else if (obj.count("x")) col = static_cast<float>(obj.at("x").asDouble());

        float row = 0.f;
        if (obj.count("row")) row = static_cast<float>(obj.at("row").asDouble());
        else if (obj.count("y")) row = static_cast<float>(obj.at("y").asDouble());

        int level = obj.count("level") ? obj.at("level").asInt() : -1;
        bool isBoss = obj.count("isBoss") ? obj.at("isBoss").asBool() : false;

        MobStance stance = MobStance::Neutral;
        bool stanceSet = false;
        if (obj.count("stance")) {
            std::string s = obj.at("stance").asString();
            if (s == "p" || s == "passive" || s == "Passive") {
                stance = MobStance::Passive;
                stanceSet = true;
            } else if (s == "v" || s == "violent" || s == "Violent") {
                stance = MobStance::Violent;
                stanceSet = true;
            } else if (s == "n" || s == "neutral" || s == "Neutral") {
                stance = MobStance::Neutral;
                stanceSet = true;
            }
        }

        if (mMobBlueprints.count(type)) {
             sf::Vector2f spawnPos = { (col + 0.5f) * cfg::Map::TILE_SIZE, (row + 0.5f) * cfg::Map::TILE_SIZE };
             std::unique_ptr<Mob> newMob;
             if (type == "goblin") {
                 newMob = std::make_unique<Goblin>(spawnPos, mMobBlueprints[type], res, mItemManager, mCombatSystem, false, level, mSkillManager);
             } else if (type == "mob_grande_1") {
                 newMob = std::make_unique<MobGrande1>(spawnPos, mMobBlueprints[type], res, mItemManager, mCombatSystem, false, level, mSkillManager);
             } else {
                 newMob = std::make_unique<Mob>(spawnPos, mMobBlueprints[type], res, mItemManager, mCombatSystem, false, level, mSkillManager);
             }
             
             if (stanceSet) {
                 newMob->setStance(stance);
             }

             if (isBoss) {
                 newMob->setIsBoss(true);
             }

             mEntityRegistry.insert(newMob.get()); // [OPTIMIZATION]
             mActiveEntities.push_back(std::move(newMob));
        }
    }
    std::cout << "[EntityManager] Loaded entities from " << entityFile << "\n";
}

void EntityManager::update(sf::Time dt, sf::Vector2f playerPos, sf::Vector2f viewSize, const TerrainDeformSystem* terrain) {
    ZoneScoped;
    if (!mPlayerPtr) return;
    updateActivationRanges(viewSize); 
    
    // Use strict camera bounds (match RenderSystem's culling margin to prevent frozen animations on screen edges)
    float margin = cfg::Map::CULLING_MARGIN_PX;
    sf::FloatRect viewRect({playerPos.x - viewSize.x/2.f - margin, playerPos.y - viewSize.y/2.f - margin}, 
                           {viewSize.x + margin*2.f, viewSize.y + margin*2.f});
    
    // 1. Move Sleeping -> Active (Interval Check)
    if (mEntityActivationTimer.getElapsedTime().asSeconds() > cfg::Optimization::WAKE_UP_CHECK_INTERVAL) {
        mEntityActivationTimer.restart();
        
        int awakenedCount = 0;
        const int MAX_WAKE_PER_FRAME = 200; // [STUTTER FIX] Throttle wake-ups
        
        auto splitPoint = std::partition(mSleepingEntities.begin(), mSleepingEntities.end(),
            [&](const std::unique_ptr<Entity>& uptr) {
                if (uptr->isAggro() || uptr->isReturningToSpawn()) return false; 
                
                float dx = uptr->getPosition().x - playerPos.x;
                float dy = uptr->getPosition().y - playerPos.y;
                float distSq = dx*dx + dy*dy;
                
                // If it should wake up, but we reached the limit, force it to sleep until next check
                if (distSq <= mActivationRangeSq) {
                    if (awakenedCount < MAX_WAKE_PER_FRAME) {
                        awakenedCount++;
                        return false; // Wake up
                    }
                    return true; // Keep sleeping to avoid CPU/Memory spike this frame
                }
                
                return true; // Keep sleeping
            });

        for (auto it = splitPoint; it != mSleepingEntities.end(); ++it) {
            (*it)->onWake();
            mActiveEntities.push_back(std::move(*it));
        }
        mSleepingEntities.erase(splitPoint, mSleepingEntities.end());
    }

    // 2. Active Logic (Update + Sleep + Remove)
    float deactSq = mDeactivationRangeSq;
    // playerPos already passed as argument to update()
    
    // [OPTIMIZATION] Static counter for staggering
    static int frameCounter = 0;
    frameCounter++;

    auto splitPoint = std::partition(mActiveEntities.begin(), mActiveEntities.end(),
        [&](const std::unique_ptr<Entity>& uptr) {
            Entity* e = uptr.get();
            if (e->isRemovable()) {
                 return false; // Automatically send to the removal block
            }
            
            if (e == mPlayerPtr) return true; // Player always active
            if (e->isAggro() || e->isReturningToSpawn() || !e->isAlive()) return true; // [FIX] Combat, Retreat or Dead overrides Logic Culling
            
            float dx = e->getPosition().x - playerPos.x;
            float dy = e->getPosition().y - playerPos.y;
            float distSq = dx*dx + dy*dy;
            
            if (distSq <= deactSq) {
                // [LOGIC THROTTLING]
                // If mob is somewhat far (> 500px approx, say 250000 sq), update less frequently.
                // 500^2 = 250,000.
                if (distSq > 250000.0f) {
                     // Update only every 3rd frame based on uniform pointer hash
                     size_t id = std::hash<void*>{}(e);
                     if ((id + frameCounter) % 3 != 0) {
                         return true; // Keep active, but SKIP update call below?
                         // Issue: partition lambda relies on us returning true/false for vector structure
                         // We can't skip 'update(dt)' here easily because we iterate later.
                         // We will handle throttling in the update loop below.
                         return true;
                     }
                }
                return true; 
            }
            
            if (e->isRemovable()) {
                 mEntityRegistry.erase(e); 
                 return false; 
            }
            
            return false;
        });

    for (auto it = splitPoint; it != mActiveEntities.end(); ++it) {
        Entity* e = it->get();
        if (e->isRemovable()) {
             mEntityRegistry.erase(e);
        } else {
             mSleepingEntities.push_back(std::move(*it));
        }
    }
    mActiveEntities.erase(splitPoint, mActiveEntities.end());

    for (auto& uptr : mActiveEntities) {
        Entity* e = uptr.get();
        if (e == mPlayerPtr) {
            if (e->isAlive()) e->setIsVisible(true);
            e->update(dt);
            continue;
        }

        e->setIsVisible(viewRect.contains(e->getPosition()));
        e->setTerrainDeform(e->isVisible() ? terrain : nullptr);

        float dx = e->getPosition().x - playerPos.x;
        float dy = e->getPosition().y - playerPos.y;
        
        // Manhattan Distance for speed check
        float manhattanDist = std::abs(dx) + std::abs(dy);
        
        if (manhattanDist > 1200.f) { // Very Far (Off-screen / Edge of world)
             size_t id = std::hash<void*>{}(e);
             // Update 1 out of 10 frames
             if ((id + frameCounter) % 10 != 0) {
                 continue; 
             }
             e->update(dt * 10.0f); // Compensate
        }
        else if (manhattanDist > 600.f) { // Somewhat Far (> 500-600px)
             size_t id = std::hash<void*>{}(e);
             // Update 1 out of 3 frames
             if ((id + frameCounter) % 3 != 0) {
                 continue; 
             }
             e->update(dt * 3.0f); // Compensate
        } 
        else {
             // Close active zone: Standard update
             e->update(dt);
        }
    }
    
    // 3. Respawn System Update
    mRespawnSystem.update(dt);
    auto readyField = mRespawnSystem.getReadyMobs();
    for (const auto& ticket : readyField) {
        spawnMobFromTicket(ticket, mResourceManager);
    }
    
    // 4. Update Spatial Grid (End of frame)
    updateSpatialGrid();
}

void EntityManager::updateActivationRanges(sf::Vector2f viewSize) {
    float viewDiagonal = std::sqrt(viewSize.x * viewSize.x + viewSize.y * viewSize.y);
    float margin = cfg::Optimization::WAKE_UP_MARGIN_PX;
    
    // [LOGIC CULLING DEBUG]
    if (cfg::Debug::ENABLE_CULLING_DEBUG) {
        margin = cfg::Debug::CULLING_DEBUG_MARGIN; 
    }

    float wakeDist = (viewDiagonal / 2.f) + margin; 
    
    float activationDist = wakeDist;
    float deactivationDist = activationDist + cfg::Optimization::SLEEP_HYSTERESIS_PX;

    mActivationRangeSq = activationDist * activationDist;
    
    // [LOGIC CULLING DEBUG] Small hysteresis to see transition on screen
    if (cfg::Debug::ENABLE_CULLING_DEBUG) {
        deactivationDist = activationDist + 10.f; 
    }
    
    mDeactivationRangeSq = deactivationDist * deactivationDist;
}

void EntityManager::cleanupDeadEntities(std::function<void(Entity*)> onDeath) {
    // [OBJECT POOLING] Use partition instead of remove_if to handle pooling efficiently
    auto splitPoint = std::partition(mActiveEntities.begin(), mActiveEntities.end(),
        [this](const auto& e) {
            // Keep if alive OR if it's the player
            return e->isAlive() || e.get() == mPlayerPtr;
        });

    // Handle dead entities (from splitPoint to end)
    for (auto it = splitPoint; it != mActiveEntities.end(); ++it) {
        Entity* deadPtr = it->get();
        if (onDeath) onDeath(deadPtr);

        if (auto* mob = dynamic_cast<Mob*>(deadPtr)) {
            mRespawnSystem.scheduleRespawn(mob->getBlueprintName(), mob->getSpawnPoint(), mob->getLevel(), mob->isBoss());
            
            // Recyle to Pool
            mMobPool.push_back(std::move(*it));
        }

        mEntityRegistry.erase(deadPtr); 
    }

    // Shrink vector
    mActiveEntities.erase(splitPoint, mActiveEntities.end());
}

void EntityManager::spawnMob(const std::string& blueprintName, sf::Vector2f pos, CombatSystem* cs, int levelOverride, bool isBoss) {
    if (mMobBlueprints.count(blueprintName)) {
         // [OBJECT POOLING] Check pool first
         if (!mMobPool.empty()) {
             // Find a mob with the SAME blueprint type?
             // Ideally yes, but if all mobs are same class (Mob) and setup via blueprint...
             // We can just re-setup stats. 
             // BUT, textures are loaded in constructor. If we reuse a "Goblin" to be an "Orc", 
             // we need to support texture swapping or keep separate pools per type.
             // Given Mob::Mob calls Load, we might need to verify.
             // HOWEVER: Mob constructor calls res.getTexture() which caches.
             // So re-running setup is fast. 
             // BUT Mob class doesn't have a "reload" method for blueprint.
             // Fix: reset takes the levelOverride
             
             auto it = std::find_if(mMobPool.begin(), mMobPool.end(), 
                [&blueprintName](const auto& m) { 
                    return static_cast<Mob*>(m.get())->getBlueprintName() == blueprintName; 
                });

             if (it != mMobPool.end()) {
                 auto recycledMob = std::move(*it);
                 mMobPool.erase(it);
                 
                 Mob* mobPtr = static_cast<Mob*>(recycledMob.get());
                 mobPtr->reset(pos, true, levelOverride); // Reset with levelOverride
                 mobPtr->setIsBoss(isBoss);
                 
                 mEntityRegistry.insert(mobPtr);
                 mActiveEntities.push_back(std::move(recycledMob));
                 return;
             }
         }

          std::unique_ptr<Mob> newMob;
          if (blueprintName == "goblin") {
              newMob = std::make_unique<Goblin>(pos, mMobBlueprints[blueprintName], mResourceManager, mItemManager, cs ? cs : mCombatSystem, true, levelOverride, mSkillManager);
          } else if (blueprintName == "mob_grande_1") {
              newMob = std::make_unique<MobGrande1>(pos, mMobBlueprints[blueprintName], mResourceManager, mItemManager, cs ? cs : mCombatSystem, true, levelOverride, mSkillManager);
          } else {
              newMob = std::make_unique<Mob>(pos, mMobBlueprints[blueprintName], mResourceManager, mItemManager, cs ? cs : mCombatSystem, true, levelOverride, mSkillManager);
          }
          newMob->setIsBoss(isBoss);
          mEntityRegistry.insert(newMob.get()); // [OPTIMIZATION]
          mActiveEntities.push_back(std::move(newMob));
    }
}

void EntityManager::spawnMobFromTicket(const RespawnTicket& ticket, ResourceManager& res) {
    spawnMob(ticket.blueprintName, ticket.spawnPos, mCombatSystem, ticket.level, ticket.isBoss);
}

// [SPATIAL GRID]
void EntityManager::updateSpatialGrid() {
    // 1. Determine Map Size (Dynamic or Fixed?)
    // Ideally we get this from WorldManager but for now we can infer or use a large enough bounds.
    // Or we rely on player position + viewSize? 
    // Actually, we can resize grid dynamically if needed, but a fixed large world is safer.
    // Let's assume a max world size of 500x500 tiles -> 32000x32000 px?
    // Better: Query active entities for bounds? No, O(N).
    // Let's us a generous fixed grid for now (e.g., 200x200 cells).
    // 200 * 256 = 51200 px. Enough for most maps.
    
    // We only clear cells, not resize every frame unless needed.
    // We only clear cells that were used!
    if (mGrid.width == 0) {
        mGrid.width = 200;
        mGrid.height = 200;
        mGrid.cells.resize(mGrid.width * mGrid.height);
        mGrid.usedIndices.reserve(1000); 
    }
    
    // [LAG FIX] Only clear used cells
    for (int idx : mGrid.usedIndices) {
        mGrid.cells[idx].clear();
    }
    mGrid.usedIndices.clear();
    
    // 2. Populate
    // Helper lambda to insert
    auto insertToGrid = [&](Entity* e) {
        sf::Vector2f pos = e->getPosition();
        int cx = static_cast<int>(pos.x) / mGrid.cellSize;
        int cy = static_cast<int>(pos.y) / mGrid.cellSize;
        
        if (cx >= 0 && cx < mGrid.width && cy >= 0 && cy < mGrid.height) {
            int idx = cy * mGrid.width + cx;
            auto& cell = mGrid.cells[idx];
            if (cell.empty()) {
                mGrid.usedIndices.push_back(idx);
            }
            cell.push_back(e);
        }
    };

    for (const auto& uptr : mActiveEntities) {
        if (Entity* e = uptr.get()) insertToGrid(e);
    }
    
    // Removed sleeping entities from grid to avoid 1500+ insertToGrid calculations every frame
    
    if (mPlayerPtr) {
        insertToGrid(mPlayerPtr);
    }
}

std::vector<Entity*> EntityManager::querySpatialGrid(sf::Vector2f pos, float radius) const {
    std::vector<Entity*> results;
    
    int cellRadius = static_cast<int>(std::ceil(radius / mGrid.cellSize));
    
    int cx = static_cast<int>(pos.x) / mGrid.cellSize;
    int cy = static_cast<int>(pos.y) / mGrid.cellSize;
    
    int startX = std::max(0, cx - cellRadius);
    int endX   = std::min(mGrid.width - 1, cx + cellRadius);
    int startY = std::max(0, cy - cellRadius);
    int endY   = std::min(mGrid.height - 1, cy + cellRadius);
    
    for (int y = startY; y <= endY; ++y) {
        for (int x = startX; x <= endX; ++x) {
            const auto& cellOps = mGrid.cells[y * mGrid.width + x];
            results.insert(results.end(), cellOps.begin(), cellOps.end());
        }
    }
    
    return results;
}
