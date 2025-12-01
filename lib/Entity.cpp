#include "Entity.h"
#include "raymath.h" // Needed for Vector2Normalize, Vector2Length, Vector2Distance
#include <cfloat> // For FLT_MAX
#include <cmath> // For cosf

Entity::Entity() : mPosition {0.0f, 0.0f}, mMovement {0.0f, 0.0f}, 
                   mVelocity {0.0f, 0.0f}, mAcceleration {0.0f, 0.0f},
                   mScale {DEFAULT_SIZE, DEFAULT_SIZE},
                   mColliderDimensions {DEFAULT_SIZE, DEFAULT_SIZE}, 
                   mTexture {}, mTextureType {SINGLE}, mAngle {0.0f},
                   mSpriteSheetDimensions {}, mDirection {RIGHT}, 
                   mAnimationAtlas {{}}, mAnimationIndices {}, mFrameSpeed {0},
                   mSpeed { DEFAULT_SPEED },
                   mEntityType {NONE} { }

Entity::Entity(Vector2 position, Vector2 scale, const char *textureFilepath, 
    EntityType entityType) : mPosition {position}, mVelocity {0.0f, 0.0f}, 
    mAcceleration {0.0f, 0.0f}, mScale {scale}, mMovement {0.0f, 0.0f}, 
    mColliderDimensions {scale}, mTexture {LoadTexture(textureFilepath)}, 
    mTextureType {SINGLE}, mDirection {RIGHT}, mAnimationAtlas {{}}, 
    mAnimationIndices {}, mFrameSpeed {0}, mSpeed {DEFAULT_SPEED}, 
    mAngle {0.0f}, mEntityType {entityType} { }

Entity::Entity(Vector2 position, Vector2 scale, const char *textureFilepath, 
        TextureType textureType, Vector2 spriteSheetDimensions, std::map<Direction, 
        std::vector<int>> animationAtlas, EntityType entityType) : 
        mPosition {position}, mVelocity {0.0f, 0.0f}, 
        mAcceleration {0.0f, 0.0f}, mMovement { 0.0f, 0.0f }, mScale {scale},
        mColliderDimensions {scale}, mTexture {LoadTexture(textureFilepath)}, 
        mTextureType {ATLAS}, mSpriteSheetDimensions {spriteSheetDimensions},
        mAnimationAtlas {animationAtlas}, mDirection {RIGHT},
        mAnimationIndices {animationAtlas.at(RIGHT)}, 
        mFrameSpeed {DEFAULT_FRAME_SPEED}, mAngle { 0.0f }, 
        mSpeed { DEFAULT_SPEED }, mEntityType {entityType} { }

Entity::~Entity() { UnloadTexture(mTexture); };

// ----------------------------------------------------------------
// COLLISION LOGIC (UPDATED FOR TOP-DOWN)
// ----------------------------------------------------------------

void Entity::checkCollisionY(Entity *collidableEntities, int collisionCheckCount)
{
    for (int i = 0; i < collisionCheckCount; i++)
    {
        Entity *collidableEntity = &collidableEntities[i];
        
        if (isColliding(collidableEntity))
        {
            float yDistance = fabs(mPosition.y - collidableEntity->mPosition.y);
            float yOverlap  = fabs(yDistance - (mColliderDimensions.y / 2.0f) - 
                              (collidableEntity->mColliderDimensions.y / 2.0f));
            
            if (mVelocity.y > 0) // Moving Down (South)
            {
                mPosition.y -= yOverlap;
                mVelocity.y  = 0;
                mIsCollidingBottom = true; 
            } 
            else if (mVelocity.y < 0) // Moving Up (North)
            {
                mPosition.y += yOverlap;
                mVelocity.y  = 0;
                mIsCollidingTop = true;
            }
        }
    }
}

void Entity::checkCollisionX(Entity *collidableEntities, int collisionCheckCount)
{
    for (int i = 0; i < collisionCheckCount; i++)
    {
        Entity *collidableEntity = &collidableEntities[i];
        
        if (isColliding(collidableEntity))
        {            
            float xDistance = fabs(mPosition.x - collidableEntity->mPosition.x);
            float xOverlap  = fabs(xDistance - (mColliderDimensions.x / 2.0f) - 
                              (collidableEntity->mColliderDimensions.x / 2.0f));

            if (mVelocity.x > 0) // Moving Right
            {
                mPosition.x     -= xOverlap;
                mVelocity.x      = 0;
                mIsCollidingRight = true;
            } 
            else if (mVelocity.x < 0) // Moving Left
            {
                mPosition.x    += xOverlap;
                mVelocity.x     = 0;
                mIsCollidingLeft = true;
            }
        }
    }
}

