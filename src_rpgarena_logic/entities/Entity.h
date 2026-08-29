//Entity.h
#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string> 
#include <functional> // Observer
#include <vector>
#include <algorithm> // remove_if
#include <map> // Stats
#include <cmath> // Visuals
#include "../Config.h"
#include "core/engine/IRenderable.h"
#include "core/stats/Stats.h" // Stats
#include "../core/items/Item.h"

class ResourceManager;

class Entity : public IRenderable {
public:
    Entity() = default;
    virtual ~Entity() = default;

    virtual void update(sf::Time dt) = 0;
    virtual void draw(sf::RenderTarget& target, sf::RenderStates states = sf::RenderStates::Default) = 0;
    virtual void drawLayer(sf::RenderTarget& target, int layer, sf::RenderStates states = sf::RenderStates::Default) {
        draw(target, states);
    }
    virtual void onWake() {} // Called when waking up from sleep

    // Implementación de IRenderable
    void getRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const override;
    void getShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture) const override;
    virtual void getWeaponShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture, int slotIndex) const {}
    virtual void getArmorShadowRenderData(std::vector<sf::Vertex>& vertices, const sf::Texture*& texture, int slotIndex) const {}
    
    // Helper para saber si se puede usar el batcher
    virtual bool isBatchable() const { return true; }

    // Optimization
    RenderType getRenderType() const override { return RenderType::Entity; }
    bool castsShadow() const override { return true; }

    // --- OBSERVER PATTERN ---
    using Observer = std::function<void()>;
    
    int addStatsObserver(Observer obs) {
        int id = mNextObserverId++;
        mStatsObservers.emplace_back(id, obs);
        return id;
    }

    void removeStatsObserver(int id) {
        auto it = std::remove_if(mStatsObservers.begin(), mStatsObservers.end(), 
            [id](const auto& pair) { return pair.first == id; });
        mStatsObservers.erase(it, mStatsObservers.end());
    }
    
    void notifyStatsChanged() {
        for (auto& pair : mStatsObservers) {
            if (pair.second) pair.second();
        }
    }
    // ------------------------

    // --- Helper Functions ---
    void setPosition(sf::Vector2f pos) { mPos = pos; }
    void setPosition(float x, float y) { mPos = {x, y}; }
    virtual void resetIK() {}
    
    sf::Vector2f getPosition() const { return mPos; }
    virtual sf::Vector2f getVelocity() const { return {0.f, 0.f}; }

    // Visual Debug Accessor
    sf::Sprite* getSprite() { return mSprite.get(); }

    // Visual Anchors
    virtual sf::Vector2f getVisualPoint(const std::string& pointName) const {
        if (pointName == "head") {
            sf::Vector2f p = getPosition();
            float h = getVisualHeight();
            if (h > 0.f) {
                p.y -= h;
            } else {
                p.y -= 40.f; // Default fallback
            }
            return p;
        }
        return getPosition(); // Default: Center/Pos
    }

    virtual float getAttackRange() const { return 50.f; }

    // Ground Point Oficial (Pies)
    virtual sf::Vector2f getGroundPosition() const {
        return mPos; // Por defecto es mPos, las clases derivadas (Player, Mob) deben sobrescribirlo
    }

    virtual sf::Vector2f getLeftFootPosition() const { return getGroundPosition(); }
    virtual sf::Vector2f getRightFootPosition() const { return getGroundPosition(); }

    // [GORE] True floor line for death animations
    virtual float getGoreFloorY() const {
        return getGroundPosition().y;
    }

    // Y-Sorting Fix
    virtual float getSortingY() const {
        // Ignorar mDepthOffset en el sorting para que el Z-Order sea puramente 2D.
        return getGroundPosition().y - mDepthOffset;
    }
    
    virtual float getLayerSortingY(int layer) const {
        if (layer == 1) return getSortingY() - 10.f;
        if (layer == 3) return getSortingY() + 10.f;
        return getSortingY();
    }
    
    // --- Física de Profundidad ---
    void setDepthOffset(float depth) { mDepthOffset = depth; }
    float getDepthOffset() const { return mDepthOffset; }
    virtual void setTerrainDeform(const class TerrainDeformSystem* terrain) {}
    
    // Y variables
