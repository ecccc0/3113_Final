#include "LevelOne.h"

// --- LEVEL DATA ---
// 1 = Wall (Solid), 0 = Floor (Passable)
// This is a 14x10 room for the prototype
unsigned int LEVEL_1_DATA[] =
{
    // 1 = Wall, 2 = Floor (Index 0 is nothing/transparent)
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
    1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

extern std::vector<std::vector<bool>> gSceneEnemyDefeated; // from main.cpp
extern int gCurrentLevelIndex; // to index gSceneEnemyDefeated

void LevelOne::initialise()
{
    mGameState.nextSceneID = -1;

    // Clean any previous allocations (re-entering scene)
    if (mGameState.worldEnemies) {
        delete[] mGameState.worldEnemies;
        mGameState.worldEnemies = nullptr;
    }

    // 1. LOAD MAP (only allocate once; recreate each initialise for simplicity)
    if (mGameState.map) {
        delete mGameState.map;
        mGameState.map = nullptr;
    }
    mGameState.map = new Map(14, 10, LEVEL_1_DATA, "assets/tileset.png", 32.0f, 4, 1, mOrigin);

    // 2. CREATE PLAYER (preserve if already exists?) For prototype recreate.
    if (mGameState.player) {
        delete mGameState.player;
        mGameState.player = nullptr;
    }
    mGameState.player = new Entity(
        { 500.0f, 250.0f },
        { 32.0f, 32.0f },
        "assets/player_joker.png",
        PLAYER
    );
    mGameState.player->setAcceleration({ 0.0f, 0.0f });

    // For prototype we have 1 enemy; ensure global defeated flags sized
    if (gCurrentLevelIndex >= 0) {
        if (gSceneEnemyDefeated.size() <= (size_t)gCurrentLevelIndex)
            gSceneEnemyDefeated.resize(gCurrentLevelIndex + 1);
        // Resize enemy flags for this level (enemyCount planned = 1)
        if (gSceneEnemyDefeated[gCurrentLevelIndex].size() < 1)
            gSceneEnemyDefeated[gCurrentLevelIndex].resize(1, false);
        mEnemyDefeated = gSceneEnemyDefeated[gCurrentLevelIndex];
    }

    // 3. CREATE ENEMY ONLY IF NOT DEFEATED
    bool enemy0Defeated = (!mEnemyDefeated.empty() && mEnemyDefeated[0]);
    if (!enemy0Defeated) {
        mGameState.enemyCount = 1;
        mGameState.worldEnemies = new Entity[mGameState.enemyCount];
        mGameState.worldEnemies[0] = Entity();
        mGameState.worldEnemies[0].setPosition({ 600.0f, 200.0f });
        mGameState.worldEnemies[0].setScale({ 32.0f, 32.0f });
        mGameState.worldEnemies[0].setTexture("assets/enemy_shadow.png");
        mGameState.worldEnemies[0].setEntityType(NPC);
        mGameState.worldEnemies[0].activate();
        mGameState.worldEnemies[0].setAcceleration({ 0.0f, 0.0f });
        mGameState.worldEnemies[0].setAIType(WANDERER);
        mGameState.worldEnemies[0].setAIState(IDLE);
    } else {
        mGameState.enemyCount = 0;
        mGameState.worldEnemies = nullptr;
    }

    // 4. SETUP CAMERA
    mGameState.camera.target = mGameState.player->getPosition();
    mGameState.camera.offset = mOrigin;
    mGameState.camera.rotation = 0.0f;
    mGameState.camera.zoom = 2.0f;
}

void LevelOne::update(float deltaTime)
{
    // 1. Update Player (Collision with Map)
    // We pass 'enemies' as collidable entities if we want physical blocking,
    // but here we handle the trigger manually below.
    mGameState.player->update(deltaTime, mGameState.player, mGameState.map, NULL, 0);
    
    // 2. Update Enemies
    for (int i = 0; i < mGameState.enemyCount; i++)
    {
        // Update Enemy Logic
        mGameState.worldEnemies[i].update(deltaTime, mGameState.player, mGameState.map, NULL, 0);

        // 3. COMBAT TRIGGER
        // If Player touches an Active Enemy
        if (mGameState.worldEnemies[i].isActive() && mGameState.player->isColliding(&mGameState.worldEnemies[i]))
        {
            mGameState.nextSceneID = 2; // combat scene index
            mGameState.engagedEnemyIndex = i; // which enemy engaged
            mGameState.returnSceneID = gCurrentLevelIndex; // come back here after combat
            std::cout << "Transitioning to Combat Scene! Enemy index=" << i << std::endl;
            // Do not mark defeated yet; only after victory in CombatScene
        }
    }

    // 4. Camera Follow
    // Smooth camera following could go here, but hard snapping is fine for prototype
    mGameState.camera.target = mGameState.player->getPosition();
}

void LevelOne::render()
{
    // Draw Room
    if (mGameState.map) mGameState.map->render();

    // Draw Enemies
    for (int i = 0; i < mGameState.enemyCount; i++)
    {
        mGameState.worldEnemies[i].render();
    }

    // Draw Player
    if (mGameState.player) mGameState.player->render();
}