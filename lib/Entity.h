#ifndef ENTITY_H
#define ENTITY_H

#include "Map.h"
#include <deque> // Required for Breadcrumb/Follower system

enum Direction    { LEFT, UP, RIGHT, DOWN              };
enum EntityStatus { ACTIVE, INACTIVE                   };
enum EntityType   { PLAYER, BLOCK, PLATFORM, NPC, NONE };
// Updated AI enums (Phase 1: Patrol/Chase/Return behaviors)
enum AIType { 
    AI_GUARD,   // Patrols and chases
    AI_SENTRY,  // Stationary until player close
    AI_TRAP     // Moving hazard (invincible)
};

enum AIState { 
    IDLE,        // Inactive / waiting
    PATROLLING,  // Moving between waypoints
    CHASING,     // Pursuing player
    RETURNING    // Returning to start position
};

class Entity
{
private:
    Vector2 mPosition;
    Vector2 mMovement;
    Vector2 mVelocity;
    Vector2 mAcceleration; 

    Vector2 mScale;
    Vector2 mColliderDimensions;
    
    Texture2D mTexture;
    TextureType mTextureType;
    Vector2 mSpriteSheetDimensions;
    
    std::map<Direction, std::vector<int>> mAnimationAtlas;
    std::vector<int> mAnimationIndices;
    Direction mDirection;
    int mFrameSpeed;

    int mCurrentFrameIndex = 0;
    float mAnimationTime = 0.0f;


    int mSpeed;
    float mAngle;

    bool mIsCollidingTop    = false;
    bool mIsCollidingBottom = false;
    bool mIsCollidingRight  = false;
    bool mIsCollidingLeft   = false;

    EntityStatus mEntityStatus = ACTIVE;
    EntityType   mEntityType;

    AIType  mAIType;
    AIState mAIState;

    // Patrol / AI navigation data
    Vector2 mStartPosition;    // Original post/guard position
    Vector2 mPatrolTarget;     // Current patrol waypoint
    float   mWaitTimer = 0.0f; // Time waiting at a waypoint

    // Follower System: Breadcrumbs
    std::deque<Vector2> mPositionHistory; 

    void checkCollisionY(Entity *collidableEntities, int collisionCheckCount);
    void checkCollisionY(Map *map);

    void checkCollisionX(Entity *collidableEntities, int collisionCheckCount);
    void checkCollisionX(Map *map);
    
    void resetColliderFlags() 
    {
        mIsCollidingTop    = false;
        mIsCollidingBottom = false;
        mIsCollidingRight  = false;
        mIsCollidingLeft   = false;
    }

    void animate(float deltaTime);

    // Phase 2 AI Core
    void aiExecute(Entity* player, Map* map, float deltaTime);
    void aiGuard(Entity* player, Map* map, float deltaTime);
    void aiPatrol(float deltaTime);
    void aiChase(Entity* player, float deltaTime);
    void moveTowards(Vector2 target, float deltaTime);

public:
    static constexpr int   DEFAULT_SIZE          = 250;
    static constexpr int   DEFAULT_SPEED         = 200;
    static constexpr int   DEFAULT_FRAME_SPEED   = 14;
    // REMOVED: Y_COLLISION_THRESHOLD (Not needed for top-down)

    Entity();
    Entity(Vector2 position, Vector2 scale, const char *textureFilepath, 
        EntityType entityType);
    Entity(Vector2 position, Vector2 scale, const char *textureFilepath, 
        TextureType textureType, Vector2 spriteSheetDimensions, 
        std::map<Direction, std::vector<int>> animationAtlas, 
        EntityType entityType);
    ~Entity();

    void update(float deltaTime, Entity *player, Map *map, 
        Entity *collidableEntities, int collisionCheckCount);
    void render();
    void normaliseMovement() { Normalise(&mMovement); }

    void activate()   { mEntityStatus  = ACTIVE;   }
    void deactivate() { mEntityStatus  = INACTIVE; }
    void displayCollider();

    bool isActive() { return mEntityStatus == ACTIVE ? true : false; }
    
