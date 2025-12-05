
#include "Scene.h"

Scene::Scene() : mOrigin{{}} {
    // Ensure audio handles start in a safe null state
    mGameState.bgm.ctxData = nullptr;
}

Scene::Scene(Vector2 origin, const char *bgHexCode) : mOrigin{origin}, mBGColourHexCode {bgHexCode} 
{
    // Ensure audio handles start in a safe null state
    mGameState.bgm.ctxData = nullptr;
    ClearBackground(ColorFromHex(bgHexCode));
}
