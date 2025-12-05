#include "LevelTwo.h"
#include "../lib/Effects.h"
#include "../lib/GameData.h"
#include <raylib.h>
extern float gMusicVolume;
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

    skull->setAIType(AI_FOLLOWER); skull->setAIState(IDLE); skull->activate(); skull->setAcceleration({0.0f,0.0f});
    mona->setAIType(AI_FOLLOWER);  mona->setAIState(IDLE);  mona->activate();  mona->setAcceleration({0.0f,0.0f});
    noir->setAIType(AI_FOLLOWER);  noir->setAIState(IDLE);  noir->activate();  noir->setAcceleration({0.0f,0.0f});

    mFollowers.push_back(skull);
    mFollowers.push_back(mona);
    mFollowers.push_back(noir);

    // --- Enemies & Props for Level Two ---
    // Define positions from user request
    std::vector<Vector2> guardPositions = {
        { 600.0f, 100.0f },
        {   2.0f,-266.0f },
        { 803.0f, 788.0f },
        { 994.0f, 191.0f }
    };
    std::vector<Vector2> sentryPositions = {
        {  65.0f, 700.0f },
        {1126.0f,-175.0f },
        { 959.0f,-136.0f },
        { -121.0f, 218.0f },
        { 840.0f, 823.0f }
    };
    // Treat "search lights" as stationary sentries with large sight range
    std::vector<Vector2> searchLightPositions = {
        { 700.0f, 660.0f },
        { 260.0f, 780.0f },
        { 937.0f,-135.0f },
        { -63.0f, 116.0f },
        {  40.0f, 302.0f },
        { 710.0f, 624.0f }
    };

    // Stable indexing: build a flat list of all enemy spawns (fixed order)
    {
        struct SpawnDef { Vector2 pos; int type; bool searchLight; };
        std::vector<SpawnDef> spawns;
        spawns.reserve(guardPositions.size() + sentryPositions.size() + searchLightPositions.size());
        for (auto p : guardPositions)       spawns.push_back({p, (int)AI_GUARD, false});
        for (auto p : sentryPositions)      spawns.push_back({p, (int)AI_SENTRY, false});
        for (auto p : searchLightPositions) spawns.push_back({p, (int)AI_SEARCHLIGHT, true});

        size_t totalEnemies = spawns.size();
        if (mGameState.defeatedEnemies.size() < totalEnemies) {
            mGameState.defeatedEnemies.resize(totalEnemies, false);
        }

        mGameState.enemyCount = static_cast<int>(totalEnemies);
        if (mGameState.enemyCount > 0) {
            mGameState.worldEnemies = new Entity[mGameState.enemyCount];

            std::map<Direction, std::vector<int>> enemyAnim = {
                { LEFT,    { 4, 5, 6, 7 } },
                { RIGHT,   { 4, 5, 6, 7 } },
                { UP,      { 4, 5, 6, 7 } },
                { DOWN,    { 4, 5, 6, 7 } },
                { NEUTRAL, { 0, 1, 2, 3 } }
            };

            for (size_t i = 0; i < spawns.size(); ++i) {
                const auto& s = spawns[i];
                mGameState.worldEnemies[i] = Entity();
                mGameState.worldEnemies[i].setPosition(s.pos);
                // Default sprite for guards/sentries
                mGameState.worldEnemies[i].setScale({ 64.0f, 50.0f });
                mGameState.worldEnemies[i].setColliderDimensions({ 32.0f, 32.0f });
                mGameState.worldEnemies[i].setTexture("assets/enemy_atlas.png");
                mGameState.worldEnemies[i].setTextureType(ATLAS);
                mGameState.worldEnemies[i].setEntityType(NPC);
                mGameState.worldEnemies[i].activate();
                mGameState.worldEnemies[i].setAcceleration({ 0.0f, 0.0f });
                mGameState.worldEnemies[i].setSpriteSheetDimensions({ 3, 8 });
                mGameState.worldEnemies[i].setAnimationAtlas(enemyAnim);
                mGameState.worldEnemies[i].setSourceFacing(true);

                if (s.type == (int)AI_GUARD) {
                    mGameState.worldEnemies[i].setAIType(AI_GUARD);
                    mGameState.worldEnemies[i].setAIState(PATROLLING);
                    Vector2 startPos = s.pos;
                    mGameState.worldEnemies[i].setStartPosition(startPos);
                    mGameState.worldEnemies[i].setPatrolTarget({ startPos.x, startPos.y + 80.0f });
                    mGameState.worldEnemies[i].setDirection(DOWN);
                    mGameState.worldEnemies[i].setSpeed(80);
                } else if (s.type == (int)AI_SEARCHLIGHT) { // Searchlight
                    // Use dedicated searchlight sprite and AI
                    mGameState.worldEnemies[i].setAIType(AI_SEARCHLIGHT);
                    mGameState.worldEnemies[i].setAIState(PATROLLING);
                    mGameState.worldEnemies[i].setTexture("assets/light_blue.png");
                    mGameState.worldEnemies[i].setTextureType(SINGLE);
                    mGameState.worldEnemies[i].setScale({ 32.0f, 32.0f });
                    mGameState.worldEnemies[i].setColliderDimensions({ 24.0f, 24.0f });
                    Vector2 startPos = s.pos;
                    mGameState.worldEnemies[i].setStartPosition(startPos);
                    mGameState.worldEnemies[i].setPatrolTarget({ startPos.x + 60.0f, startPos.y });
                    mGameState.worldEnemies[i].setDirection(RIGHT);
                    mGameState.worldEnemies[i].setSpeed(60);
                } else { // Sentry
                    mGameState.worldEnemies[i].setAIType(AI_SENTRY);
                    mGameState.worldEnemies[i].setAIState(IDLE);
                    mGameState.worldEnemies[i].setSpeed(0);
                }

                if (mGameState.defeatedEnemies[i]) {
                    mGameState.worldEnemies[i].deactivate();
                }
            }
        } else {
            mGameState.worldEnemies = nullptr;
        }
    }

    // Spawn chests
    {
        std::vector<Vector2> chestPositions = {
            {  65.0f, 845.0f },
            {1056.0f,-264.0f },
            { 860.0f, 625.0f },
            { 515.0f,-265.0f },
            { -33.0f, 247.0f },
            {1091.0f, 571.0f }
        };
        // Ensure opened chest flags match chest count
        if (mGameState.openedChests.size() < chestPositions.size()) {
            mGameState.openedChests.resize(chestPositions.size(), false);
        }

        // Count only unopened chests for allocation
        int activeChestCount = 0;
        for (size_t i = 0; i < chestPositions.size(); ++i) {
            if (!mGameState.openedChests[i]) {
                ++activeChestCount;
            }
        }

        mPropCount = activeChestCount;
        mWorldProps = (mPropCount > 0) ? new Entity[mPropCount] : nullptr;

        int propIndex = 0;
        for (size_t i = 0; i < chestPositions.size(); ++i) {
            if (mGameState.openedChests[i]) continue; // Skip spawning opened chests

            mWorldProps[propIndex] = Entity();
            mWorldProps[propIndex].setEntityType(PROP);
            mWorldProps[propIndex].setIsChest(true);
            mWorldProps[propIndex].setPosition(chestPositions[i]);
            mWorldProps[propIndex].setScale({ 28.0f, 28.0f });
            mWorldProps[propIndex].setColliderDimensions({ 28.0f, 28.0f });
            mWorldProps[propIndex].setTexture("assets/chest.png");
            mWorldProps[propIndex].setTint(GOLD);
            mWorldProps[propIndex].activate();
            mWorldProps[propIndex].setAcceleration({0.0f,0.0f});
            ++propIndex;
        }
    }

    // Camera
    mGameState.camera.target = mGameState.player->getPosition();
    mGameState.camera.offset = mOrigin;
    mGameState.camera.rotation = 0.0f;
    mGameState.camera.zoom = 2.0f;

    // Restore any saved exploration state for this map
    if (mGameState.map && !mGameState.revealedTiles.empty()) {
        mGameState.map->setExploredTiles(mGameState.revealedTiles);
    }

    // Effects
    if (mEffects) delete mEffects;
    mEffects = new Effects(mOrigin, 1000.0f, 600.0f);
    mEffects->setEffectSpeed(2.0f);
    mIsTransitioning = false;

    // BGM
    if (mGameState.bgm.ctxData) { StopMusicStream(mGameState.bgm); UnloadMusicStream(mGameState.bgm); }
    if (FileExists("assets/audio/levelmusic.mp3")) {
        mGameState.bgm = LoadMusicStream("assets/audio/levelmusic.mp3");
        SetMusicVolume(mGameState.bgm, gMusicVolume);
        PlayMusicStream(mGameState.bgm);
    }
}

