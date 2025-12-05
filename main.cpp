#include "lib/cs3113.h"
#include "lib/GameTypes.h"
#include "lib/GameData.h"
#include "scenes/LevelOne.h"
#include "scenes/LevelTwo.h"
#include "scenes/LevelThree.h"
#include "scenes/CombatScene.h"
#include "scenes/StartMenu.h"
#include "lib/ShaderProgram.h"
#include <iostream>
#include <unordered_map>

// --- GLOBALS ---
constexpr int SCREEN_WIDTH  = 1000;
constexpr int SCREEN_HEIGHT = 600;
constexpr int TARGET_FPS    = 60;

// Scene indices (order in gLevels)
constexpr int IDX_LEVEL_ONE   = 0;
constexpr int IDX_LEVEL_TWO   = 1;
constexpr int IDX_COMBAT      = 2;
constexpr int IDX_START_MENU  = 3;
constexpr int IDX_LEVEL_THREE = 4;

GameStatus gGameStatus = EXPLORATION;
AppStatus gAppStatus   = RUNNING;

float gPreviousTicks   = 0.0f;
int gCurrentLevelIndex = -1;

Scene *gCurrentScene = nullptr;
std::vector<Scene*> gLevels;
std::vector<Combatant> gParty; // Defined here, populated from GameData
std::vector<Item> gInventory;  // Player inventory
// --- EQUIPMENT GLOBALS ---
std::vector<Equipment> gOwnedEquipment; // The "Bag" of unequipped items
int gSelectedEquipSlot = 0; // 0=Melee, 1=Gun, 2=Armor

// --- HUD ICONS ---
Texture2D gPartyIcons[4]; // Joker, Skull, Mona, Noir

// --- PERSONA GLOBALS ---
std::vector<Persona> gOwnedPersonas;
int gEquippedPersonaIdx = 0; // Index in gOwnedPersonas

// Helper to Apply Persona Stats to Joker
void EquipPersona(int index) {
    if (index < 0 || index >= (int)gOwnedPersonas.size()) return;
    if (gParty.empty()) return; // Safety

    gEquippedPersonaIdx = index;
    Persona& p = gOwnedPersonas[index];
    Combatant& joker = gParty[0]; // Joker is always index 0

    // Overwrite Joker's Stats
    joker.baseAttack  = p.baseAttack;
    joker.baseDefense = p.baseDefense;
    joker.skills      = p.skills;
    joker.weaknesses  = p.weaknesses;

    std::cout << "Equipped Persona: " << p.name << std::endl;
}

// Global Tracking
std::vector<std::vector<bool>> gSceneEnemyDefeated; 

ShaderProgram gShader;

// --- GLOBAL TRANSITION (Fade In/Out) ---
enum TransitionPhase { T_NONE, T_FADE_OUT, T_SWITCH, T_FADE_IN };
TransitionPhase gTransitionPhase = T_NONE;
float gTransitionAlpha = 0.0f; // 0.0 = visible, 1.0 = black
int gPendingSceneID = -1;

// --- PAUSE MENU GLOBALS ---
enum PauseState {
    P_MAIN,
    P_PARTY_SELECT, // For choosing who to view (Skill/Equip)
    P_SKILL_LIST,
    P_SKILL_TARGET_ALLY, // NEW: For selecting who to heal
    P_ITEM_LIST,
    P_ITEM_TARGET_ALLY,
    P_EQUIP_VIEW,
    P_EQUIP_LIST,   // Select Item to Equip
    P_PERSONA,      // Joker only
    P_SYSTEM,
    P_AUDIO_SETTINGS     // NEW: Sliders
};

PauseState gPauseState = P_MAIN;

// Selection Variables
int gMenuSelection = 0;      // Generic selection index (Main Menu)
int gSubMenuSelection = 0;   // Selection for Lists (Party/Skills)
int gSelectedMemberIdx = 0;  // Which party member was chosen?
int gSelectedSkillIdx = -1;  // Tracks the specific skill chosen from the list

// Legacy pause helpers used during transition
int gPauseMenuSelection = 0; // 0=Skill, 1=Equip, 2=Persona, 3=System
bool gInSubMenu = false;
std::string gSubMenuMessage = ""; // Text to show in the sub-menu
bool gPauseJustOpened = false;     // Prevent immediate ESC unpause on same frame
int gItemGroupSelection = 0;       // Cursor for grouped item list

// Volume Settings (0.0f to 1.0f)
float gMasterVolume = 1.0f;
float gMusicVolume  = 0.8f;
float gSFXVolume    = 0.8f;

// Aliases requested: lowercase variants used in settings
float gmusicvolume  = 0.8f;
float gsfxvolume    = 0.8f;

// --- SFX (loaded at init) ---
Sound gSndBack{};
Sound gSndCrit{};
Sound gSndGun{};
Sound gSndHeal{};
Sound gSndHit{};
Sound gSndMenu{};

static void ApplySFXVolumes() {
    if (gSndBack.frameCount) SetSoundVolume(gSndBack, gSFXVolume);
    if (gSndCrit.frameCount) SetSoundVolume(gSndCrit, gSFXVolume);
    if (gSndGun.frameCount)  SetSoundVolume(gSndGun,  gSFXVolume);
    if (gSndHeal.frameCount) SetSoundVolume(gSndHeal, gSFXVolume);
    if (gSndHit.frameCount)  SetSoundVolume(gSndHit,  gSFXVolume);
    if (gSndMenu.frameCount) SetSoundVolume(gSndMenu, gSFXVolume);
}

// Helper: Draw a slanted/parallelogram rectangle (Persona-style)
void DrawSlantedRect(int x, int y, int width, int height, int skew, Color color) {
    Vector2 v1 = { (float)x + (float)skew, (float)y };
    Vector2 v2 = { (float)x, (float)y + (float)height };
    Vector2 v3 = { (float)x + (float)width, (float)y + (float)height };
    Vector2 v4 = { (float)x + (float)width + (float)skew, (float)y };
    DrawTriangle(v1, v2, v3, color);
    DrawTriangle(v1, v3, v4, color);
}

// Helper to reset menu state when opening
void OpenPauseMenu() {
    gGameStatus = PAUSED;
    gPauseState = P_MAIN;
    gMenuSelection = 0;
    gPauseJustOpened = true;
    gItemGroupSelection = 0;
}
// Group identical inventory items by name for stacked display and selection
struct ItemGroup {
    Item item;
    int count;
    std::vector<int> indices; // original indices in gInventory
};

