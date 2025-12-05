#include "LevelTwo.h"
#include "../lib/Effects.h"
#include "../lib/GameData.h"
#include <cmath>

extern int gCurrentLevelIndex;

void LevelTwo::initialise()
{
    mGameState.nextSceneID = -1;

    // Clean previous allocations
    if (mGameState.worldEnemies) { delete[] mGameState.worldEnemies; mGameState.worldEnemies = nullptr; }
    if (!mFollowers.empty()) { for (Entity* f : mFollowers) { delete f; } mFollowers.clear(); }
    if (mWorldProps) { delete[] mWorldProps; mWorldProps = nullptr; mPropCount = 0; }

    // Load map
    if (mGameState.map) { delete mGameState.map; mGameState.map = nullptr; }
    mGameState.map = new Map(50, 50, mLevelData, "assets/tileset.png", 32.0f, 4, 1, mOrigin);

    // Player setup
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

    // Restore return spawn position if coming from combat
    if (mGameState.hasReturnSpawnPos) {
        mGameState.player->setPosition(mGameState.returnSpawnPos);
        mGameState.hasReturnSpawnPos = false;
    }

    // Followers
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

    skull->setAIType(AI_SENTRY); skull->setAIState(IDLE); skull->activate(); skull->setAcceleration({0.0f,0.0f});
    mona->setAIType(AI_SENTRY);  mona->setAIState(IDLE);  mona->activate();  mona->setAcceleration({0.0f,0.0f});
    noir->setAIType(AI_SENTRY);  noir->setAIState(IDLE);  noir->activate();  noir->setAcceleration({0.0f,0.0f});

    mFollowers.push_back(skull);
    mFollowers.push_back(mona);
    mFollowers.push_back(noir);

    // Ensure defeated flags exist but do not spawn any enemies
    mGameState.enemyCount = 0;
    mGameState.worldEnemies = nullptr;

    // Spawn chests at same positions as Level One
    {
        std::vector<Vector2> chestPositions = {
            { 867.0f,  176.0f },
            { 612.0f, 653.0f },
            {  37.0f, 218.0f },
            { 515.0f, -265.0f }
        };
        mPropCount = static_cast<int>(chestPositions.size());
        mWorldProps = new Entity[mPropCount];
        for (int i = 0; i < mPropCount; ++i) {
            mWorldProps[i] = Entity();
            mWorldProps[i].setEntityType(PROP);
            mWorldProps[i].setIsChest(true);
            mWorldProps[i].setPosition(chestPositions[i]);
            mWorldProps[i].setScale({ 28.0f, 28.0f });
            mWorldProps[i].setColliderDimensions({ 28.0f, 28.0f });
            mWorldProps[i].setTexture("assets/chest.png");
            mWorldProps[i].setTint(GOLD);
            mWorldProps[i].activate();
            mWorldProps[i].setAcceleration({0.0f,0.0f});
        }
    }

    // Camera
    mGameState.camera.target = mGameState.player->getPosition();
    mGameState.camera.offset = mOrigin;
    mGameState.camera.rotation = 0.0f;
    mGameState.camera.zoom = 2.0f;

    // Effects
    if (mEffects) delete mEffects;
    mEffects = new Effects(mOrigin, 1000.0f, 600.0f);
    mEffects->setEffectSpeed(2.0f);
    mIsTransitioning = false;
}

void LevelTwo::update(float deltaTime)
{
    if (mIsTransitioning)
    {
        Vector2 camTarget = mGameState.camera.target;
        if (mEffects) mEffects->update(deltaTime, &camTarget);
        mGameState.camera.zoom += 2.0f * deltaTime;
        if (mGameState.camera.zoom > mTargetZoom) mGameState.camera.zoom = mTargetZoom;
        if (mEffects && mEffects->getAlpha() >= Effects::SOLID) {
             mGameState.nextSceneID = 2; // Switch to Combat (unused here)
        }
        return;
    }

    // Player update & map interaction
    mGameState.player->update(deltaTime, mGameState.player, mGameState.map, mWorldProps, mPropCount);
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

    // No enemies or chest interactions in Level Two

    // Camera follow
    mGameState.camera.target = mGameState.player->getPosition();
}

void LevelTwo::render()
{
    if (mGameState.map) mGameState.map->render();

    // No props or enemies to render

    for (Entity* f : mFollowers) { if (f) f->render(); }
    if (mGameState.player) mGameState.player->render();

    if (mEffects) mEffects->render();
}

void LevelTwo::shutdown()
{
    // Clean up followers
    for (Entity* f : mFollowers) { delete f; }
    mFollowers.clear();
    if (mWorldProps) { delete[] mWorldProps; mWorldProps = nullptr; }
    if (mGameState.worldEnemies) { delete[] mGameState.worldEnemies; mGameState.worldEnemies = nullptr; }
    if (mEffects) { delete mEffects; mEffects = nullptr; }
}
