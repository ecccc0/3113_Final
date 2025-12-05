#include "CombatScene.h"
#include "../lib/Effects.h"
#include "../lib/GameData.h" // for getRandomEnemyForLevel
#include <cmath>
#include "raymath.h"

// Access exploration HUD icons
extern Texture2D gPartyIcons[4];

// CombatScene should use its own GameState.party rather than the global gParty

// Local helper to draw a slanted rectangle (parallelogram) for HUD bars
static void DrawSlantedRect(int x, int y, int width, int height, int skew, Color color) {
    Vector2 v1 = { (float)x + (float)skew, (float)y };
    Vector2 v2 = { (float)x, (float)y + (float)height };
    Vector2 v3 = { (float)x + (float)width, (float)y + (float)height };
    Vector2 v4 = { (float)x + (float)width + (float)skew, (float)y };
    DrawTriangle(v1, v2, v3, color);
    DrawTriangle(v1, v3, v4, color);
}

// Helper: compute enemy atlas source rect based on combat state and timer
Rectangle CombatScene::GetEnemyFrameRect(Combatant& enemy, CombatState state, float timer)
{
    int frameIndex = 0; // default idle
    float animSpeed = 0.15f; // seconds per frame

    if (!enemy.isAlive) {
        frameIndex = 20; // death last frame
    }
    else if (state == ANIMATION_WAIT && (&enemy - &mGameState.battleEnemies[0]) == mSelectedTargetIndex) {
        int offset = (int)(timer / animSpeed) % 4; // 13-16
        frameIndex = 13 + offset;
    }
    else if (state == ENEMY_TURN && (&enemy - &mGameState.battleEnemies[0]) == mActiveEnemyIndex) {
        int offset = (int)(timer / animSpeed) % 5; // 8-12
        frameIndex = 8 + offset;
    }
    else {
        int offset = (int)(GetTime() / 0.2f) % 4; // 0-3 idle
        frameIndex = 0 + offset;
    }

    int col = frameIndex % 8;
    int row = frameIndex / 8;
    Rectangle source = { (float)col * 32.0f, (float)row * 25.0f, 32.0f, 25.0f };
    return source;
}

void CombatScene::SpawnFloatingText(Vector2 pos, const std::string& text, Color color, float lifetime)
{
    FloatingText ft;
    ft.pos = pos;
    ft.text = text;
    ft.color = color;
    ft.lifetime = lifetime;
    ft.elapsed = 0.0f;
    mFloatingTexts.push_back(ft);
}