void Entity::checkCollisionY(Map *map)
{
    if (map == nullptr) return;

    Vector2 topCentreProbe    = { mPosition.x, mPosition.y - (mColliderDimensions.y / 2.0f) };
    Vector2 topLeftProbe      = { mPosition.x - (mColliderDimensions.x / 2.0f), mPosition.y - (mColliderDimensions.y / 2.0f) };
    Vector2 topRightProbe     = { mPosition.x + (mColliderDimensions.x / 2.0f), mPosition.y - (mColliderDimensions.y / 2.0f) };

    Vector2 bottomCentreProbe = { mPosition.x, mPosition.y + (mColliderDimensions.y / 2.0f) };
    Vector2 bottomLeftProbe   = { mPosition.x - (mColliderDimensions.x / 2.0f), mPosition.y + (mColliderDimensions.y / 2.0f) };
    Vector2 bottomRightProbe  = { mPosition.x + (mColliderDimensions.x / 2.0f), mPosition.y + (mColliderDimensions.y / 2.0f) };

    float xOverlap = 0.0f;
    float yOverlap = 0.0f;

    // COLLISION NORTH (Moving Up)
    if (mVelocity.y < 0.0f &&
        (map->isSolidTileAt(topCentreProbe, &xOverlap, &yOverlap) ||
         map->isSolidTileAt(topLeftProbe, &xOverlap, &yOverlap)   ||
         map->isSolidTileAt(topRightProbe, &xOverlap, &yOverlap)))
    {
        mPosition.y += yOverlap; 
        mVelocity.y  = 0.0f;
        mIsCollidingTop = true;
    }

    // COLLISION SOUTH (Moving Down)
    if (mVelocity.y > 0.0f && 
        (map->isSolidTileAt(bottomCentreProbe, &xOverlap, &yOverlap) ||
         map->isSolidTileAt(bottomLeftProbe, &xOverlap, &yOverlap)   ||
         map->isSolidTileAt(bottomRightProbe, &xOverlap, &yOverlap)))
    {
        mPosition.y -= yOverlap; 
        mVelocity.y  = 0.0f;
        mIsCollidingBottom = true;
    } 
}

void Entity::checkCollisionX(Map *map)
{
    if (map == nullptr) return;

    Vector2 leftCentreProbe   = { mPosition.x - (mColliderDimensions.x / 2.0f), mPosition.y };
    Vector2 rightCentreProbe  = { mPosition.x + (mColliderDimensions.x / 2.0f), mPosition.y };

    float xOverlap = 0.0f;
    float yOverlap = 0.0f;

    // COLLISION RIGHT
    if (mVelocity.x > 0.0f && 
        map->isSolidTileAt(rightCentreProbe, &xOverlap, &yOverlap))
    {
        mPosition.x -= xOverlap;
        mVelocity.x  = 0.0f;
        mIsCollidingRight = true;
    }

    // COLLISION LEFT
    if (mVelocity.x < 0.0f && 
        map->isSolidTileAt(leftCentreProbe, &xOverlap, &yOverlap))
    {
        mPosition.x += xOverlap;
        mVelocity.x  = 0.0f;
        mIsCollidingLeft = true;
    }
}

bool Entity::isColliding(Entity *other) const 
{
    if (!other->isActive() || other == this) return false;

    float xDistance = fabs(mPosition.x - other->getPosition().x) - 
        ((mColliderDimensions.x + other->getColliderDimensions().x) / 2.0f);
    float yDistance = fabs(mPosition.y - other->getPosition().y) - 
        ((mColliderDimensions.y + other->getColliderDimensions().y) / 2.0f);

    if (xDistance < 0.0f && yDistance < 0.0f) return true;

    return false;
}

// ----------------------------------------------------------------
// AI & UPDATE LOGIC
// ----------------------------------------------------------------

void Entity::animate(float deltaTime)
{
    mAnimationIndices = mAnimationAtlas.at(mDirection);

    mAnimationTime += deltaTime;
    float framesPerSecond = 1.0f / mFrameSpeed;

    if (mAnimationTime >= framesPerSecond)
    {
        mAnimationTime = 0.0f;
        mCurrentFrameIndex++;
        mCurrentFrameIndex %= mAnimationIndices.size();
    }
}


