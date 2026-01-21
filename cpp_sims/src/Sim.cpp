#include "Sim.hpp"
#include "raymath.h"
#include <cstdlib>
#include <cmath>
#include <iostream>

int Sim::nextId = 1;

Sim::Sim(std::string name, Vector3 position, Color color) 
    : id(nextId++), name(name), position(position), color(color), currentState(IDLE) {
    
    // Initialize stats
    hunger = 0.0f;     // Not hungry
    energy = 100.0f;   // Fully energized
    anxiety = 0.0f;    // Calm
    stress = 0.0f;     // Relaxed
    
    decisionTimer = 0.0f;
    actionDuration = 0.0f;
    targetPosition = position;
}

void Sim::Update(float deltaTime, std::vector<Tree>& trees, std::vector<Sim>& allSims, const std::vector<House>& houses) {
    // 1. Update Physiological Stats
    hunger += 2.0f * deltaTime; 
    energy -= 1.5f * deltaTime;

    // HOUSE ANXIETY REDUCTION
    bool insideHouse = false;
    for (const auto& h : houses) {
        // Simple bounds check (X, Z against Rect X, Y with Width, Height)
        if (position.x >= h.rect.x && position.x <= h.rect.x + h.rect.width &&
            position.z >= h.rect.y && position.z <= h.rect.y + h.rect.height) {
            insideHouse = true;
            break;
        }
    }
    
    if (insideHouse) {
        anxiety -= 5.0f * deltaTime; // Drastic reduction
    } else {
        // Random fluctuation for anxiety/stress outside
        if (rand() % 100 < 2) anxiety += 0.5f;
    }
    
    if (anxiety > 50) stress += 0.2f * deltaTime;
    // Anxiety decays slowly if not stressed (and not in house handled above)
    if (!insideHouse && stress < 20) anxiety -= 0.1f * deltaTime;

    // Clamp stats
    if (hunger > 100) hunger = 100; if (hunger < 0) hunger = 0;
    if (energy > 100) energy = 100; if (energy < 0) energy = 0;
    if (anxiety > 100) anxiety = 100; if (anxiety < 0) anxiety = 0;
    if (stress > 100) stress = 100; if (stress < 0) stress = 0;

    // SOCIAL INTERACTION (Talking)
    if (talkCooldown > 0) talkCooldown -= deltaTime;

    // If currently talking
    if (currentState == TALKING) {
        if (actionDuration > 0) {
            actionDuration -= deltaTime;
            anxiety -= 2.0f * deltaTime; // Talking reduces anxiety
            hunger += 0.5f * deltaTime;  // Talking takes energy/food

            // Face Partner?
            if (conversationPartner) {
                Vector3 dir = Vector3Subtract(conversationPartner->GetPosition(), position);
                // Update velocity just for orientation (hacky but works for drawing direction)
                if (Vector3Length(dir) > 0.1f) velocity = Vector3Normalize(dir);
            }
        } else {
            // Finished talking
            currentState = IDLE;
            conversationPartner = nullptr;
            MakeDecision(); // Decide what to do next immediatey
        }
        return; // Skip movement logic while talking
    }

    // Try to initiate talk if free
    if (currentState != TALKING && currentState != SLEEPING && talkCooldown <= 0) {
        for (auto& other : allSims) {
            if (other.GetId() == this->id) continue; // Don't talk to self
            
            if (other.IsAvailableForTalk()) {
                float dist = Vector3Distance(position, other.GetPosition());
                if (dist < 4.0f) { // Close enough
                    // Start Talking
                    this->StartTalking(&other);
                    other.StartTalking(this);
                    break;
                }
            }
        }
    }

    // Check Tree Interaction (Eat Fruit)
    if (currentState == WANDERING || currentState == IDLE) { // Can eat while moving
        for (auto& tree : trees) {
            if (tree.fruitCount > 0) {
                 float dist = Vector3Distance(position, tree.position);
                 if (dist < 2.0f) { // Under tree radius
                     // Eat fruit
                     tree.fruitCount--;
                     hunger -= 10.0f; // Heal hunger
                     currentState = EATING; // Just for visual check
                     actionDuration = 1.0f; // Brief stop
                     break; 
                 }
            }
        }
    }

    // 2. State Machine Logic
    if (actionDuration > 0) {
        actionDuration -= deltaTime;
        
        if (currentState == EATING) {
            hunger -= 15.0f * deltaTime; // Recover hunger fast (if eating meal, not just fruit)
            if (hunger < 0) hunger = 0;
        } else if (currentState == SLEEPING) {
            energy += 10.0f * deltaTime; // Recover energy fast
            stress -= 5.0f * deltaTime;  // Sleep reduces stress
            if (energy > 100) energy = 100;
        } else if (currentState == WAKING_UP) {
            // Just wait (waking up animation phase)
        }
        
        // Movement Logic
        if (currentState == WANDERING) {
            // Speed Calculation
            float speed = 1.5f; // Slower wandering base speed
            if (IsOnRoad(position)) {
                speed = 4.0f; // Boost on roads!
            }
            speed *= deltaTime;


            Vector3 diff = Vector3Subtract(targetPosition, position);
            float dist = Vector3Length(diff);
            
            if (dist > 0.1f) {
                Vector3 move = Vector3Scale(Vector3Normalize(diff), speed);
                velocity = Vector3Normalize(diff); // Update orientation
                position = Vector3Add(position, move);
            } else {
                // Reached destination early
                currentState = IDLE;
                actionDuration = 0;
            }
        }
    } else {
        // If we just finished Sleeping, go to Waking Up
        if (currentState == SLEEPING) {
            currentState = WAKING_UP;
            actionDuration = 1.0f; // Wait 1 second
            return;
        }
        
        // Current action finished, make a new decision
        MakeDecision();
    }
}

