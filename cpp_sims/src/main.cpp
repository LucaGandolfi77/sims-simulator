#include "raylib.h"
#include "raymath.h"
#include "Sim.hpp"
#include "World.hpp"
#include "Map.hpp" // Added Map.hpp
#include <vector>
#include <ctime>
#include <cstdlib>
#include <string>

#ifdef __APPLE__
#include <mach/mach.h>
#endif

// Helper to get RAM usage
long GetMemoryUsage() {
#ifdef __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &infoCount) == KERN_SUCCESS) {
        return (long)info.resident_size;
    }
#endif
    return 0;
}

// Function to draw the large checkerboard ground
void DrawCheckerboardGround() {
    const int gridSize = 50; // 50x50 tiles, centered
    const float tileSize = 2.0f; 
    
    // Draw from -50 to 50
    for(int x = -gridSize; x < gridSize; x++) {
        for(int z = -gridSize; z < gridSize; z++) {
            Vector3 pos = { (float)x * tileSize + tileSize/2, 0.0f, (float)z * tileSize + tileSize/2 };
            Color col = ((x + z) % 2 == 0) ? DARKGREEN : LIME; 
            
            // Draw plane tile
            DrawCube(pos, tileSize, 0.1f, tileSize, col);
        }
    }
    
    // Draw Roads (as overrides)
    // Center Cross
    DrawCube((Vector3){0, 0.11f, 0}, 200.0f, 0.1f, 6.0f, GRAY); // X-Axis Road
    DrawCube((Vector3){0, 0.11f, 0}, 6.0f, 0.1f, 200.0f, GRAY); // Z-Axis Road
    
    // Ring Road (Radius 30, Width 6)
    // Top
    DrawCube((Vector3){0, 0.11f, 30.0f}, 66.0f, 0.1f, 6.0f, GRAY); 
    // Bottom
    DrawCube((Vector3){0, 0.11f, -30.0f}, 66.0f, 0.1f, 6.0f, GRAY);
    // Left
    DrawCube((Vector3){-30.0f, 0.11f, 0}, 6.0f, 0.1f, 66.0f, GRAY);
    // Right
    DrawCube((Vector3){30.0f, 0.11f, 0}, 6.0f, 0.1f, 66.0f, GRAY);
}

void DrawTrees(const std::vector<Tree>& trees) {
    for(const auto& tree : trees) {
        // Trunk
        DrawCylinder(tree.position, 0.5f, 0.5f, 3.0f, 8, BROWN);
        
        // Leaves (Darker Green)
        Vector3 leavesPos = { tree.position.x, tree.position.y + 2.5f, tree.position.z };
        Color leavesColor = { 0, 100, 0, 255 }; // Custom Dark Green
        DrawSphere(leavesPos, 2.0f, leavesColor);
        
        // Fruits (Simple small red spheres)
        for(int i = 0; i < tree.fruitCount; i++) {
            float angle = (float)i * (360.0f / 5.0f);
            float rad = angle * (3.14159f / 180.0f);
            // Distribute around the leaves
            Vector3 fruitPos = {
                leavesPos.x + cosf(rad) * 1.5f,
                leavesPos.y - 0.5f, // Hang a bit low
                leavesPos.z + sinf(rad) * 1.5f
            };
            DrawSphere(fruitPos, 0.3f, RED);
        }
    }
}

