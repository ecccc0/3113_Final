#include "../lib/Scene.h"
#include "../lib/GameTypes.h"

class CombatScene : public Scene
{
private:
    enum BattleState {
        PLAYER_TURN_MAIN,    // Choosing Action (Attack, Skill, Guard)
        PLAYER_TURN_SKILLS,  // Choosing specific skill
        PLAYER_TURN_TARGET,  // Choosing target
        PLAYER_TURN_TARGET_ALLY, // For  ally-targeting skills
        ANIMATION_WAIT,      
        ENEMY_TURN,
        HOLD_UP              // All enemies down -> All-Out Attack prompt
    };

    BattleState mState;
    int mActiveMemberIndex; 
    int mSelectedSkillIndex; // Which skill is highlighted?
    int mSelectedTargetIndex;// Which enemy is highlighted?
    int mActiveEnemyIndex; // For enemy turn processing
    
    std::string mLog;        // Battle text
    float mTimer;            // For delays

    // Helper Functions
    void CheckWeakness(Combatant& attacker, Combatant& defender, Ability skill);
    void CheckHoldUp();      // Checks if all enemies are Down
    void NextTurn();         // Cycles to next character

public:
    CombatScene(Vector2 origin, const char* hex) : Scene(origin, hex) {}
    void initialise() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override {};
};