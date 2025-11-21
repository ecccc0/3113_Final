#include "lib/cs3113.h"
#include "scenes/LevelOne.h"
#include "scenes/CombatScene.h"
constexpr int SCREEN_WIDTH     = 1000,
              SCREEN_HEIGHT    = 600,
              FPS              = 120;

constexpr Vector2 ORIGIN      = { SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 };
enum GameStatus   { EXPLORATION, COMBAT, PAUSED, GAME_OVER };

GameStatus gGameStatus = EXPLORATION;

AppStatus gAppStatus   = RUNNING;

float gPreviousTicks   = 0.0f;

int gCurrentLevelIndex = -1;
Scene *gCurrentScene = nullptr;
std::vector<Scene*> gLevels = {};
std::vector<Combatant> gParty = {};

// Persistent defeated enemy tracking per scene (sceneIndex -> vector of enemy flags)
std::vector<std::vector<bool>> gSceneEnemyDefeated; // sized after scenes are created


void switchToScene(int sceneIndex);
void initialise();
void processInput();
void update();
void render();
void shutdown();

void switchToScene(int sceneIndex)
{   
    if (sceneIndex < 0 || sceneIndex >= gLevels.size()) return; // Safety check

    std::cout << "[main] switchToScene: switching to scene " << sceneIndex << std::endl;
    gCurrentScene = gLevels[sceneIndex];
    gCurrentLevelIndex = sceneIndex; // set before initialise so scene can use index
    // Ensure transition request flag starts cleared to avoid accidental immediate switches
    gCurrentScene->getState().nextSceneID = -1;
    gCurrentScene->initialise();
}


void initialise() 
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "CS3113 Final Project");
    SetTargetFPS(FPS);

    gParty.clear(); 

    // --- JOKER (Balanced / Curse / Gun) ---
    Combatant joker;
    joker.name = "Joker";
    joker.texturePath = "assets/player_joker.png";
    joker.maxHp = 100; joker.currentHp = 100;
    joker.maxSp = 50;  joker.currentSp = 50;
    joker.attack = 15; joker.defense = 10; joker.speed = 12;
    joker.isAlive = true;
    joker.skills = {
        {"Eiha", 4, 30, CURSE},
        {"Gun",  2, 20, GUN}
    };
    gParty.push_back(joker);

    // --- SKULL (Physical Tank / Electric) ---
    Combatant skull;
    skull.name = "Skull";
    skull.texturePath = "assets/player_skull.png";
    skull.maxHp = 150; skull.currentHp = 150;
    skull.maxSp = 30;  skull.currentSp = 30;
    skull.attack = 20; skull.defense = 15; skull.speed = 8;
    skull.isAlive = true;
    skull.skills = {
        {"Lunge", 0, 25, PHYS},
        {"Zio",   4, 25, ELEC}
    };
    gParty.push_back(skull);

    // --- MONA (Healer / Wind) ---
    Combatant mona;
    mona.name = "Mona";
    mona.texturePath = "assets/player_mona.png";
    mona.maxHp = 80;  mona.currentHp = 80;
    mona.maxSp = 60;  mona.currentSp = 60;
    mona.attack = 12; mona.defense = 8; mona.speed = 15;
    mona.isAlive = true;
    mona.skills = {
        {"Garu", 4, 20, WIND},
        {"Dia",  4, -30, ELEMENT_NONE} // Negative damage = Healing
    };
    gParty.push_back(mona);

    // --- NOIR (Burst Damage / Psi) ---
    Combatant noir;
    noir.name = "Noir";
    noir.texturePath = "assets/player_noir.png";
    noir.maxHp = 110; noir.currentHp = 110;
    noir.maxSp = 45;  noir.currentSp = 45;
    noir.attack = 18; noir.defense = 12; noir.speed = 10;
    noir.isAlive = true;
    noir.skills = {
        {"Psi",       4, 35, PSI},
        {"Mapsi",     8, 25, PSI} 
    };
    gParty.push_back(noir);



}

void processInput() 
{
    if (IsKeyPressed(KEY_ESCAPE) || WindowShouldClose()) gAppStatus = TERMINATED;
    if (gGameStatus == EXPLORATION) {
        // Reset movement each frame so entity only moves while keys are down
        gCurrentScene->getState().player->resetMovement();

        // Movements during exploration (use IsKeyDown so holding the key moves continuously)
        if (IsKeyDown(KEY_A)) gCurrentScene->getState().player->moveLeft();
        if (IsKeyDown(KEY_D)) gCurrentScene->getState().player->moveRight();
        if (IsKeyDown(KEY_W)) gCurrentScene->getState().player->moveUp();
        if (IsKeyDown(KEY_S)) gCurrentScene->getState().player->moveDown();

        // Normalize diagonal movement so speed is consistent
        if (GetLength(gCurrentScene->getState().player->getMovement()) > 1.0f) {
            gCurrentScene->getState().player->normaliseMovement();
        }
    }
    else if  ( gGameStatus == GAME_OVER) {
        if (IsKeyPressed(KEY_R)) {
            gCurrentScene->shutdown();
            initialise();
            switchToScene(0);
            gGameStatus = EXPLORATION;
        }
    }

    // Combat input is handled within CombatScene itself
    // menu scene not implemented yet
}


