#include "lib/cs3113.h"
#include "lib/GameTypes.h"
#include "lib/GameData.h"
#include "scenes/LevelOne.h"
#include "scenes/CombatScene.h"
#include "lib/ShaderProgram.h"

// --- GLOBALS ---
constexpr int SCREEN_WIDTH  = 1000;
constexpr int SCREEN_HEIGHT = 600;
constexpr int TARGET_FPS    = 60;

GameStatus gGameStatus = EXPLORATION;
AppStatus gAppStatus   = RUNNING;

float gPreviousTicks   = 0.0f;
int gCurrentLevelIndex = -1;

Scene *gCurrentScene = nullptr;
std::vector<Scene*> gLevels;
std::vector<Combatant> gParty; // Defined here, populated from GameData
std::vector<Item> gInventory;  // Player inventory

// Global Tracking
std::vector<std::vector<bool>> gSceneEnemyDefeated; 

ShaderProgram gShader;


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
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Persona 5 Demake");
    SetTargetFPS(TARGET_FPS);
    SetExitKey(KEY_NULL);


    // Load Initial Party from Data Header
    gParty = INITIAL_PARTY(); 
    gInventory = INITIAL_INVENTORY();
    
    // Initialize Audio (Requirement 6)
    InitAudioDevice();

    gShader.load("shaders/vertex.glsl", "shaders/fragment.glsl");

    // Reset global enemy defeat tracking at app start (new session)
    gSceneEnemyDefeated.clear();
}

void processInput() 
{
    if ( WindowShouldClose()) gAppStatus = TERMINATED;
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
        gShader.begin();

        gShader.setInt("status", gCurrentScene->getState().shaderStatus);

        if (gCurrentScene->getState().player) {
            Vector2 playerPos = gCurrentScene->getState().player->getPosition();
            gShader.setVector2("lightPosition", playerPos);
        }
        // --- WORLD RENDERING ---
        // We assume the current scene has a valid camera setup
        BeginMode2D(gCurrentScene->getState().camera);
        gCurrentScene->render();
        EndMode2D();

        gShader.end();

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

        // (Removed debug HUD for enemies)
    }
    // 2. STATIC SCENES: Menu, Combat, Game Over
    else 
    {
        // These scenes handle their own screen-space positioning
        gCurrentScene->render();
    }

    EndDrawing();
}

void Shutdown() 
{
    gShader.unload();
    CloseAudioDevice();
    CloseWindow();
}

int main()
{
    initialise();

    // Scene Setup
    gLevels.push_back(new LevelOne({SCREEN_WIDTH/2, SCREEN_HEIGHT/2}, "#000000"));
    gLevels.push_back(nullptr); // Placeholder
    gLevels.push_back(new CombatScene({SCREEN_WIDTH/2, SCREEN_HEIGHT/2}, "#000000"));
    
    gSceneEnemyDefeated.resize(gLevels.size());

    switchToScene(0);

    while (gAppStatus != TERMINATED) {
        processInput();
        update();

        // Scene Switching Logic (Keep existing logic)
        if (gCurrentScene->getState().nextSceneID != -1) {
             int nextID = gCurrentScene->getState().nextSceneID;
             
             // Pass Party State to Combat
            if (nextID == 2) { 
                gLevels[nextID]->getState().party = gParty;
                gLevels[nextID]->getState().inventory = gInventory;
                gLevels[nextID]->getState().returnSceneID = gCurrentLevelIndex;
                gLevels[nextID]->getState().engagedEnemyIndex = gCurrentScene->getState().engagedEnemyIndex;
                // Pass combat advantage determined in exploration
                gLevels[nextID]->getState().combatAdvantage = gCurrentScene->getState().combatAdvantage;
                // Save player position for return after combat
                if (gCurrentScene->getState().player) {
                    gLevels[nextID]->getState().returnSpawnPos = gCurrentScene->getState().player->getPosition();
                    gLevels[nextID]->getState().hasReturnSpawnPos = true;
                }
                // Copy defeated enemy flags from level to combat scene
                gLevels[nextID]->getState().defeatedEnemies = gCurrentScene->getState().defeatedEnemies;
            }
             
            // Return from Combat: Update Global Party and restore spawn position in target level
            if (gCurrentLevelIndex == 2 && nextID != 2) {
                gParty = gCurrentScene->getState().party;
                gInventory = gCurrentScene->getState().inventory;
                gLevels[nextID]->getState().returnSpawnPos = gCurrentScene->getState().returnSpawnPos;
                gLevels[nextID]->getState().hasReturnSpawnPos = gCurrentScene->getState().hasReturnSpawnPos;
                // Copy defeated enemy flags back to target level scene
                gLevels[nextID]->getState().defeatedEnemies = gCurrentScene->getState().defeatedEnemies;
            }

            gCurrentScene->shutdown();
            switchToScene(nextID);
            gGameStatus = (nextID == 2) ? COMBAT : EXPLORATION;
        }

        render();
    }

    CloseAudioDevice();
    CloseWindow();
    return 0;
}