// ------------------------------------------------------------
// Phase 2: Central AI Execution
// ------------------------------------------------------------
void Entity::aiExecute(Entity* player, Map* map, float deltaTime)
{
    switch (mAIType)
    {
    case AI_GUARD:
        aiGuard(player, map, deltaTime);
        break;

    case AI_SENTRY:
        // Wake if player close
        if (Vector2Distance(mPosition, player->getPosition()) < 150.0f) {
            mAIState = CHASING;
        }
        if (mAIState == CHASING) aiChase(player, deltaTime);
        break;

    case AI_TRAP:
        // Reuse patrol for simple hazard movement
        aiPatrol(deltaTime);
        break;
    default:
        break;
    }
}

// Basic patrol between start and patrol target, with waypoint wait timer.
void Entity::aiPatrol(float deltaTime)
{
    // Simple logic: Move to Target, then wait, then flip back to Start
    if (mAIState == IDLE) {
        mWaitTimer += deltaTime;
        if (mWaitTimer > 2.0f) {
            mAIState = PATROLLING;
            mWaitTimer = 0.0f;
        }
        mMovement = {0.0f, 0.0f};
        return;
    }
    else if (mAIState == PATROLLING) {
        moveTowards(mPatrolTarget, deltaTime);
        if (Vector2Distance(mPosition, mPatrolTarget) < 5.0f) {
            // Swap Target and Start to loop
            Vector2 temp = mStartPosition;
            mStartPosition = mPatrolTarget;
            mPatrolTarget = temp;
            mAIState = IDLE;
            mMovement = {0.0f, 0.0f};
            // flip direction. left to right or up to down
            switch (mDirection)
            {
            case LEFT:
                mDirection = RIGHT;
                break;
            case RIGHT:
                mDirection = LEFT;
                break;
            case UP:
                mDirection = DOWN;
                break;
            case DOWN:
                mDirection = UP;
                break;
            default:
                break;
            }
            
        }
    }
}

// Chase logic: move directly toward player until far; then transition to RETURNING.
void Entity::aiChase(Entity* player, float deltaTime)
{
    if (!player) return;
    moveTowards(player->getPosition(), deltaTime);
    // Face the target explicitly
    Vector2 dir = Vector2Subtract(player->getPosition(), mPosition);
    if (fabs(dir.x) > fabs(dir.y)) {
        mDirection = (dir.x > 0) ? RIGHT : LEFT;
    } else {
        mDirection = (dir.y > 0) ? DOWN : UP;
    }
}

// Guard composite: handle patrol/chase/return transitions and detection.
void Entity::aiGuard(Entity* player, Map* map, float deltaTime)
{
    (void)map; // LOS can be integrated later
    bool canSeePlayer = player ? isEntityInSight(player, 200.0f, 90.0f) : false;

    switch (mAIState)
    {
    case IDLE:
    case PATROLLING:
        if (canSeePlayer) {
            mAIState = CHASING;
            mSpeed = 100; // accelerate when chasing
        }
        aiPatrol(deltaTime); // continue patrolling until chase triggers
        break;

    case CHASING:
        if (!canSeePlayer) {
            mWaitTimer += deltaTime;
            if (mWaitTimer > 2.0f) {
                mAIState = RETURNING;
                mWaitTimer = 0.0f;
                mSpeed = 70; // slow back down
            }
        } else {
            mWaitTimer = 0.0f;
        }
        aiChase(player, deltaTime);
        break;

    case RETURNING:
        moveTowards(mStartPosition, deltaTime);
        if (Vector2Distance(mPosition, mStartPosition) < 5.0f) {
            mAIState = IDLE;
            mSpeed = 80; // ensure base speed
        }
        if (canSeePlayer) {
            mAIState = CHASING;
            mSpeed = 140;
        }
        break;
    }
}

void Entity::moveTowards(Vector2 target, float deltaTime)
{
    (void)deltaTime; // Not needed; movement scaled later by update()
    Vector2 directionRaw = Vector2Subtract(target, mPosition);
    if (Vector2Length(directionRaw) > 0.0f)
        mMovement = Vector2Normalize(directionRaw);
    else
        mMovement = {0.0f, 0.0f};
}