void Sim::MakeDecision() {
    // Decision tree based on probabilities and stats

    float randomVal = (float)rand() / RAND_MAX;

    // High priority: Survival
    if (energy < 20.0f) {
        // Very tired -> High chance to sleep
        if (randomVal < 0.9f) {
            currentState = SLEEPING;
            actionDuration = 5.0f; // Sleep for 5 seconds
            return;
        }
    }

    if (hunger > 70.0f) {
        // Very hungry -> High chance to eat
        if (randomVal < 0.8f) {
            currentState = EATING;
            actionDuration = 3.0f; // Eat for 3 seconds
            return;
        }
    }

    // Low priority: Idle or Wander
    if (randomVal < 0.3f) {
        currentState = IDLE;
        actionDuration = 1.0f + ((float)rand() / RAND_MAX) * 2.0f; // 1-3 seconds idle
    } else {
        currentState = WANDERING;
        PickRandomDestination();
        // Calculate time needed to get there roughly, or fixed time
        actionDuration = 4.0f; 
    }
}

void Sim::PickRandomDestination() {
    float range = 45.0f; // Larger map range (roughly -45 to 45)
    float x = ((float)rand() / RAND_MAX) * 2 * range - range;
    float z = ((float)rand() / RAND_MAX) * 2 * range - range;
    targetPosition = { x, 0.5f, z }; // Y is 0.5 (half height)
}

