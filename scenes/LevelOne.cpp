#include "LevelOne.h"
#include "../lib/Effects.h" // Include full definition here
#include "../lib/GameData.h" // Loot helpers
#include <cmath> // atan2f for debug cone rendering

// --- LEVEL DATA ---


// Defeated enemies are tracked per scene in mGameState.defeatedEnemies now.
extern int gCurrentLevelIndex; // used to set returnSceneID during combat transitions

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
    // Clean previous props array
    if (mWorldProps) { delete[] mWorldProps; mWorldProps = nullptr; mPropCount = 0; }

    // 1. LOAD MAP (only allocate once; recreate each initialise for simplicity)
    if (mGameState.map) {
        delete mGameState.map;
        mGameState.map = nullptr;
    }
    mGameState.map = new Map(50, 50, mLevelData, "assets/tileset.png", 32.0f, 4, 1, mOrigin);

    // 1b. DEFINE WALKING ANIMATION ATLAS (5 cols x 4 rows)
    std::map<Direction, std::vector<int>> walkingAnimation = {
        // Row layout with 5 columns per row:
        // Row 0: 0..4, Row 1: 5..9, Row 2: 10..14, Row 3: 15..19
        { DOWN,  { 0, 1, 2, 3 } },
        { RIGHT, { 10, 11, 12, 13 } },
        { LEFT,  { 10, 11, 12, 13 } },
        { UP,    { 15, 16, 17, 18 } }
    };

    // 2. CREATE PLAYER (JOKER) using default ctor, then configure atlas
    if (mGameState.player) {
        delete mGameState.player;
        mGameState.player = nullptr;
    }
    mGameState.player = new Entity();
    mGameState.player->setEntityType(PLAYER);
    mGameState.player->setPosition({ 500.0f, 250.0f });
    mGameState.player->setScale({ 32.0f, 32.0f });
    mGameState.player->setColliderDimensions({ 28.0f, 28.0f });
    mGameState.player->setTexture("assets/characters.png");
    mGameState.player->setTextureType(ATLAS);
    mGameState.player->setSpriteSheetDimensions({ 5, 4 });
    mGameState.player->setAnimationAtlas(walkingAnimation);
    mGameState.player->setFrameSpeed(Entity::DEFAULT_FRAME_SPEED);
    mGameState.player->setAcceleration({ 0.0f, 0.0f });
    mGameState.player->setTint(WHITE);

    // If coming back from combat, restore player position before spawning followers
    if (mGameState.hasReturnSpawnPos) {
        mGameState.player->setPosition(mGameState.returnSpawnPos);
        mGameState.hasReturnSpawnPos = false; // consume so we don't reuse it on next initialise
    }

    // 2b. CREATE PARTY FOLLOWERS (Skull, Mona, Noir) using atlas + tints
    // Spawn behind player with slight offsets
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


    // 3. CREATE ENEMIES (guards) ONLY IF NOT DEFEATED
    // Expand to multiple guards at specified positions
    std::vector<Vector2> guardPositions = {
        { 570.0f,  350.0f },
        { 784.0f,  688.0f },
        { 272.0f,  688.0f },
        { -23.0f,  344.0f}
    };
    // Ensure defeated flags to match guard count
    if (mGameState.defeatedEnemies.size() < guardPositions.size()) {
        mGameState.defeatedEnemies.resize(guardPositions.size(), false);
    }

    // Stable indexing: allocate full array and deactivate defeated ones
    mGameState.enemyCount = static_cast<int>(guardPositions.size());
    if (mGameState.enemyCount > 0) {
        mGameState.worldEnemies = new Entity[mGameState.enemyCount];
        for (size_t i = 0; i < guardPositions.size(); ++i) {
            mGameState.worldEnemies[i] = Entity();
            mGameState.worldEnemies[i].setPosition(guardPositions[i]);
            mGameState.worldEnemies[i].setScale({ 32.0f, 32.0f });
            mGameState.worldEnemies[i].setColliderDimensions({ 32.0f, 32.0f });
            mGameState.worldEnemies[i].setTexture("assets/enemy_atlas.png");
            mGameState.worldEnemies[i].setTextureType(ATLAS);
            mGameState.worldEnemies[i].setEntityType(NPC);
            mGameState.worldEnemies[i].activate();
            mGameState.worldEnemies[i].setAcceleration({ 0.0f, 0.0f });
            mGameState.worldEnemies[i].setAIType(AI_GUARD);
            mGameState.worldEnemies[i].setAIState(PATROLLING);

            Vector2 startPos = guardPositions[i];
            mGameState.worldEnemies[i].setStartPosition(startPos);
            mGameState.worldEnemies[i].setPatrolTarget({ startPos.x, startPos.y + 80.0f });
            mGameState.worldEnemies[i].setDirection(DOWN);
            mGameState.worldEnemies[i].setSpeed(80);

            mGameState.worldEnemies[i].setSpriteSheetDimensions({ 3, 8 });
            std::map<Direction, std::vector<int>> enemyAnim = {
                { LEFT,    { 4, 5, 6, 7 } },
                { RIGHT,   { 4, 5, 6, 7 } },
                { UP,      { 4, 5, 6, 7 } },
                { DOWN,    { 4, 5, 6, 7 } },
                { NEUTRAL, { 0, 1, 2, 3 } }
            };
            mGameState.worldEnemies[i].setAnimationAtlas(enemyAnim);
            mGameState.worldEnemies[i].setSourceFacing(true);
            mGameState.worldEnemies[i].setScale({ 64.0f, 50.0f });

            if (mGameState.defeatedEnemies[i]) {
                mGameState.worldEnemies[i].deactivate();
            }
        }
    } else {
        mGameState.worldEnemies = nullptr;
    }

    // 3b. CREATE CHEST PROPS (array form for collisions)
    std::vector<Vector2> chestPositions = {
        { 867.0f,  176.0f },
        { 612.0f, 653.0f },
        { 37.0f, 218.0f },
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

    // 4. SETUP CAMERA
    mGameState.camera.target = mGameState.player->getPosition();
    mGameState.camera.offset = mOrigin;
    mGameState.camera.rotation = 0.0f;
    mGameState.camera.zoom = 2.0f;

    // --- 5. INITIALISE EFFECTS ---
    // Initialize Effects with screen dimensions (1000x600)
    if (mEffects) delete mEffects;
    mEffects = new Effects(mOrigin, 1000.0f, 600.0f);
    mEffects->setEffectSpeed(2.0f); // Fast Fade (0.5 seconds)

    mIsTransitioning = false;
}