protected:
    float mDepthOffset = 0.f;
    float mWeightKg = 0.f;
    bool mIsVisible = true;
    bool mIsBoss = false;
public:
    void setIsVisible(bool v) { mIsVisible = v; }
    bool isVisible() const { return mIsVisible; }
    
    void setIsBoss(bool isBoss) { mIsBoss = isBoss; }
    bool isBoss() const { return mIsBoss; }
    
    void setScale(sf::Vector2f scale) { mBaseScale = scale; }
    void setScale(float x, float y) { mBaseScale = {x, y}; }

    void setSpeed(float speed) { mSpeed = speed; }

    bool isCharging() const { return mIsCharging; }
    virtual void setCharging(bool charging) { 
        mIsCharging = charging; 
        notifyStatsChanged();
    }

    bool isCasting() const { return mIsCasting; }
    virtual void setCasting(bool casting) {
        mIsCasting = casting;
        notifyStatsChanged();
    }
    // --- Weight System ---
    void setWeightKg(float weight) { mWeightKg = weight; }
    float getWeightKg() const { return mWeightKg; }
    // ---

    // --- Bounds Accessor ---
    virtual sf::FloatRect getGlobalBounds() const {
        if (mSprite) return mSprite->getGlobalBounds();
        return {};
    }

    // [VISUAL FIX] Bounds specifically for combat visual feedback (Hit rings, particles)
    virtual sf::FloatRect getHitImpactBounds() const {
        sf::FloatRect bounds;
        bounds.size.x = 32.f;
        bounds.size.y = getVisualHeight();
        bounds.position.x = getPosition().x - bounds.size.x * 0.5f;
        bounds.position.y = getPosition().y - bounds.size.y;
        return bounds;
    }

    // [ESTABLE] Altura visual base sin contar escalado dinámico (respiración)
    virtual float getVisualHeight() const { return 0.f; }
    // ---

    // --- Stats Virtuales ---
    virtual std::string getName() const { return "Entidad"; }
    virtual std::string getTitleName() const { return ""; }
    virtual sf::Color getTitleColor() const { return sf::Color::White; }
    virtual int getLevel() const { return mLevel; }
    virtual int getCurrentHp() const { return mCurrentHp; }
    virtual int getMaxHp() const { return mMaxHp; }
    virtual int getCurrentMp() const { return mCurrentMp; }
    virtual int getMaxMp() const { return mMaxMp; }
    
    // Primary Stats
    virtual int getStrength() const { return mStrength; }
    virtual int getAgility() const { return mAgility; }
    virtual int getIntelligence() const { return mIntelligence; }
    virtual int getVitality() const { return mVitality; }
    // ---

    virtual int getDefense() const { return mDefense; }

    // --- Stats de Combate Virtuales ---
    virtual int getAttack() const { return mAttack; }
    virtual float getAtkSpeed() const { return mAtkSpeed; }
    virtual float getCritChance() const { return mCritChance; }
    virtual float getCritDamage() const { return mCritDamage; }
    virtual int getArmorPenetration() const { return mArmorPenetration; }
    virtual float getArmorPenetrationPercent() const { return mArmorPenetrationPercent; }
    virtual float getPhysicalDamageBonus() const { return mPhysicalDamageBonus; }
    virtual int getTrueDamagePercent() const { return mTrueDamagePercent; }
    
    virtual int getAccuracy() const { return mAccuracy; }
    virtual int getEvasion() const { return mEvasion; }
    virtual float getMalice() const { return mMalice; }
    virtual void setMalice(float val) { mMalice = val; }

    virtual float getMovementSpeed() const { 
        if (mIsCharging) return 1000.f;
        return mSpeed; 
    }

    // Execute Logic 
    virtual float getExecuteThresholdFactor() const { return mExecuteThresholdFactor; } 
    virtual float getExecuteDamageMultiplier() const { return mExecuteDamageMultiplier; }
    virtual int getExecuteDamagePercent() const { return mExecuteDamagePercent; }
    virtual int getExecuteHealthThresholdPercent() const { return mExecuteHealthThresholdPercent; }

    // Bonus Damage
    virtual float getEnemyMaxHpDamagePercent() const { return mEnemyMaxHpDamagePercent; }

    // Effects
    virtual float getLifestealPercent() const { return mLifestealPercent; }
    virtual float getCooldownReductionPercent() const { return mCooldownReductionPercent; }

    virtual float getDoubleStrikeChance() const { return mDoubleStrikeChance; }
    virtual float getTripleStrikeChance() const { return mTripleStrikeChance; }
    
    // AoE
    virtual float getAoeRadius() const { return mAoeRadius; }
    virtual float getAoeDamagePercent() const { return mAoeDamagePercent; }

    // Defensive & Regen
    virtual float getBlockChance() const { return mBlockChance; }
    virtual float getBlockValuePercent() const { return mBlockValuePercent; }
    virtual float getThornsPercent() const { return mThornsPercent; }
    virtual float getHpRegenPercent() const { return mHpRegenPercent; }
    virtual float getMpRegenPercent() const { return mMpRegenPercent; }

    virtual bool debugAddStat(const std::string& statL, float amount, bool isFixed = false) { return false; }

    // --- Cooldowns [SKILLS] ---
    void setSkillCooldown(int skillId, float duration, float baseCooldown = -1.f) {
        mSkillCooldowns[skillId] = duration;
        if (baseCooldown >= 0.f) {
            mSkillBaseCooldowns[skillId] = baseCooldown;
            mSkillOriginalDurations[skillId] = duration;
        } else {
            mSkillBaseCooldowns[skillId] = duration;
            mSkillOriginalDurations[skillId] = duration;
        }
    }
    float getSkillCooldown(int skillId) const {
        auto it = mSkillCooldowns.find(skillId);
        if (it != mSkillCooldowns.end())
            return it->second;
        return 0.f;
    }
    bool isSkillReady(int skillId) const {
        return getSkillCooldown(skillId) <= 0.f;
    }
    void updateCooldowns(sf::Time dt) {
        float s = dt.asSeconds();
        for (auto it = mSkillCooldowns.begin(); it != mSkillCooldowns.end();) {
            it->second -= s;
            if (it->second <= 0.f) {
                mSkillBaseCooldowns.erase(it->first);
                mSkillOriginalDurations.erase(it->first);
                it = mSkillCooldowns.erase(it);
            } else {
                ++it;
            }
        }
    }
    void recalculateCooldowns(float oldCDR, float newCDR) {
        for (auto& pair : mSkillCooldowns) {
            int skillId = pair.first;
            float remaining = pair.second;
            
            float baseCd = mSkillBaseCooldowns[skillId];
            float originalDur = mSkillOriginalDurations[skillId];
            
            if (originalDur > 0.f) {
                float percentElapsed = 1.0f - (remaining / originalDur);
                percentElapsed = std::clamp(percentElapsed, 0.f, 1.f);
                
                float newDur = baseCd * (1.0f - newCDR / 100.f);
                float newRemaining = newDur * (1.0f - percentElapsed);
                
                pair.second = std::max(0.f, newRemaining);
                mSkillOriginalDurations[skillId] = newDur;
            }
        }
        
        // Clean up finished cooldowns
        for (auto it = mSkillCooldowns.begin(); it != mSkillCooldowns.end();) {
            if (it->second <= 0.f) {
                mSkillBaseCooldowns.erase(it->first);
                mSkillOriginalDurations.erase(it->first);
                it = mSkillCooldowns.erase(it);
            } else {
                ++it;
            }
        }
    }

    virtual void onAggroedBy(Entity* attacker) {}

    // --- DATA-DRIVEN STAT MODIFIERS ---
    void setStatModifier(Stat stat, float value) {
        mStatModifiers[stat] = value;
        notifyStatsChanged(); // Observer
    }

    void addStatModifier(Stat stat, float value) {
        mStatModifiers[stat] += value;
        notifyStatsChanged(); // Observer
    }

    void removeStatModifier(Stat stat, float value) {
        mStatModifiers[stat] -= value;
        // Clean up small float errors?
        if (std::abs(mStatModifiers[stat]) < 0.001f) {
            mStatModifiers.erase(stat);
        }
        notifyStatsChanged();
    }

    float getStatModifier(Stat stat) const {
        auto it = mStatModifiers.find(stat);
        if (it != mStatModifiers.end()) return it->second;
        return 0.0f;
    }

    // --- BUFF SYSTEM ---
    struct ActiveBuff {
        Stat stat;      // Stat being buffered
        float value;    // Amount being buffered
        float duration; // Time remaining
        int skillId;    // Source skill (optional)
        std::string statusEffectId; // [NEW] Associated UI effect id
    };

    void applyBuff(Stat stat, float value, float duration, int skillId = -1, const std::string& statusEffectId = "") {
        // 1. Add instant modifier
        addStatModifier(stat, value);
        
        // 2. Add to active list for timer
        if (duration > 0.f) {
            mActiveBuffs.push_back({stat, value, duration, skillId, statusEffectId});
        }
        
        onBuffsChanged();
    }

    void updateBuffs(sf::Time dt) {
        float s = dt.asSeconds();
        bool changed = false;
        // Iterate backwards to remove easily
        for (int i = mActiveBuffs.size() - 1; i >= 0; --i) {
            mActiveBuffs[i].duration -= s;
            if (mActiveBuffs[i].duration <= 0.f) {
                // Remove expired buff
                removeStatModifier(mActiveBuffs[i].stat, mActiveBuffs[i].value);
                mActiveBuffs.erase(mActiveBuffs.begin() + i);
                changed = true;
            }
        }

        // Tick down new crowd control/status effects
        for (auto it = mActiveEffects.begin(); it != mActiveEffects.end(); ) {
            it->second -= s;
            if (it->second <= 0.f) {
                it = mActiveEffects.erase(it);
                changed = true;
            } else {
                ++it;
            }
        }

        if (changed) {
            // [FIX] Ensure derived classes recalculate (Player needs it)
            // But verify if notifyStatsChanged triggers it? 
            // Entity::notifyStatsChanged only notifies observers (UI).
            // It does NOT call recalculateStats().
            // Player overrides notifyStatsChanged? Use virtual onBuffExpired?
            // Easiest: call virtual notifyStatsChanged() which we already do in removeStatModifier.
            // Wait, removeStatModifier CALLS notifyStatsChanged().
            // BUT notifyStatsChanged just updates UI. It doesn't update 'mAttack', 'mAtkSpeed'.
            // Player::recalculateStats() is what updates those.
            // We need a virtual hook.
            onBuffsChanged(); 
        }
    }
    
    virtual void onBuffsChanged() {
         // Default empty. Player will override.
         notifyStatsChanged();
    }

    // --- HELPER STRUCTS ---
    struct BleedState {
        bool  active = false;
        float durationFlat = 0.f;    
        float durationPercent = 0.f; 
        
        float tickTimer = 0.f; 
        int   flatDamage = 0;
        float percentDamage = 0.f;
        Entity* source = nullptr;

        void reset() {
            active = false;
            durationFlat = 0.f;
            durationPercent = 0.f;
            tickTimer = 0.f;
            flatDamage = 0;
            percentDamage = 0.f;
            source = nullptr;
        }
        bool isBleeding() const { return durationFlat > 0.f || durationPercent > 0.f; }
    };

    struct StunState {
        bool active = false;
        float duration = 0.f;

        void reset() {
             active = false;
             duration = 0.f;
        }
        bool isStunned() const { return active && duration > 0.f; }
    };
    
    struct DebuffState {
        float slowMoveTimer = 0.f;
        float slowMovePercent = 0.f;

        float slowAttackTimer = 0.f;
        float slowAttackPercent = 0.f;

        void reset() {
            slowMoveTimer = 0.f;
            slowMovePercent = 0.f;
            slowAttackTimer = 0.f;
            slowAttackPercent = 0.f;
        }

        bool isSlowedMove() const { return slowMoveTimer > 0.f; }
        bool isSlowedAttack() const { return slowAttackTimer > 0.f; }
    };
    // -----------------------
    
    // [BLEED STATS]
    virtual float getBleedDurationFlat() const { return mBleedDurationFlat; }
    virtual float getBleedDurationPercent() const { return mBleedDurationPercent; }
    virtual int   getBleedFlat() const { return mBleedFlat; }
    virtual float getBleedPercent() const { return mBleedPercent; }

    // Función para recibir daño
    virtual int takeDamage(int damageAmount, Entity* attacker, bool isCrit = false, bool isTrueDamage = false) { return 0; }
    virtual void heal(int amount) { /* Por defecto no hace nada, override en Player/Mob */ }
    virtual void restoreMana(int amount) { /* Por defecto no hace nada, override en Player/Mob */ }
    virtual void setCurrentHp(int hp) {}
    virtual void setCurrentMp(int mp) {}
    
    virtual void startAttackAnimation(Entity* target = nullptr, float speedMultiplier = 1.0f) { /* Por defecto, no hace nada */ }
    virtual void startShieldAttackAnimation(Entity* target = nullptr, float speedMultiplier = 1.0f) { startAttackAnimation(target, speedMultiplier); }
    virtual int getFacingDir() const { return 1; }
    virtual void setFacingDir(int dir) {}
    
    // Función para saber si está vivo
    virtual bool isAlive() const { return true; }

    // (Para que la UI sepa si debe pintar el nombre de rojo)
    virtual bool isAggro() const { return false; }


    virtual bool isRemovable() const { return false; }

    virtual void returnToSpawn() {  }

    // --- Unified Weapon Interface ---
    virtual std::shared_ptr<class Item> getWeapon(int slotIndex = 0) const { return nullptr; }
    virtual void equipWeapon(std::shared_ptr<class Item> item, ResourceManager& res, int slotIndex = 0) {}
    virtual void unequipWeapon(int slotIndex = 0) {}
    virtual bool hasWeaponEquipped() const { return false; }
    virtual bool hasTwoHandedWeaponEquipped() const { return false; }
    virtual void emitWeaponGibs(class GoreSystem& gore, float floorY, sf::Vector2f sourcePos = {0.f, 0.f}, float forceMultiplier = 1.0f) {}

    // --- Unified Shield / Guard Interface ---
    virtual bool hasShieldEquipped() const { return false; }
    virtual bool isGuardActive() const { return mIsGuardActive; }
    virtual void setGuardActive(bool active) { mIsGuardActive = active; }

    // --- Unified Equipment Interface ---
    virtual void equipItem(std::shared_ptr<class Item> item, EquipmentSlot slot, ResourceManager& res) {}
    virtual void unequipItem(EquipmentSlot slot) {}
    virtual std::shared_ptr<class Item> getEquippedItem(EquipmentSlot slot) const { return nullptr; }


    // Devuelve true si la entidad está en proceso de "evadir" y volver a su spawn.
    virtual bool isReturningToSpawn() const { return false; }
    virtual std::vector<std::string> getNodeNames() const { return {}; }

    // New Stats: Virtual Getters
    virtual float getTenacityPercent() const { return mTenacityPercent; }
    virtual float getDamageReductionPercent() const { return mDamageReductionPercent; }
    virtual float getCritAvoidancePercent() const { return mCritAvoidancePercent; }
    virtual float getAntiArmorPenPercent() const { return mAntiArmorPenPercent; }
    virtual int   getAntiArmorPenFlat() const { return mAntiArmorPenFlat; }
    virtual float getManaStealPercent() const { return mManaStealPercent; }
    virtual float getXpBonusPercent() const { return mXpBonusPercent; }

    // --- UNIFIED STATUS EFFECT SYSTEM ---
    void applyStatusEffect(StatusEffect effect, float duration) {
        if (effect == StatusEffect::Stun) {
            applyStun(duration);
            return;
        }

        if (duration <= 0.f) return;
        
        float effectiveDuration = duration;
        // Tenacity reduces CC duration
        if (effect == StatusEffect::Fear || effect == StatusEffect::Root || 
            effect == StatusEffect::Silence || effect == StatusEffect::Polymorph) {
            float tenacity = getTenacityPercent();
            effectiveDuration = duration * (1.0f - (tenacity / 100.0f));
        }

        if (effectiveDuration <= 0.05f) return;

        mActiveEffects[effect] = std::max(mActiveEffects[effect], effectiveDuration);
        notifyStatsChanged();
    }

    bool hasStatusEffect(StatusEffect effect) const {
        if (effect == StatusEffect::Stun) {
            return isStunned();
        }
        auto it = mActiveEffects.find(effect);
        return it != mActiveEffects.end() && it->second > 0.f;
    }

    float getStatusEffectDuration(StatusEffect effect) const {
        if (effect == StatusEffect::Stun) {
            return mStunState.duration;
        }
        auto it = mActiveEffects.find(effect);
        if (it != mActiveEffects.end()) return it->second;
        return 0.f;
    }

    void removeStatusEffect(StatusEffect effect) {
        if (effect == StatusEffect::Stun) {
            mStunState.reset();
            notifyStatsChanged();
            return;
        }
        if (mActiveEffects.erase(effect) > 0) {
            notifyStatsChanged();
        }
    }

    // Stun
    void applyStun(float duration) {
        if (duration <= 0.f) return;
        
        // Tenacity Logic: Reduce duration by %
        float tenacity = getTenacityPercent();
        float effectiveDuration = duration * (1.0f - (tenacity / 100.0f));
        
        if (effectiveDuration <= 0.05f) return; // Immune if reduces to ~0

        mStunState.active = true;
        mStunState.duration = effectiveDuration; 
    }
    
    // Debuffs: Application Helpers
    void applySlowMove(float duration, float percent) {
        if (duration <= 0.f || percent <= 0.f) return;
        mDebuffState.slowMoveTimer = duration;
        mDebuffState.slowMovePercent = percent; 
        // Logic: Re-applying resets timer and updates percent (could be smarter implies MAX but simple overwrite is standard)
    }

    void applySlowAttack(float duration, float percent) {
        if (duration <= 0.f || percent <= 0.f) return;
        mDebuffState.slowAttackTimer = duration;
        mDebuffState.slowAttackPercent = percent;
    }
    
    // Bleed: Helper to apply bleed
    void applyBleed(float durationFlat, float durationPct, int flatDmg, float pctDmg, Entity* source) {
        if (durationFlat <= 0.f && durationPct <= 0.f) return;
        
        mBleedState.active = true;
        mBleedState.active = true;
        
        // [BLEED REFACTOR] Refresh Duration (Set to new, don't just max)
        // This ensures consistent duration on re-application.
        mBleedState.durationFlat = durationFlat;
        mBleedState.durationPercent = durationPct;
        
        // [BLEED REFACTOR] Overwrite damage values. 
        // We want the bleed to reflect the CURRENT stats of the attacker (Base Stats), 
        // not snapshot a "High Damage" event (like a Crit or Skill) forever.
        mBleedState.flatDamage = flatDmg;
        mBleedState.percentDamage = pctDmg;
        
        mBleedState.source = source;
        // mBleedState.tickTimer = 0.f; // [FIX] Don't reset tick on refresh!
    }
    
    // Fix: Clear all combat effects
    void clearDebuffs() {
        mBleedState.reset();
        mStunState.reset();
        mDebuffState.reset();
        mActiveEffects.clear();
        notifyStatsChanged();
    }

    // Debuffs: Logic Helpers
    float getSpeedMultiplier() const {
        if (mStunState.isStunned()) return 0.f;
        float mult = 1.0f;
        if (mDebuffState.isSlowedMove()) {
            mult *= std::max(0.1f, 1.0f - mDebuffState.slowMovePercent / 100.f);
        }
        if (isGuardActive()) {
            mult *= 0.70f; // [SHIELD] 30% movement speed reduction while guarding
        }
        if (isCasting()) {
            mult *= 0.70f; // [CASTING] 30% movement speed reduction while casting/channeling
        }
        return mult;
    }

    float getAttackSpeedMultiplier() const {
        if (mStunState.isStunned()) return 0.f; // Cant attack if stunned
        if (mDebuffState.isSlowedAttack()) return std::max(0.1f, 1.0f - mDebuffState.slowAttackPercent / 100.f);
        return 1.0f;
    }

    bool isStunned() const { return mStunState.isStunned(); }
    StunState& getStunState() { return mStunState; }
    const StunState& getStunState() const { return mStunState; }
    
    // Bleed: Accessor
    BleedState& getBleedState() { return mBleedState; }
    const BleedState& getBleedState() const { return mBleedState; }

    DebuffState& getDebuffState() { return mDebuffState; }
    const DebuffState& getDebuffState() const { return mDebuffState; }
    
    
    // Getters for Stats (Offensive - what I apply to others)
    virtual float getStunChance() const { return mStunChance; }
    virtual float getStunDuration() const { return mStunDuration; }

    virtual float getSlowMovePercent() const { return mSlowMovePercent; }
    virtual float getSlowMoveDuration() const { return mSlowMoveDuration; }
    virtual float getSlowAttackPercent() const { return mSlowAttackPercent; }
    virtual float getSlowAttackDuration() const { return mSlowAttackDuration; }


