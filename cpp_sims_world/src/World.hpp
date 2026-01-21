#pragma once
#include "raylib.h"
#include <vector>
#include <string>

struct Building {
    int id;
    Vector3 position;
    Color color;
    bool isWorkplace; // true = workplace, false = house
    // Dimensions
    float width;
    float length;
    float height;
};

struct City {
    std::string name;
    Vector3 center;
    float radius;
    std::vector<Building> buildings;
};

// Global helper for random float
inline float GetRandomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}
