#ifndef COMBAT_SCENE_H
#define COMBAT_SCENE_H

#include "../lib/Scene.h"
#include "../lib/GameTypes.h"
#include "../lib/Entity.h"
#include <vector>
#include <string>


enum CombatState {
    PLAYER_TURN_MAIN,
    PLAYER_TURN_SKILLS,
    PLAYER_TURN_TARGET,
    PLAYER_TURN_TARGET_ALLY,
    PLAYER_TURN_ITEM,
    ENEMY_TURN,
    ANIMATION_WAIT,
    HOLD_UP,
    VICTORY
};

// Forward declare
class Effects;

class CombatScene : public Scene
{
public:
    CombatScene(Vector2 origin, const char* hexColor) : Scene(origin, hexColor) {}
    void initialise() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override;

private:
    void NextTurn();
    void CheckWeakness(Combatant& attacker, Combatant& defender, Ability skill);
    void CheckHoldUp();

    // UI State
    CombatState mState;
    std::string mLog;
    float mTimer;

    int mActiveMemberIndex = 0;
    int mActiveEnemyIndex = 0;
    
    // Selection Indices
    int mSelectedSkillIndex = 0;
    int mSelectedTargetIndex = 0;
    int mSelectedActionIndex = 0; // 0=Attack, 1=Gun, 2=Skill, 3=Guard, 4=Item

    // UI Assets
    Texture2D mIconAttack;
    Texture2D mIconGun;
    Texture2D mIconSkill;
    Texture2D mIconGuard;
    Texture2D mIconItem;
    Texture2D mUiCursor;

    // Animation
    float mWheelRotation = 0.0f; // Current rotation angle

    // --- TRANSITION EFFECTS ---
    Effects* mEffects = nullptr;
    float mTargetZoom = 1.0f; // Normal Zoom

    // --- PARTY VISUAL SPRITES ---
    std::vector<Entity*> mPartySprites;
};

#endif