void LevelTwo::update(float deltaTime)
{
    if (mGameState.bgm.ctxData) { SetMusicVolume(mGameState.bgm, gMusicVolume); UpdateMusicStream(mGameState.bgm); }
    if (mIsTransitioning)
    {
        Vector2 camTarget = mGameState.camera.target;
        if (mEffects) mEffects->update(deltaTime, &camTarget);
        mGameState.camera.zoom += 2.0f * deltaTime;
        if (mGameState.camera.zoom > mTargetZoom) mGameState.camera.zoom = mTargetZoom;
        if (mEffects && mEffects->getAlpha() >= Effects::SOLID) {
             mGameState.nextSceneID = 2; // Switch to Combat
        }
        return;
    }

    // Player update & map interaction
    mGameState.player->update(deltaTime, mGameState.player, mGameState.map, mWorldProps, mPropCount);
    if (mGameState.map && mGameState.player) {
        Vector2 pPos = mGameState.player->getPosition();
        mGameState.map->revealTiles(pPos, 200.0f);
        mGameState.revealedTiles = mGameState.map->getExploredTiles();
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

    // STEALTH / DETECTION CONSTANTS
    const float AMBUSH_DISTANCE = 60.0f;
    const float SIGHT_DISTANCE  = 100.0f;
    const float SIGHT_ANGLE     = 90.0f;
    bool isSpotted = false;

    // Chest interaction
    if (IsKeyPressed(KEY_SPACE)) {
        Entity* player = mGameState.player;
        for (int i = 0; i < mPropCount; ++i) {
            Entity* prop = &mWorldProps[i];
            if (!prop->isActive() || !prop->isChest()) continue;
            float dist = Vector2Distance(player->getPosition(), prop->getPosition());
            if (dist < 50.0f) {
                if (player->isEntityInSight(prop, 60.0f, 60.0f)) {
                    Item loot = getRandomChestItem(1);
                    mGameState.inventory.push_back(loot);
                    mGameState.itemToast = std::string("Obtained: ") + loot.name;
                    mGameState.itemToastTimer = 2.0f;
                    prop->deactivate();

                    // Persist chest opened flag
                    if (i < (int)mGameState.openedChests.size()) {
                        mGameState.openedChests[i] = true;
                    }
                }
            }
        }
    }

    bool allChestsOpened = true;
    for (int i = 0; i < mPropCount; ++i) {
        Entity* prop = &mWorldProps[i];
        if (prop->isChest() && prop->isActive()) { allChestsOpened = false; break; }
    }
    if (allChestsOpened) {
        mGameState.nextSceneID = 4; 
    }

    // Enemies update & combat triggers
    for (int i = 0; i < mGameState.enemyCount; i++)
    {
        Entity* enemy  = &mGameState.worldEnemies[i];
        Entity* player = mGameState.player;

        enemy->update(deltaTime, player, mGameState.map, NULL, 0);
        if (!enemy->isActive()) continue;

        // Only guards have vision cones; sentries/searchlights do not.
        bool inSight = false;
        if (enemy->getAIType() == AI_GUARD) {
            inSight = enemy->isEntityInSight(player, SIGHT_DISTANCE, SIGHT_ANGLE);
            if (inSight && mGameState.map) {
                 if (!mGameState.map->hasLineOfSight(enemy->getPosition(), player->getPosition())) {
                     inSight = false;
                 }
            }
            if (inSight) {
                isSpotted = true;
                enemy->setAIState(CHASING);
            }
        }

        mGameState.shaderStatus = isSpotted ? 1 : 0;

        // Ambush attempt (SPACE) should not work on searchlights
        if (IsKeyPressed(KEY_SPACE)) {
            if (enemy->getAIType() != AI_SEARCHLIGHT) {
                float distToEnemy = Vector2Distance(player->getPosition(), enemy->getPosition());
                if (distToEnemy < AMBUSH_DISTANCE) {
                    if (player->checkAmbush(enemy)) {
                        mIsTransitioning = true;
                        if (mEffects) mEffects->start(FADEOUT);
                        mGameState.engagedEnemyIndex = i;
                        mGameState.returnSceneID     = gCurrentLevelIndex;
                        mGameState.combatAdvantage   = true;
                        return;
                    }
                }
            }
        }

        if (player->isColliding(enemy)) {
            // If colliding with a searchlight: awaken all sentries, 
            if (enemy->getAIType() == AI_SEARCHLIGHT) {
                for (int j = 0; j < mGameState.enemyCount; ++j) {
                    Entity* e2 = &mGameState.worldEnemies[j];
                    if (!e2->isActive()) continue;
                    if (e2->getAIType() == AI_SENTRY) {
                        e2->setAIState(CHASING);
                    }
                }
                // Visual cue: mark as spotted without transitioning to combat
                isSpotted = true;
                mGameState.shaderStatus = 1;
            } else {
                // Normal collision with enemies triggers combat
                mIsTransitioning = true;
                if (mEffects) mEffects->start(FADEOUT);
                mGameState.engagedEnemyIndex = i;
                mGameState.returnSceneID     = gCurrentLevelIndex;
                if (enemy->getAIState() == CHASING) {
                    mGameState.combatAdvantage = false;
                }
                return;
            }
        }
    }

    if (!isSpotted) {
        mGameState.shaderStatus = 0;
    }

    // Camera follow
    mGameState.camera.target = mGameState.player->getPosition();
}

void LevelTwo::render()
{
    if (mGameState.map) mGameState.map->render();

    // Props
    for (int i = 0; i < mPropCount; ++i) {
        if (mWorldProps[i].isActive()) mWorldProps[i].render();
    }

    // Enemies
    for (int i = 0; i < mGameState.enemyCount; i++)
    {
        mGameState.worldEnemies[i].render();
    }

    // Debug cones: only draw for guards
    for (int i = 0; i < mGameState.enemyCount; i++)
    {
        Entity* e = &mGameState.worldEnemies[i];
        if (!e->isActive()) continue;
        if (e->getAIType() != AI_GUARD) continue;
        Vector2 pos = e->getPosition();
        Vector2 dir = e->getDirectionVector();
        float angleDeg = atan2f(dir.y, dir.x) * (180.0f / PI);
        DrawCircleSector(pos, 100.0f, angleDeg - 45.0f, angleDeg + 45.0f, 10, Fade(RED, 0.2f));
    }

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