protected:
    float mMalice = 0.f;
    // Bleed
    BleedState mBleedState;
    // Stun
    StunState mStunState;
    DebuffState mDebuffState;
    
    // Stats System
    std::map<Stat, float> mStatModifiers;
    std::vector<ActiveBuff> mActiveBuffs;
    std::map<StatusEffect, float> mActiveEffects;
    // --- VISUAL HIT FEEDBACK ---
    sf::Vector2f mVisualOffset = {0.f, 0.f};
    float mFlashTimer = 0.f;
    float mShakeTimer = 0.f;
    sf::Vector2f mShakeDir = {0.f, 0.f};

    // --- LAST ATTACKER ---
    Entity* mLastAttacker = nullptr;
    bool mLastHitWasDirectAttack = false; // [GORE] true = basic attack, false = AoE/bleed/thorns/effect
public:
    Entity* getLastAttacker() const { return mLastAttacker; }
    void setLastAttacker(Entity* attacker) { mLastAttacker = attacker; }
    
    bool hasBuffFromSkill(int skillId) const {
        for (const auto& buff : mActiveBuffs) {
            if (buff.skillId == skillId) return true;
        }
        return false;
    }

    struct ActiveStatusEffect {
        std::string id;
        float remainingDuration;
        int stacks = 1;
    };
    std::vector<ActiveStatusEffect> getActiveStatusEffects() const;
    const std::vector<ActiveBuff>& getActiveBuffs() const { return mActiveBuffs; }
    
    // [GORE] Track if the hit was a direct melee/basic attack (for knockback physics)
    void setLastHitDirect(bool direct) { mLastHitWasDirectAttack = direct; }
    bool wasLastHitDirectAttack() const { return mLastHitWasDirectAttack; }
    
    // Config constants (can be virtual getters if needed)
    const float HIT_FLASH_DURATION = 0.15f; 
    const float HIT_SHAKE_DURATION = 0.20f;

