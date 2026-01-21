#pragma once
#include "raylib.h"
#include <string>

// --- Data Components (POD) ---

struct TransformComponent {
    Vector3 position;
    float rotation; // Y-axis rotation in degrees
};

struct MovementComponent {
    // Waypoints queue. Increased size for more granular pathing (turns)
    Vector3 path[16]; 
    int pathIndex;
    int pathLength;
    
    float speed;
    bool isMoving;
};

enum class SimState {
    SLEEPING,
    WALKING_TO_CAR,
    DRIVING_TO_WORK,
    WORKING,
    WALKING_FROM_WORK_CAR,
    DRIVING_HOME,
    WALKING_TO_HOUSE
};

struct StateComponent {
    SimState currentState;
    float actionTimer;
};

struct SimStatsComponent {
    int id;
    int age;
    float money;
    float sleep; // 0-100
};

struct IdentityComponent {
    std::string firstName;
    std::string lastName;
};

struct VisualComponent {
    Color color;
    Color carColor;
};

// Relation Components 
struct HomeReferenceComponent {
    int buildingId;
    int cityId;       // To find the city "hub" for highway navigation
    Vector3 doorPosition; 
    Vector3 parkingPosition; 
};

struct WorkReferenceComponent {
    int buildingId;
    int cityId;       // To find the city "hub" for highway navigation
    Vector3 doorPosition;
    Vector3 parkingPosition;
};

struct RenderBatchComponent {
    int modelId; 
};
