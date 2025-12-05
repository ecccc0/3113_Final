#include "LevelThree.h"
#include "../lib/Effects.h"
#include "../lib/GameData.h"
#include <raylib.h>
extern float gMusicVolume;
#include <cmath>

extern int gCurrentLevelIndex;

void LevelThree::initialise()
{
    mGameState.nextSceneID = -1;

    if (mGameState.worldEnemies) { delete[] mGameState.worldEnemies; mGameState.worldEnemies = nullptr; }
    for (Entity* f : mFollowers) { delete f; }
    mFollowers.clear();
    if (mWorldProps) { delete[] mWorldProps; mWorldProps = nullptr; mPropCount = 0; }

    if (mGameState.map) { delete mGameState.map; mGameState.map = nullptr; }
    // Use CombatScene-sized arena map (20x20)
    mGameState.map = new Map(20, 20, mLevelData, "assets/tileset.png", 32.0f, 4, 1, mOrigin);
    if (mGameState.map) {
        mGameState.map->revealTiles(mOrigin, 2000.0f);
    }

    if (mGameState.player) { delete mGameState.player; mGameState.player = nullptr; }
    mGameState.player = new Entity();
    mGameState.player->setEntityType(PLAYER);
    mGameState.player->setPosition({ 500.0f, 250.0f });
    mGameState.player->setScale({ 32.0f, 32.0f });
    mGameState.player->setColliderDimensions({ 28.0f, 28.0f });
    mGameState.player->setTexture("assets/characters.png");
    mGameState.player->setTextureType(ATLAS);
    mGameState.player->setSpriteSheetDimensions({ 5, 4 });
    std::map<Direction, std::vector<int>> walkingAnimation = {
        { DOWN,  { 0, 1, 2, 3 } },
        { RIGHT, { 10, 11, 12, 13 } },
        { LEFT,  { 10, 11, 12, 13 } },
        { UP,    { 15, 16, 17, 18 } }
    };
    mGameState.player->setAnimationAtlas(walkingAnimation);
    mGameState.player->setFrameSpeed(Entity::DEFAULT_FRAME_SPEED);
    mGameState.player->setAcceleration({ 0.0f, 0.0f });
    mGameState.player->setTint(WHITE);

    if (mGameState.hasReturnSpawnPos) {
        mGameState.player->setPosition(mGameState.returnSpawnPos);
        mGameState.hasReturnSpawnPos = false;
    }

    // Followers (same as LevelTwo)
    Vector2 playerPos = mGameState.player->getPosition();
    const float spacing = 40.0f;
    Entity* skull = new Entity();
    skull->setEntityType(NPC);
    skull->setPosition({ playerPos.x - spacing, playerPos.y + spacing });
    skull->setScale({ 32.0f, 32.0f });
    skull->setColliderDimensions({ 28.0f, 28.0f });
    skull->setTexture("assets/characters.png");
    skull->setTextureType(ATLAS);
    skull->setSpriteSheetDimensions({ 5, 4 });
    skull->setAnimationAtlas(walkingAnimation);
    skull->setFrameSpeed(Entity::DEFAULT_FRAME_SPEED);

    Entity* mona = new Entity();
    mona->setEntityType(NPC);
    mona->setPosition({ playerPos.x - spacing * 2.0f, playerPos.y + spacing });
    mona->setScale({ 32.0f, 32.0f });
    mona->setColliderDimensions({ 28.0f, 28.0f });
    mona->setTexture("assets/characters.png");
    mona->setTextureType(ATLAS);
    mona->setSpriteSheetDimensions({ 5, 4 });
    mona->setAnimationAtlas(walkingAnimation);
    mona->setFrameSpeed(Entity::DEFAULT_FRAME_SPEED);

    Entity* noir = new Entity();
    noir->setEntityType(NPC);
    noir->setPosition({ playerPos.x - spacing * 3.0f, playerPos.y + spacing });
    noir->setScale({ 32.0f, 32.0f });
    noir->setColliderDimensions({ 28.0f, 28.0f });
    noir->setTexture("assets/characters.png");
    noir->setTextureType(ATLAS);
    noir->setSpriteSheetDimensions({ 5, 4 });
    noir->setAnimationAtlas(walkingAnimation);
    noir->setFrameSpeed(Entity::DEFAULT_FRAME_SPEED);

    skull->setTint(YELLOW);
    mona->setTint(SKYBLUE);
    noir->setTint(VIOLET);

    skull->setAIType(AI_FOLLOWER); skull->setAIState(IDLE); skull->activate(); skull->setAcceleration({0.0f,0.0f});
    mona->setAIType(AI_FOLLOWER);  mona->setAIState(IDLE);  mona->activate();  mona->setAcceleration({0.0f,0.0f});
    noir->setAIType(AI_FOLLOWER);  noir->setAIState(IDLE);  noir->activate();  noir->setAcceleration({0.0f,0.0f});

    mFollowers.push_back(skull);
    mFollowers.push_back(mona);
    mFollowers.push_back(noir);

    // One boss sentry on the right, scaled up
    {
        mGameState.enemyCount = 1;
        mGameState.worldEnemies = new Entity[mGameState.enemyCount];
        mGameState.worldEnemies[0] = Entity();
        mGameState.worldEnemies[0].setPosition({ 630.0f, 270.0f });
        mGameState.worldEnemies[0].setTexture("assets/enemy_atlas.png");
        mGameState.worldEnemies[0].setTextureType(ATLAS);
        mGameState.worldEnemies[0].setSpriteSheetDimensions({ 3, 8 });
        mGameState.worldEnemies[0].setEntityType(NPC);
        mGameState.worldEnemies[0].activate();
        mGameState.worldEnemies[0].setAcceleration({ 0.0f, 0.0f });
        mGameState.worldEnemies[0].setAIType(AI_BOSS);
        mGameState.worldEnemies[0].setAIState(IDLE);
        mGameState.worldEnemies[0].setDirection(NEUTRAL);
        mGameState.worldEnemies[0].setSourceFacing(true);
        std::map<Direction, std::vector<int>> enemyAnim = {
            { LEFT,    { 4, 5, 6, 7 } },
            { RIGHT,   { 4, 5, 6, 7 } },
            { UP,      { 4, 5, 6, 7 } },
            { DOWN,    { 4, 5, 6, 7 } },
            { NEUTRAL, { 0, 1, 2, 3 } }
        };
        mGameState.worldEnemies[0].setAnimationAtlas(enemyAnim);
        mGameState.worldEnemies[0].setDirection(NEUTRAL);
        mGameState.worldEnemies[0].setSpeed(0);
        mGameState.worldEnemies[0].setScale({ 96.0f, 75.0f }); // scaled up boss
        mGameState.worldEnemies[0].setColliderDimensions({ 40.0f, 40.0f });
    }

    // No props for LevelThree (boss arena)
    mWorldProps = nullptr;
    mPropCount = 0;

    // Camera
    mGameState.camera.target = mGameState.player->getPosition();
    mGameState.camera.offset = mOrigin;
    mGameState.camera.rotation = 0.0f;
    mGameState.camera.zoom = 2.0f;

    if (mEffects) delete mEffects;
    mEffects = new Effects(mOrigin, 1000.0f, 600.0f);
    mEffects->setEffectSpeed(2.0f);
    mIsTransitioning = false;

    // --- BGM: Level Exploration (same track) ---
    if (mGameState.bgm.ctxData) { StopMusicStream(mGameState.bgm); UnloadMusicStream(mGameState.bgm); }
    if (FileExists("assets/audio/levelmusic.mp3")) {
        mGameState.bgm = LoadMusicStream("assets/audio/levelmusic.mp3");
        SetMusicVolume(mGameState.bgm, gMusicVolume);
        PlayMusicStream(mGameState.bgm);
    }
}