public:
    // Call this when taking damage
    void triggerHitEffect(sf::Vector2f sourcePos);

    void updateVisuals(sf::Time dt) {
        float s = dt.asSeconds();
        
        // Flash decay
        if (mFlashTimer > 0.f) mFlashTimer -= s;
        
        // Shake/Knockback logic
        if (mShakeTimer > 0.f) {
            mShakeTimer -= s;
            
            // Simple ease-out: starts at mShakeDir, goes to 0
            float progress = mShakeTimer / HIT_SHAKE_DURATION; // 1.0 -> 0.0
            if (progress < 0.f) progress = 0.f;
            
            // "Elastic" or simple linear return?
            // ongoing visual offset is linear interpolation based on remaining time
            mVisualOffset = mShakeDir * progress; 
            
            // Optional: Add random noise for "Shake"
            // float noiseX = (rand() % 10 - 5) * 0.5f * progress;
            // float noiseY = (rand() % 10 - 5) * 0.5f * progress;
            // mVisualOffset += sf::Vector2f(noiseX, noiseY);
            
        } else {
            mVisualOffset = {0.f, 0.f};
        }
    }
    
    bool isFlashing() const { return mFlashTimer > 0.f; }
    sf::Vector2f getVisualPosition() const { return mPos + mVisualOffset + sf::Vector2f(0.f, mDepthOffset); }
    virtual class Animation* getAnimation() { return nullptr; }
    virtual const class Animation* getAnimation() const { return nullptr; }