void CombatScene::initialise() {
    // Set initial turn state based on advantage
        mActiveMemberIndex = 0;
        mSelectedSkillIndex = 0;
        mSelectedTargetIndex = 0;
        mActiveEnemyIndex = 0;
        mTimer = 0.0f;
        if (!mGameState.combatAdvantage) {
            mState = ENEMY_TURN;
            mLog = "Surprise Attack! Shadows act first.";
        } else {
            mState = PLAYER_TURN_MAIN; // Player advantage
            mLog = "Ambush! Phantom Thieves have the advantage.";
        }
        // Clear any stale transition request
        mGameState.nextSceneID = -1;

        // Reset Party "Acted" flags for the new battle
        for (auto& member : mGameState.party) {
            member.hasActed = false;
            member.isDown = false;
        }

        // Dynamic encounter generation based on the level that triggered combat
        mGameState.battleEnemies.clear();

        int levelIdx = mGameState.returnSceneID; // 0: Level 1, 1: Level 2, 2: Mini-boss
        int enemyCount = (levelIdx == 0) ? 2 : 3; // L1: 2 enemies, L2+: 3 enemies

        if (levelIdx == 2) {
            // Mini-boss: Boss + 2 minions from Level 1 pool
            Combatant boss = getEnemyData(99);
            mGameState.battleEnemies.push_back(boss);
            for (int i = 0; i < 2; ++i) {
                Combatant minion = getRandomEnemyForLevel(0);
                mGameState.battleEnemies.push_back(minion);
            }
        } else {
            for (int i = 0; i < enemyCount; ++i) {
                Combatant enemy = getRandomEnemyForLevel(levelIdx);
                mGameState.battleEnemies.push_back(enemy);
            }
        }

        // Assign on-screen positions to avoid overlap in UI
        for (int i = 0; i < (int)mGameState.battleEnemies.size(); ++i) {
            mGameState.battleEnemies[i].position = { 600.0f + (i * 120.0f), 200.0f };
        }

        // --- LOAD UI ASSETS ---
        mIconAttack = LoadTexture("assets/ui/icon_attack.png");
        mIconGun    = LoadTexture("assets/ui/icon_gun.png");
        mIconSkill  = LoadTexture("assets/ui/icon_skill.png");
        mIconGuard  = LoadTexture("assets/ui/icon_guard.png");
        mIconItem   = LoadTexture("assets/ui/icon_item.png");
        mUiCursor   = LoadTexture("assets/ui/ui_cursor.png");

        // Initialize Rotation
        mWheelRotation = 0.0f;
        mSelectedActionIndex = 0;

        // Ammo reset
        for (auto& member : mGameState.party) {
            member.currentAmmo = member.gunWeapon.magazineSize;
            member.isGuarding = false;
            member.isDown = false;
            member.hasActed = false;
        }

        // Build party visual sprites (simple atlas-based entities)
        // Clean previous
        for (Entity* e : mPartySprites) { delete e; }
        mPartySprites.clear();
        for (int i = 0; i < (int)mGameState.party.size(); i++) {
            Entity* sprite = new Entity();
            sprite->setEntityType(NPC);
            // Use characters atlas like LevelOne
            sprite->setTexture("assets/characters.png");
            sprite->setTextureType(ATLAS);
            sprite->setSpriteSheetDimensions({5,4});
            sprite->setScale({32.0f, 32.0f});
            sprite->setColliderDimensions({ 28.0f, 28.0f });
            sprite->setTint(mGameState.party[i].tint);
            // Position will be set during render for simplicity
            mPartySprites.push_back(sprite);
        }

        // --- INITIALISE EFFECTS ---
        if (mEffects) delete mEffects;
        mEffects = new Effects(mOrigin, 1000.0f, 600.0f); // Screen Size
        mEffects->setEffectSpeed(2.0f); // Fast Fade In
        mEffects->start(FADEIN); // Start black, fade to transparent

        // --- CAMERA SETUP ---
        // Start VERY zoomed in for dynamic entry
        mGameState.camera.zoom = 2.5f; 
        mGameState.camera.offset = mOrigin;
        mGameState.camera.rotation = 0.0f;
        // Center camera roughly on the battle (assuming fixed positions in render)
        mGameState.camera.target = { 500.0f, 300.0f }; // Center of screen

        // Load enemy atlas for combat sprites
        if (FileExists("assets/enemy_atlas.png")) {
            mEnemyAtlas = LoadTexture("assets/enemy_atlas.png");
        } else {
            mEnemyAtlas = LoadTextureFromImage(GenImageColor(32,25,RED)); // fallback
        }
}

    void CombatScene::shutdown() {
        // copy inventory and party state back to global


        // Unload UI Assets
        UnloadTexture(mIconAttack);
        UnloadTexture(mIconGun);
        UnloadTexture(mIconSkill);
        UnloadTexture(mIconGuard);
        UnloadTexture(mIconItem);
        UnloadTexture(mUiCursor);
        if (mEffects) { delete mEffects; mEffects = nullptr; }
        for (Entity* e : mPartySprites) { delete e; }
        mPartySprites.clear();
        // Unload enemy atlas
        UnloadTexture(mEnemyAtlas);
    }

    void CombatScene::NextTurn() {
        bool foundNext = false;
        // Check for next available party member who hasn't acted
        for (int i = 0; i < mGameState.party.size(); i++) {
            if (mGameState.party[i].isAlive && !mGameState.party[i].hasActed) {
                mActiveMemberIndex = i;
                mState = PLAYER_TURN_MAIN;
                // Reset Guard state
                mGameState.party[i].isGuarding = false;
                mLog = mGameState.party[i].name + "'s Turn!";
                foundNext = true;
                break;
            }
        }
        // if none found, switch to enemy turn
        if (!foundNext) {
            mState = ENEMY_TURN;
            mActiveEnemyIndex = 0;
            mTimer = 0.0f;
            mLog = "Enemy Turn...";

            for (auto& enemy : mGameState.battleEnemies) {
                enemy.isDown = false;
            }
        }
    }

    void CombatScene::update(float deltaTime) {
        // --- 1. UPDATE EFFECTS & ZOOM ---
        if (mEffects) {
            // Center effect on camera target
            Vector2 camTarget = mGameState.camera.target;
            mEffects->update(deltaTime, &camTarget);
        }

        // Smoothly Zoom Out to 1.0f
        if (mGameState.camera.zoom > 1.0f) {
            // "Quicker zoom out" - faster speed than zoom in
            mGameState.camera.zoom -= 4.0f * deltaTime; 
            if (mGameState.camera.zoom < 1.0f) mGameState.camera.zoom = 1.0f;
        }
        bool enemiesAlive = false;
        for (auto& e : mGameState.battleEnemies) if (e.isAlive) enemiesAlive = true;
        if (!enemiesAlive) {
            mLog = "VICTORY! Press SPACE.";
            if (IsKeyPressed(KEY_SPACE)) {
                // Mark the engaged enemy (if valid) as defeated in this scene's state
                if (mGameState.engagedEnemyIndex >= 0) {
                    if (mGameState.defeatedEnemies.size() <= (size_t)mGameState.engagedEnemyIndex)
                        mGameState.defeatedEnemies.resize(mGameState.engagedEnemyIndex + 1, false);
                    mGameState.defeatedEnemies[mGameState.engagedEnemyIndex] = true;
                }
                mGameState.nextSceneID = mGameState.returnSceneID >= 0 ? mGameState.returnSceneID : 0;
            }
            return;
        }

        if (mState == ANIMATION_WAIT) {
            mTimer += deltaTime;
            if (mTimer > 0.8f) {
                CheckHoldUp();
                if (mState != HOLD_UP) {
                    if (mState != ENEMY_TURN && !mGameState.party[mActiveMemberIndex].hasActed) {
                            mState = PLAYER_TURN_MAIN;
                            mLog = "1 MORE! Go again!";
                        } else {
                        NextTurn();
                    }
                }
                mTimer = 0.0f;
            }
            return;
        }

        if (mState == HOLD_UP) {
            if (IsKeyPressed(KEY_Y) || IsKeyPressed(KEY_SPACE)) {
                for (auto& enemy : mGameState.battleEnemies) {
                    enemy.currentHp = 0;
                    enemy.isAlive = false;
                }
                mLog = "ALL-OUT ATTACK! It's over!";
                mState = ANIMATION_WAIT;
                mTimer = 0.0f;
            }
            return;
        }

        if (mState == ENEMY_TURN) {
            mTimer += deltaTime;
            if (mTimer > 1.0f) {
                while (mActiveEnemyIndex < mGameState.battleEnemies.size() && !mGameState.battleEnemies[mActiveEnemyIndex].isAlive) {
                    mActiveEnemyIndex++;
                }

                if (mActiveEnemyIndex < mGameState.battleEnemies.size()) {
                    Combatant& enemy = mGameState.battleEnemies[mActiveEnemyIndex];
                    int partySize = (int)mGameState.party.size();
                    if (partySize > 0) {
                        int targetID = GetRandomValue(0, partySize - 1);
                        if (mGameState.party[targetID].isAlive) {
                            mGameState.party[targetID].currentHp -= enemy.baseAttack;
                            mLog = enemy.name + " attacks " + mGameState.party[targetID].name + "!";
                            // Spawn damage number near target ally
                            float tx = 100.0f; // left HUD base
                            float ty = 100.0f + (targetID * 90.0f);
                            SpawnFloatingText({ tx + 220.0f, ty }, TextFormat("-%d", enemy.baseAttack), RED, 0.8f);
                        }
                    }

                    mActiveEnemyIndex++;
                    mTimer = 0.0f;
                } else {
                    for (auto& p : mGameState.party) p.hasActed = false;
                    mState = PLAYER_TURN_MAIN;
                    NextTurn();
                }
            }
            return;
        }

        Combatant& actor = mGameState.party[mActiveMemberIndex];

        if (mState == PLAYER_TURN_MAIN) {
            // Command Wheel rotation via UP/DOWN
            if (IsKeyPressed(KEY_DOWN)) {
                mSelectedActionIndex = (mSelectedActionIndex + 1) % 5;
            }
            else if (IsKeyPressed(KEY_UP)) {
                mSelectedActionIndex = (mSelectedActionIndex - 1 + 5) % 5;
            }

            // Smoothly rotate wheel towards target angle, taking shortest wrap
            float targetBase = (float)mSelectedActionIndex * -72.0f; // 360/5 = 72
            // Find equivalent target angle closest to current rotation
            float turns = roundf((mWheelRotation - targetBase) / 360.0f);
            float targetRot = targetBase + turns * 360.0f;
            mWheelRotation = Lerp(mWheelRotation, targetRot, 10.0f * deltaTime);

            // Confirm selection
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z)) {
                switch (mSelectedActionIndex) {
                    case 0: { // Attack
                        for (int i = 0; i < mGameState.battleEnemies.size(); i++) {
                            if (mGameState.battleEnemies[i].isAlive) { mSelectedTargetIndex = i; break; }
                        }
                        mSelectedSkillIndex = -1;
                        mState = PLAYER_TURN_TARGET;
                        break;
                    }
                    case 1: { // Gun
                        if (actor.currentAmmo > 0) {
                            mState = PLAYER_TURN_TARGET;
                            mSelectedSkillIndex = -2; // Gun indicator
                            mLog = "Ammo left: " + std::to_string(actor.currentAmmo) + " / " + std::to_string(actor.gunWeapon.magazineSize);
                        }
                        else {
                            mLog = "Out of Ammo!";
                        }
                        break;
                    }
                    case 2: { // Skill
                        mState = PLAYER_TURN_SKILLS;
                        mSelectedSkillIndex = 0;
                        break;
                    }
                    case 3: { // Guard
                        actor.isGuarding = true;
                        mLog = actor.name + " is Guarding...";
                        actor.hasActed = true;
                        mState = ANIMATION_WAIT;
                        mTimer = 0.0f;
                        break;
                    }
                    case 4: { // Item
                        if (!mGameState.inventory.empty()) {
                            mState = PLAYER_TURN_ITEM;
                            mSelectedSkillIndex = 0; // Reuse skill index for item selection
                        } else {
                            mLog = "No items!";
                        }
                        break;
                    }
                }
            }
        } else if (mState == PLAYER_TURN_SKILLS) {
            if (IsKeyPressed(KEY_DOWN)) mSelectedSkillIndex = (mSelectedSkillIndex + 1) % actor.skills.size();
            else if (IsKeyPressed(KEY_UP)) mSelectedSkillIndex = (mSelectedSkillIndex - 1 + actor.skills.size()) % actor.skills.size();

            if (IsKeyPressed(KEY_Z ) || IsKeyPressed(KEY_SPACE)) {
                Ability chosenSkill = actor.skills[mSelectedSkillIndex];
                if (chosenSkill.damage < 0) {
                    // Healing skill - target ally only
                    mSelectedTargetIndex = mActiveMemberIndex; // default to self
                    mState = PLAYER_TURN_TARGET_ALLY;
                } else {
                    // Offensive skill - pick first alive enemy and go to enemy targeting
                    for (int i = 0; i < mGameState.battleEnemies.size(); i++) {
                        if (mGameState.battleEnemies[i].isAlive) { mSelectedTargetIndex = i; break; }
                    }
                    mState = PLAYER_TURN_TARGET;
                }
            } else if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_ESCAPE)) mState = PLAYER_TURN_MAIN;
        } else if (mState == PLAYER_TURN_TARGET) {
            if (IsKeyPressed(KEY_RIGHT)) mSelectedTargetIndex = (mSelectedTargetIndex + 1) % mGameState.battleEnemies.size();
            else if (IsKeyPressed(KEY_LEFT)) mSelectedTargetIndex = (mSelectedTargetIndex - 1 + mGameState.battleEnemies.size()) % mGameState.battleEnemies.size();

            while (!mGameState.battleEnemies[mSelectedTargetIndex].isAlive) {
                mSelectedTargetIndex = (mSelectedTargetIndex + 1) % mGameState.battleEnemies.size();
            }

            if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                Ability action;
                if (mSelectedSkillIndex == -1) {
                    action = {"Melee", 0, 0, PHYS, false};
                }
                else if (mSelectedSkillIndex == -2) {
                    action = {"Gun", 0, 0, GUN, false};
                    actor.currentAmmo--;
                }
                else {
                    action = actor.skills[mSelectedSkillIndex];
                    if (action.isMagic) actor.currentSp -= action.cost;
                    else actor.currentHp -= action.cost;
                }    
                CheckWeakness(actor, mGameState.battleEnemies[mSelectedTargetIndex], action);
            } else if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_ESCAPE)) mState = PLAYER_TURN_MAIN;
        }
        else if (mState == PLAYER_TURN_TARGET_ALLY) 
        {
            // 1. Navigate Party List (Up/Down matches the visual layout)
            if (IsKeyPressed(KEY_DOWN)) mSelectedTargetIndex = (mSelectedTargetIndex + 1) % mGameState.party.size();
            else if (IsKeyPressed(KEY_UP)) mSelectedTargetIndex = (mSelectedTargetIndex - 1 + mGameState.party.size()) % mGameState.party.size();

            if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) // CONFIRM HEAL
            {
                Combatant& actor = mGameState.party[mActiveMemberIndex];
                Combatant& target = mGameState.party[mSelectedTargetIndex];

                if (mSelectedSkillIndex <= -100) {
                    // Using an item: decode item index from sentinel
                    int itemIndex = -mSelectedSkillIndex - 100;
                    if (itemIndex >= 0 && itemIndex < (int)mGameState.inventory.size()) {
                        Item item = mGameState.inventory[itemIndex];
                        if (item.isRevive) {
                            if (!target.isAlive) {
                                target.isAlive = true;
                                target.isDown = false;
                                target.currentHp = item.value;
                                if (target.currentHp > target.maxHp) target.currentHp = target.maxHp;
                                mLog = "Revived " + target.name + " to " + std::to_string(target.currentHp) + " HP!";
                            } else {
                                mLog = target.name + " is already alive.";
                            }
                        } else if (item.isSP) {
                            target.currentSp += item.value;
                            if (target.currentSp > target.maxSp) target.currentSp = target.maxSp;
                            mLog = "Restored " + std::to_string(item.value) + " SP to " + target.name + "!";
                        } else {
                            target.currentHp += item.value;
                            if (target.currentHp > target.maxHp) target.currentHp = target.maxHp;
                            mLog = "Healed " + target.name + " for " + std::to_string(item.value) + " HP!";
                        }
                        // Consume item
                        mGameState.inventory.erase(mGameState.inventory.begin() + itemIndex);
                        actor.hasActed = true;
                        mState = ANIMATION_WAIT;
                        mTimer = 0.0f;
                    } else {
                        mLog = "Item use canceled.";
                        mState = PLAYER_TURN_ITEM;
                    }
                } else {
                    // Using a healing skill
                    Ability& skill = actor.skills[mSelectedSkillIndex];
                    if (skill.isMagic) actor.currentSp -= skill.cost;
                    else actor.currentHp -= skill.cost;
                    int healAmount = -skill.damage; 
                    target.currentHp += healAmount;
                    if (target.currentHp > target.maxHp) target.currentHp = target.maxHp;
                    mLog = "Healed " + target.name + " for " + std::to_string(healAmount) + " HP!";
                    actor.hasActed = true;
                    mState = ANIMATION_WAIT;
                    mTimer = 0.0f;
                }
            }
            else if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_ESCAPE)) 
            {
                // Return to correct menu based on origin
                if (mSelectedSkillIndex <= -100) {
                    // Reset selection index for items
                    mSelectedSkillIndex = 0;
                    mState = PLAYER_TURN_ITEM;
                } else {
                    mState = PLAYER_TURN_SKILLS;
                }
            }
        }
        else if (mState == PLAYER_TURN_ITEM) 
        {
            if (IsKeyPressed(KEY_DOWN)) 
                mSelectedSkillIndex = (mSelectedSkillIndex + 1) % mGameState.inventory.size();
            if (IsKeyPressed(KEY_UP))   
                mSelectedSkillIndex = (mSelectedSkillIndex - 1 + mGameState.inventory.size()) % mGameState.inventory.size();
            if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_ESCAPE)) 
                mState = PLAYER_TURN_MAIN; // Cancel back to main
            // Use Item
            if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                // Encode the selected item into a negative sentinel to distinguish later
                mSelectedSkillIndex = -(100 + mSelectedSkillIndex);
                mState = PLAYER_TURN_TARGET_ALLY;
                mSelectedTargetIndex = mActiveMemberIndex; // Default to self
            }
        }
        // --- UPDATE FLOATING DAMAGE TEXTS ---
        for (int i = (int)mFloatingTexts.size() - 1; i >= 0; --i) {
            FloatingText& ft = mFloatingTexts[i];
            ft.elapsed += deltaTime;
            // move up and fade out over lifetime
            ft.pos.y -= 40.0f * deltaTime;
            if (ft.elapsed >= ft.lifetime) {
                mFloatingTexts.erase(mFloatingTexts.begin() + i);
            }
        }
    }

    void CombatScene::CheckWeakness(Combatant& attacker, Combatant& defender, Ability skill) {
        bool isWeakness = false;
        if (!defender.isGuarding) {
            for (Element w : defender.weaknesses) {
                if (skill.element == w) { isWeakness = true; break; }
            }
        }

        // calculate damage
        int attatckPower = skill.damage;
        
        // if melee or gun, factor in attacker's stats
        if (skill.element == PHYS) {
            attatckPower += attacker.getTotalAttack();
        } else if (skill.element == GUN) {
            attatckPower += attacker.getTotalGunAttack();
        }

        int damage = attatckPower - defender.getTotalDefense();
        if (damage < 1) damage = 1; // Minimum damage
        // apply modifiers
        if (isWeakness) damage = (int)(damage * 1.5f);
        if (defender.isGuarding) damage = (int) (damage * 0.5f);

        // apply damage
        defender.currentHp -= damage;
        if (defender.currentHp <= 0) {
            defender.currentHp = 0;
            defender.isAlive = false;
            defender.isDown = true;
        }
        // One more mechanics
        if (isWeakness && !defender.isDown && defender.isAlive) {
            defender.isDown = true;
            mLog = "WEAKNESS! 1 More!";
            attacker.hasActed = false;
        } else {
            mLog = "Hit! Dealt " + std::to_string(damage) + " damage.";
            attacker.hasActed = true;
        }
        // Spawn damage floating text at enemy position (right side column)
        // Find index of defender in enemy list if present
        int defIndex = -1;
        for (int i = 0; i < (int)mGameState.battleEnemies.size(); ++i) {
            if (mGameState.battleEnemies[i].name == defender.name) { defIndex = i; break; }
        }
        if (defIndex >= 0) {
            float ex = 600.0f + (defIndex * 120.0f);
            float ey = 200.0f;
            SpawnFloatingText({ ex + 10.0f, ey - 40.0f }, TextFormat("-%d", damage), YELLOW, 0.8f);
        }

        mState = ANIMATION_WAIT;
        mTimer = 0.0f;
    }

    void CombatScene::CheckHoldUp() {
        bool allDown = true;
        bool anyAlive = false;
        for (auto& enemy : mGameState.battleEnemies) {
            if (enemy.isAlive) {
                anyAlive = true;
                if (!enemy.isDown) allDown = false;
            }
        }
        if (anyAlive && allDown) {
            mState = HOLD_UP;
            mLog = "HOLD UP! Press [Y] for All-Out Attack!";
        }
    }

    void CombatScene::render() {
        ClearBackground(BLACK);

            // 1. Draw Party Sprites (Visuals) - left of center, vertical lineup
            for (int i = 0; i < (int)mPartySprites.size(); i++) {
                float spriteX = 340.0f;            // fixed x left of center
                float spriteY = 150.0f + (i * 90.0f); // vertical stack

                Texture2D tex = mPartySprites[i]->getTexture();
                // Use atlas frame #15 in a 4x5 sheet (matches your asset)
                const int cols = 4;
                const int rows = 5;
                const int frameIndex = 15;
                float frameW = (float)tex.width / (float)cols;
                float frameH = (float)tex.height / (float)rows;
                int col = frameIndex % cols;
                int row = frameIndex / cols;
                Rectangle src = { col * frameW, row * frameH, frameW, frameH };

                // Force name-based tint to ensure color shows clearly
                Color tint = WHITE;
                if (mGameState.party[i].name == "Skull") tint = YELLOW;
                else if (mGameState.party[i].name == "Mona") tint = SKYBLUE;
                else if (mGameState.party[i].name == "Noir") tint = VIOLET;

                DrawTexturePro(
                    tex,
                    src,
                    { spriteX, spriteY, 48, 48 },
                    { 24, 24 },
                    0.0f,
                    tint
                );
            }

        for (int i = 0; i < mGameState.party.size(); i++) {
            Combatant& member = mGameState.party[i];
            float x = 20; 
            float y = 100 + (i * 90);

            Color c = LIGHTGRAY;
            if (member.name == "Skull") c = YELLOW;
            else if (member.name == "Mona") c = SKYBLUE;
            else if (member.name == "Noir") c = PURPLE;

            // Active acting member highlight
            if (mState != ENEMY_TURN && mActiveMemberIndex == i) {
                DrawRectangleLines(x-10, y-10, 220, 90, WHITE);
                DrawText(">", x-30, y+20, 20, WHITE);
            }

            // Ally heal target highlight (distinct color)
            if (mState == PLAYER_TURN_TARGET_ALLY && mSelectedTargetIndex == i) {
                DrawRectangleLines(x-14, y-14, 228, 98, SKYBLUE);
                DrawText("HEAL", x+220, y+20, 20, SKYBLUE);
            }

            // Icon placeholder and HUD block near left edge
            // Use character icon textures for HUD
            int iconIndex = i % 4;
            Color tint = mGameState.party[i].tint;
            if (tint.a == 0) {
                if (member.name == "Skull") tint = YELLOW;
                else if (member.name == "Mona") tint = SKYBLUE;
                else if (member.name == "Noir") tint = VIOLET;
                else tint = WHITE;
            }
            DrawTextureEx(gPartyIcons[iconIndex], { x, y }, 0.0f, 0.4f, tint);
            DrawText(member.name.c_str(), x + 60, y - 10, 22, WHITE);

            // Slanted HP/SP bars (reuse Task 1 style)
            float hpPercent = (member.maxHp > 0) ? ((float)member.currentHp / (float)member.maxHp) : 0.0f;
            float spPercent = (member.maxSp > 0) ? ((float)member.currentSp / (float)member.maxSp) : 0.0f;
            if (hpPercent < 0) hpPercent = 0; if (hpPercent > 1) hpPercent = 1;
            if (spPercent < 0) spPercent = 0; if (spPercent > 1) spPercent = 1;

            Color hpColor = GREEN;
            if (hpPercent < 0.5f) hpColor = YELLOW;
            if (hpPercent < 0.2f) hpColor = RED;

            int barX = (int)(x + 60);
            // Background HP
            DrawSlantedRect(barX, (int)y + 8, 150, 14, -10, Fade(BLACK, 0.5f));
            // HP foreground
            DrawSlantedRect(barX, (int)y + 8, (int)(150 * hpPercent), 14, -10, hpColor);
            // SP bar
            DrawSlantedRect(barX + 5, (int)y + 28, (int)(120 * spPercent), 8, -10, SKYBLUE);

            // Text overlays
            DrawText(TextFormat("%d / %d", member.currentHp, member.maxHp), barX + 4, (int)y + 6, 14, BLACK);
            DrawText(TextFormat("SP %d / %d", member.currentSp, member.maxSp), barX + 8, (int)y + 24, 12, WHITE);
        }

        for (int i = 0; i < mGameState.battleEnemies.size(); i++) {
            Combatant& enemy = mGameState.battleEnemies[i];

            float x = 600 + (i * 120);
            float y = 200;

            // Cursor indicator when targeting
            if (mState == PLAYER_TURN_TARGET && mSelectedTargetIndex == i) {
                DrawCircle(x+40, y-20, 10, RED);
            }

            // Animation frame selection
            Rectangle source = GetEnemyFrameRect(enemy, mState, mTimer);
            Rectangle dest = { x, y, 64.0f, 50.0f };

            // Draw atlas-based enemy (face left naturally; no flip)
            if (enemy.isAlive || (int)(source.x/32.0f) + (int)(source.y/25.0f)*8 == 20) {
                DrawTexturePro(mEnemyAtlas, source, dest, {0,0}, 0.0f, WHITE);
            }

            // Name and HP bar overlays above enemy
            DrawText(enemy.name.c_str(), (int)x, (int)(y-50), 20, WHITE);

            // Simple thin HP bar
            int barWidth = 80;
            int barHeight = 6;
            int barX = (int)x;
            int barY = (int)(y - 30);
            float hpPercent = (enemy.maxHp > 0) ? (float)enemy.currentHp / (float)enemy.maxHp : 0.0f;
            if (hpPercent < 0) hpPercent = 0; if (hpPercent > 1) hpPercent = 1;
            Color hpColor = (hpPercent < 0.2f) ? RED : (hpPercent < 0.5f ? YELLOW : GREEN);
            DrawRectangle(barX, barY, barWidth, barHeight, Fade(BLACK, 0.6f));
            DrawRectangle(barX, barY, (int)(barWidth * hpPercent), barHeight, hpColor);
            DrawRectangleLines(barX, barY, barWidth, barHeight, WHITE);
            // Move HP text below the bar, still above the enemy sprite
            DrawText(TextFormat("%d / %d", enemy.currentHp, enemy.maxHp), barX, barY + 8, 16, WHITE);

            if (enemy.isDown) DrawText("DOWN", (int)x, (int)(y+90), 20, SKYBLUE);
        }

        // Bottom UI Panel (bottom-right half, shows only mLog)
        DrawRectangleLines(500, 500, 500, 100, WHITE);
        DrawText(mLog.c_str(), 520, 510, 20, WHITE);

        // Context-aware control & action hints
        Combatant& actor = mGameState.party[mActiveMemberIndex];

        if (mState == PLAYER_TURN_MAIN) {
            // Move context-aware control & action hints to top-right corner
            DrawText("[UP/DOWN] Rotate  [Z/SPACE] Select", 650, 20, 18, LIGHTGRAY);
            DrawText("Select an action.", 650, 45, 18, DARKGRAY);
        }
        else if (mState == PLAYER_TURN_SKILLS) {
            DrawRectangle(600, 100, 240, 240, BLACK);
            DrawRectangleLines(600, 100, 240, 240, WHITE);

            for (int i = 0; i < actor.skills.size(); i++) {
                const Ability &ab = actor.skills[i];
                Color c = (i == mSelectedSkillIndex) ? YELLOW : WHITE;
                const char* costType = ab.isMagic ? "SP" : "HP";
                DrawText(TextFormat("%s (%d %s)", ab.name.c_str(), ab.cost, costType), 610, 110 + (i*30), 20, c);
            }

            // Description of currently highlighted skill (mirrors target selection details)
            if (!actor.skills.empty()) {
                const Ability &chosen = actor.skills[mSelectedSkillIndex];
                const char* costType = chosen.isMagic ? "SP" : "HP";
                if (chosen.damage < 0) {
                    DrawText(TextFormat("Action: %s (Heals %d HP, %d %s)", chosen.name.c_str(), -chosen.damage, chosen.cost, costType), 20, 535, 18, SKYBLUE);
                } else {
                    DrawText(TextFormat("Action: %s (Dmg %d, %d %s)", chosen.name.c_str(), chosen.damage, chosen.cost, costType), 20, 535, 18, YELLOW);
                }
            }

            // Hints in top-right
            DrawText("[UP/DOWN] Navigate", 650, 20, 18, LIGHTGRAY);
            DrawText("[Z] Confirm  [C] Back", 650, 45, 18, LIGHTGRAY);
        }
        else if (mState == PLAYER_TURN_TARGET) {
            // Show chosen action details
            Ability chosen;
            if (mSelectedSkillIndex == -1) {
                chosen = {"Melee", 0, actor.getTotalAttack(), PHYS, false};
            } else if (mSelectedSkillIndex == -2) {
                chosen = {"Gun", 0, actor.getTotalGunAttack(), GUN, false};
            } else {
                chosen = actor.skills[mSelectedSkillIndex];
            }
            const char* costType = chosen.isMagic ? "SP" : "HP";
            if (chosen.damage < 0) {
                DrawText(TextFormat("Action: %s (Heals %d HP, %d %s)", chosen.name.c_str(), -chosen.damage, chosen.cost, costType), 20, 535, 18, SKYBLUE);
            } else {
                DrawText(TextFormat("Action: %s (Dmg %d, %d %s)", chosen.name.c_str(), chosen.damage, chosen.cost, costType), 20, 535, 18, YELLOW);
            }
            // Hints in top-right
            DrawText("[LEFT/RIGHT] Target", 650, 20, 18, LIGHTGRAY);
            DrawText("[Z] Confirm  [C] Cancel", 650, 45, 18, LIGHTGRAY);
        }
        else if (mState == PLAYER_TURN_TARGET_ALLY) {
            // Show context based on whether we're using a skill or an item
            if (mSelectedSkillIndex <= -100) {
                int itemIndex = -mSelectedSkillIndex - 100;
                if (itemIndex >= 0 && itemIndex < (int)mGameState.inventory.size()) {
                    const Item& item = mGameState.inventory[itemIndex];
                    if (item.isRevive) {
                        DrawText(TextFormat("Item: %s (Revive to %d HP)", item.name.c_str(), item.value), 20, 535, 18, SKYBLUE);
                    } else if (item.isSP) {
                        DrawText(TextFormat("Item: %s (Restore %d SP)", item.name.c_str(), item.value), 20, 535, 18, SKYBLUE);
                    } else {
                        DrawText(TextFormat("Item: %s (Heal %d HP)", item.name.c_str(), item.value), 20, 535, 18, SKYBLUE);
                    }
                }
            } else {
                Ability chosen = actor.skills[mSelectedSkillIndex];
                const char* costType = chosen.isMagic ? "SP" : "HP";
                DrawText(TextFormat("Healing: %s (Heals %d HP, %d %s)", chosen.name.c_str(), -chosen.damage, chosen.cost, costType), 20, 535, 18, SKYBLUE);
            }
            // Hints in top-right
            DrawText("[UP/DOWN] Select Ally", 650, 20, 18, LIGHTGRAY);
            DrawText("[Z] Confirm  [C] Cancel", 650, 45, 18, LIGHTGRAY);
        }
        else if (mState == ENEMY_TURN) {
            DrawText("Enemy Phase...", 650, 20, 18, DARKGRAY);
        }
        else if (mState == ANIMATION_WAIT) {
            DrawText("Resolving action...", 650, 20, 18, DARKGRAY);
        }
        else if (mState == PLAYER_TURN_ITEM) {
            DrawRectangle(600, 100, 300, 300, BLACK);
            DrawRectangleLines(600, 100, 300, 300, WHITE);
        
            for (int i = 0; i < mGameState.inventory.size(); i++) {
                Color c = (i == mSelectedSkillIndex) ? YELLOW : WHITE;
                DrawText(mGameState.inventory[i].name.c_str(), 610, 110 + (i*30), 20, c);
                DrawText(TextFormat("x1"), 850, 110 + (i*30), 20, c);
            }
        
            // Description
            const Item& item = mGameState.inventory[mSelectedSkillIndex];
            DrawText(item.description.c_str(), 20, 535, 18, WHITE);
        }
        if (mState == HOLD_UP) {
            DrawText("HOLD UP! [Y] ALL OUT ATTACK", 300, 300, 30, RED);
        }

        // --- RENDER COMBAT UI: Command Wheel ---
        if (mState == PLAYER_TURN_MAIN) {
            // Shift command wheel slightly right to avoid HUD overlap
            Vector2 center = { 150.0f, 500.0f };
            float radius = 80.0f;
            DrawCircleV(center, radius + 10, Fade(RED, 0.5f));

            Texture2D icons[] = { mIconAttack, mIconGun, mIconSkill, mIconGuard, mIconItem };
            const char* labels[] = { "ATTACK", "GUN", "SKILL", "GUARD", "ITEM" };
            for (int i = 0; i < 5; i++) {
                float angleDeg = mWheelRotation + (i * 72.0f);
                float angleRad = angleDeg * DEG2RAD;
                Vector2 itemPos = { center.x + cosf(angleRad) * radius, center.y + sinf(angleRad) * radius };

                DrawTexturePro(
                    icons[i],
                    { 0, 0, (float)icons[i].width, (float)icons[i].height },
                    { itemPos.x, itemPos.y, 48, 48 },
                    { 24, 24 },
                    0.0f,
                    (i == mSelectedActionIndex) ? WHITE : GRAY
                );

                if (i == mSelectedActionIndex) {
                    DrawText(labels[i], center.x + 100, center.y - 10, 30, WHITE);
                }
            }
        }

        // --- RENDER CURSOR OVER ENEMY DURING TARGETING ---
        if (mState == PLAYER_TURN_TARGET) {
            if (mSelectedTargetIndex >= 0 && mSelectedTargetIndex < mGameState.battleEnemies.size()) {
                float ex = 600 + (mSelectedTargetIndex * 120);
                float ey = 200;
                float bounce = sinf(GetTime() * 5.0f) * 10.0f;
                DrawTexture(mUiCursor, ex + 20, ey - 60 + bounce, WHITE);
            }
        }
        // --- RENDER FLOATING DAMAGE TEXTS ---
        for (const FloatingText& ft : mFloatingTexts) {
            float alpha = 1.0f - (ft.elapsed / ft.lifetime);
            Color c = ft.color; c.a = (unsigned char)(alpha * 255);
            DrawText(ft.text.c_str(), (int)ft.pos.x, (int)ft.pos.y, 20, c);
        }
        // --- RENDER EFFECT OVERLAY ---
        if (mEffects) mEffects->render();
    }