    bool isColliding(Entity *other) const;

    // Updated Movement Setters
    void moveUp()    { mMovement.y = -1; mDirection = UP;    }
    void moveDown()  { mMovement.y =  1; mDirection = DOWN;  }
    void moveLeft()  { mMovement.x = -1; mDirection = LEFT;  }
    void moveRight() { mMovement.x =  1; mDirection = RIGHT; }

    void resetMovement() { mMovement = { 0.0f, 0.0f }; }

    // Stealth / Ambush Helpers
    Vector2 getDirectionVector() const; // New helper for Dot Product calculation
    bool isEntityInSight(Entity* other, float viewDistance = 150.0f, float viewAngleDeg = 90.0f); // View cone check (defaults)
    bool checkAmbush(Entity* victim); // Direction alignment (attacker behind victim)

    // Getters
    Vector2     getPosition()              const { return mPosition;              }
    Vector2     getMovement()              const { return mMovement;              }
    Vector2     getVelocity()              const { return mVelocity;              }
    Vector2     getScale()                 const { return mScale;                 }
    Vector2     getColliderDimensions()    const { return mScale;                 }
    Vector2     getSpriteSheetDimensions() const { return mSpriteSheetDimensions; }
    Texture2D   getTexture()               const { return mTexture;               }
    TextureType getTextureType()           const { return mTextureType;           }
    Direction   getDirection()             const { return mDirection;             }
    int         getFrameSpeed()            const { return mFrameSpeed;            }
    int         getSpeed()                 const { return mSpeed;                 }
    float       getAngle()                 const { return mAngle;                 }
    EntityType  getEntityType()            const { return mEntityType;            }
    AIType      getAIType()                const { return mAIType;                }
    AIState     getAIState()               const { return mAIState;               }
    std::deque<Vector2>& getPositionHistory()    { return mPositionHistory;       }

    bool isCollidingTop()    const { return mIsCollidingTop;    }
    bool isCollidingBottom() const { return mIsCollidingBottom; }
    bool isCollidingLeft()   const { return mIsCollidingLeft;   }
    bool isCollidingRight()  const { return mIsCollidingRight;  }

    std::map<Direction, std::vector<int>> getAnimationAtlas() const { return mAnimationAtlas; }

    // Setters
    void setPosition(Vector2 newPosition)       { mPosition = newPosition;                 }
    void setMovement(Vector2 newMovement)       { mMovement = newMovement;                 }
    void setAcceleration(Vector2 newAcceleration){ mAcceleration = newAcceleration;         }
    void setScale(Vector2 newScale)             { mScale = newScale;                       }
    void setTexture(const char *textureFilepath){ mTexture = LoadTexture(textureFilepath); }
    void setColliderDimensions(Vector2 newDimensions) { mColliderDimensions = newDimensions; }
    void setSpriteSheetDimensions(Vector2 newDimensions) { mSpriteSheetDimensions = newDimensions; }
    void setSpeed(int newSpeed)                 { mSpeed  = newSpeed;                      }
    void setFrameSpeed(int newSpeed)            { mFrameSpeed = newSpeed;                  }
    void setAngle(float newAngle)               { mAngle = newAngle;                       }
    void setEntityType(EntityType entityType)   { mEntityType = entityType;                }
    void setDirection(Direction newDirection)
    { 
        mDirection = newDirection;
        if (mTextureType == ATLAS) mAnimationIndices = mAnimationAtlas.at(mDirection);
    }
    void setAIState(AIState newState)           { mAIState = newState;                     }
    void setAIType(AIType newType)              { mAIType = newType;                       }
    void setStartPosition(Vector2 pos)          { mStartPosition = pos;                    }
    void setPatrolTarget(Vector2 pos)           { mPatrolTarget  = pos;                    }

    // Advanced follower physics (tether + separation + jitter + integration)
    void updateFollowerPhysics(Entity* leader, const std::vector<Entity*>& followers,
        Map* map, float deltaTime, float tetherSpeed, float repelStrength,
        float jitterStrength, float damping);
};

#endif // ENTITY_H