// Function to draw Houses
void DrawHouses(const std::vector<House>& houses) {
    Color wallColor = { 200, 200, 255, 60 }; // Light Blue Transparent, more transparent
    Color outlineColor = BLUE;
    float wallThick = 0.5f;

    for (const auto& h : houses) {
        float x = h.rect.x;
        float z = h.rect.y; 
        float w = h.rect.width; // width (x-axis)
        float l = h.rect.height; // length (z-axis)
        float hgt = h.height;
        
        // Center position of the house footprint
        float centerX = x + w/2;
        float centerZ = z + l/2;
        float centerY = hgt/2;

        // Draw 4 walls individually to perform "no roof"
        
        // Wall 1 (Back, along X)
        Vector3 posBack = { centerX, centerY, z };
        DrawCube(posBack, w, hgt, wallThick, wallColor);
        DrawCubeWires(posBack, w, hgt, wallThick, outlineColor);
        
        // Wall 2 (Front, along X)
        Vector3 posFront = { centerX, centerY, z + l };
        DrawCube(posFront, w, hgt, wallThick, wallColor);
        DrawCubeWires(posFront, w, hgt, wallThick, outlineColor);
        
        // Wall 3 (Left, along Z)
        Vector3 posLeft = { x, centerY, centerZ };
        DrawCube(posLeft, wallThick, hgt, l, wallColor);
        DrawCubeWires(posLeft, wallThick, hgt, l, outlineColor);
        
        // Wall 4 (Right, along Z)
        Vector3 posRight = { x + w, centerY, centerZ };
        DrawCube(posRight, wallThick, hgt, l, wallColor);
        DrawCubeWires(posRight, wallThick, hgt, l, outlineColor);
        
        // Floor (Optional, but looks nice to define the zone)
        DrawCube({centerX, 0.15f, centerZ}, w, 0.1f, l, Fade(BLUE, 0.3f));
    }
}

void DrawOffice(const Office& off) {
    Color wallColor = { 100, 100, 100, 80 }; // Gray Transparent
    Color outlineColor = DARKGRAY;
    float wallThick = 0.5f;
    float hgt = 8.0f; // Office is tall

    if (off.rect.width == 0) return; // Safety

    float x = off.rect.x;
    float z = off.rect.y;
    float w = off.rect.width;
    float l = off.rect.height;
    
    float centerX = x + w/2;
    float centerZ = z + l/2;
    float centerY = hgt/2;

    // Walls (Gap for door? Keep it simple)
    // Wall 1 (Back)
    DrawCube({centerX, centerY, z}, w, hgt, wallThick, wallColor);
    DrawCubeWires({centerX, centerY, z}, w, hgt, wallThick, outlineColor);
    // Wall 2 (Front)
    DrawCube({centerX, centerY, z + l}, w, hgt, wallThick, wallColor);
    DrawCubeWires({centerX, centerY, z + l}, w, hgt, wallThick, outlineColor);
    // Wall 3 (Left)
    DrawCube({x, centerY, centerZ}, wallThick, hgt, l, wallColor);
    DrawCubeWires({x, centerY, centerZ}, wallThick, hgt, l, outlineColor);
    // Wall 4 (Right)
    DrawCube({x + w, centerY, centerZ}, wallThick, hgt, l, wallColor);
    DrawCubeWires({x + w, centerY, centerZ}, wallThick, hgt, l, outlineColor);
    
    // Desks inside
    for (const auto& desk : off.desks) {
        DrawCube(desk, 2.0f, 1.0f, 1.0f, BROWN); // Desk
        DrawCube({desk.x, 1.0f, desk.z}, 0.5f, 0.2f, 0.5f, BLACK); // Computer Screen logic?
    }
}