static std::vector<ItemGroup> BuildInventoryGroups() {
    std::vector<ItemGroup> groups;
    std::unordered_map<std::string, int> nameToIdx;
    for (int i = 0; i < (int)gInventory.size(); ++i) {
        const Item &it = gInventory[i];
        auto itFind = nameToIdx.find(it.name);
        if (itFind == nameToIdx.end()) {
            ItemGroup g; g.item = it; g.count = 1; g.indices = { i };
            groups.push_back(g);
            nameToIdx[it.name] = (int)groups.size() - 1;
        } else {
            ItemGroup &g = groups[itFind->second];
            g.count += 1;
            g.indices.push_back(i);
        }
    }
    return groups;
}

// Helper to filter healing skills
// Returns a list of indices into the original skills vector
std::vector<int> GetHealingSkillIndices(const Combatant& c) {
    std::vector<int> indices;
    for (int i = 0; i < (int)c.skills.size(); i++) {
        if (c.skills[i].damage < 0) { // Negative damage = Healing
            indices.push_back(i);
        }
    }
    return indices;
}

// Helper to get indices of items in the bag that match the requested slot
std::vector<int> GetEquipIndicesByType(EquipmentType type) {
    std::vector<int> indices;
    for (int i = 0; i < (int)gOwnedEquipment.size(); i++) {
        if (gOwnedEquipment[i].type == type) {
            indices.push_back(i);
        }
    }
    return indices;
}


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

    // Ensure scene state starts with the current global inventory
    // Prevents first update() from overriding with empty scene inventory
    gCurrentScene->getState().inventory = gInventory;
}


void initialise() 
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Persona 5 Demake");
    SetTargetFPS(TARGET_FPS);
    SetExitKey(KEY_NULL);


    // Load Initial Party from Data Header
    gParty = INITIAL_PARTY(); 
    gInventory = INITIAL_INVENTORY();
    gOwnedEquipment = INITIAL_EQUIPMENTS();
    
    // LOAD PERSONAS & EQUIP STARTING ONE
    gOwnedPersonas = INITIAL_PERSONAS();
    EquipPersona(0); // Equip Arsene
    
    // Initialize Audio (Requirement 6)
    InitAudioDevice();
    // Load SFX
    gSndBack = LoadSound("assets/audio/back.wav");
    gSndCrit = LoadSound("assets/audio/crit.wav");
    gSndGun  = LoadSound("assets/audio/gun.wav");
    gSndHeal = LoadSound("assets/audio/heal.wav");
    gSndHit  = LoadSound("assets/audio/hit.wav");
    gSndMenu = LoadSound("assets/audio/menu.wav");
    ApplySFXVolumes();

    gShader.load("shaders/vertex.glsl", "shaders/fragment.glsl");

    // Load HUD icons (Task 0 prerequisite)
    gPartyIcons[0] = LoadTexture("assets/icon_joker.png");
    gPartyIcons[1] = LoadTexture("assets/icon_skull.png");
    gPartyIcons[2] = LoadTexture("assets/icon_mona.png");
    gPartyIcons[3] = LoadTexture("assets/icon_noir.png");

    // Reset global enemy defeat tracking at app start (new session)
    gSceneEnemyDefeated.clear();
}

