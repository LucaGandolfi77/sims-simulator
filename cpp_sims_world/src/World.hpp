#pragma once
#include "raylib.h"
#include <vector>
#include <string>

struct Building {
    int id;
    int cityId;
    Vector3 position;
    float width;
    float length;
    float height;
    // replaced bool isWorkplace with type
    enum Type { HOUSE = 0, OFFICE, SCHOOL, UNIVERSITY, PARK } type; 
    bool isWorkplace() const { return type == OFFICE || type == SCHOOL || type == UNIVERSITY || type == PARK; } // Helper for old checks
    Color color;
};

struct Road {
    Vector3 start;
    Vector3 end;
    float width;
};

struct City {
    std::string name;
    Vector3 center;
    float radius;
    std::vector<Building> buildings;
    std::vector<Road> localRoads;
    std::vector<Road> highwayRing; // New: Ring around city
    // Exits for pathfinding
    Vector3 exitNorth;
    Vector3 exitSouth;
    Vector3 exitEast;
    Vector3 exitWest;
};

// Shared data available to all units
extern std::vector<City> nation;
extern std::vector<Road> highways; // Roads connecting cities