void LevelThree::update(float deltaTime)
{
    if (mGameState.bgm.ctxData) { SetMusicVolume(mGameState.bgm, gMusicVolume); UpdateMusicStream(mGameState.bgm); }
    // Player update & map interaction (guard against nulls)
    if (mGameState.player) {
        mGameState.player->update(deltaTime, mGameState.player, mGameState.map, mWorldProps, mPropCount);
    }
    if (mGameState.map && mGameState.player) {
        Vector2 pPos = mGameState.player->getPosition();
        mGameState.map->revealTiles(pPos, 200.0f);
    }

    // Followers physics
    constexpr float TETHER_SPEED    = 0.08f;
    constexpr float REPEL_STRENGTH  = 20000.0f;
    constexpr float JITTER_STRENGTH = 5.0f;
    constexpr float DAMPING         = 0.90f;
    for (Entity* f : mFollowers) {
        if (!f) continue;
        f->updateFollowerPhysics(mGameState.player, mFollowers, mGameState.map, deltaTime,
            TETHER_SPEED, REPEL_STRENGTH, JITTER_STRENGTH, DAMPING);
    }

    const float AMBUSH_DISTANCE = 60.0f;
    bool isSpotted = false;

    // Boss sentry: idle until collision/ambush triggers combat
    for (int i = 0; i < mGameState.enemyCount; ++i) {
        if (!mGameState.worldEnemies) break;
        Entity* enemy = &mGameState.worldEnemies[i];
        Entity* player = mGameState.player;
        if (enemy) enemy->update(deltaTime, player, mGameState.map, NULL, 0);
        if (!enemy || !enemy->isActive()) continue;

        // Ambush attempt
        if (IsKeyPressed(KEY_SPACE) && player) {
            float distToEnemy = Vector2Distance(player->getPosition(), enemy->getPosition());
            if (distToEnemy < AMBUSH_DISTANCE) {
                if (player->checkAmbush(enemy)) {
                    mGameState.engagedEnemyIndex = i;
                    mGameState.returnSceneID     = gCurrentLevelIndex;
                    mGameState.combatAdvantage   = true;
                    mGameState.nextSceneID       = 2; // Combat
                    return;
                }
            }
        }

        // Collision triggers combat (no vision cones here)
        if (player && player->isColliding(enemy)) {
            mGameState.engagedEnemyIndex = i;
            mGameState.returnSceneID     = gCurrentLevelIndex;
            mGameState.combatAdvantage   = false;
            mGameState.nextSceneID       = 2; // Combat
            return;
        }
    }

    mGameState.shaderStatus = isSpotted ? 1 : 0;
    mGameState.camera.target = mGameState.player->getPosition();
}

void LevelThree::render()
{
    if (mGameState.map) mGameState.map->render();


    // No props

    if (mGameState.worldEnemies) {
        for (int i = 0; i < mGameState.enemyCount; ++i) {
            mGameState.worldEnemies[i].render();
        }
    }

    for (Entity* f : mFollowers) { if (f) f->render(); }
    if (mGameState.player) mGameState.player->render();

    if (mEffects) mEffects->render();
}

void LevelThree::shutdown()
{
    for (Entity* f : mFollowers) { delete f; }
    mFollowers.clear();
    if (mWorldProps) { delete[] mWorldProps; mWorldProps = nullptr; }
    if (mGameState.worldEnemies) { delete[] mGameState.worldEnemies; mGameState.worldEnemies = nullptr; }
    if (mEffects) { delete mEffects; mEffects = nullptr; }
}