void processInput() 
{
    if ( WindowShouldClose()) gAppStatus = TERMINATED;
    // Enter pause menu with ESC from exploration
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (gGameStatus == EXPLORATION) {
            OpenPauseMenu();
        }
    }
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

    // PAUSED input: extended navigation (Skills -> Target Ally, System -> Audio)
    if (gGameStatus == PAUSED) 
    {
        // --- 1. BACK / CANCEL LOGIC ---
        if (!gPauseJustOpened && (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_C))) {
            if (gSndBack.frameCount) PlaySound(gSndBack);
            switch (gPauseState) {
                case P_SKILL_TARGET_ALLY:
                    gPauseState = P_SKILL_LIST; // Cancel targeting, return to list
                    break;
                case P_ITEM_TARGET_ALLY:
                    gPauseState = P_ITEM_LIST; // Cancel item targeting, return to item list
                    break;
                case P_SKILL_LIST:
                case P_EQUIP_VIEW:
                    gPauseState = P_PARTY_SELECT; 
                    gSubMenuSelection = gSelectedMemberIdx; 
                    break;
                case P_EQUIP_LIST:
                    // From bag list, go back to slot view
                    gPauseState = P_EQUIP_VIEW;
                    break;
                case P_ITEM_LIST:
                    // Back to main menu, keep cursor on ITEM
                    gPauseState = P_MAIN;
                    gMenuSelection = 1;
                    break;
                case P_AUDIO_SETTINGS:
                    gPauseState = P_SYSTEM;
                    gSubMenuSelection = 0; // Highlight "Audio Settings"
                    break;
                case P_PARTY_SELECT:
                case P_PERSONA:
                case P_SYSTEM:
                    gPauseState = P_MAIN; 
                    break;
                case P_MAIN:
                    gGameStatus = EXPLORATION; 
                    gPreviousTicks = (float) GetTime();
                    break;
            }
            return; // Skip other input this frame
        }

        // --- 2. NAVIGATION & INTERACTION ---
        
        // A. MAIN MENU
        if (gPauseState == P_MAIN) {
            // Options: SKILL, ITEM, EQUIP, PERSONA, SYSTEM
            const int OPTION_COUNT = 5;
            if (IsKeyPressed(KEY_UP))   gMenuSelection = (gMenuSelection - 1 + OPTION_COUNT) % OPTION_COUNT;
            if (IsKeyPressed(KEY_DOWN)) gMenuSelection = (gMenuSelection + 1) % OPTION_COUNT;
            
            if (!gPauseJustOpened && (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER))) {
                if (gSndMenu.frameCount) PlaySound(gSndMenu);
                if (gMenuSelection == 0) { // SKILL
                    gPauseState = P_PARTY_SELECT;
                    gSubMenuSelection = 0;
                }
                else if (gMenuSelection == 1) { // ITEM
                    gPauseState = P_ITEM_LIST;
                    gSubMenuSelection = 0;
                }
                else if (gMenuSelection == 2) { // EQUIP
                     gPauseState = P_PARTY_SELECT;
                     gSubMenuSelection = 0;
                }
                else if (gMenuSelection == 3) { // PERSONA
                    gPauseState = P_PERSONA;
                }
                else if (gMenuSelection == 4) { // SYSTEM
                    gPauseState = P_SYSTEM;
                    gSubMenuSelection = 0;
                }
            }
        }

        // B. PARTY SELECTION (Shared for SKILL & EQUIP)
        else if (gPauseState == P_PARTY_SELECT) {
            if (!gParty.empty()) {
                if (IsKeyPressed(KEY_UP))   { gSubMenuSelection = (gSubMenuSelection - 1 + (int)gParty.size()) % (int)gParty.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }
                if (IsKeyPressed(KEY_DOWN)) { gSubMenuSelection = (gSubMenuSelection + 1) % (int)gParty.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }
            }

            if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                if (gSndMenu.frameCount) PlaySound(gSndMenu);
                gSelectedMemberIdx = gSubMenuSelection;
                if (gMenuSelection == 0) { // SKILL
                    gPauseState = P_SKILL_LIST;
                    gSubMenuSelection = 0; 
                } 
                else { // EQUIP
                    gPauseState = P_EQUIP_VIEW;
                }
            }
        }
        // C2. ITEM LIST (Inventory) -> choose item
        else if (gPauseState == P_ITEM_LIST) {
            std::vector<ItemGroup> groups = BuildInventoryGroups();
            if (groups.empty()) {
                // No items; back to main
                if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                    if (gSndBack.frameCount) PlaySound(gSndBack);
                    gPauseState = P_MAIN;
                    gMenuSelection = 0;
                }
            } else {
                if (IsKeyPressed(KEY_UP))   { gItemGroupSelection = (gItemGroupSelection - 1 + (int)groups.size()) % (int)groups.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }
                if (IsKeyPressed(KEY_DOWN)) { gItemGroupSelection = (gItemGroupSelection + 1) % (int)groups.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }
                if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                    if (gSndMenu.frameCount) PlaySound(gSndMenu);
                    // Go to target selection to apply item
                    gPauseState = P_ITEM_TARGET_ALLY;
                    // Default target to active member
                    gSubMenuSelection = gSelectedMemberIdx;
                }
            }
        }

        // D2. ITEM TARGETING (Apply item to ally)
        else if (gPauseState == P_ITEM_TARGET_ALLY) {
            if (!gParty.empty()) {
                if (IsKeyPressed(KEY_UP))   { gSubMenuSelection = (gSubMenuSelection - 1 + (int)gParty.size()) % (int)gParty.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }
                if (IsKeyPressed(KEY_DOWN)) { gSubMenuSelection = (gSubMenuSelection + 1) % (int)gParty.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }
            }

            if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                // Use first occurrence of selected grouped item
                std::vector<ItemGroup> groups = BuildInventoryGroups();
                int idxItem = -1;
                if (!groups.empty() && gItemGroupSelection >= 0 && gItemGroupSelection < (int)groups.size()) {
                    idxItem = groups[gItemGroupSelection].indices.front();
                }
                int idxTarget = gSubMenuSelection;
                if (idxTarget < 0) idxTarget = 0;
                if (idxTarget >= (int)gParty.size()) idxTarget = (int)gParty.size() - 1;
                // Apply selected item to selected target
                Combatant& target = gParty[idxTarget];
                const Item& chosen = (idxItem >= 0) ? gInventory[idxItem] : Item{};
                if (chosen.isRevive) {
                    if (!target.isAlive) {
                        target.isAlive = true;
                        target.isDown = false;
                        target.currentHp = chosen.value;
                        if (target.currentHp > target.maxHp) target.currentHp = target.maxHp;
                    }
                } else if (chosen.isSP) {
                    target.currentSp += chosen.value;
                    if (target.currentSp > target.maxSp) target.currentSp = target.maxSp;
                } else {
                    target.currentHp += chosen.value;
                    if (target.currentHp > target.maxHp) target.currentHp = target.maxHp;
                }
                if (gSndHeal.frameCount) PlaySound(gSndHeal);
                // Consume item
                if (idxItem >= 0) {
                    gInventory.erase(gInventory.begin() + idxItem);
                }
                // Return to item list
                gPauseState = P_ITEM_LIST;
                gItemGroupSelection = 0;
            }
        }

        // C. SKILL LIST (Healing Only)
        else if (gPauseState == P_SKILL_LIST) {
            Combatant& actor = gParty[gSelectedMemberIdx];
            std::vector<int> healIndices = GetHealingSkillIndices(actor);

            if (!healIndices.empty()) {
                if (IsKeyPressed(KEY_UP))   { gSubMenuSelection = (gSubMenuSelection - 1 + (int)healIndices.size()) % (int)healIndices.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }
                if (IsKeyPressed(KEY_DOWN)) { gSubMenuSelection = (gSubMenuSelection + 1) % (int)healIndices.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }

                // SELECT SKILL -> GO TO TARGETING
                if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                    gSelectedSkillIdx = healIndices[gSubMenuSelection]; // Store actual index
                    Ability& skill = actor.skills[gSelectedSkillIdx];

                    // Check Cost
                    bool canCast = skill.isMagic ? (actor.currentSp >= skill.cost) : (actor.currentHp > skill.cost);
                    
                    if (canCast) {
                        gPauseState = P_SKILL_TARGET_ALLY;
                        gSubMenuSelection = gSelectedMemberIdx; // Default target = Self
                        if (gSndMenu.frameCount) PlaySound(gSndMenu);
                    } else {
                        // PlaySound(gSndError); // Optional: Feedback for low SP
                    }
                }
            }
        }

        // D. SKILL TARGETING (Apply the Heal)
        else if (gPauseState == P_SKILL_TARGET_ALLY) {
            if (!gParty.empty()) {
                if (IsKeyPressed(KEY_UP))   { gSubMenuSelection = (gSubMenuSelection - 1 + (int)gParty.size()) % (int)gParty.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }
                if (IsKeyPressed(KEY_DOWN)) { gSubMenuSelection = (gSubMenuSelection + 1) % (int)gParty.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }
            }

            // CONFIRM HEAL
            if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                Combatant& actor = gParty[gSelectedMemberIdx];
                Combatant& target = gParty[gSubMenuSelection];
                Ability& skill = actor.skills[gSelectedSkillIdx];

                // Deduct Cost
                if (skill.isMagic) actor.currentSp -= skill.cost;
                else actor.currentHp -= skill.cost;

                // Apply Heal (Negative damage * -1)
                int amount = -skill.damage;
                target.currentHp += amount;
                if (target.currentHp > target.maxHp) target.currentHp = target.maxHp;

                if (gSndHeal.frameCount) PlaySound(gSndHeal); // Audio Feedback
                
                // Return to list or stay? Usually return to list.
                gPauseState = P_SKILL_LIST;
                gSubMenuSelection = 0; // Reset cursor or keep it
            }
        }

        // E. SYSTEM MENU
        else if (gPauseState == P_SYSTEM) {
            // Options: 0: Audio, 1: Quit
            if (IsKeyPressed(KEY_UP))   { gSubMenuSelection = (gSubMenuSelection - 1 + 2) % 2; if (gSndMenu.frameCount) PlaySound(gSndMenu); }
            if (IsKeyPressed(KEY_DOWN)) { gSubMenuSelection = (gSubMenuSelection + 1) % 2; if (gSndMenu.frameCount) PlaySound(gSndMenu); }

            if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                if (gSubMenuSelection == 0) {
                    if (gSndMenu.frameCount) PlaySound(gSndMenu);
                    gPauseState = P_AUDIO_SETTINGS;
                    gSubMenuSelection = 0; // Reset to Master Volume
                } else {
                    gAppStatus = TERMINATED;
                }
            }
        }

        // F. AUDIO SETTINGS
        else if (gPauseState == P_AUDIO_SETTINGS) {
            // 0: Master, 1: Music, 2: SFX
            if (IsKeyPressed(KEY_UP))   { gSubMenuSelection = (gSubMenuSelection - 1 + 3) % 3; if (gSndMenu.frameCount) PlaySound(gSndMenu); }
            if (IsKeyPressed(KEY_DOWN)) { gSubMenuSelection = (gSubMenuSelection + 1) % 3; if (gSndMenu.frameCount) PlaySound(gSndMenu); }

            // Adjust Volume
            float* targetVol = nullptr;
            if (gSubMenuSelection == 0) {
                targetVol = &gMasterVolume;
            } else if (gSubMenuSelection == 1) {
                targetVol = &gmusicvolume;
            } else if (gSubMenuSelection == 2) {
                targetVol = &gsfxvolume;
            }

            if (targetVol) {
                if (IsKeyDown(KEY_LEFT))  *targetVol -= 0.01f;
                if (IsKeyDown(KEY_RIGHT)) *targetVol += 0.01f;
                
                // Clamp
                if (*targetVol < 0.0f) *targetVol = 0.0f;
                if (*targetVol > 1.0f) *targetVol = 1.0f;

                // Apply Master immediately (others are used when playing sounds)
                // Apply Master immediately
                if (gSubMenuSelection == 0) {
                    SetMasterVolume(gMasterVolume);
                }
                // Mirror lowercase values into canonical globals for use elsewhere
                gMusicVolume = gmusicvolume;
                gSFXVolume   = gsfxvolume;
                ApplySFXVolumes();
            }
        }
        
        // G. PERSONA MENU (Switching)
        else if (gPauseState == P_PERSONA) {
            if (!gOwnedPersonas.empty()) {
                // Navigate List
                if (IsKeyPressed(KEY_UP))   gSubMenuSelection = (gSubMenuSelection - 1 + (int)gOwnedPersonas.size()) % (int)gOwnedPersonas.size();
                if (IsKeyPressed(KEY_DOWN)) gSubMenuSelection = (gSubMenuSelection + 1) % (int)gOwnedPersonas.size();

                // Equip Selection
                if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                    EquipPersona(gSubMenuSelection);
                    // PlaySound(gSndEquip);
                }
            }
        }

        // H. EQUIP VIEW (Select Slot)
        else if (gPauseState == P_EQUIP_VIEW) {
            // 0=Melee, 1=Gun, 2=Armor
            if (IsKeyPressed(KEY_UP))   { gSelectedEquipSlot = (gSelectedEquipSlot - 1 + 3) % 3; if (gSndMenu.frameCount) PlaySound(gSndMenu); }
            if (IsKeyPressed(KEY_DOWN)) { gSelectedEquipSlot = (gSelectedEquipSlot + 1) % 3; if (gSndMenu.frameCount) PlaySound(gSndMenu); }

            // Enter Selection -> Go to Bag List
            if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                if (gSndMenu.frameCount) PlaySound(gSndMenu);
                gPauseState = P_EQUIP_LIST;
                gSubMenuSelection = 0;
            }
        }

        // I. EQUIP LIST (Select Item & Swap)
        else if (gPauseState == P_EQUIP_LIST) {
            // Determine Type based on previous slot selection
            EquipmentType targetType = (gSelectedEquipSlot == 0) ? EQUIP_MELEE : 
                                       (gSelectedEquipSlot == 1) ? EQUIP_GUN : EQUIP_ARMOR;
            
            std::vector<int> validIndices = GetEquipIndicesByType(targetType);

            if (!validIndices.empty()) {
                if (IsKeyPressed(KEY_UP))   { gSubMenuSelection = (gSubMenuSelection - 1 + (int)validIndices.size()) % (int)validIndices.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }
                if (IsKeyPressed(KEY_DOWN)) { gSubMenuSelection = (gSubMenuSelection + 1) % (int)validIndices.size(); if (gSndMenu.frameCount) PlaySound(gSndMenu); }

                // SWAP ITEM
                if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_SPACE)) {
                    if (gSndMenu.frameCount) PlaySound(gSndMenu);
                    Combatant& c = gParty[gSelectedMemberIdx];
                    int realIdx = validIndices[gSubMenuSelection];
                    
                    // 1. Get references
                    Equipment newEquip = gOwnedEquipment[realIdx];
                    Equipment oldEquip;

                    // 2. Identify current equip to swap out
                    if (targetType == EQUIP_MELEE) oldEquip = c.meleeWeapon;
                    else if (targetType == EQUIP_GUN) oldEquip = c.gunWeapon;
                    else oldEquip = c.armor;

                    // 3. Perform Swap
                    // Remove new item from bag
                    gOwnedEquipment.erase(gOwnedEquipment.begin() + realIdx);
                    // Add old item to bag
                    gOwnedEquipment.push_back(oldEquip);
                    // Apply new item to combatant
                    if (targetType == EQUIP_MELEE) c.meleeWeapon = newEquip;
                    else if (targetType == EQUIP_GUN) c.gunWeapon = newEquip;
                    else c.armor = newEquip;

                    
                    // Return to Slot View
                    gPauseState = P_EQUIP_VIEW;
                }
            }
        }
        gPauseJustOpened = false; // Reset after first frame
    }

    // Combat input is handled within CombatScene itself
    // menu scene not implemented yet
}


