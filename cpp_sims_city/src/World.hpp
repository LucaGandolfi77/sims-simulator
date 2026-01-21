#pragma once
#include "raylib.h"
#include <vector>
#include <cmath>

struct Tree {
    Vector3 position;
    int fruitCount;
    float regenTimer;
    
    Tree(Vector3 pos) : position(pos), fruitCount(5), regenTimer(0.0f) {}
};

struct House {
    Rectangle rect; // x, z, width, length
    float height;
    
    House(float x, float z, float w, float l) : rect{x, z, w, l}, height(4.0f) {}
};

struct Office {
    Rectangle rect;
    std::vector<Vector3> desks;
    
    Office() : rect{0,0,0,0} {}
};

// Start ColorLerp Helper
inline Color ColorLerp(Color c1, Color c2, float t) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return (Color){
        (unsigned char)(c1.r + (int)(t * (c2.r - c1.r))),
        (unsigned char)(c1.g + (int)(t * (c2.g - c1.g))),
        (unsigned char)(c1.b + (int)(t * (c2.b - c1.b))),
        255
    };
}
// End Helper

// Check if a position is on the road network
// Design: A central cross road (X and Z axes) and a ring road
inline bool IsOnRoad(Vector3 pos) {
    // Check Central Cross
    // Road width 6.0f (Running from -3 to 3 on the axis)
    const float roadWidth = 3.0f; 
    
    // Axis roads
    bool onAxisX = (std::abs(pos.z) < roadWidth); // Road running along X axis
    bool onAxisZ = (std::abs(pos.x) < roadWidth); // Road running along Z axis
    
    if (onAxisX || onAxisZ) return true;
    
    // Ring road roughly at distance 30 from center, width 4
    const float ringRadius = 30.0f;
    const float ringWidth = 4.0f;
    
    // Simple square ring logic for grid world
    bool inRingX = (std::abs(pos.x) > (ringRadius - ringWidth)) && (std::abs(pos.x) < (ringRadius + ringWidth));
    bool inRingZ = (std::abs(pos.z) > (ringRadius - ringWidth)) && (std::abs(pos.z) < (ringRadius + ringWidth));
    
    // For a square ring, we are on the road if we are within the ranges
    // If x is within ring bands, z can be anywhere within outer bounds, and vice versa?
    // Let's simplify: A road outline.
    // X bands
    if (inRingX && (std::abs(pos.z) < ringRadius + ringWidth)) return true;
    // Z bands
    if (inRingZ && (std::abs(pos.x) < ringRadius + ringWidth)) return true;

    return false;
}
