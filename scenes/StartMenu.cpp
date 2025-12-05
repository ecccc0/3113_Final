#include "StartMenu.h"
#include "raylib.h"
#include <string>

// Access global game status declared in main.cpp
extern GameStatus gGameStatus;

void StartMenu::initialise()
{
    mGameState.nextSceneID = -1;
    mPhase = PRESS_START;
    mSelection = 0;
    mBlinkTimer = 0.0f;
    mShowPrompt = true;
}

void StartMenu::update(float deltaTime)
{
    // Blink the prompt while on the press-start phase
    mBlinkTimer += deltaTime;
    if (mBlinkTimer > 0.5f) {
        mBlinkTimer = 0.0f;
        mShowPrompt = !mShowPrompt;
    }

    if (mPhase == PRESS_START) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            mPhase = LEVEL_SELECT;
            gGameStatus = TITLE; // keep input handling out of exploration branch
        }
        return;
    }

    // Level selection
    if (IsKeyPressed(KEY_UP)) {
        mSelection = (mSelection - 1 + (int)mOptions.size()) % (int)mOptions.size();
    } else if (IsKeyPressed(KEY_DOWN)) {
        mSelection = (mSelection + 1) % (int)mOptions.size();
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        // Map selection to scene indices: 0 = Level One, 1 = Level Two, 4 = Level Three
        if (mSelection == 0) mGameState.nextSceneID = 0;
        else if (mSelection == 1) mGameState.nextSceneID = 1;
        else mGameState.nextSceneID = 4;
    }
}

void StartMenu::render()
{
    ClearBackground(BLACK);

    const int titleFontSize = 40;
    const char* titleText = "Persona 5 Demake";
    int titleWidth = MeasureText(titleText, titleFontSize);
    DrawText(titleText, (int)(mOrigin.x - titleWidth / 2), 120, titleFontSize, RED);

    if (mPhase == PRESS_START) {
        if (mShowPrompt) {
            const char* prompt = "Press ENTER to Start";
            int promptWidth = MeasureText(prompt, 24);
            DrawText(prompt, (int)(mOrigin.x - promptWidth / 2), 250, 24, WHITE);
        }
        return;
    }

    const char* selectLabel = "Select Starting Level";
    int labelWidth = MeasureText(selectLabel, 24);
    DrawText(selectLabel, (int)(mOrigin.x - labelWidth / 2), 220, 24, LIGHTGRAY);

    for (size_t i = 0; i < mOptions.size(); ++i) {
        Color c = (i == (size_t)mSelection) ? YELLOW : RAYWHITE;
        int textWidth = MeasureText(mOptions[i], 22);
        int y = 270 + (int)i * 36;
        DrawText(mOptions[i], (int)(mOrigin.x - textWidth / 2), y, 22, c);
        if (i == (size_t)mSelection) {
            DrawText(">", (int)(mOrigin.x - textWidth / 2) - 24, y, 22, c);
        }
    }
}
