#pragma once
#include "Skill.h"
#include <vector>
#include <map>
#include <memory> // [ARCHI] unique_ptr
#include <functional> // [ARCHI] Factory
#include <string>
#include "../engine/ResourceManager.h"

// TinyJson forward decl or include in cpp
// We need to verify where TinyJson is, usually utils/TinyJson.h

class SkillManager {
public:
    SkillManager(ResourceManager& res);
    
    // Load from assets/data/skills.json
    bool loadSkills(const std::string& filepath);
    
    const Skill* getSkill(int id) const;
    std::unique_ptr<Skill> cloneSkill(int id) const;
    
    const std::map<int, std::unique_ptr<Skill>>& getSkills() const { return mSkills; }

    // 1. Sistema de Registro de Clases (Factory Pattern)
    using SkillFactory = std::function<std::unique_ptr<Skill>()>;
    
    template<typename T>
    void registerSkillType(int id) {
        mFactories[id] = []() { return std::make_unique<T>(); };
    }

private:
    ResourceManager& mRes;
    std::map<int, std::unique_ptr<Skill>> mSkills;
    std::map<int, SkillFactory> mFactories; // [ARCHI] Factory Map
};