void update() 
{
    // Keep music updating if desired; world updates only when not paused
    // Always update current scene music stream if valid
    if (gCurrentScene && gCurrentScene->getState().bgm.ctxData) {
        UpdateMusicStream(gCurrentScene->getState().bgm);
        SetMusicVolume(gCurrentScene->getState().bgm, gMusicVolume);
    }
    if (gGameStatus != PAUSED) {
        float ticks = (float) GetTime();
        float deltaTime = ticks - gPreviousTicks;
        gPreviousTicks  = ticks;

        // Handle global fade transition phases
        if (gTransitionPhase == T_FADE_OUT) {
            gTransitionAlpha += deltaTime * 2.0f; // ~0.5s fade
            if (gTransitionAlpha >= 1.0f) { gTransitionAlpha = 1.0f; gTransitionPhase = T_SWITCH; }
        }
        if (gTransitionPhase == T_SWITCH) {
            if (gPendingSceneID >= 0) {
                // Apply party stat boosts exactly once at the actual switch moment
                if (gCurrentLevelIndex == IDX_LEVEL_ONE && gPendingSceneID == IDX_LEVEL_TWO) {
                    const int hpBoost = 30;
                    const int spBoost = 15;
                    const int atkBoost = 10;
                    const int defBoost = 8;
                    for (auto &m : gParty) {
                        m.maxHp += hpBoost;
                        m.currentHp = std::min(m.currentHp + hpBoost, m.maxHp);
                        m.maxSp += spBoost;
                        m.currentSp = std::min(m.currentSp + spBoost, m.maxSp);
                        m.baseAttack += atkBoost;
                        m.baseDefense += defBoost;
                    }
                }

                gCurrentScene->shutdown();
                switchToScene(gPendingSceneID);
                // Set game status based on target
                gGameStatus = (gPendingSceneID == IDX_COMBAT) ? COMBAT : EXPLORATION;
                gPendingSceneID = -1;
            }
            gTransitionPhase = T_FADE_IN;
        }
        if (gTransitionPhase == T_FADE_IN) {
            gTransitionAlpha -= deltaTime * 2.0f;
            if (gTransitionAlpha <= 0.0f) { gTransitionAlpha = 0.0f; gTransitionPhase = T_NONE; }
        }

        // Update current scene only when not switching
        if (gTransitionPhase != T_SWITCH) {
            gCurrentScene->update(deltaTime);
        }

        // Tick down global item toast timer
        GameState& st = gCurrentScene->getState();
        if (st.itemToastTimer > 0.0f) {
            st.itemToastTimer -= deltaTime;
            if (st.itemToastTimer < 0.0f) st.itemToastTimer = 0.0f;
        }

        // Sync scene inventory back to global during exploration/title
        // Ensures chest loot reflects in pause menu and global inventory
        if (gGameStatus == EXPLORATION || gGameStatus == TITLE) {
            gInventory = st.inventory;
        }
    }
}