void Sim::Draw() {
    // Visual feedback for states
    if (currentState == SLEEPING) {
        // Draw Lying Down (Horizontal)
        // From current position along X axis
        Vector3 startPos = { position.x - 1.0f, 0.5f, position.z };
        Vector3 endPos   = { position.x + 1.0f, 0.5f, position.z };
        
        Color sleepColor = ColorTint(color, GRAY);
        DrawCylinderEx(startPos, endPos, 0.5f, 0.5f, 8, sleepColor);
        
        // Head on one side
        Vector3 headPos = { position.x + 1.3f, 0.5f, position.z };
        DrawSphere(headPos, 0.4f, BEIGE);
    } 
    else {
        // Draw Standing
        Vector3 drawPos = position;
        drawPos.y += 1.0f; 

        Color drawColor = color;
        
        if (currentState == WAKING_UP) {
             drawColor = WHITE; // Flash white when waking up
        }

        DrawCylinder(position, 0.5f, 0.5f, 2.0f, 8, drawColor);
        DrawCylinderWires(position, 0.5f, 0.5f, 2.0f, 8, DARKGRAY);
        
        // Head
        Vector3 headPos = { position.x, position.y + 2.0f, position.z };
        DrawSphere(headPos, 0.4f, BEIGE);
        
        // Eyes & Vision (only when not sleeping)
        // Calculate orientation angle from velocity
        float angle = atan2f(velocity.x, velocity.z); // Raylib uses Z as forward usually? X/Z plane.
        
        // Eyes
        // Transform offset by rotation
        float eyeOffsetSide = 0.15f;
        float eyeOffsetFwd = 0.35f;
        
        Vector3 leftEyePos = {
            headPos.x + sinf(angle - 0.5f) * eyeOffsetFwd,
            headPos.y + 0.1f,
            headPos.z + cosf(angle - 0.5f) * eyeOffsetFwd
        };
         Vector3 rightEyePos = {
            headPos.x + sinf(angle + 0.5f) * eyeOffsetFwd,
            headPos.y + 0.1f,
            headPos.z + cosf(angle + 0.5f) * eyeOffsetFwd
        };
        
        // Actually, simple sin/cos on angle is easier
        Vector3 fwd = Vector3Normalize(velocity);
        // Right vector (Cross product with Up)
        Vector3 right = Vector3CrossProduct(fwd, {0,1,0});
        
        // Left Eye
        Vector3 lEye = Vector3Add(headPos, Vector3Scale(fwd, 0.35f));
        lEye = Vector3Add(lEye, Vector3Scale(right, -0.12f));
        lEye.y += 0.1f;
        
        // Right Eye
        Vector3 rEye = Vector3Add(headPos, Vector3Scale(fwd, 0.35f));
        rEye = Vector3Add(rEye, Vector3Scale(right, 0.12f));
        rEye.y += 0.1f;
        
        DrawSphere(lEye, 0.05f, BLACK);
        DrawSphere(rEye, 0.05f, BLACK);
        
        // Vision Cone (Red Light Beam)
        // Draw a triangle fan on the ground or semi-transparent cone
        // Drawing a flat triangle fan "field of view" at ground level is clearer
        Color visionColor = { 255, 0, 0, 80 }; // Red semi-transparent
        
        Vector3 fovStart = { position.x, 0.1f, position.z };
        // 45 degree spread, length 10
        float coneLength = 10.0f;
        float baseAngle = atan2f(velocity.x, velocity.z); // Radians
        float halfFov = 45.0f * DEG2RAD / 2.0f;
        
        // Raylib doesn't have a 3D sector primitive easily, let's draw lines or a triangle
        // Left edge
        Vector3 pLeft = {
            position.x + sinf(baseAngle - halfFov) * coneLength,
            0.1f,
            position.z + cosf(baseAngle - halfFov) * coneLength
        };
        // Right edge
        Vector3 pRight = {
             position.x + sinf(baseAngle + halfFov) * coneLength,
            0.1f,
            position.z + cosf(baseAngle + halfFov) * coneLength
        };
        
        // Draw Triangle (Double sided for visibility)
        DrawTriangle3D(fovStart, pLeft, pRight, visionColor);
        DrawTriangle3D(fovStart, pRight, pLeft, visionColor);
    }
}

int Sim::GetId() const { return id; }
std::string Sim::GetName() const { return name; }
float Sim::GetHunger() const { return hunger; }
float Sim::GetEnergy() const { return energy; }
float Sim::GetAnxiety() const { return anxiety; }
float Sim::GetStress() const { return stress; }
Vector3 Sim::GetPosition() const { return position; }

std::string Sim::GetStateString() const {
    switch (currentState) {
        case IDLE: return "Idle";
        case WANDERING: return "Wandering";
        case SLEEPING: return "Sleeping";
        case WAKING_UP: return "Waking Up";
        case EATING: return "Eating";
        case TALKING: return "Talking";
        default: return "Unknown";
    }
}

bool Sim::IsAvailableForTalk() const {
    return (currentState == IDLE || currentState == WANDERING) && talkCooldown <= 0;
}

void Sim::StartTalking(Sim* partner) {
    currentState = TALKING;
    conversationPartner = partner;
    actionDuration = 3.0f; // Talk for 3 seconds
    talkCooldown = 10.0f;  // Cannot talk again for 10 seconds
    
    // Stop moving
    velocity = {0,0,0}; 
}

