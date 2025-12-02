#include "CombatScene.h"
#include <cmath>
#include "raymath.h"

// CombatScene should use its own GameState.party rather than the global gParty

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

        // Encounter generation could depend on engagedEnemyIndex
        // For prototype keep two test enemies; future improvement: load from level data
        mGameState.battleEnemies.clear();
        Combatant shadow1;
        shadow1.name = "Pixie";
        shadow1.maxHp = 80; shadow1.currentHp = 80;
        shadow1.baseAttack = 10; shadow1.baseDefense = 5;
        shadow1.isAlive = true;
        shadow1.weaknesses = { GUN, ICE, CURSE };
        mGameState.battleEnemies.push_back(shadow1);
        Combatant shadow2;
        shadow2.name = "Jack";
        shadow2.maxHp = 120; shadow2.currentHp = 120;
        shadow2.baseAttack = 15; shadow2.baseDefense = 8;
        shadow2.isAlive = true;
        shadow2.weaknesses = { WIND };
        mGameState.battleEnemies.push_back(shadow2);

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

        // copy inventory from global to local state
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

            if (IsKeyPressed(KEY_Z)) // CONFIRM HEAL
            {
                Combatant& actor = mGameState.party[mActiveMemberIndex];
                Combatant& target = mGameState.party[mSelectedTargetIndex];
                Ability& skill = actor.skills[mSelectedSkillIndex];

                // Pay Cost
                if (skill.isMagic) actor.currentSp -= skill.cost;
                else actor.currentHp -= skill.cost;

                // Apply Healing (Negative damage * -1 = Positive HP)
                int healAmount = -skill.damage; 
                target.currentHp += healAmount;
                
                // Cap at Max HP
                if (target.currentHp > target.maxHp) target.currentHp = target.maxHp;

                mLog = "Healed " + target.name + " for " + std::to_string(healAmount) + " HP!";
                actor.hasActed = true; // Healing ends turn (usually doesn't grant 1 More)
                mState = ANIMATION_WAIT;
            }
            else if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_ESCAPE)) 
            {
                mState = PLAYER_TURN_SKILLS; // Cancel back to menu
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
                mState = PLAYER_TURN_TARGET_ALLY;
                mSelectedTargetIndex = mActiveMemberIndex; // Default to self
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
        mState = ANIMATION_WAIT;
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

        for (int i = 0; i < mGameState.party.size(); i++) {
            Combatant& member = mGameState.party[i];
            float x = 50;
            float y = 50 + (i * 100);

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

            DrawRectangle(x, y, 60, 60, c);
            DrawText(member.name.c_str(), x+70, y, 20, c);
            DrawText(TextFormat("HP: %d", member.currentHp), x+70, y+25, 20, WHITE);
            DrawText(TextFormat("SP: %d", member.currentSp), x+70, y+50, 20, SKYBLUE);
        }

        for (int i = 0; i < mGameState.battleEnemies.size(); i++) {
            Combatant& enemy = mGameState.battleEnemies[i];
            if (!enemy.isAlive) continue;

            float x = 600 + (i * 120);
            float y = 200;

            if (mState == PLAYER_TURN_TARGET && mSelectedTargetIndex == i) {
                DrawCircle(x+40, y-20, 10, RED);
            }

            Color ec = RED;
            if (enemy.isDown) ec = BLUE;

            DrawRectangle(x, y, 80, 80, ec);
            DrawText(enemy.name.c_str(), x, y-30, 20, WHITE);
            DrawText(TextFormat("%d", enemy.currentHp), x+20, y+30, 20, WHITE);
            if (enemy.isDown) DrawText("DOWN", x, y+90, 20, SKYBLUE);
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
            Ability chosen = actor.skills[mSelectedSkillIndex];
            const char* costType = chosen.isMagic ? "SP" : "HP";
            DrawText(TextFormat("Healing: %s (Heals %d HP, %d %s)", chosen.name.c_str(), -chosen.damage, chosen.cost, costType), 20, 535, 18, SKYBLUE);
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
            Vector2 center = { 100.0f, 500.0f };
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
    }