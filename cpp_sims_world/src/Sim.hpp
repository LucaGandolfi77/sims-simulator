#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include "World.hpp" // Building struct

class Sim {
public:
    enum State {
        SLEEPING,
        WALKING_TO_CAR,
        DRIVING_TO_WORK,
        WORKING,
        WALKING_FROM_WORK_CAR, // Walking from parked car to office
        DRIVING_HOME,
        WALKING_TO_HOUSE
    };

    Sim(int id, std::string name, Building* home, Building* work);

    void Update(float deltaTime);
    void Draw(Vector3 cameraPos); // Takes camera pos for LOD

    // Getters
    int GetId() const { return id; }
    std::string GetName() const { return name; }
    State GetState() const { return currentState; }
    float GetMoney() const { return money; }
    int GetAge() const { return age; }
    float GetSleep() const { return sleep; } // 0-100

    // Static lists for global tracking
    static std::vector<Sim*> simsAtHome;
    static std::vector<Sim*> simsAtWork;
    static std::vector<Sim*> simsInCar;

    static void UpdateLists(const std::vector<Sim>& allSims);

private:
    int id;
    std::string name;
    int age;
    float money;
    float sleep; // 100 = full energy, 0 = exhausted

    // References to world objects
    Building* home;
    Building* workplace;

    // Physics / Transform
    Vector3 position;
    Vector3 velocity;
    Color bodyColor;
    
    // Car properties
    Color carColor;
    Vector3 carPosition; // Sim might be in car, car moves with sim

    State currentState;
    float actionTimer; // Helper for delays (working duration etc)

    // Movement Helpers
    void MoveTo(Vector3 target, float speed, float dt);
    bool Reached(Vector3 target);
};

// Database Helper to manage Sims (Moved here to have complete Sim type)
class SimDatabase {
public:
    std::vector<Sim> allSims; // Vector is now happy because Sim is defined above

    void AddSim(const Sim& sim) {
        allSims.push_back(sim);
    }

    Sim* GetSimById(int id) {
        if (id >= 0 && id < allSims.size()) return &allSims[id];
        return nullptr;
    }
};
