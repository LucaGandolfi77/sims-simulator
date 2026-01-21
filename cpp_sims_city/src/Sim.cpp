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
    
    // Random Age and Money
    age = 18 + rand() % 42; // 18-60
    money = (float)(rand() % 1000); // 0-1000 Simoleons

    decisionTimer = 0.0f;
    actionDuration = 0.0f;
    targetPosition = position;
    globalTime = 0.0f;
}

void Sim::Update(float deltaTime, std::vector<Tree>& trees, std::vector<Sim>& allSims, const std::vector<House>& houses, const Office& office) {
    globalTime += deltaTime;

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
            MakeDecision(houses, office); // Decide what to do next immediatey
        }
        return; // Skip movement logic while talking
    }

    // Try to initiate talk if free
    if (currentState != TALKING && currentState != SLEEPING && currentState != WORKING && currentState != WALKING_TO_WORK && currentState != WALKING_TO_BED && talkCooldown <= 0) {
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
        } else if (currentState == WORKING) {
            energy -= 4.0f * deltaTime; // Work takes energy
            stress += 1.0f * deltaTime; // Work increases stress
            money += 10.0f * deltaTime; // Earn Money!
            
            // If exhausted, stop working early
            if (energy < 5.0f) actionDuration = 0;
            
        } else if (currentState == WAKING_UP) {
            // Just wait (waking up animation phase)
        }
        
        // Movement Logic
        if (currentState == WANDERING || currentState == WALKING_TO_BED || currentState == WALKING_TO_WORK) {
            // Speed Calculation
            float speed = 1.5f; // Slower wandering base speed
            if (IsOnRoad(position)) {
                speed = 4.0f; // Boost on roads!
            }
            if (currentState == WALKING_TO_BED || currentState == WALKING_TO_WORK) speed = 3.5f; // Walk with purpose

            speed *= deltaTime;


            Vector3 diff = Vector3Subtract(targetPosition, position);
            float dist = Vector3Length(diff);
            
            if (dist > 0.1f) {
                Vector3 move = Vector3Scale(Vector3Normalize(diff), speed);
                velocity = Vector3Normalize(diff); // Update orientation
                position = Vector3Add(position, move);
            } else {
                // Reached destination early
                if (currentState == WALKING_TO_BED) {
                     currentState = SLEEPING;
                     actionDuration = 15.0f; // Sleep longer in bed!
                     anxiety = 0; // Safe in home, reset anxiety
                     stress -= 20.0f; // Huge stress relief
                } else if (currentState == WALKING_TO_WORK) {
                     currentState = WORKING;
                     actionDuration = 10.0f + (rand() % 10); // Work for 10-20 seconds
                } else {
                    currentState = IDLE;
                    actionDuration = 0;
                }
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
        MakeDecision(houses, office);
    }
}

void Sim::MakeDecision(const std::vector<House>& houses, const Office& office) {
    // Better AI: Priority-Based Decision Making
    
    // 1. CRITICAL SURVIVAL NEEDS
    // If very tired, go to sleep.
    if (energy < 30.0f) {
        if (!houses.empty()) {
            // Find a random house to sleep in
            int hIndex = rand() % houses.size();
            const House& h = houses[hIndex];
            float hx = h.rect.x + h.rect.width/2;
            float hz = h.rect.y + h.rect.height/2;
            
            targetPosition = { hx, 0.5f, hz };
            currentState = WALKING_TO_BED;
            actionDuration = 60.0f; // Give proper time to walk
        } else {
             // No house? Sleep on ground
             currentState = SLEEPING;
             actionDuration = 10.0f; 
        }
        return;
    }

    // If very hungry, eat
    if (hunger > 60.0f) {
        // Look for food (Trees are everywhere)
        // In improved version, we should walk to a tree.
        // For now, we set state to EATING and assume they find food or pull a snack.
        currentState = EATING;
        actionDuration = 5.0f; 
        return;
    }

    // 2. WORK / MONEY (If needs are met)
    // If poor and has enough energy, go to work
    if (money < 500.0f && energy > 60.0f) {
        // 70% chance to be responsible
        if ((rand() % 100) < 70) {
            if (!office.desks.empty()) {
                int dIndex = rand() % office.desks.size();
                targetPosition = office.desks[dIndex];
                currentState = WALKING_TO_WORK;
                actionDuration = 60.0f;
                return;
            }
        }
    }

    // 3. SOCIAL / LEISURE
    // If lonely (high anxiety), try to socialize is handled in Update loop via proximity.
    // Here we just wander or idle to facilitate meetings.
    
    float activityRoll = (float)rand() / RAND_MAX;
    if (activityRoll < 0.3f) {
        currentState = IDLE;
        actionDuration = 2.0f + ((float)rand() / RAND_MAX) * 3.0f; // Chill
    } else {
        currentState = WANDERING;
        PickRandomDestination();
        actionDuration = 5.0f; 
    }
}

void Sim::PickRandomDestination() {
    float range = 45.0f; 
    float x = ((float)rand() / RAND_MAX) * 2 * range - range;
    float z = ((float)rand() / RAND_MAX) * 2 * range - range;
    targetPosition = { x, 0.5f, z }; 
}