protected: // Back to protected for existing members if any
    sf::Vector2f mPos;
    float mSpeed = 0.f;
    bool mIsCharging = false;
    bool mIsCasting = false;
    sf::Vector2f mBaseScale = {1.f, 1.f};
    sf::Vector2i mFrameSize{0, 0}; 
    
    // Observer
    std::vector<std::pair<int, Observer>> mStatsObservers;
    int mNextObserverId = 1;
    std::unique_ptr<sf::Sprite> mSprite;

    // --- Shared Combat Stats ---
    int mLevel = 1;
    int mCurrentHp = 0;
    int mMaxHp = 0;
    int mCurrentMp = 0;
    int mMaxMp = 0;
    int mStrength = 0;
    int mAgility = 0;
    int mIntelligence = 0;
    int mVitality = 0;
    int mAttack = 0;
    int mDefense = 0;
    int mAccuracy = 0;
    int mEvasion = 0;

    float mCritChance = 0.f;
    float mCritDamage = 1.5f;
    float mAtkSpeed = 1.f;
    int mArmorPenetration = 0;
    float mArmorPenetrationPercent = 0.f;
    float mPhysicalDamageBonus = 0.f;
    float mLifestealPercent = 0.f;
    float mCooldownReductionPercent = 0.f;

    float mDoubleStrikeChance = 0.f;
    float mTripleStrikeChance = 0.f;
    float mEnemyMaxHpDamagePercent = 0.f;

    float mBlockChance = 0.f;
    float mBlockValuePercent = 0.f;
    float mThornsPercent = 0.f;
    float mHpRegenPercent = 0.f;
    float mMpRegenPercent = 0.f;

    float mTenacityPercent = 0.f;
    float mDamageReductionPercent = 0.f;
    float mCritAvoidancePercent = 0.f;
    float mAntiArmorPenPercent = 0.f;
    int mAntiArmorPenFlat = 0;
    float mManaStealPercent = 0.f;
    float mXpBonusPercent = 0.f;

    float mAoeRadius = 0.f;
    float mAoeDamagePercent = 0.f;

    float mBleedDurationFlat = 0.f;
    float mBleedDurationPercent = 0.f;
    int mBleedFlat = 0;
    float mBleedPercent = 0.f;

    float mStunChance = 0.f;
    float mStunDuration = 0.f;

    float mSlowMovePercent = 0.f;
    float mSlowMoveDuration = 0.f;
    float mSlowAttackPercent = 0.f;
    float mSlowAttackDuration = 0.f;

    int mExecuteDamagePercent = 0;
    int mExecuteHealthThresholdPercent = 0;
    float mExecuteDamageMultiplier = 1.0f;
    float mExecuteThresholdFactor = 0.0f;
    int mTrueDamagePercent = 0;
    bool mIsGuardActive = false;

protected:
    std::map<int, float> mSkillCooldowns;
    std::map<int, float> mSkillBaseCooldowns;
    std::map<int, float> mSkillOriginalDurations;
};