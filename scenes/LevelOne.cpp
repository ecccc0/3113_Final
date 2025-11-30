#include "LevelOne.h"
#include <cmath> // atan2f for debug cone rendering

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
    // Clean previous followers if re-entering
    if (!mFollowers.empty()) {
        for (Entity* f : mFollowers) {
            delete f;
        }
        mFollowers.clear();
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

    // 2b. CREATE PARTY FOLLOWERS (Skull, Mona, Noir)
    // Spawn behind player with slight offsets
    Vector2 playerPos = mGameState.player->getPosition();
    const float spacing = 40.0f;
    Entity* skull = new Entity({ playerPos.x - spacing, playerPos.y + spacing }, { 32.0f, 32.0f }, "assets/player_skull.png", NPC);
    Entity* mona  = new Entity({ playerPos.x - spacing * 2.0f, playerPos.y + spacing }, { 32.0f, 32.0f }, "assets/player_mona.png", NPC);
    Entity* noir  = new Entity({ playerPos.x - spacing * 3.0f, playerPos.y + spacing }, { 32.0f, 32.0f }, "assets/player_noir.png", NPC);

    skull->setAIType(FOLLOWER); skull->setAIState(IDLE); skull->activate(); skull->setAcceleration({0.0f,0.0f});
    mona->setAIType(FOLLOWER);  mona->setAIState(IDLE);  mona->activate();  mona->setAcceleration({0.0f,0.0f});
    noir->setAIType(FOLLOWER);  noir->setAIState(IDLE);  noir->activate();  noir->setAcceleration({0.0f,0.0f});

    mFollowers.push_back(skull);
    mFollowers.push_back(mona);
    mFollowers.push_back(noir);

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
        mGameState.worldEnemies[0].setSpeed(80);
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
    // --- PLAYER UPDATE & MAP INTERACTION ---
    mGameState.player->update(deltaTime, mGameState.player, mGameState.map, NULL, 0);
    if (mGameState.map && mGameState.player)
    {
        Vector2 pPos = mGameState.player->getPosition();
        mGameState.map->revealTiles(pPos, 200.0f);
    }

    // --- FOLLOWER PARTY PHYSICS ---
    constexpr float TETHER_SPEED    = 0.08f;
    constexpr float REPEL_STRENGTH  = 20000.0f;
    constexpr float JITTER_STRENGTH = 5.0f;
    constexpr float DAMPING         = 0.90f;
    for (Entity* f : mFollowers) {
        if (!f) continue;
        f->updateFollowerPhysics(mGameState.player, mFollowers, mGameState.map, deltaTime,
            TETHER_SPEED, REPEL_STRENGTH, JITTER_STRENGTH, DAMPING);
    }

    // --- STEALTH / DETECTION CONSTANTS ---
    const float AMBUSH_DISTANCE = 60.0f; // Close range for back attack
    const float SIGHT_DISTANCE  = 200.0f;
    const float SIGHT_ANGLE     = 90.0f; // Full cone angle
    bool isSpotted = false; // Aggregate spotted state (for shader/effects)

    // --- ENEMY UPDATE & COMBAT TRIGGERS ---
    for (int i = 0; i < mGameState.enemyCount; i++)
    {
        Entity* enemy  = &mGameState.worldEnemies[i];
        Entity* player = mGameState.player;

        enemy->update(deltaTime, player, mGameState.map, NULL, 0);
        if (!enemy->isActive()) continue;

        // A. Detection (view cone)
        bool inSight = enemy->isEntityInSight(player, SIGHT_DISTANCE, SIGHT_ANGLE);
        // Optional: line-of-sight check via map raycast (commented placeholder)
        // if (inSight && mGameState.map && !mGameState.map->hasLineOfSight(enemy->getPosition(), player->getPosition())) inSight = false;
        if (inSight) {
            isSpotted = true;
            // Potential future behavior: enemy->setAIState(FOLLOWING);
        }

        // B. Ambush Attempt (player advantage)
        if (IsKeyPressed(KEY_SPACE)) {
            float distToEnemy = Vector2Distance(player->getPosition(), enemy->getPosition());
            if (distToEnemy < AMBUSH_DISTANCE) {
                if (player->checkAmbush(enemy)) {
                    std::cout << "AMBUSH SUCCESS!" << std::endl;
                    mGameState.nextSceneID       = 2; // CombatScene
                    mGameState.engagedEnemyIndex = i;
                    mGameState.returnSceneID     = gCurrentLevelIndex;
                    // Advantage flag could be set here (e.g., gCombatAdvantage = PLAYER_ADVANTAGE)
                    return; // Early transition
                }
            }
        }

        // C. Collision Trigger (Enemy advantage / neutral)
        if (player->isColliding(enemy)) {
            std::cout << "DETECTED/COLLISION!" << std::endl;
            mGameState.nextSceneID       = 2;
            mGameState.engagedEnemyIndex = i;
            mGameState.returnSceneID     = gCurrentLevelIndex;
            // Determine advantage placeholder:
            // if (inSight || enemy->getAIState() == FOLLOWING) { /* gCombatAdvantage = ENEMY_ADVANTAGE; */ }
            // else { /* gCombatAdvantage = NEUTRAL; */ }
            return;
        }
    }

    // Potential shader/effect hook: pass isSpotted
    // mShader.setInt("status", isSpotted ? 1 : 0);

    // --- CAMERA FOLLOW ---
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

    // Draw Followers
    for (Entity* f : mFollowers) {
        if (f) f->render();
    }

    // Draw Player
    if (mGameState.player) mGameState.player->render();

    // DEBUG: Draw enemy view cones (simple sectors)
    for (int i = 0; i < mGameState.enemyCount; i++)
    {
        Entity* e = &mGameState.worldEnemies[i];
        if (!e->isActive()) continue;

        Vector2 pos = e->getPosition();
        Vector2 dir = e->getDirectionVector();
        // Convert facing vector to angle in degrees (Raylib expects degrees)
        float angleDeg = atan2f(dir.y, dir.x) * (180.0f / PI);
        // Draw a 90-degree cone (±45°) with radius matching SIGHT_DISTANCE (200)
        DrawCircleSector(pos, 200.0f, angleDeg - 45.0f, angleDeg + 45.0f, 10, Fade(RED, 0.2f));
    }
}