void update() 
{
    // UpdateMusicStream(gBGM);
    float ticks = (float) GetTime();
    float deltaTime = ticks - gPreviousTicks;
    gPreviousTicks  = ticks;

    gCurrentScene->update(deltaTime);
}

void render()
{
    BeginDrawing();
    
    ClearBackground(BLACK);

    // 1. EXPLORATION: Draw World + Party HUD
    if (gGameStatus == EXPLORATION)
    {
        // --- WORLD RENDERING ---
        // We assume the current scene has a valid camera setup
        BeginMode2D(gCurrentScene->getState().camera);
        gCurrentScene->render();
        EndMode2D();

        // --- HUD RENDERING (Fixed on screen) ---
        // Draw a semi-transparent box behind the stats for readability
        DrawRectangle(10, 10, 220, 150 + (gParty.size() * 40), Fade(BLACK, 0.6f));
        DrawRectangleLines(10, 10, 220, 150 + (gParty.size() * 40), WHITE);

        DrawText("PHANTOM THIEVES", 30, 20, 20, WHITE);

        int yPos = 50;
        for (const auto& member : gParty)
        {
            Color nameColor = LIGHTGRAY; // Default (Joker)
            if (member.name == "Skull") nameColor = YELLOW;
            else if (member.name == "Mona") nameColor = SKYBLUE;
            else if (member.name == "Noir") nameColor = VIOLET;

            // 1. Name
            DrawText(member.name.c_str(), 20, yPos, 20, nameColor);

            // 2. HP (Green)
            std::string hpStr = "HP: " + std::to_string(member.currentHp);
            DrawText(hpStr.c_str(), 100, yPos, 20, GREEN);

            // 3. SP (Blue/Cyan)
            std::string spStr = "SP: " + std::to_string(member.currentSp);
            DrawText(spStr.c_str(), 170, yPos, 20, SKYBLUE);

            yPos += 30; // Spacing for next member
        }
    }
    // 2. STATIC SCENES: Menu, Combat, Game Over
    else 
    {
        // These scenes handle their own screen-space positioning
        gCurrentScene->render();
    }

    EndDrawing();
}

int main()
{
    // Initialise systems and global party
    initialise();

    // Populate scenes (indexing chosen so LevelOne=0, CombatScene=2 as expected)
    gLevels.push_back(new LevelOne(ORIGIN, "#000000")); // 0
    gLevels.push_back(nullptr);                          // 1 placeholder
    gLevels.push_back(new CombatScene(ORIGIN, "#000000")); // 2
    // Size defeated flags vector to number of scenes
    gSceneEnemyDefeated.resize(gLevels.size());

    switchToScene(0);

    // Main loop
    while (gAppStatus != TERMINATED) {
        processInput();
        update();

        // Scene transition handling: if the active scene set a nextSceneID,
        // safely switch to that scene and copy party data for combat.
        if (gCurrentScene) {
            int nextScene = gCurrentScene->getState().nextSceneID;
            if (nextScene != -1) {
                std::cout << "[main] Detected nextSceneID = " << nextScene << " from current scene " << gCurrentLevelIndex << std::endl;
                // Clear the request on the previous scene
                gCurrentScene->getState().nextSceneID = -1;

                // Validate and perform the transition
                if (nextScene >= 0 && nextScene < (int)gLevels.size() && gLevels[nextScene] != nullptr) {
                    Scene* target = gLevels[nextScene];

                    // If target is combat, seed it with the current global party
                    if (nextScene == 2) {
                        std::cout << "[main] Copying global party (size=" << gParty.size() << ") into CombatScene before initialise." << std::endl;
                        target->getState().party = gParty;
                        // Pass return scene and engaged enemy index from current scene's GameState
                        target->getState().returnSceneID = gCurrentScene->getState().returnSceneID >= 0 ? gCurrentScene->getState().returnSceneID : gCurrentLevelIndex;
                        target->getState().engagedEnemyIndex = gCurrentScene->getState().engagedEnemyIndex;
                        std::cout << "[main] CombatScene returnSceneID=" << target->getState().returnSceneID << " engagedEnemyIndex=" << target->getState().engagedEnemyIndex << std::endl;
                    }

                    // Shutdown current scene then initialise the next one
                    if (gCurrentScene) {
                        std::cout << "[main] Shutting down current scene " << gCurrentLevelIndex << std::endl;
                        gCurrentScene->shutdown();
                    }

                    switchToScene(nextScene);

                    // Update global game status to reflect the new scene
                    if (nextScene == 2) gGameStatus = COMBAT;
                    else gGameStatus = EXPLORATION;
                    std::cout << "[main] Completed transition to scene " << nextScene << std::endl;
                }
            }
        }

        render();
    }

    // Shutdown current scene if exists
    if (gCurrentScene) gCurrentScene->shutdown();
    return 0;
}