void Sim::Draw() {
    // Visual feedback for states
    if (currentState == SLEEPING) {
        Vector3 startPos = { position.x - 1.0f, 0.5f, position.z };
        Vector3 endPos   = { position.x + 1.0f, 0.5f, position.z };
        // Don't change color when sleeping, keep original id color
        // Just rotate geometry to look like lying down
        DrawCylinderEx(startPos, endPos, 0.5f, 0.5f, 8, color);
        DrawSphere({ position.x + 1.3f, 0.5f, position.z }, 0.4f, BEIGE);
    } 
    else if (currentState == WORKING) {
        // Sitting at desk
        Vector3 sitPos = position;
        sitPos.y = 0.5f;
        DrawCylinder(sitPos, 0.5f, 0.5f, 1.2f, 8, color);
        DrawSphere({ position.x, 1.8f, position.z }, 0.4f, BEIGE);
        // DrawText3D removed for compilation safety.
    }
    else {
        // Draw Standing
        Vector3 drawPos = position;
        
        // Talking Animation: Little jumps
        if (currentState == TALKING) {
            float jump = sinf(GetTime() * 15.0f) * 0.15f; 
            if (jump < 0) jump = -jump; // Bounce
            drawPos.y += jump;
        }

        // Base Cylinder is drawn from center? No, Center.
        // If height is 2, center is at Y=1. 
        // We want feet at Y=0 ? 
        // Position is usually feet in games, but Sim Constructor sets Y=1.0 initially?
        // Let's assume position is Ground point for X,Z.
        Vector3 cylPos = drawPos;
        cylPos.y += 1.0f; // Lift center up
        
        Color drawColor = color;
        if (currentState == WAKING_UP) drawColor = WHITE;

        DrawCylinder(position, 0.5f, 0.5f, 2.0f, 8, drawColor);
        DrawCylinderWires(position, 0.5f, 0.5f, 2.0f, 8, DARKGRAY);
        
        // Head
        Vector3 headPos = { drawPos.x, drawPos.y + 2.0f, drawPos.z };
        DrawSphere(headPos, 0.4f, BEIGE);
        
        // Eyes & Vision
        if (currentState != SLEEPING) {
             // Eyes
            Vector3 fwd = {0,0,1}; 
            if (Vector3Length(velocity) > 0.1f) fwd = Vector3Normalize(velocity);
            else if (conversationPartner) {
                Vector3 dir = Vector3Subtract(conversationPartner->GetPosition(), position);
                if (Vector3Length(dir) > 0.1f) fwd = Vector3Normalize(dir);
            }

            Vector3 right = Vector3CrossProduct(fwd, {0,1,0});
            Vector3 lEye = Vector3Add(headPos, Vector3Scale(fwd, 0.35f)); 
            lEye = Vector3Add(lEye, Vector3Scale(right, -0.12f)); lEye.y += 0.1f;
            Vector3 rEye = Vector3Add(headPos, Vector3Scale(fwd, 0.35f));
            rEye = Vector3Add(rEye, Vector3Scale(right, 0.12f)); rEye.y += 0.1f;
            
            DrawSphere(lEye, 0.05f, BLACK);
            DrawSphere(rEye, 0.05f, BLACK);

            // Vision Cone
            if (currentState == IDLE || currentState == WANDERING || currentState == TALKING) {
                Color visionColor = { 255, 0, 0, 40 }; 
                if (currentState == TALKING) visionColor = { 0, 255, 0, 40 };
                
                float coneLength = 8.0f;
                float baseAngle = atan2f(fwd.x, fwd.z); 
                float halfFov = 45.0f * DEG2RAD / 2.0f;
                Vector3 pLeft = {
                    position.x + sinf(baseAngle - halfFov) * coneLength,
                    0.1f,
                    position.z + cosf(baseAngle - halfFov) * coneLength
                };
                Vector3 pRight = {
                     position.x + sinf(baseAngle + halfFov) * coneLength,
                    0.1f,
                    position.z + cosf(baseAngle + halfFov) * coneLength
                };
                DrawTriangle3D({position.x, 0.1f, position.z}, pLeft, pRight, visionColor);
                DrawTriangle3D({position.x, 0.1f, position.z}, pRight, pLeft, visionColor);
            }
        }
    }
}

int Sim::GetId() const { return id; }
std::string Sim::GetName() const { return name; }
float Sim::GetHunger() const { return hunger; }
float Sim::GetEnergy() const { return energy; }
float Sim::GetAnxiety() const { return anxiety; }
float Sim::GetStress() const { return stress; }
int Sim::GetAge() const { return age; }
float Sim::GetMoney() const { return money; }
Vector3 Sim::GetPosition() const { return position; }

std::string Sim::GetStateString() const {
    switch (currentState) {
        case IDLE: return "Idle";
        case WANDERING: return "Wandering";
        case SLEEPING: return "Sleeping";
        case WAKING_UP: return "Waking Up";
        case EATING: return "Eating";
        case TALKING: return "Talking";
        case WORKING: return "Working";
        case WALKING_TO_BED: return "Go Home";
        case WALKING_TO_WORK: return "Go Work";
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

