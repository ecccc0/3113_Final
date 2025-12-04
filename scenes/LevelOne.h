#include "../lib/Scene.h"
#include "../lib/Map.h"

// Forward declare Effects to avoid circular dependency
class Effects;

#ifndef LEVEL_ONE_H
#define LEVEL_ONE_H

class LevelOne : public Scene
{
public:
    // We pass origin and hex color to the base Scene constructor
    LevelOne(Vector2 origin, const char* hexColor) : Scene(origin, hexColor) {}

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

    // --- TRANSITION EFFECTS ---
    Effects* mEffects = nullptr;
    bool mIsTransitioning = false;
    float mTargetZoom = 3.0f; // How far to zoom in before switching
};

#endif