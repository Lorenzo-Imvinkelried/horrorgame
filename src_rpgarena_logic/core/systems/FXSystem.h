#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <cstdint> 

// Enum para tipos de texto flotante
enum class FloatingTextType {
    Damage,
    Crit,
    TrueDamage,
    Bleed, // [NEW] Distinct type to avoid conflict
    Heal,
    Mana,
    XP,
    Miss,
    Generic
};

// [OPTIMIZATION] Batching: struct is now POD-like
struct FloatingText {
    // Physics
    sf::Vector2f position;
    sf::Vector2f velocity;
    float lifetime; 
    float maxLifetime;
    bool isActive = false;

    // Render Data
    int value = 0;           // Numbers only
    char cachedText[16];     // [OPTIMIZATION] Pre-formatted text (avoid to_string every frame)
    sf::Color color = sf::Color::White;
    float currentScale = 1.0f;
    float targetScale = 1.0f;
    
    // Logic keys
    FloatingTextType type = FloatingTextType::Generic;
    const void* owner = nullptr;
    float offsetY = 0.0f; // [NEW] Used to distinguish between hits of a multi-strike
};

// [VFX] Animated Hit Ring
struct HitRing {
    sf::Vector2f position;
    float timer;
    float fps;
    int currentFrame;
    bool active;
};

class FXSystem {
public:
    FXSystem();
    
    void update(sf::Time dt);
    void draw(sf::RenderTarget& target);
    void clear(); 

    void addMiss(const sf::FloatRect& targetBounds, const void* owner = nullptr);
    void addDamageNumber(int damage, const sf::FloatRect& mobBounds, float offsetY = 50.f, bool isCrit = false, float scale = 1.0f, sf::Color colorOverride = sf::Color::Transparent, const void* owner = nullptr);
    void addBleedNumber(int damage, const sf::FloatRect& mobBounds, const void* owner = nullptr); // [NEW]
    void addTrueDamageNumber(int damage, const sf::FloatRect& mobBounds, float offsetY = 65.f, bool isCrit = false, float scale = 1.0f, const void* owner = nullptr);
    void addHealNumber(int amount, const sf::FloatRect& targetBounds, float offsetY = 50.f, const void* owner = nullptr);
    void addManaNumber(int amount, const sf::FloatRect& targetBounds, float offsetY = 50.f, const void* owner = nullptr);
    void addExperienceNumber(int amount, const sf::FloatRect& mobBounds, const void* owner = nullptr);
    void createFloatingText(sf::Vector2f pos, const std::string& msg, sf::Color color, int fontSize, float scale = 1.0f, float offsetY = -30.f);

    // [VFX]
    void addHitRing(sf::Vector2f pos, float fps = 60.f);

    static void setInstance(FXSystem* instance); 
    static FXSystem* getInstance();              

private:
    FloatingText& getNextFreeText(); 

private:
    static FXSystem* sInstance;
    
    // [BATCHING] Text
    sf::Texture mNumberTexture;
    sf::Texture mCritFontTexture; // [NEW] Critical Font (31x7)
    sf::VertexArray mBatchArray; 
    sf::VertexArray mCritBatchArray; // [NEW] Critical Batch
    std::vector<FloatingText> mTexts;
    std::size_t mActiveCount = 0; 

    // [VFX] Hit Rings
    sf::Texture mRingTexture;
    sf::VertexArray mRingBatch;
    std::vector<HitRing> mHitRings;
    std::size_t mActiveRingCount = 0;
};