#pragma once
#include "raylib.h" // Using raylib for vector math and colors
#include <string>

#include "World.hpp" // For Tree/House definition
#include <vector>

class Sim {
public:
    enum State { IDLE, WANDERING, SLEEPING, WAKING_UP, EATING, TALKING, WORKING, WALKING_TO_BED, WALKING_TO_WORK };

    Sim(std::string name, Vector3 position, Color color);
    
    // Main update loop for the character's brain and physics
    void Update(float deltaTime, std::vector<Tree>& trees, std::vector<Sim>& allSims, const std::vector<House>& houses, const Office& office);
    
    // Visualization
    void Draw();
    
    // Getters for UI/Debug
    int GetId() const;
    std::string GetName() const;
    std::string GetStateString() const;
    float GetHunger() const;
    float GetEnergy() const;
    float GetAnxiety() const;
    float GetStress() const;
    int GetAge() const;
    float GetMoney() const;
    Vector3 GetPosition() const;
    
    // Helpers for interaction
    bool IsAvailableForTalk() const;
    void StartTalking(Sim* partner);

private:
    static int nextId;
    int id;
    std::string name;
    int age;
    float money;

    Vector3 position;
    Vector3 targetPosition;
    Vector3 velocity; // Current movement direction
    Color color;
    State currentState;
    
    // Interaction
    Sim* conversationPartner;
    float talkCooldown;
    float globalTime;

    // Stats (0-100)
    float hunger;  // 0 = full, 100 = starving
    float energy;  // 100 = energetic, 0 = exhausted
    float anxiety; // 0 = calm, 100 = panic
    float stress;  // 0 = relaxed, 100 = stressed out
    
    // Timers
    float decisionTimer; 
    float actionDuration;

    // AI Helpers
    void MakeDecision(const std::vector<House>& houses, const Office& office);
    void PickRandomDestination();
    bool CanSee(const Vector3& targetPos);
};