void Entity::update(float deltaTime, Entity *player, Map *map, 
    Entity *collidableEntities, int collisionCheckCount)
{
    if (isActive()) {
        // Phase 3: central AI hook
        if (mEntityType == NPC) {
            aiExecute(player, map, deltaTime);
        }

    resetColliderFlags();

    // 1. NORMALIZE MOVEMENT (Prevent fast diagonal movement)
    if (Vector2Length(mMovement) > 0) {
        mMovement = Vector2Normalize(mMovement);
    }

    // 2. UPDATE VELOCITY (No Gravity)
    // We can still add Acceleration here if we want "slippery" movement,
    // otherwise, we just set velocity directly from movement * speed.
    mVelocity.x = mMovement.x * mSpeed;
    mVelocity.y = mMovement.y * mSpeed;

    // 3. APPLY X MOVEMENT & COLLISION
    mPosition.x += mVelocity.x * deltaTime;
    checkCollisionX(collidableEntities, collisionCheckCount);
    checkCollisionX(map);

    // 4. APPLY Y MOVEMENT & COLLISION
    mPosition.y += mVelocity.y * deltaTime;
    checkCollisionY(collidableEntities, collisionCheckCount);
    checkCollisionY(map);

    // 5. UPDATE POSITION HISTORY (Breadcrumbs)
    // Always record for Player to ensure consistent breadcrumb trail;
    // for other entities, only when they moved.
    if (mEntityType == PLAYER || Vector2Length(mVelocity) > 0.0f) {
        mPositionHistory.push_front(mPosition);
        // Maintain manageable buffer size (200 is sufficient; need >=6 for follower logic).
        if (mPositionHistory.size() > 200) {
            mPositionHistory.pop_back();
        }
    }

    // 6. ANIMATE
    if (mTextureType == ATLAS && Vector2Length(mMovement) != 0) 
        animate(deltaTime);
    }
}

void Entity::render()
{
    if(mEntityStatus == INACTIVE) return;

    Rectangle textureArea;

    switch (mTextureType)
    {
        case SINGLE:
            textureArea = {
                0.0f, 0.0f,
                static_cast<float>(mTexture.width),
                static_cast<float>(mTexture.height)
            };
            break;
        case ATLAS:
            textureArea = getUVRectangle(
                &mTexture, 
                mAnimationIndices[mCurrentFrameIndex], 
                mSpriteSheetDimensions.x, 
                mSpriteSheetDimensions.y
            );
        
        default: break;
    }

    Rectangle destinationArea = {
        mPosition.x,
        mPosition.y,
        static_cast<float>(mScale.x),
        static_cast<float>(mScale.y)
    };

    Vector2 originOffset = {
        static_cast<float>(mScale.x) / 2.0f,
        static_cast<float>(mScale.y) / 2.0f
    };

    DrawTexturePro(
        mTexture, 
        textureArea, destinationArea, originOffset,
        mAngle, WHITE
    );
}

void Entity::displayCollider() 
{
    Rectangle colliderBox = {
        mPosition.x - mColliderDimensions.x / 2.0f,  
        mPosition.y - mColliderDimensions.y / 2.0f,  
        mColliderDimensions.x,                        
        mColliderDimensions.y                        
    };

    DrawRectangleLines(
        colliderBox.x,      
        colliderBox.y,      
        colliderBox.width,  
        colliderBox.height, 
        GREEN               
    );
}

Vector2 Entity::getDirectionVector() const
{
    switch (mDirection) {
        case LEFT:  return { -1.0f,  0.0f };
        case RIGHT: return {  1.0f,  0.0f };
        case UP:    return {  0.0f, -1.0f };
        case DOWN:  return {  0.0f,  1.0f };
        default:    return {  0.0f,  1.0f }; // Default DOWN
    }
}

// ----------------------------------------------------------------
// SIGHT & AMBUSH LOGIC
// ----------------------------------------------------------------
// Check if 'other' is within this entity's view cone.
// viewDistance: max distance in pixels
// viewAngleDeg: full cone angle (e.g., 90 means 45 deg each side)
bool Entity::isEntityInSight(Entity* other, float viewDistance, float viewAngleDeg)
{
    if (!other || !other->isActive() || !this->isActive()) return false;

    Vector2 myPos    = this->getPosition();
    Vector2 otherPos = other->getPosition();

    // 1. Distance check
    float dist = Vector2Distance(myPos, otherPos);
    if (dist > viewDistance) return false;

    // 2. Angle check via dot product
    Vector2 toTarget = Vector2Subtract(otherPos, myPos);
    if (Vector2Length(toTarget) <= 0.001f) return true; // Same spot -> seen
    toTarget = Vector2Normalize(toTarget);
    Vector2 facing   = this->getDirectionVector();

    float dot = Vector2DotProduct(facing, toTarget);

    float angleRad  = (viewAngleDeg / 2.0f) * (PI / 180.0f);
    float threshold = cosf(angleRad); // Minimum dot to be inside cone

    return (dot > threshold);
}