int main() {
    // Initialization
    const int screenWidth = 800;
    const int screenHeight = 600;

    InitWindow(screenWidth, screenHeight, "Sims C++ Simulator (Mac M1)");

    // Define the camera
    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 50.0f, 50.0f };  // Higher view for larger map
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };      // Looking at center
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    SetTargetFPS(60);

    // Seed Random
    std::srand(std::time(nullptr));

    // Create World Elements
    std::vector<Tree> trees;
    std::vector<House> houses;

    // Initialize Map from Map.hpp
    InitializeMap(trees, houses);

    // Create Sims
    std::vector<Sim> sims;
    sims.emplace_back("Alex", (Vector3){ 0.0f, 1.0f, 0.0f }, RED);
    sims.emplace_back("Bob", (Vector3){ 5.0f, 1.0f, -5.0f }, BLUE);
    sims.emplace_back("Charlie", (Vector3){ -5.0f, 1.0f, 5.0f }, GREEN);
    sims.emplace_back("Diana", (Vector3){ 3.0f, 1.0f, 3.0f }, YELLOW);
    sims.emplace_back("Edward", (Vector3){ -3.0f, 1.0f, -3.0f }, PURPLE);
    // 5 New Sims
    sims.emplace_back("Fiona", (Vector3){ 10.0f, 1.0f, 10.0f }, ORANGE);
    sims.emplace_back("George", (Vector3){ -10.0f, 1.0f, -10.0f }, BROWN);
    sims.emplace_back("Hannah", (Vector3){ 15.0f, 1.0f, -5.0f }, PINK);
    sims.emplace_back("Ian", (Vector3){ -12.0f, 1.0f, 4.0f }, LIME);
    sims.emplace_back("Julia", (Vector3){ 8.0f, 1.0f, 8.0f }, MAGENTA);

    bool showHUD = true;
    
    // Day Night Cycle
    float timeOfDay = 0.3f; // Start at morning
    float timeSpeedMultiplier = 1.0f;

    // Main game loop
    while (!WindowShouldClose()) {      // Detect window close button or ESC key
        // Update
        float realDt = GetFrameTime();
        
        // Time Speed Controls
        if (IsKeyPressed(KEY_ONE)) timeSpeedMultiplier = 1.0f;
        if (IsKeyPressed(KEY_TWO)) timeSpeedMultiplier = 2.0f;
        if (IsKeyPressed(KEY_THREE)) timeSpeedMultiplier = 4.0f;
        if (IsKeyPressed(KEY_FOUR)) timeSpeedMultiplier = 8.0f;
        
        float dt = realDt * timeSpeedMultiplier;
        
        // Update Time of Day (2 Minutes per day)
        timeOfDay += (dt / 120.0f); 
        if (timeOfDay >= 1.0f) timeOfDay -= 1.0f;

        if (IsKeyPressed(KEY_H)) {
            showHUD = !showHUD;
        }

        // Update World (Trees)
        for(auto& tree : trees) {
            if (tree.fruitCount < 5) {
                tree.regenTimer += dt;
                if (tree.regenTimer >= 15.0f) {
                    tree.fruitCount++;
                    tree.regenTimer = 0; // Reset timer
                }
            }
        }

        // --- Custom Camera Controls ---
        // Zoom
        float wheel = GetMouseWheelMove();
        if (wheel != 0) {
            // Move camera forward/backward along view factor
            Vector3 view = Vector3Subtract(camera.target, camera.position);
            // Don't zoom too close
            if (Vector3Length(view) > 10.0f || wheel < 0) {
                 camera.position = Vector3Add(camera.position, Vector3Scale(Vector3Normalize(view), wheel * 2.0f));
            }
        }
        
        // Pan (WASD) - Move both Position and Target to keep angle
        float panSpeed = 20.0f * dt; // Faster pan for larger world
        if (IsKeyDown(KEY_W)) {
            camera.position.z -= panSpeed;
            camera.target.z -= panSpeed;
        }
        if (IsKeyDown(KEY_S)) {
            camera.position.z += panSpeed;
            camera.target.z += panSpeed;
        }
        if (IsKeyDown(KEY_A)) {
            camera.position.x -= panSpeed;
            camera.target.x -= panSpeed;
        }
        if (IsKeyDown(KEY_D)) {
            camera.position.x += panSpeed;
            camera.target.x += panSpeed;
        }
        
        // Update Sims
        for (auto& sim : sims) {
            sim.Update(dt, trees, sims, houses, office);
        }

        // Calculate Sun/Sky
        // 0.0=Night, 0.25=Sunrise, 0.5=Noon, 0.75=Sunset
        Color skyColor = BLUE;
        float lightIntensity = 1.0f;
        
        if (timeOfDay < 0.2f || timeOfDay > 0.8f) {
            skyColor = { 10, 10, 40, 255 }; // Deep Night
            lightIntensity = 0.4f; // Dim but visible
        } else if (timeOfDay < 0.3f) {
            // Sunrise
            skyColor = ColorLerp({ 10, 10, 40, 255 }, SKYBLUE, (timeOfDay - 0.2f) * 10.0f);
        } else if (timeOfDay > 0.7f) {
            // Sunset
            skyColor = ColorLerp(SKYBLUE, { 10, 10, 40, 255 }, (timeOfDay - 0.7f) * 10.0f);
        } else {
            skyColor = SKYBLUE;
        }

        // Draw
        BeginDrawing();
            ClearBackground(skyColor);

            BeginMode3D(camera);
                
                // Draw World
                Color grndColor = WHITE; 
                if (lightIntensity < 1.0f) grndColor = ColorTint(WHITE, {150, 150, 180, 255}); // Dim ground at night
                
                DrawCheckerboardGround(); // (Should update this to take color if possible, effectively lighting)
                
                DrawOffice(office);
                DrawHouses(houses);
                DrawTrees(trees);

                // Draw Sims with "Shadows"
                Vector3 sunPos = { 0, 100, 0 }; // Approx
                // Move sun based on time
                float timeAngle = (timeOfDay - 0.25f) * 2.0f * PI; // Noon at top
                sunPos.x = cosf(timeAngle) * 100.0f;
                sunPos.y = sinf(timeAngle) * 100.0f;
                if (sunPos.y < 0) sunPos.y = 0; // Don't go below ground for shadow calc
                
                Vector3 shadowOffset = Vector3Scale(Vector3Normalize(sunPos), -0.8f); // Offset opposite to sun
                shadowOffset.y = 0; // Flat on ground

                for (auto& sim : sims) {
                    // Simple Shadow: Flattened black cylinder offset
                    if (lightIntensity > 0.5f) {
                        Vector3 sPos = Vector3Add(sim.GetPosition(), shadowOffset);
                        sPos.y = 0.05f; // Just above ground
                        DrawCylinder(sPos, 0.5f, 0.5f, 0.1f, 8, {0, 0, 0, 100});
                    }
                    sim.Draw();
                }

                // Draw Sun
                if (sunPos.y > 0) { // Visible?
                    DrawSphere(Vector3Scale(Vector3Normalize(sunPos), 90.0f), 5.0f, YELLOW);
                }

            EndMode3D();

            if (showHUD) {
                // UI / HUD
                DrawText("Controls: WASD=Pan, Scroll=Zoom, 1-4=Speed, H=Safe", 10, 5, 20, DARKGRAY);
                
                // Clock
                int hour = (int)(timeOfDay * 24.0f);
                int minute = (int)((timeOfDay * 24.0f - hour) * 60.0f);
                char clockText[64];
                snprintf(clockText, sizeof(clockText), "Time: %02d:%02d (x%.0f)", hour, minute, timeSpeedMultiplier);
                DrawText(clockText, screenWidth/2 - 50, 10, 20, BLACK);


                DrawText("Sims Status:", 10, 30, 20, BLACK);
                
                int yPos = 60;
                for (auto& sim : sims) {
                     // Info string with abbreviated stats
                    std::string info = "[" + std::to_string(sim.GetId()) + "] " + sim.GetName() + " (" + sim.GetStateString() + ")";
                    char stats[128];
                    snprintf(stats, sizeof(stats), "H:%.0f E:%.0f A:%.0f S:%.0f Age:%d $%.0f", 
                        sim.GetHunger(), sim.GetEnergy(), sim.GetAnxiety(), sim.GetStress(), sim.GetAge(), sim.GetMoney());
                    
                    DrawText(info.c_str(), 10, yPos, 20, BLACK);
                    DrawText(stats, 300, yPos, 20, DARKGRAY); // Stats in Gray to differentiate
                    
                    yPos += 30;
                }

                // Legend
                int legX = screenWidth - 150;
                int legY = screenHeight - 110;
                DrawRectangle(legX - 10, legY - 10, 160, 120, Fade(WHITE, 0.8f));
                DrawRectangleLines(legX - 10, legY - 10, 160, 120, GRAY);
                DrawText("LEGEND:", legX, legY, 20, BLACK);
                DrawText("H: Hunger", legX, legY + 20, 10, BLACK);
                DrawText("E: Energy", legX, legY + 35, 10, BLACK);
                DrawText("A: Anxiety", legX, legY + 50, 10, BLACK);
                DrawText("S: Stress", legX, legY + 65, 10, BLACK);
                DrawText("$: Simoleons", legX, legY + 80, 10, BLACK);

                // Performance
                DrawFPS(screenWidth - 100, 10);
                
                long mem = GetMemoryUsage();
                char memText[64];
                snprintf(memText, sizeof(memText), "RAM: %.1f MB", (float)mem / 1024.0f / 1024.0f);
                DrawText(memText, screenWidth - 140, 35, 20, DARKGREEN);
            }

        EndDrawing();
    }

    // De-Initialization
    CloseWindow();        // Close window and OpenGL context

    return 0;
}
