#include "Map.h"

Map::Map(int mapColumns, int mapRows, unsigned int *levelData,
         const char *textureFilePath, float tileSize, int textureColumns,
         int textureRows, Vector2 origin) : 
         mMapColumns {mapColumns}, mMapRows {mapRows}, 
         mTextureAtlas { LoadTexture(textureFilePath) },
         mLevelData {levelData }, mTileSize {tileSize}, 
         mTextureColumns {textureColumns}, mTextureRows {textureRows},
         mOrigin {origin} {
    // Initialize exploration state for all tiles to false
    mTileExplored.resize(mMapColumns * mMapRows, false);
    build();
}

Map::~Map() { UnloadTexture(mTextureAtlas); }

void Map::build()
{
    // Calculate map boundaries in world coordinates
    mLeftBoundary   = mOrigin.x - (mMapColumns * mTileSize) / 2.0f;
    mRightBoundary  = mOrigin.x + (mMapColumns * mTileSize) / 2.0f;
    mTopBoundary    = mOrigin.y - (mMapRows * mTileSize) / 2.0f;
    mBottomBoundary = mOrigin.y + (mMapRows * mTileSize) / 2.0f;

    // Precompute texture areas for each tile
    for (int row = 0; row < mTextureRows; row++)
    {
        for (int col = 0; col < mTextureColumns; col++)
        {
            Rectangle textureArea = {
                (float) col * (mTextureAtlas.width / mTextureColumns),
                (float) row * (mTextureAtlas.height / mTextureRows),
                (float) mTextureAtlas.width / mTextureColumns,
                (float) mTextureAtlas.height / mTextureRows
            };

            mTextureAreas.push_back(textureArea);
        }
    }
}

int Map::getTileIndex(int x, int y)
{
    return y * mMapColumns + x;
}

Vector2 Map::worldToTile(Vector2 pos)
{
    // Convert world position to tile indices via tile size
    return { (float)((int)(pos.x / mTileSize)), (float)((int)(pos.y / mTileSize)) };
}

void Map::revealTiles(Vector2 playerPos, float radius)
{
    // Convert player world position to tile coordinates
    Vector2 centerTile = worldToTile({ playerPos.x - mLeftBoundary, playerPos.y - mTopBoundary });
    int centerX = (int)centerTile.x;
    int centerY = (int)centerTile.y;

    // Convert radius in world units to tile units
    float tileRadiusF = radius / mTileSize;
    int   tileRadius  = (int)ceilf(tileRadiusF);

    // Bounds for square iteration
    int startX = std::max(0, centerX - tileRadius);
    int endX   = std::min(mMapColumns - 1, centerX + tileRadius);
    int startY = std::max(0, centerY - tileRadius);
    int endY   = std::min(mMapRows - 1, centerY + tileRadius);

    for (int y = startY; y <= endY; ++y)
    {
        for (int x = startX; x <= endX; ++x)
        {
            float dx = (float)(x - centerX);
            float dy = (float)(y - centerY);
            float dist = sqrtf(dx*dx + dy*dy);
            if (dist <= tileRadiusF)
            {
                int idx = getTileIndex(x, y);
                if (idx >= 0 && idx < (int)mTileExplored.size())
                    mTileExplored[idx] = true;
            }
        }
    }
}

void Map::render()
{
    // Draw each tile in the map
    for (int row = 0; row < mMapRows; row++)
    {
        // Draw each column in the row
        for (int col = 0; col < mMapColumns; col++)
        {
            // Get the tile index at the current row and column
            int tile = mLevelData[row * mMapColumns + col];

            // If the tile index is 0, we do not draw anything
            if (tile == 0) continue;

            Rectangle destinationArea = {
                mLeftBoundary + col * mTileSize,
                mTopBoundary  + row * mTileSize, // y-axis is inverted
                mTileSize,
                mTileSize
            };

            // Draw the tile
            DrawTexturePro(
                mTextureAtlas,
                mTextureAreas[tile - 1], // -1 because tile indices start at 1
                destinationArea,
                {0.0f, 0.0f}, // origin
                0.0f,         // rotation
                WHITE         // tint
            );

            // Fog overlay pass: draw dark mask over unexplored tiles
            int idx = getTileIndex(col, row);
            if (idx >= 0 && idx < (int)mTileExplored.size() && !mTileExplored[idx])
            {
                DrawRectangleRec(destinationArea, Fade(BLACK, 0.75f));
            }
        }
    }
}

bool Map::isSolidTileAt(Vector2 position, float *xOverlap, float *yOverlap)
{
    *xOverlap = 0.0f;
    *yOverlap = 0.0f;

    if (position.x < mLeftBoundary || position.x > mRightBoundary ||
        position.y < mTopBoundary  || position.y > mBottomBoundary)
        return false;

    int tileXIndex = floor((position.x - mLeftBoundary) / mTileSize);
    int tileYIndex = floor((position.y - mTopBoundary) / mTileSize);

    if (tileXIndex < 0 || tileXIndex >= mMapColumns ||
        tileYIndex < 0 || tileYIndex >= mMapRows)
        return false;

    int tile = mLevelData[tileYIndex * mMapColumns + tileXIndex];
    if (tile == 0) return false;

    // Only tile index 1 is considered a solid wall in this tileset.
    // Tile index 2 (floor) should be walkable for top-down movement.
    if (tile != 1) return false;

    float tileCentreX = mLeftBoundary + tileXIndex * mTileSize + mTileSize / 2.0f;
    float tileCentreY = mTopBoundary + tileYIndex * mTileSize + mTileSize / 2.0f;

    // Compute overlaps only for solid tiles
    *xOverlap = fmaxf(0.0f, (mTileSize / 2.0f) - fabs(position.x - tileCentreX));
    *yOverlap = fmaxf(0.0f, (mTileSize / 2.0f) - fabs(position.y - tileCentreY));

    return true;
}

bool Map::hasLineOfSight(Vector2 start, Vector2 end)
{
    // Calculate direction and distance
    Vector2 diff = Vector2Subtract(end, start);
    float dist = Vector2Length(diff);
    
    // If points are essentially the same, LOS is clear
    if (dist < 1.0f) return true;

    Vector2 dir = Vector2Normalize(diff);

    float stepSize = mTileSize / 3.0f; 
    
    // Iterate from Start to End
    for (float d = 0; d < dist; d += stepSize)
    {
        Vector2 checkPos = Vector2Add(start, Vector2Scale(dir, d));

        // Convert World Pos -> Map Grid Coordinates
        int tx = floor((checkPos.x - mLeftBoundary) / mTileSize);
        int ty = floor((checkPos.y - mTopBoundary) / mTileSize);

        // Check Bounds
        if (tx >= 0 && tx < mMapColumns && ty >= 0 && ty < mMapRows)
        {
            // Check Collision
            int tileID = mLevelData[ty * mMapColumns + tx];
            if (tileID == 1) // 1 = Wall
            {
                return false; // LOS Blocked
            }
        }
    }

    return true; // No walls hit
}