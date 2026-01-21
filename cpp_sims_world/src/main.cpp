#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>

// Include our headers
#include "World.hpp"
#include "Sim.hpp"

#ifdef __APPLE__
#include <mach/mach.h>
#endif

// --- global stats ---
long GetMemoryUsage() {
#ifdef __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS) {
        return (long)info.resident_size;
    }
#endif
    return 0; // Windows/Linux placeholders usually
}

// Global World Data
std::vector<City> nation;
SimDatabase simDB; // Use the Database wrapper

void GenerateNation() {
    nation.clear();
    simDB.allSims.clear(); // Clear vector inside DB

    const int NUM_CITIES = 5;
    const int NUM_SIMS = 200; // Increased to 200 Sims 
    
    // Create Cities
    for(int i=0; i<NUM_CITIES; i++) {
        City c;
        c.name = "City " + std::to_string(i + 1);
        
        // Scatter cities on a large map (500x500 area)
        c.center = { GetRandomFloat(-400, 400), 0, GetRandomFloat(-400, 400) };
        c.radius = GetRandomFloat(40, 80);
        
        // Generate Buildings in City
        int numBuildings = 30 + rand() % 20;
        for (int b=0; b<numBuildings; b++) {
            Building build;
            build.id = i * 1000 + b;
            
            // Random pos inside city radius
            float angle = GetRandomFloat(0, 360);
            float dist = GetRandomFloat(5, c.radius);
            build.position.x = c.center.x + cosf(angle) * dist;
            build.position.z = c.center.z + sinf(angle) * dist;
            build.position.y = 0; // Ground

            // 70% House, 30% Work
            build.isWorkplace = (rand() % 100) < 30;
            
            if (build.isWorkplace) {
                build.width = GetRandomFloat(8, 15);
                build.length = GetRandomFloat(8, 15);
                build.height = GetRandomFloat(10, 30); // Tall offices
                build.color = DARKGRAY;
            } else {
                build.width = GetRandomFloat(4, 6);
                build.length = GetRandomFloat(4, 6);
                build.height = GetRandomFloat(3, 6); // Small houses
                build.color = LIGHTGRAY; // Corrected lowercase
            }
            // Align Y to sit on ground
            build.position.y = build.height / 2.0f;

            c.buildings.push_back(build);
        }
        nation.push_back(c);
    }
    
    // Collect all valid homes/workplaces for assignment
    std::vector<Building*> allHomes;
    std::vector<Building*> allWorkplaces;
    
    for(auto& c : nation) {
        for(auto& b : c.buildings) {
            if (b.isWorkplace) allWorkplaces.push_back(&b);
            else allHomes.push_back(&b);
        }
    }

    if (allHomes.empty() || allWorkplaces.empty()) {
        std::cerr << "Error: Generation failed, not enough buildings." << std::endl;
        exit(1);
    }

    // Create Sims
    for(int i=0; i<NUM_SIMS; i++) {
        Building* home = allHomes[rand() % allHomes.size()];
        Building* work = allWorkplaces[rand() % allWorkplaces.size()];
        
        // Sim ctor requires id, name, home, work
        Sim s(i, "Sim " + std::to_string(i), home, work);
        simDB.AddSim(s);
    }
}

