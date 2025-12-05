#ifndef SCENE_H
#define SCENE_H

#include <vector>
#include <string>
#include "Entity.h" 
#include "GameTypes.h" // Use shared Element, Ability, Combatant


struct GameState
{
    Entity *player = nullptr;
    Entity *worldEnemies = nullptr;    
    int enemyCount = 0;   

    Map *map = nullptr;

    std::vector<Combatant> party;
    std::vector<Combatant> battleEnemies;

    std::vector<Item> inventory;

    int currentTurnIndex = 0;
    bool isPlayerTurn = true;
    std::string battleLog;
    
    // Combat advantage: true = player advantage, false = enemy advantage
    bool combatAdvantage = true;




    Music bgm;
    Sound deathSound;
    Sound winSound;
    Sound AttackSound;
    

    Camera2D camera;

    int nextSceneID = -1;
    // New generic combat transition tracking
    int returnSceneID = -1;      // Scene to return to after combat ends
    int engagedEnemyIndex = -1;  // Which enemy was engaged to start combat

    // Position to respawn player when returning from combat
    Vector2 returnSpawnPos = {0, 0};
    bool hasReturnSpawnPos = false;

    // Per-level defeated enemy flags (owned by scene)
    std::vector<bool> defeatedEnemies;

    // Per-level opened chest flags (owned by scene)
    std::vector<bool> openedChests;

    // Per-level map exploration (fog-of-war) state, flattened per-tile flags
    std::vector<bool> revealedTiles;

    int shaderStatus = 0; // 0 = normal, 1 = spotted, 2 = hidden

    // UI Toast for item acquisition
    std::string itemToast = "";
    float itemToastTimer = 0.0f; // seconds remaining
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