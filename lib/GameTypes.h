#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <string>
#include <vector>
#include "raylib.h" // Needed for Vector2, Color

// --- ENUMS ---
enum Element { ELEMENT_NONE, PHYS, GUN, FIRE, ICE, ELEC, WIND, PSI, NUKE, BLESS, CURSE };
enum GameStatus { TITLE, EXPLORATION, COMBAT, PAUSED, GAME_OVER, WIN };
enum AppStatus { RUNNING, TERMINATED };
enum EquipmentType { EQUIP_MELEE, EQUIP_GUN, EQUIP_ARMOR };

// --- STRUCTS ---
struct Ability {
    std::string name;
    int cost;       // SP cost (or HP if not magic)
    int damage;     // Negative for healing
    Element element;
    bool isMagic;   // true = costs SP, false = costs HP
};

struct Item {
    std::string name;
    std::string description;
    int value;      // Amount to heal
    bool isSP;      // If true, heals SP. If false, heals HP.
    bool isRevive;  // If true, revives dead ally
    bool isBattle;  // If true, can be used in battle
};

struct Equipment {
    std::string name;
    EquipmentType type;
    int attackPower = 0;
    int defensePower = 0;
    int magazineSize = 0; // Only for GUN
    Element element = ELEMENT_NONE;  // Usually PHYS for melee, GUN for guns
    std::string description;
};

// Persona data that can be swapped to modify Joker's combat profile
struct Persona {
    std::string name;
    
    // Stats that this Persona confers to Joker
    int baseAttack;
    int baseDefense;
    // int speed; // removed: speed stat unused
    
    std::vector<Ability> skills;
    std::vector<Element> weaknesses;
};

struct Combatant {
    int id;
    std::string name;
    std::string texturePath;
    
    // Stats
    int currentHp, maxHp;
    int currentSp, maxSp;
    // Base Stats
    int baseAttack;
    int baseDefense;
    // int speed; // removed: speed stat unused
    
    // State
    bool isAlive = true;
    bool isDown = false;
    bool hasActed = false;

    //Combat Flags
    bool isGuarding = false;
    int currentAmmo = 0;

    Equipment meleeWeapon;
    Equipment gunWeapon;
    Equipment armor;
    Equipment accessory;
    
    // Combat Data
    std::vector<Ability> skills;
    std::vector<Element> weaknesses;

    
    // Visuals
    Vector2 position = {0,0}; 
    Color tint = WHITE;

    int getTotalAttack() const { return baseAttack + meleeWeapon.attackPower; }
    int getTotalGunAttack() const { return baseAttack + gunWeapon.attackPower; } // Guns scale off base stats + gun power
    int getTotalDefense() const { return baseDefense + armor.defensePower + accessory.defensePower; }
};

#endif // GAME_TYPES_H