int main() {
    InitWindow(1280, 720, "Nation Simulator C++");
    SetTargetFPS(60);
    
    srand(time(NULL));
    GenerateNation();

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 100.0f, 100.0f };
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        float timeScale = 2.0f; // Speed up simulation

        // Mouse Wheel Zoom
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            Vector3 view = Vector3Subtract(camera.target, camera.position);
            // Move closer/further
            camera.position = Vector3Add(camera.position, Vector3Scale(Vector3Normalize(view), wheel * 10.0f));
        }

        // Camera Pan using Forward/Right vectors relative to camera view
        // This makes movement feel like "flying" over the map rather than just moving coordinates blindly
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D)) {
            float panSpeed = 100.0f * dt;
            // Adjust pan speed based on height (Zoom level)
            panSpeed *= (camera.position.y / 50.0f); 

            // Calculate Forward vector on XZ plane
            Vector3 forward = Vector3Subtract(camera.target, camera.position);
            forward.y = 0; // Flatten
            forward = Vector3Normalize(forward);
            
            // Calculate Right vector
            Vector3 right = Vector3CrossProduct(forward, camera.up);
            
            Vector3 move = {0,0,0};

            if (IsKeyDown(KEY_W)) move = Vector3Add(move, forward);
            if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, forward);
            if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);
            if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);
            
            // Apply movement to both Position and Target (Pan)
            move = Vector3Scale(Vector3Normalize(move), panSpeed);
            
            camera.position = Vector3Add(camera.position, move);
            camera.target = Vector3Add(camera.target, move);
        }
        
        // Update Logic
        for(auto& sim : simDB.allSims) {
            sim.Update(dt * timeScale);
        }
        
        // Update Stats
        Sim::UpdateLists(simDB.allSims);

        // Draw
        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(camera);
            
            // Draw Ground Plane (Massive)
            DrawPlane({0,0,0}, {2000.0f, 2000.0f}, (Color){200, 230, 200, 255}); // Light Green ground

            // Identify visible range (simple distance check from camera target)
            // If camera is very high (Satellite view), draw cities as Blobs
            bool satelliteView = camera.position.y > 300.0f;

            for(const auto& city : nation) {
                float distToCity = Vector3Distance(city.center, camera.target);
                
                // LOD: If satellite view, just draw city center
                if (satelliteView) {
                    DrawCylinder(city.center, city.radius, city.radius, 1.0f, 10, Fade(GRAY, 0.5f));
                    DrawSphere(city.center, 5.0f, RED);
                } else {
                    // Draw Buildings if closer
                    // Simple Frustum culling approximation: check distance
                    if (distToCity < 600.0f) { // Render city contents if somewhat close
                        // Draw Roads (Simple lines from City Center to buildings? Or just implicit)
                        // Let's draw a "Region" circle
                        DrawCircle3D(city.center, city.radius, {0,0,0}, 90.0f, Fade(LIGHTGRAY, 0.3f));

                        // Buildings
                        for(const auto& b : city.buildings) {
                            if (Vector3Distance(b.position, camera.position) < 400.0f) {
                                Color bCol = b.isWorkplace ? BLUE : ORANGE;
                                if (b.isWorkplace) bCol = SKYBLUE;
                                DrawCube(b.position, b.width, b.height, b.length, bCol);
                                DrawCubeWires(b.position, b.width, b.height, b.length, DARKGRAY);
                            }
                        }
                    }
                }
            }

            // Draw Sims & Cars
            for(auto& sim : simDB.allSims) {
                // Sim's Draw handles its own LOD
                sim.Draw(camera.position);
            }

        EndMode3D();

        // UI / HUD
        DrawRectangle(0, 0, 300, 220, Fade(BLACK, 0.7f));
        DrawFPS(10, 10);
        
        long mem = GetMemoryUsage();
        DrawText(TextFormat("Memory: %.2f MB", mem / 1024.0f / 1024.0f), 10, 40, 20, WHITE);
        
        DrawText("NATION SIMULATOR", 10, 70, 20, YELLOW);
        DrawText(TextFormat("Total Sims: %d", (int)simDB.allSims.size()), 10, 100, 20, WHITE);
        DrawText(TextFormat("- In Car (Driving): %d", (int)Sim::simsInCar.size()), 20, 130, 20, GREEN);
        DrawText(TextFormat("- At Work: %d", (int)Sim::simsAtWork.size()), 20, 155, 20, BLUE);
        DrawText(TextFormat("- At Home/Sleep: %d", (int)Sim::simsAtHome.size()), 20, 180, 20, ORANGE);

        DrawText("Controls: WASD to Pan, Mouse Wheel to Zoom", 320, 680, 20, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