void render()
{
    BeginDrawing();
    
    ClearBackground(BLACK);

    // 1. EXPLORATION/PAUSED: Draw World + Party HUD with camera
    if (gGameStatus == EXPLORATION || gGameStatus == PAUSED)
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

        // --- NEW HUD RENDERING: Angled Bars per character ---
        int startY = 20;
        for (int i = 0; i < (int)gParty.size(); i++) {
            Combatant& m = gParty[i];

            // 1. Icon (scaled)
            DrawTextureEx(gPartyIcons[i], { 20.0f, (float)startY }, 0.0f, 0.45f, WHITE);

            // 2. Percentages
            float hpPercent = (m.maxHp > 0) ? ((float)m.currentHp / (float)m.maxHp) : 0.0f;
            float spPercent = (m.maxSp > 0) ? ((float)m.currentSp / (float)m.maxSp) : 0.0f;

            // Clamp for safety
            if (hpPercent < 0.0f) hpPercent = 0.0f; if (hpPercent > 1.0f) hpPercent = 1.0f;
            if (spPercent < 0.0f) spPercent = 0.0f; if (spPercent > 1.0f) spPercent = 1.0f;

            // 3. Dynamic HP Color
            Color hpColor = GREEN;
            if (hpPercent < 0.5f) hpColor = YELLOW;
            if (hpPercent < 0.2f) hpColor = RED;

            // 4. Bars (slanted) - slightly smaller and more spaced
            int baseX = 85;
            // Background HP bar
            DrawSlantedRect(baseX, startY + 8, 140, 12, 10, Fade(BLACK, 0.5f));
            // HP bar
            DrawSlantedRect(baseX, startY + 8, (int)(140 * hpPercent), 12, 10, hpColor);
            // SP bar (thinner, below HP)
            DrawSlantedRect(baseX + 5, startY + 26, (int)(110 * spPercent), 7, 10, SKYBLUE);

            // 5. Text - larger, HP black with max, SP smaller white
            DrawText(m.name.c_str(), baseX, startY - 12, 22, WHITE);
            DrawText(TextFormat("%d / %d", m.currentHp, m.maxHp), baseX + 5, startY + 6, 14, BLACK);
            DrawText(TextFormat("SP %d / %d", m.currentSp, m.maxSp), baseX + 8, startY + 22, 12, WHITE);

            startY += 92;
        }

        // --- DEBUG HUD: Player world position (top-right) ---
        if (gGameStatus == EXPLORATION && gCurrentScene->getState().player) {
            Vector2 p = gCurrentScene->getState().player->getPosition();
            const int fontSize = 18;
            char buf[128];
            snprintf(buf, sizeof(buf), "Pos: (%.0f, %.0f)", p.x, p.y);
            int tw = MeasureText(buf, fontSize);
            int x = SCREEN_WIDTH - tw - 12;
            int y = 8;
            DrawText(buf, x, y, fontSize, LIGHTGRAY);
        }
    }
    // 2. STATIC SCENES: Menu, Combat, Game Over
    else 
    {
        // These scenes handle their own screen-space positioning
        gCurrentScene->render();
    }

    // PAUSED Overlay and Menu (rendered on top of scene)
    if (gGameStatus == PAUSED) 
    {
        // 1. DARK OVERLAY
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.9f));

        // 2. MAIN MENU (Right-side alignment without shape)
        if (gPauseState == P_MAIN) {
            const char* options[] = { "SKILL", "ITEM", "EQUIP", "PERSONA", "SYSTEM" };
            for (int i = 0; i < 5; i++) {
                int x = 700 - (i * 20);
                int y = 150 + (i * 70);
                Color col = (i == gMenuSelection) ? WHITE : GRAY;
                if (i == gMenuSelection) {
                    DrawTriangle({(float)x-30, (float)y+20}, {(float)x-15, (float)y+5}, {(float)x-15, (float)y+35}, RED);
                }
                DrawText(options[i], x, y, 40, col);
            }
        }
        // 4b. ITEM LIST (Inventory)
        else if (gPauseState == P_ITEM_LIST) {
            DrawText("ITEMS", 50, 50, 40, WHITE);
            std::vector<ItemGroup> groups = BuildInventoryGroups();
            if (groups.empty()) {
                DrawText("No items.", 100, 200, 30, GRAY);
            } else {
                for (int i = 0; i < (int)groups.size(); i++) {
                    int y = 150 + (i * 60);
                    bool isSel = (i == gItemGroupSelection);
                    if (isSel) DrawRectangle(0, y - 8, 500, 40, WHITE);
                    DrawText(TextFormat("%s x%d", groups[i].item.name.c_str(), groups[i].count), 100, y, 30, isSel ? BLACK : GRAY);
                    DrawText(groups[i].item.description.c_str(), 350, y+5, 18, isSel ? BLACK : DARKGRAY);
                }
            }
            DrawText("[Z] Use  [ESC] Back", 50, 550, 20, GRAY);
        }

        // 5b. ITEM TARGETING (Apply)
        else if (gPauseState == P_ITEM_TARGET_ALLY) {
            DrawText("SELECT TARGET", 50, 50, 30, YELLOW);
            for (int i = 0; i < (int)gParty.size(); i++) {
                int x = 100 + (i * 40);
                int y = 150 + (i * 80);
                bool isTarget = (i == gSubMenuSelection);
                Color bg = isTarget ? Fade(YELLOW, 0.2f) : BLACK;
                DrawRectangle(x, y, 350, 70, bg);
                DrawRectangleLines(x, y, 350, 70, isTarget ? YELLOW : DARKGRAY);
                DrawText(gParty[i].name.c_str(), x + 20, y + 20, 30, WHITE);
                DrawText(TextFormat("HP %d/%d", gParty[i].currentHp, gParty[i].maxHp), x + 160, y + 25, 20, GREEN);
                DrawText(TextFormat("SP %d/%d", gParty[i].currentSp, gParty[i].maxSp), x + 160, y + 45, 18, SKYBLUE);
                if (isTarget) DrawText("USE", x + 300, y + 25, 20, YELLOW);
            }
        }

        // 3. PARTY SELECTION (Used for SKILL & EQUIP entry)
        else if (gPauseState == P_PARTY_SELECT) {
            DrawText("SELECT PARTY MEMBER", 50, 50, 30, WHITE);
            for (int i = 0; i < (int)gParty.size(); i++) {
                int x = 100 + (i * 40);
                int y = 150 + (i * 80);
                bool isSel = (i == gSubMenuSelection);
                DrawRectangle(x, y, 350, 70, isSel ? DARKGRAY : BLACK);
                DrawRectangleLines(x, y, 350, 70, isSel ? RED : DARKGRAY);
                DrawText(gParty[i].name.c_str(), x + 20, y + 20, 30, WHITE);
                DrawText(TextFormat("HP %d/%d", gParty[i].currentHp, gParty[i].maxHp), x + 160, y + 15, 20, GREEN);
                DrawText(TextFormat("SP %d/%d", gParty[i].currentSp, gParty[i].maxSp), x + 160, y + 40, 20, SKYBLUE);
            }
        }

        // 4. SKILL LIST (Healing Only)
        else if (gPauseState == P_SKILL_LIST) {
            Combatant& c = gParty[gSelectedMemberIdx];
            std::vector<int> healIndices = GetHealingSkillIndices(c);

            DrawText(TextFormat("HEALING: %s", c.name.c_str()), 50, 50, 40, WHITE);
            DrawText(TextFormat("SP: %d", c.currentSp), 500, 60, 30, SKYBLUE);

            if (healIndices.empty()) {
                DrawText("No Healing Skills.", 100, 200, 30, GRAY);
            } else {
                for (int i = 0; i < (int)healIndices.size(); i++) {
                    int realIdx = healIndices[i];
                    Ability& s = c.skills[realIdx];
                    int y = 150 + (i * 60);
                    bool isSel = (i == gSubMenuSelection);

                    if (isSel) {
                        // Selection strip background
                        DrawRectangle(0, y - 5, 500, 40, WHITE);
                        DrawText(s.name.c_str(), 100, y, 30, BLACK);
                        DrawText(TextFormat("%d SP", s.cost), 300, y+5, 20, BLACK);
                        DrawText(TextFormat("Heals %d", -s.damage), 400, y+5, 20, BLACK);
                    } else {
                        DrawText(s.name.c_str(), 100, y, 30, GRAY);
                        DrawText(TextFormat("%d SP", s.cost), 300, y+5, 20, SKYBLUE);
                        DrawText(TextFormat("Heals %d", -s.damage), 400, y+5, 20, GREEN);
                    }
                }
            }
            DrawText("[Z] Use  [ESC] Back", 50, 550, 20, GRAY);
        }

        // 5. SKILL TARGETING (Overlay on top of Party List)
        else if (gPauseState == P_SKILL_TARGET_ALLY) {
            DrawText("SELECT TARGET", 50, 50, 30, GREEN);
            for (int i = 0; i < (int)gParty.size(); i++) {
                int x = 100 + (i * 40);
                int y = 150 + (i * 80);
                bool isTarget = (i == gSubMenuSelection);
                Color bg = isTarget ? Fade(GREEN, 0.2f) : BLACK;
                DrawRectangle(x, y, 350, 70, bg);
                DrawRectangleLines(x, y, 350, 70, isTarget ? GREEN : DARKGRAY);
                DrawText(gParty[i].name.c_str(), x + 20, y + 20, 30, WHITE);
                DrawText(TextFormat("HP %d/%d", gParty[i].currentHp, gParty[i].maxHp), x + 160, y + 25, 20, GREEN);
                if (isTarget) DrawText("HEAL", x + 280, y + 25, 20, GREEN);
            }
        }

        // 6. SYSTEM MENU
        else if (gPauseState == P_SYSTEM) {
            DrawText("SYSTEM", 50, 50, 40, WHITE);
            const char* sysOps[] = { "Audio Settings", "Quit to Title" };
            for (int i = 0; i < 2; i++) {
                int y = 200 + (i*60);
                bool isSel = (i == gSubMenuSelection);
                DrawText(sysOps[i], 100, y, 30, isSel ? WHITE : GRAY);
                if (isSel) DrawText(">", 70, y, 30, RED);
            }
        }

        // 7. AUDIO SETTINGS
        else if (gPauseState == P_AUDIO_SETTINGS) {
            DrawText("AUDIO SETTINGS", 50, 50, 40, WHITE);
            DrawText("[UP/DOWN] Select   [LEFT/RIGHT] Adjust", 50, 100, 20, GRAY);

            const char* labels[] = { "Master Volume", "Music Volume", "SFX Volume" };
            float values[] = { gMasterVolume, gMusicVolume, gSFXVolume };

            for (int i = 0; i < 3; i++) {
                int y = 200 + (i * 80);
                bool isSel = (i == gSubMenuSelection);
                Color c = isSel ? WHITE : GRAY;
                DrawText(labels[i], 100, y, 25, c);
                if (isSel) DrawText(">", 70, y, 25, RED);
                DrawRectangleLines(300, y + 5, 200, 20, c);
                DrawRectangle(302, y + 7, (int)(values[i] * 196), 16, isSel ? RED : DARKGRAY);
                DrawText(TextFormat("%d%%", (int)(values[i]*100)), 520, y+5, 20, c);
            }
            DrawText("[ESC] Back", 50, 550, 20, GRAY);
        }

        // 8. PERSONA MENU
        else if (gPauseState == P_PERSONA) {
            DrawText("JOKER'S PERSONAS", 50, 50, 40, WHITE);
            DrawText("Active stats applied to Joker.", 50, 90, 20, GRAY);

            // Left Column: List of Personas
            for (int i = 0; i < (int)gOwnedPersonas.size(); i++) {
                int y = 150 + (i * 60);
                bool isSel = (i == gSubMenuSelection);
                bool isEquipped = (i == gEquippedPersonaIdx);

                // Highlight Selection
                if (isSel) DrawText(">", 30, y, 30, RED);

                // Color Logic: White if selected, Gray if not. Yellow if Equipped.
                Color c = isSel ? WHITE : GRAY;
                if (isEquipped) c = YELLOW;

                DrawText(gOwnedPersonas[i].name.c_str(), 60, y, 30, c);
                if (isEquipped) DrawText("[E]", 250, y, 25, YELLOW);
            }

            if (!gOwnedPersonas.empty()) {
                // Right Column: Stats Preview of SELECTED Persona
                Persona& p = gOwnedPersonas[gSubMenuSelection];
                int statX = 400;
                int statY = 150;

                DrawText("STATS:", statX, statY, 25, RED);
                DrawText(TextFormat("Atk: %d", p.baseAttack), statX, statY + 40, 25, WHITE);
                DrawText(TextFormat("Def: %d", p.baseDefense), statX, statY + 80, 25, WHITE);
                // Speed stat removed

                DrawText("SKILLS:", statX, statY + 180, 25, RED);
                for(int k=0; k<(int)p.skills.size(); k++) {
                    DrawText(p.skills[k].name.c_str(), statX, statY + 220 + (k*30), 20, LIGHTGRAY);
                }

                DrawText("WEAK:", statX, statY + 300, 25, RED);
                for(int k=0; k<(int)p.weaknesses.size(); k++) {
                    std::string elemName = "???";
                    if (p.weaknesses[k] == FIRE) elemName = "Fire";
                    else if (p.weaknesses[k] == ICE) elemName = "Ice";
                    else if (p.weaknesses[k] == ELEC) elemName = "Elec";
                    else if (p.weaknesses[k] == WIND) elemName = "Wind";
                    else if (p.weaknesses[k] == PHYS) elemName = "Phys";
                    else if (p.weaknesses[k] == GUN)  elemName = "Gun";
                    else if (p.weaknesses[k] == BLESS) elemName = "Bless";
                    else if (p.weaknesses[k] == CURSE) elemName = "Curse";
                    else if (p.weaknesses[k] == NUKE) elemName = "Nuke";
                    else if (p.weaknesses[k] == PSI) elemName = "Psi";
                    DrawText(elemName.c_str(), statX + (k*80), statY + 340, 20, SKYBLUE);
                }
            }
            DrawText("[Z] Equip  [ESC] Back", 50, 550, 20, GRAY);
        }

        // 9. EQUIP VIEW (Select Slot)
        else if (gPauseState == P_EQUIP_VIEW) {
            Combatant& c = gParty[gSelectedMemberIdx];
            DrawText(TextFormat("EQUIP: %s", c.name.c_str()), 50, 50, 40, WHITE);
            
            const char* slotNames[] = { "MELEE", "GUN", "ARMOR" };
            Equipment* currentGear[] = { &c.meleeWeapon, &c.gunWeapon, &c.armor };

            for (int i = 0; i < 3; i++) {
                int y = 150 + (i * 100);
                bool isSel = (i == gSelectedEquipSlot);

                // Draw Slot Header
                Color headerCol = isSel ? RED : DARKGRAY;
                DrawText(slotNames[i], 100, y, 25, headerCol);
                if(isSel) DrawText(">", 70, y, 25, RED);

                // Draw Item Name & Desc
                DrawText(currentGear[i]->name.c_str(), 200, y, 30, WHITE);
                DrawText(currentGear[i]->description.c_str(), 200, y + 35, 20, GRAY);

                // Draw Stats Summary
                if (i == 0) DrawText(TextFormat("Atk: %d", currentGear[i]->attackPower), 600, y, 25, YELLOW);
                if (i == 1) DrawText(TextFormat("Atk: %d  Mag: %d", currentGear[i]->attackPower, currentGear[i]->magazineSize), 600, y, 25, YELLOW);
                if (i == 2) DrawText(TextFormat("Def: %d", currentGear[i]->defensePower), 600, y, 25, SKYBLUE);
            }
            DrawText("[Z] Change  [ESC] Back", 50, 550, 20, GRAY);
        }

        // 10. EQUIP LIST (Comparisons)
        else if (gPauseState == P_EQUIP_LIST) {
            Combatant& c = gParty[gSelectedMemberIdx];
            EquipmentType targetType = (gSelectedEquipSlot == 0) ? EQUIP_MELEE : 
                                       (gSelectedEquipSlot == 1) ? EQUIP_GUN : EQUIP_ARMOR;
            
            Equipment* current = (targetType == EQUIP_MELEE) ? &c.meleeWeapon : 
                                 (targetType == EQUIP_GUN) ? &c.gunWeapon : &c.armor;

            std::vector<int> validIndices = GetEquipIndicesByType(targetType);

            DrawText("SELECT ITEM", 50, 50, 40, WHITE);

            if (validIndices.empty()) {
                DrawText("No equipment of this type.", 100, 200, 30, GRAY);
            } else {
                for (int i = 0; i < (int)validIndices.size(); i++) {
                    int realIdx = validIndices[i];
                    Equipment& item = gOwnedEquipment[realIdx];
                    
                    int y = 150 + (i * 80);
                    bool isSel = (i == gSubMenuSelection);
                    
                    if (isSel) {
                        // Selection strip background to highlight
                        DrawRectangle(0, y - 8, 700, 50, WHITE);
                    }
                    
                    // Name
                    DrawText(item.name.c_str(), 60, y, 30, isSel ? BLACK : GRAY);
                    
                    // --- STAT COMPARISON LOGIC ---
                    int statX = 400;
                    
                    // Compare Attack (Melee/Gun)
                    if (targetType != EQUIP_ARMOR) {
                        int diff = item.attackPower - current->attackPower;
                        Color statColor = LIGHTGRAY;
                        const char* arrow = "";
                        
                        if (diff > 0) { statColor = GREEN; arrow = "^"; }
                        if (diff < 0) { statColor = RED;   arrow = "v"; }

                        DrawText(TextFormat("Atk: %d %s", item.attackPower, arrow), statX, y, 25, statColor);
                        statX += 150;
                    }

                    // Compare Magazine (Gun Only)
                    if (targetType == EQUIP_GUN) {
                        int diff = item.magazineSize - current->magazineSize;
                        Color statColor = LIGHTGRAY;
                        const char* arrow = "";

                        if (diff > 0) { statColor = GREEN; arrow = "^"; }
                        if (diff < 0) { statColor = RED;   arrow = "v"; }

                        DrawText(TextFormat("Mag: %d %s", item.magazineSize, arrow), statX, y, 25, statColor);
                    }

                    // Compare Defense (Armor Only)
                    if (targetType == EQUIP_ARMOR) {
                        int diff = item.defensePower - current->defensePower;
                        Color statColor = LIGHTGRAY;
                        const char* arrow = "";

                        if (diff > 0) { statColor = GREEN; arrow = "^"; }
                        if (diff < 0) { statColor = RED;   arrow = "v"; }

                        DrawText(TextFormat("Def: %d %s", item.defensePower, arrow), statX, y, 25, statColor);
                    }
                }
            }
        }
    }

    // GLOBAL FADE OVERLAY (applies during transitions between levels/combat)
    if (gTransitionPhase != T_NONE) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, gTransitionAlpha));
    }

    // GLOBAL TOAST: Item acquisition (bottom-right, 2s)
    const GameState& st = gCurrentScene->getState();
    if (st.itemToastTimer > 0.0f && !st.itemToast.empty()) {
        int fontSize = 20;
        int tw = MeasureText(st.itemToast.c_str(), fontSize);
        int x = SCREEN_WIDTH - tw - 20;
        int y = SCREEN_HEIGHT - 30;
        DrawText(st.itemToast.c_str(), x, y, fontSize, WHITE);
    }

    EndDrawing();
}

