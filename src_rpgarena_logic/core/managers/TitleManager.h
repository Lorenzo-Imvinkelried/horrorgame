#pragma once
#include <string>
#include <vector>
#include <map>
#include <SFML/Graphics.hpp>
#include "core/stats/Stats.h"

struct Title {
    std::string id;
    std::string name;
    sf::Color color = sf::Color::White;
    std::map<Stat, float> stats;
};

class TitleManager {
public:
    static TitleManager& getInstance() {
        static TitleManager instance;
        return instance;
    }

    bool loadTitles(const std::string& filepath);
    const Title* getTitle(const std::string& id) const;
    const std::vector<Title>& getAllTitles() const { return mTitles; }

private:
    TitleManager() = default;
    ~TitleManager() = default;
    TitleManager(const TitleManager&) = delete;
    TitleManager& operator=(const TitleManager&) = delete;

    std::vector<Title> mTitles;
    std::map<std::string, size_t> mTitleLookup; // id -> index in mTitles
};