void LevelOne::update(float deltaTime)
{
    // --- 1. HANDLE TRANSITION SEQUENCE ---
    if (mIsTransitioning)
    {
        // A. Update Effect (Fade Out)
        // Pass camera target as view offset so overlay stays centered on screen
        Vector2 camTarget = mGameState.camera.target;
        if (mEffects) mEffects->update(deltaTime, &camTarget);

        // B. Zoom In Camera
        mGameState.camera.zoom += 2.0f * deltaTime; // Zoom speed
        if (mGameState.camera.zoom > mTargetZoom) mGameState.camera.zoom = mTargetZoom;

        // C. Check Completion
        // If alpha is fully solid (Black), switch scene
        if (mEffects && mEffects->getAlpha() >= Effects::SOLID) {
             mGameState.nextSceneID = 2; // Switch to Combat
        }
        return; // Block other updates (Movement/Input) during transition
    }

    // --- 2. STANDARD UPDATE (Keep existing logic) ---
    // --- PLAYER UPDATE & MAP INTERACTION ---
    // Let player collide against props for tactile interaction
    mGameState.player->update(deltaTime, mGameState.player, mGameState.map, mWorldProps, mPropCount);
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
    const float SIGHT_DISTANCE  = 100.0f;
    const float SIGHT_ANGLE     = 90.0f; // Full cone angle
    bool isSpotted = false; // Aggregate spotted state (for shader/effects)

    // --- PROP INTERACTION (CHESTS) ---
    if (IsKeyPressed(KEY_SPACE)) {
        Entity* player = mGameState.player;
        for (int i = 0; i < mPropCount; ++i) {
            Entity* prop = &mWorldProps[i];
            if (!prop->isActive() || !prop->isChest()) continue;
            float dist = Vector2Distance(player->getPosition(), prop->getPosition());
            if (dist < 50.0f) {
                // Use player's sight cone to validate facing the chest
                if (player->isEntityInSight(prop, 60.0f, 60.0f)) {
                    Item loot = getRandomChestItem(0);
                    mGameState.inventory.push_back(loot);
                    // Set global toast notification (2 seconds)
                    mGameState.itemToast = std::string("Obtained: ") + loot.name;
                    mGameState.itemToastTimer = 2.0f;
                    prop->deactivate();
                }
            }
        }
    }

    // --- PROGRESSION: Advance when all chests are opened ---
    {
        bool allChestsOpened = true;
        for (int i = 0; i < mPropCount; ++i) {
            Entity* prop = &mWorldProps[i];
            if (prop->isChest() && prop->isActive()) { allChestsOpened = false; break; }
        }
        if (allChestsOpened) {
            mGameState.nextSceneID = 1; // LevelTwo
        }
    }

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
        if (inSight && mGameState.map) {
             if (!mGameState.map->hasLineOfSight(enemy->getPosition(), player->getPosition())) {
                 inSight = false;
             }
        }
        if (inSight) {
            isSpotted = true;
            enemy->setAIState(CHASING);
        }

        if (isSpotted) {
            mGameState.shaderStatus = 1; // Spotted
        }
        else {
            mGameState.shaderStatus = 0; // Normal
        }

        // B. Ambush Attempt (player advantage)
        if (IsKeyPressed(KEY_SPACE)) {
            float distToEnemy = Vector2Distance(player->getPosition(), enemy->getPosition());
            if (distToEnemy < AMBUSH_DISTANCE) {
                if (player->checkAmbush(enemy)) {
                    std::cout << "AMBUSH SUCCESS! Transitioning..." << std::endl;
                    // START TRANSITION
                    mIsTransitioning = true;
                    if (mEffects) mEffects->start(FADEOUT);
                    // Set State for next scene (but don't switch ID yet)
                    mGameState.engagedEnemyIndex = i;
                    mGameState.returnSceneID     = gCurrentLevelIndex;
                    mGameState.combatAdvantage   = true;
                    return; 
                }
            }
        }

        // C. Collision Trigger
        if (player->isColliding(enemy)) {
            std::cout << "COLLISION! Transitioning..." << std::endl;
            // START TRANSITION
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

    if (!isSpotted) {
        mGameState.shaderStatus = 0; // Normal
    }

    // --- CAMERA FOLLOW ---
    mGameState.camera.target = mGameState.player->getPosition();
}

void LevelOne::render()
{
    // Draw Room
    if (mGameState.map) mGameState.map->render();

    // Draw Props (chests)
    for (int i = 0; i < mPropCount; ++i) {
        if (mWorldProps[i].isActive()) mWorldProps[i].render();
    }

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
        DrawCircleSector(pos, 100.0f, angleDeg - 45.0f, angleDeg + 45.0f, 10, Fade(RED, 0.2f));
    }

    // RENDER EFFECT OVERLAY (Draws black rect over camera view)
    if (mEffects) mEffects->render();


}