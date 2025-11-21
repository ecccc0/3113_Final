#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>
#include "Entity.h" 

// The Core Persona Elements
enum Element { PHYS, GUN, FIRE, ICE, ELEC, WIND, PSI, NUKE, BLESS, CURSE, ELEMENT_NONE };

struct Ability {
    std::string name;
    int cost;        // HP for Phys, SP for Magic
    int damage;
    Element element;
    bool isMagic;    // True = costs SP, False = costs HP
};

struct Combatant {
    std::string name;
    
    // Stats
    int currentHp, maxHp;
    int currentSp, maxSp;
    int attack;
    int defense;
    int speed;
    std::string texturePath;
    
    // Mechanics
    std::vector<Ability> skills;
    std::vector<Element> weaknesses; // What knocks this unit down?
    
    // Battle State
    bool isAlive = true;
    bool isDown = false;      // The critical "Hold Up" condition
    bool hasActed = false;    // To track round completion
};


struct GameState
{
    Entity *player = nullptr;
    Entity *worldEnemies = nullptr;    
    int enemyCount = 0;   

    Map *map = nullptr;

    std::vector<Combatant> party;
    std::vector<Combatant> battleEnemies;

    int currentTurnIndex = 0;
    bool isPlayerTurn = true;
    std::string battleLog;




    Music bgm;
    Sound deathSound;
    Sound winSound;
    Sound AttackSound;
    

    Camera2D camera;

    int nextSceneID = -1;
    // New generic combat transition tracking
    int returnSceneID = -1;      // Scene to return to after combat ends
    int engagedEnemyIndex = -1;  // Which enemy was engaged to start combat
};

class Scene 
{
protected:
    GameState mGameState;
    Vector2 mOrigin;
    const char *mBGColourHexCode = "#000000";
    
public:
    Scene();
    Scene(Vector2 origin, const char *bgHexCode);

    virtual void initialise() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
    virtual void shutdown() = 0;
    
    GameState&  getState()                 { return mGameState; }
    const GameState& getState() const     { return mGameState; }
    Vector2     getOrigin()          const { return mOrigin;    }
    const char* getBGColourHexCode() const { return mBGColourHexCode; }
};

#endif