// Attacker is considered "behind" victim if they face roughly the same direction.
// Dot > 0.5 (~60 deg alignment) considered acceptable.
bool Entity::checkAmbush(Entity* victim)
{
    if (!victim || !victim->isActive() || !this->isActive()) return false;
    Vector2 myDir    = this->getDirectionVector();
    Vector2 theirDir = victim->getDirectionVector();
    return (Vector2DotProduct(myDir, theirDir) > 0.5f);
}

// ----------------------------------------------------------------
// FOLLOWER PHYSICS (Elastic Tether + Personal Space + Jitter + Integration)
// ----------------------------------------------------------------
void Entity::updateFollowerPhysics(Entity* leader, const std::vector<Entity*>& followers,
    Map* map, float deltaTime, float tetherSpeed, float repelStrength,
    float jitterStrength, float damping)
{
    if (mEntityType != NPC || mAIType != AI_SENTRY || leader == nullptr) return;

    Vector2 currentPos = mPosition;

    // A. TETHER FORCE (Attraction to past leader position for smoother path)
    Vector2 targetPos;
    auto& history = leader->getPositionHistory();
    if (history.size() > 5) targetPos = history[5];
    else targetPos = leader->getPosition();
    Vector2 tether = Vector2Scale(Vector2Subtract(targetPos, currentPos), tetherSpeed);

    // B. SEPARATION FORCE (Inverse-square repulsion)
    Vector2 separation = {0.0f, 0.0f};
    auto accumulateRepel = [&](Entity* neighbor){
        if (!neighbor || neighbor == this) return;
        float dist = Vector2Distance(currentPos, neighbor->getPosition());
        if (dist < 30.0f) {
            if (dist < 1.0f) dist = 1.0f; // Clamp
            Vector2 pushDir = Vector2Subtract(currentPos, neighbor->getPosition());
            if (Vector2Length(pushDir) > 0.0f) pushDir = Vector2Normalize(pushDir);
            float scale = repelStrength / (dist * dist);
            separation = Vector2Add(separation, Vector2Scale(pushDir, scale));
        }
    };
    accumulateRepel(leader);
    for (Entity* f : followers) accumulateRepel(f);

    // C. IDLE JITTER (Ambient life)
    Vector2 jitter = {0.0f, 0.0f};
    if (Vector2Length(mVelocity) < 5.0f) {
        float time = GetTime();
        float entityID = static_cast<float>((reinterpret_cast<uintptr_t>(this) & 0xFFF));
        Vector2 noise = { sinf(time + entityID), cosf(time + entityID) };
        jitter = Vector2Scale(noise, jitterStrength);
    }

    // D. INTEGRATE FORCES -> ACCELERATION
    Vector2 acceleration = tether;
    acceleration = Vector2Add(acceleration, Vector2Scale(separation, 2.0f)); // Separation weighted
    acceleration = Vector2Add(acceleration, jitter);

    // Apply to velocity (note: remove extra deltaTime factor to avoid tiny movement)
    mVelocity = Vector2Add(mVelocity, acceleration);
    // Clamp max velocity to avoid jitter explosions
    const float MAX_FOLLOWER_SPEED = 250.0f; // pixels/second (similar to DEFAULT_SPEED)
    float velMag = Vector2Length(mVelocity);
    if (velMag > MAX_FOLLOWER_SPEED) {
        mVelocity = Vector2Scale(Vector2Normalize(mVelocity), MAX_FOLLOWER_SPEED);
    }
    // Apply damping (frictional decay)
    mVelocity = Vector2Scale(mVelocity, damping);

    // Collision preparation
    resetColliderFlags();

    // Move X then collide
    mPosition.x += mVelocity.x * deltaTime;
    checkCollisionX(map);
    // Move Y then collide
    mPosition.y += mVelocity.y * deltaTime;
    checkCollisionY(map);

    // Record breadcrumb if moved (maintain history for next followers)
    if (Vector2Length(mVelocity) > 0.0f) {
        mPositionHistory.push_front(mPosition);
        if (mPositionHistory.size() > 200) mPositionHistory.pop_back();
    }

    // Derive movement vector for animation/direction (not used for speed calc now)
    if (Vector2Length(mVelocity) > 0.0f) {
        mMovement = Vector2Normalize(mVelocity);
        if (fabs(mVelocity.x) > fabs(mVelocity.y)) {
            mDirection = (mVelocity.x < 0) ? LEFT : RIGHT;
        } else {
            mDirection = (mVelocity.y < 0) ? UP : DOWN;
        }
    } else {
        mMovement = {0.0f, 0.0f};
    }

    // Animation (if atlas)
    if (mTextureType == ATLAS && Vector2Length(mVelocity) > 1.0f) {
        animate(deltaTime);
    }
}

