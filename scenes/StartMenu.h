#ifndef START_MENU_H
#define START_MENU_H

#include "../lib/Scene.h"
#include <vector>

class StartMenu : public Scene
{
public:
    StartMenu(Vector2 origin, const char* hexColor) : Scene(origin, hexColor) {}

    void initialise() override;
    void update(float deltaTime) override;
    void render() override;
    void shutdown() override {};

private:
    enum MenuPhase { PRESS_START, LEVEL_SELECT };
    MenuPhase mPhase = PRESS_START;
    int mSelection = 0;
    float mBlinkTimer = 0.0f;
    bool mShowPrompt = true;
    std::vector<const char*> mOptions { "Level One", "Level Two" };
};

#endif // START_MENU_H
