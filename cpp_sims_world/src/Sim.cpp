#include "Sim.hpp"
#include "raymath.h"
#include <iostream>

// Define static members
std::vector<Sim*> Sim::simsAtHome;
std::vector<Sim*> Sim::simsAtWork;
std::vector<Sim*> Sim::simsInCar;

Sim::Sim(int id, std::string name, Building* home, Building* work) 
    : id(id), name(name), home(home), workplace(work) {
    
    age = 18 + rand() % 50;
    money = 100.0f + (rand() % 1000);
    sleep = 50.0f + (rand() % 50); // Start reasonably rested

    // Start at home
    currentState = SLEEPING;
    position = home->position; // Inside house
    
    // Random colors
    bodyColor = (Color){ (unsigned char)GetRandomValue(50, 255), (unsigned char)GetRandomValue(50, 255), (unsigned char)GetRandomValue(50, 255), 255 };
    carColor = (Color){ (unsigned char)GetRandomValue(20, 200), (unsigned char)GetRandomValue(20, 200), (unsigned char)GetRandomValue(20, 200), 255 };
    
    actionTimer = 0.0f;
}

void Sim::UpdateLists(const std::vector<Sim>& allSims) {
    simsAtHome.clear();
    simsAtWork.clear();
    simsInCar.clear();

    // Note: iterating const vector requires us to cast const away if we store mutable pointers
    // OR we just store const pointers. For this simple sim, we'll const_cast carefully 
    // or just change signature. Let's just use the enum check.
    
    for (const auto& sim : allSims) {
        // We need non-const pointers for the static lists usually, 
        // but let's just cheat slightly for the UI display logic or use const_cast
        Sim* s = const_cast<Sim*>(&sim);

        if (s->currentState == SLEEPING || s->currentState == WALKING_TO_HOUSE) {
            simsAtHome.push_back(s);
        } else if (s->currentState == WORKING || s->currentState == WALKING_FROM_WORK_CAR) {
            simsAtWork.push_back(s);
        } else if (s->currentState == DRIVING_TO_WORK || s->currentState == DRIVING_HOME) {
            simsInCar.push_back(s);
        }
    }
}

void Sim::MoveTo(Vector3 target, float speed, float dt) {
    Vector3 dir = Vector3Subtract(target, position);
    float dist = Vector3Length(dir);
    if (dist > 0.1f) {
        dir = Vector3Normalize(dir);
        position = Vector3Add(position, Vector3Scale(dir, speed * dt));
    } else {
        position = target;
    }
}

bool Sim::Reached(Vector3 target) {
    return Vector3Distance(position, target) < 0.5f;
}

void Sim::Update(float deltaTime) {
    // Stats decay/gran
    if (currentState == SLEEPING) {
        sleep += 10.0f * deltaTime;
        if (sleep > 100) sleep = 100;
    } else {
        sleep -= 0.5f * deltaTime; // Get tired over time
        if (sleep < 0) sleep = 0;
    }

    if (currentState == WORKING) {
        money += 5.0f * deltaTime; // Earn money
    }

    // State Machine
    switch (currentState) {
        case SLEEPING:
            // Wake up if rested and it's time (simulated by sleep > 90 for loop)
            if (sleep >= 100.0f) {
                currentState = WALKING_TO_CAR;
            }
            break;

        case WALKING_TO_CAR: {
            // "Car" is parked roughly outside the house
            Vector3 carParkPos = Vector3Add(home->position, {2.0f, 0.0f, 2.0f});
            MoveTo(carParkPos, 5.0f, deltaTime); // Walking speed 5
            if (Reached(carParkPos)) {
                currentState = DRIVING_TO_WORK;
            }
            break;
        }

        case DRIVING_TO_WORK: {
            Vector3 workParkPos = Vector3Add(workplace->position, {-2.0f, 0.0f, -2.0f});
            MoveTo(workParkPos, 30.0f, deltaTime); // Driving speed 30!
            if (Reached(workParkPos)) {
                currentState = WALKING_FROM_WORK_CAR;
            }
            break;
        }

        case WALKING_FROM_WORK_CAR: {
            MoveTo(workplace->position, 5.0f, deltaTime);
            if (Reached(workplace->position)) {
                currentState = WORKING;
                actionTimer = 20.0f; // Work for 20 seconds (simulated hours)
            }
            break;
        }

        case WORKING:
            actionTimer -= deltaTime;
            if (actionTimer <= 0 || sleep < 20.0f) { // Go home if shift done or too tired
                currentState = WALKING_TO_CAR; // Reuse 'Walking to car' state logic? 
                                             // It points to Home car park? No. 
                                             // Need specific transition.
                // Resetting to explicit logic:
                currentState = DRIVING_HOME; 
                // Teleport to car start pos for simplicity or add WALKING_TO_CAR_AT_WORK state.
                // Let's assume car is right outside.
            }
            break;

        case DRIVING_HOME: {
            Vector3 homeParkPos = Vector3Add(home->position, {2.0f, 0.0f, 2.0f});
            MoveTo(homeParkPos, 30.0f, deltaTime);
            if (Reached(homeParkPos)) {
                currentState = WALKING_TO_HOUSE;
            }
            break;
        }

        case WALKING_TO_HOUSE: {
            MoveTo(home->position, 5.0f, deltaTime);
            if (Reached(home->position)) {
                currentState = SLEEPING;
            }
            break;
        }
    }
}

void Sim::Draw(Vector3 cameraPos) {
    float dist = Vector3Distance(position, cameraPos);
    
    // LOD 0: Too far, draw nothing or single pixel (handled by main usually, but self-culling here)
    if (dist > 800.0f) return; // Increased view distance slightly for small dots

    if (currentState == DRIVING_TO_WORK || currentState == DRIVING_HOME) {
        // Draw Car as short rectangle (box)
        // Shorter height (0.5f), standard length/width
        DrawCube(position, 2.0f, 0.5f, 3.5f, carColor);
        DrawCubeWires(position, 2.0f, 0.5f, 3.5f, DARKGRAY);
    } else {
        // Always draw sims as spheres (pallini)
        // Position is usually their feet in Sim logic, so lift sphere up by radius
        DrawSphere({position.x, position.y + 0.5f, position.z}, 0.5f, bodyColor);
    }
}

