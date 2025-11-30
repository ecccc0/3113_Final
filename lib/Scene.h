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