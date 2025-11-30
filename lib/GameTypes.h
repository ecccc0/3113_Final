#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <string>
#include <vector>
#include "raylib.h" // Needed for Vector2, Color

// --- ENUMS ---
enum Element { ELEMENT_NONE, PHYS, GUN, FIRE, ICE, ELEC, WIND, PSI, NUKE, BLESS, CURSE };
enum GameStatus { TITLE, EXPLORATION, COMBAT, PAUSED, GAME_OVER, WIN };
enum AppStatus { RUNNING, TERMINATED };

// --- STRUCTS ---
struct Ability {
    std::string name;
    int cost;       // SP cost (or HP if not magic)
    int damage;     // Negative for healing
    Element element;
    bool isMagic;   // true = costs SP, false = costs HP
};

struct Combatant {
    int id;
    std::string name;
    std::string texturePath;
    
    // Stats
    int currentHp, maxHp;
    int currentSp, maxSp;
    int attack;
    int defense;
    int speed;
    
    // State
    bool isAlive = true;
    bool isDown = false;
    bool hasActed = false;
    
    // Combat Data
    std::vector<Ability> skills;
    std::vector<Element> weaknesses;
    
    // Visuals (Optional for now)
    Vector2 position = {0,0}; 
    Color tint = WHITE;
};

#endif // GAME_TYPES_H