void shutdown() 
{
    gShader.unload();
    // Unload HUD icons
    for (int i = 0; i < 4; ++i) {
        if (gPartyIcons[i].id != 0) {
            UnloadTexture(gPartyIcons[i]);
            gPartyIcons[i] = Texture2D{};
        }
    }
    // Unload SFX before closing audio
    if (gSndBack.frameCount) UnloadSound(gSndBack);
    if (gSndCrit.frameCount) UnloadSound(gSndCrit);
    if (gSndGun.frameCount)  UnloadSound(gSndGun);
    if (gSndHeal.frameCount) UnloadSound(gSndHeal);
    if (gSndHit.frameCount)  UnloadSound(gSndHit);
    if (gSndMenu.frameCount) UnloadSound(gSndMenu);
    CloseAudioDevice();
    CloseWindow();
}

int main()
{
    initialise();

    // Scene Setup
    gLevels.push_back(new LevelOne({SCREEN_WIDTH/2, SCREEN_HEIGHT/2}, "#000000"));
    gLevels.push_back(new LevelTwo({SCREEN_WIDTH/2, SCREEN_HEIGHT/2}, "#000000"));
    gLevels.push_back(new CombatScene({SCREEN_WIDTH/2, SCREEN_HEIGHT/2}, "#000000"));
    gLevels.push_back(new StartMenu({SCREEN_WIDTH/2, SCREEN_HEIGHT/2}, "#000000"));
    gLevels.push_back(new LevelThree({SCREEN_WIDTH/2, SCREEN_HEIGHT/2}, "#000000"));
    
    gSceneEnemyDefeated.resize(gLevels.size());

    switchToScene(IDX_START_MENU);
    gGameStatus = TITLE;

    while (gAppStatus != TERMINATED) {
        processInput();
        update();

        // Scene Switching Logic (Keep existing logic)
           if (gCurrentScene->getState().nextSceneID != -1) {
               int nextID = gCurrentScene->getState().nextSceneID;
               // Reset the request immediately to avoid reprocessing on subsequent frames
               gCurrentScene->getState().nextSceneID = -1;
             
             // Pass Party State to Combat
            if (nextID == IDX_COMBAT) { 
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
            if (gCurrentLevelIndex == IDX_COMBAT && nextID != IDX_COMBAT) {
                gParty = gCurrentScene->getState().party;
                gInventory = gCurrentScene->getState().inventory;
                gLevels[nextID]->getState().returnSpawnPos = gCurrentScene->getState().returnSpawnPos;
                gLevels[nextID]->getState().hasReturnSpawnPos = gCurrentScene->getState().hasReturnSpawnPos;
                // Copy defeated enemy flags back to target level scene
                gLevels[nextID]->getState().defeatedEnemies = gCurrentScene->getState().defeatedEnemies;
            }

            // Use global fade transition for level/combat switches (no camera tween)
            if (nextID == IDX_START_MENU) {
                // Immediate switch for title
                gCurrentScene->shutdown();
                switchToScene(nextID);
                gGameStatus = TITLE;
            } else {
                gPendingSceneID = nextID;
                gTransitionPhase = T_FADE_OUT;
            }
        }

        render();
    }

    // Ensure SFX are unloaded before closing (if not already)
    if (gSndBack.frameCount) UnloadSound(gSndBack);
    if (gSndCrit.frameCount) UnloadSound(gSndCrit);
    if (gSndGun.frameCount)  UnloadSound(gSndGun);
    if (gSndHeal.frameCount) UnloadSound(gSndHeal);
    if (gSndHit.frameCount)  UnloadSound(gSndHit);
    if (gSndMenu.frameCount) UnloadSound(gSndMenu);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}