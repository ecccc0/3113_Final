#include "../lib/Scene.h"

#ifndef LEVEL_ONE_H
#define LEVEL_ONE_H

class LevelOne : public Scene
{
public:
    // We pass origin and hex color to the base Scene constructor
    LevelOne(Vector2 origin, const char* hex) : Scene(origin, hex) {}

    void initialise() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override {}; 
private:
    // Level-specific enemy defeat flags cached locally for convenience
    // (Mirror of global gSceneEnemyDefeated[sceneIndex])
    std::vector<bool> mEnemyDefeated;
    // Party follower entities (Skull, Mona, Noir)
    std::vector<Entity*> mFollowers;
};

#endif