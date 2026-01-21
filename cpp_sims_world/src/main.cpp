#include "raylib.h"
#include "raymath.h"
#include <entt/entt.hpp>
#include <vector>
#include <iostream>
#include <string>
#include <cmath>
#include <map>

#include "World.hpp"
#include "Components.hpp"
#include "Systems.hpp"

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
    return 0; 
}

std::vector<City> nation;
std::vector<Road> highways;
entt::registry registry; 

int countDriving = 0;
int countWorking = 0;
int countSleeping = 0;

float GetRandomFloat(float min, float max) {
    return min + (float)rand() / (float)(RAND_MAX / (max - min));
}

void GenerateWorld(int numSims) {
    nation.clear();
    highways.clear();
    registry.clear(); // Clear ECS

    const int NUM_CITIES = 12;      // More cities
    const float MAP_SIZE = 4000.0f; // Much bigger map
    
    // Names Data
    const std::vector<std::string> firstNames = { "Mario", "Luigi", "Giovanni", "Paolo", "Francesca", "Laura", "Anna", "Marco", "Antonio", "Giulia", "Sofia", "Luca", "Matteo", "Alessandro", "Federico", "Chiara", "Silvia", "Elena" };
    const std::vector<std::string> lastNames = { "Rossi", "Bianchi", "Verdi", "Russo", "Ferrari", "Esposito", "Romano", "Gallo", "Colombo", "Ricci", "Moretti", "Barbieri", "Conti", "De Luca", "Mancini" };

    // Need to include set and algorithm
    #include <set>
    #include <algorithm>
    
    // 1. Generate Cities (Grid-like spread to avoid overlap)
    int rows = 3; 
    int cols = 4;
    float cellWidth = MAP_SIZE / cols;
    float cellHeight = MAP_SIZE / rows;

    for(int r=0; r<rows; r++) {
        for(int c=0; c<cols; c++) {
            City city;
            int idx = r * cols + c;
            city.name = "City " + std::to_string(idx + 1);
            
            // Random center within the cell
            float cx = -MAP_SIZE/2 + c * cellWidth + cellWidth/2 + GetRandomFloat(-200, 200);
            float cz = -MAP_SIZE/2 + r * cellHeight + cellHeight/2 + GetRandomFloat(-200, 200);
            city.center = { cx, 0, cz };
            city.radius = 450.0f; // More radius for more houses
            
            // 2. City Layout (Grid)
            // We create a grid of roads and buildings
            // More Density: 10x10 blocks
            int gridRows = 10;
            int gridCols = 10;
            float blockSize = 40.0f; 
            
            // Offset to center the grid on city center
            float startX = city.center.x - (gridCols * blockSize) / 2.0f;
            float startZ = city.center.z - (gridRows * blockSize) / 2.0f;

            // Grid Occupancy Map for Road Culling
            // -1 = Empty, otherwise Building ID
            std::vector<std::vector<int>> blockMap(gridRows, std::vector<int>(gridCols, -1));
            
            int currentBuildId = 0;

            // A. Place Buildings (Logic)
            for (int gr=0; gr<gridRows; gr++) {
                for (int gc=0; gc<gridCols; gc++) {
                    if (blockMap[gr][gc] != -1) continue;

                    Building build;
                    build.id = idx * 10000 + gr * 100 + gc;
                    build.cityId = idx;
                    
                    bool tryWorkplace = (rand() % 100) < 30;
                    bool placed = false;

                    // Deciding Special vs House
                    if (tryWorkplace) {
                        // Try 2x2 Expansion for "4 houses size"
                        // Check bounds and occupancy
                        bool canExpand2x2 = (gr + 1 < gridRows) && (gc + 1 < gridCols) &&
                                            (blockMap[gr][gc+1] == -1) &&
                                            (blockMap[gr+1][gc] == -1) &&
                                            (blockMap[gr+1][gc+1] == -1);
                        
                        if (canExpand2x2) {
                            currentBuildId++;
                            blockMap[gr][gc] = currentBuildId;
                            blockMap[gr][gc+1] = currentBuildId;
                            blockMap[gr+1][gc] = currentBuildId;
                            blockMap[gr+1][gc+1] = currentBuildId;

                            // TYPE SELECTION: Office, School, Uni, PARK
                            int pick = rand() % 100;
                            if (pick < 15) build.type = Building::SCHOOL;
                            else if (pick < 25) build.type = Building::UNIVERSITY;
                            else if (pick < 35) build.type = Building::PARK;
                            else build.type = Building::OFFICE;
                             
                            // 2 blocks = 80 units wide. 
                            // Building size: Smaller Parallelepiped in the center
                            build.width = 24.0f; 
                            build.length = 24.0f;
                            
                            if (build.type == Building::PARK) {
                                build.height = 1.0f; // Very low
                                build.color = Fade(GREEN, 0.6f);
                            } else {
                                build.height = GetRandomFloat(30, 80); 
                            }
                            
                            if (build.type == Building::SCHOOL) {
                                build.color = Fade(YELLOW, 0.6f);
                                build.height = 30.0f;
                            } else if (build.type == Building::UNIVERSITY) {
                                build.color = Fade(PURPLE, 0.6f);
                                build.height = 45.0f;
                            } else if (build.type == Building::OFFICE) {
                                build.color = Fade(DARKBLUE, 0.6f);
                            }
                            
                            // Center in the 2x2 area (80x80)
                            // startX + gc*BS is the top-left corner of the 2x2 block
                            // + blockSize gives the exact center of 2 blocks (40.0f)
                            float centerX = startX + gc * blockSize + blockSize;
                            float centerZ = startZ + gr * blockSize + blockSize;
                            build.position = { centerX, build.height / 2.0f, centerZ };
                            
                            city.buildings.push_back(build);
                            placed = true;
                        }
                    }

                    if (!placed) {
                         // Standard 1x1 House
                         currentBuildId++;
                         blockMap[gr][gc] = currentBuildId;
                         
                         build.type = Building::HOUSE;
                         build.width = 12.0f; 
                         build.length = 12.0f; 
                         build.height = GetRandomFloat(8, 15);
                         build.color = Fade(BROWN, 0.6f); 
                         
                         float bx = startX + gc * blockSize + blockSize/2.0f; 
                         float bz = startZ + gr * blockSize + blockSize/2.0f;
                         build.position = { bx, build.height / 2.0f, bz };
                         
                         city.buildings.push_back(build);
                    }
                }
            }

            // B. Generate Roads (Culling those inside mega-blocks)
            // Horizontal Roads (between rows gr-1 and gr)
            // We iterate gr from 0 to gridRows.
            // gr=0 is Top Edge. gr=gridRows is Bottom Edge.
            for (int gr=0; gr<=gridRows; gr++) {
                 // For the road segment at column gc
                 for (int gc=0; gc<gridCols; gc++) {
                      bool skip = false;
                      // If not edge, check if spans same building
                      if (gr > 0 && gr < gridRows) {
                          if (blockMap[gr-1][gc] == blockMap[gr][gc] && blockMap[gr][gc] != -1) {
                              skip = true;
                          }
                      }
                      
                      if (!skip) {
                          Road hRoad;
                          hRoad.width = 12.0f;
                          // Segment start/end
                          float zPos = startZ + gr * blockSize;
                          hRoad.start = { startX + gc * blockSize, 0.05f, zPos };
                          hRoad.end   = { startX + (gc+1) * blockSize, 0.05f, zPos };
                          city.localRoads.push_back(hRoad);
                      }
                 }
            }
            
            // Vertical Roads (between cols gc-1 and gc)
            for (int gc=0; gc<=gridCols; gc++) {
                 for (int gr=0; gr<gridRows; gr++) {
                      bool skip = false;
                      if (gc > 0 && gc < gridCols) {
                          if (blockMap[gr][gc-1] == blockMap[gr][gc] && blockMap[gr][gc] != -1) {
                              skip = true;
                          }
                      }
                      
                      if (!skip) {
                          Road vRoad;
                          vRoad.width = 12.0f;
                          float xPos = startX + gc * blockSize;
                          vRoad.start = { xPos, 0.05f, startZ + gr * blockSize };
                          vRoad.end   = { xPos, 0.05f, startZ + (gr+1) * blockSize };
                          city.localRoads.push_back(vRoad);
                      }
                 }
            }
            
            // C. Generate Highway Ring (Around the grid)
            // Grid is roughly centered at (startX, startZ) to (startX + gridWidth, startZ + gridHeight)
            float gridW = gridCols * blockSize;
            float gridH = gridRows * blockSize;
            
            // Ring is slightly larger
            float ringMargin = 60.0f; 
            float rLeft = startX - ringMargin;
            float rRight = startX + gridW + ringMargin;
            float rTop = startZ - ringMargin;
            float rBottom = startZ + gridH + ringMargin;
            
            float roadW = 20.0f;
            
            // 4 Segments
            Road r1 = { {rLeft, 0.1f, rTop}, {rRight, 0.1f, rTop}, roadW };       // Top
            Road r2 = { {rRight, 0.1f, rTop}, {rRight, 0.1f, rBottom}, roadW };   // Right
            Road r3 = { {rRight, 0.1f, rBottom}, {rLeft, 0.1f, rBottom}, roadW }; // Bottom
            Road r4 = { {rLeft, 0.1f, rBottom}, {rLeft, 0.1f, rTop}, roadW };     // Left
            
            city.highwayRing.push_back(r1);
            city.highwayRing.push_back(r2);
            city.highwayRing.push_back(r3);
            city.highwayRing.push_back(r4);
            
            // D. Define Exits (Midpoints of ring segments)
            city.exitNorth = { (rLeft + rRight)/2.0f, 0.1f, rTop };
            city.exitEast = { rRight, 0.1f, (rTop + rBottom)/2.0f };
            city.exitSouth = { (rLeft + rRight)/2.0f, 0.1f, rBottom };
            city.exitWest = { rLeft, 0.1f, (rTop + rBottom)/2.0f };

            // Add extensions from Grid to Ring (Arteries)
            // Top/Bottom/Left/Right Center roads connect to Exits
            // North Artery
            city.localRoads.push_back({ {city.exitNorth.x, 0.05f, startZ}, city.exitNorth, 15.0f });
            // South Artery
            city.localRoads.push_back({ {city.exitSouth.x, 0.05f, startZ + gridH}, city.exitSouth, 15.0f });
            // East Artery
            city.localRoads.push_back({ {startX + gridW, 0.05f, city.exitEast.z}, city.exitEast, 15.0f });
            // West Artery
            city.localRoads.push_back({ {startX, 0.05f, city.exitWest.z}, city.exitWest, 15.0f });

            nation.push_back(city);
        }
    }

    
    // 3. Generate Inter-City Highways (Connect neighbor cities RINGS)
    // Connect (r,c) to (r, c+1) and (r+1, c)
    for(int r=0; r<rows; r++) {
        for(int c=0; c<cols; c++) {
            int currentIdx = r * cols + c;
            
            // Connect Right
            if (c < cols - 1) {
                int rightIdx = r * cols + (c + 1);
                Road highway;
                highway.width = 25.0f; // Wider
                highway.start = nation[currentIdx].exitEast; // East Exit of Current
                highway.end = nation[rightIdx].exitWest;     // West Exit of Next
                highways.push_back(highway);
            }
            // Connect Down
            if (r < rows - 1) {
                int downIdx = (r + 1) * cols + c;
                Road highway;
                highway.width = 25.0f;
                highway.start = nation[currentIdx].exitSouth; // South Exit of Current
                highway.end = nation[downIdx].exitNorth;      // North Exit of Next
                highways.push_back(highway);
            }
        }
    }
    
    // 4. Populate Sims
    std::vector<Building*> allHomes;
    std::vector<Building*> allOffices;
    std::vector<Building*> allSchools;
    std::vector<Building*> allUniversities;
    std::vector<Building*> allParks;
    
    for(auto& c : nation) {
        for(auto& b : c.buildings) {
            if (b.type == Building::HOUSE) allHomes.push_back(&b);
            else if (b.type == Building::OFFICE) allOffices.push_back(&b);
            else if (b.type == Building::SCHOOL) allSchools.push_back(&b);
            else if (b.type == Building::UNIVERSITY) allUniversities.push_back(&b);
            else if (b.type == Building::PARK) allParks.push_back(&b);
        }
    }

    if (allHomes.empty()) exit(1);
    
    // Maps to track occupancy for parking/work
    std::map<int, int> workOccupancy;
    std::map<int, int> homeOccupancy;
    // Map to assign surnames to homes
    std::map<int, std::string> homeSurnames;

    for(int i=0; i<numSims; i++) {
        auto entity = registry.create();
        
        Building* home = allHomes[rand() % allHomes.size()];
        
        // Assign Surname based on Home
        std::string lastName;
        if (homeSurnames.find(home->id) == homeSurnames.end()) {
             lastName = lastNames[rand() % lastNames.size()];
             homeSurnames[home->id] = lastName;
        } else {
             lastName = homeSurnames[home->id];
        }
        std::string firstName = firstNames[rand() % firstNames.size()];
        
        registry.emplace<IdentityComponent>(entity, firstName, lastName);

        // Age Generation (Weighted)
        int age = 0;
        int r = rand() % 100;
        if (r < 15) age = 1 + rand() % 18;       // 15% Kids (1-18)
        else if (r < 25) age = 19 + rand() % 7;  // 10% Students (19-25)
        else if (r < 85) age = 26 + rand() % 45; // 60% Adults (26-70)
        else age = 71 + rand() % 20;             // 15% Seniors (71+)

        float initialMoney = 100.0f;
        if (age < 25) initialMoney = 1000.0f; // Students/Kids have 1000

        registry.emplace<SimStatsComponent>(entity, i, age, initialMoney, 100.0f);
        registry.emplace<TransformComponent>(entity, home->position); 
        registry.emplace<MovementComponent>(entity); 
        registry.emplace<StateComponent>(entity, SimState::SLEEPING, 0.0f);
        
        Color body = (Color){ (unsigned char)GetRandomFloat(50,255), (unsigned char)GetRandomFloat(50,255), (unsigned char)GetRandomFloat(50,255), 255 };
        if (age > 70) body = LIGHTGRAY; // Visual cue for seniors
        
        Color car = body; 
        registry.emplace<VisualComponent>(entity, body, car);
        
        Vector3 homeDoor = Vector3Add(home->position, {0, -home->height/2.0f + 1.0f, home->length/2.0f + 1.0f}); 

        // Work Assignment
        Building* work = nullptr;
        if (age <= 18) {
            if (!allSchools.empty()) work = allSchools[rand() % allSchools.size()];
        } else if (age <= 25) {
            if (!allUniversities.empty()) work = allUniversities[rand() % allUniversities.size()];
        } else if (age <= 70) {
            if (!allOffices.empty()) work = allOffices[rand() % allOffices.size()];
        } else {
             // Seniors (>70) go to Parks
             if (!allParks.empty()) work = allParks[rand() % allParks.size()];
             // If no parks are available, they will fallback to stay at home (nullptr)
        }
        // Seniors (age > 70) have work = nullptr (Retired) -> Now have work=PARK potentially
        
        // ... Parking Logic ...
        // Smart Parking Slot Assignment for Home (Cap 3)
        int hSlot = homeOccupancy[home->id]++;
        Vector3 homePark;

        if (hSlot < 3) {
            float zOff = (hSlot - 1.0f) * 4.0f; 
            Vector3 baseHomePark = Vector3Add(home->position, {home->width/2.0f + 8.0f, -home->height/2.0f + 0.5f, 0});
            homePark = Vector3Add(baseHomePark, {0, 0, zOff});
        } else {
            homePark = Vector3Add(home->position, {home->width/2.0f + 15.0f, -home->height/2.0f + 0.5f, 0});
        }

        Vector3 workDoor = {0,0,0};
        Vector3 workPark = {0,0,0};
        int wCityId = -1;
        int wBuildId = -1;

        if (work) {
            wCityId = work->cityId;
            wBuildId = work->id;
            workDoor = Vector3Add(work->position, {0, -work->height/2.0f + 1.0f, work->length/2.0f + 1.0f});
            
            // Work Reference logic handled below
            
            wCityId = work->cityId;
            wBuildId = work->id;
            workDoor = Vector3Add(work->position, {0, -work->height/2.0f + 1.0f, work->length/2.0f + 1.0f});
            
            // Shared Parking Logic for Office/School/Uni/Park (All use the big 2x2 lots currently)
            // Note: Parks are now in 2x2.
            
            int wSlot = workOccupancy[work->id]++;
            
            if (wSlot < 60) {
                 // Parking Ring Logic: 4 sides of 15 spots
                 int side = wSlot / 15; // 0, 1, 2, 3
                 int idx = wSlot % 15;
                 
                 float localOffset = (idx - 7.0f) * 3.0f; // Centered
                 float dist = 28.0f; // Distance from building center

                 Vector3 offset = {0,0,0};
                 if (side == 0) offset = { localOffset, 0, -dist }; // North
                 else if (side == 1) offset = { dist, 0, localOffset }; // East
                 else if (side == 2) offset = { -localOffset, 0, dist }; // South (Inverted X for visual symmetry or keep regular?) regular is fine
                 else if (side == 3) offset = { -dist, 0, -localOffset }; // West
                 
                 Vector3 baseWorkPark = Vector3Add(work->position, {0, -work->height/2.0f + 0.5f, 0});
                 workPark = Vector3Add(baseWorkPark, offset);

            } else {
                 // Overflow Street Parking
                 workPark = Vector3Add(work->position, {40.0f, -work->height/2.0f + 0.5f, 40.0f});
            }
        } else {
            // Retired / Unemployed
            // Assignments stay 0/null/home
            workDoor = homeDoor; // Stay home?
            workPark = homePark;
            wCityId = home->cityId;
            wBuildId = home->id;
        }

        registry.emplace<HomeReferenceComponent>(entity, home->id, home->cityId, homeDoor, homePark);
        registry.emplace<WorkReferenceComponent>(entity, wBuildId, wCityId, workDoor, workPark);
    }
}

int main() {
    InitWindow(1280, 720, "Nation Simulator ECS - Huge Map");
    SetTargetFPS(60);
    
    int numSims = 5000; 
    
    srand(time(NULL));
    GenerateWorld(numSims);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 800.0f, 800.0f }; // Higher view
    camera.target = (Vector3){ 0.0f, 0.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 55.0f; // Wider FOV
    camera.projection = CAMERA_PERSPECTIVE;

    // Simulation Time Settings
    float gameTime = 6.0f * 3600.0f; // Start at 06:00 AM (in seconds)
    int dayCounter = 1;
    float timeScale = 60.0f; // Default speed: 1 real sec = 60 game secs (1 min)

    // UI/Control State
    bool isPaused = false;
    bool showStats = false;
    entt::entity selectedSim = entt::null;
    float statsScroll = 0.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        
        // --- Input ---
        if (IsKeyPressed(KEY_P)) isPaused = !isPaused;
        if (IsKeyPressed(KEY_H)) showStats = !showStats;

        // Time Control (Only if not paused, or allowed? User said "pause simulation")
        // We allow changing speed setting while paused, but it won't apply until unpaused.
        if (IsKeyPressed(KEY_ONE)) timeScale = 1.0f;     // Real-time (1x)
        if (IsKeyPressed(KEY_TWO)) timeScale = 30.0f;    // Fast (30x)
        if (IsKeyPressed(KEY_THREE)) timeScale = 240.0f; // Very Fast (240x)
        if (IsKeyPressed(KEY_FOUR)) timeScale = 1000.0f; // Ultra (1000x)

        // Advance Time & Physics
        if (!isPaused) {
            gameTime += dt * timeScale;
            if (gameTime >= 24.0f * 3600.0f) {
                gameTime -= 24.0f * 3600.0f;
                dayCounter++;
            }
        }

        // Camera Controls
        float wheel = GetMouseWheelMove();
        if (showStats) {
             statsScroll -= wheel * 30.0f;
             if (statsScroll < 0) statsScroll = 0;
        } else if (wheel != 0) {
            Vector3 view = Vector3Subtract(camera.target, camera.position);
            camera.position = Vector3Add(camera.position, Vector3Scale(Vector3Normalize(view), wheel * 30.0f));
        }

        // Selection Logic (Raycast)
        if (!showStats && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Ray ray = GetMouseRay(GetMousePosition(), camera);
            selectedSim = entt::null;
            float closestDist = 100000.0f;
            
            auto view = registry.view<TransformComponent>();
            view.each([&](auto entity, auto& trans) {
                 // Simple sphere collision for selection (Radius 2.0f)
                 RayCollision col = GetRayCollisionSphere(ray, trans.position, 2.0f);
                 if (col.hit && col.distance < closestDist) {
                     closestDist = col.distance;
                     selectedSim = entity;
                 }
            });
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsKeyDown(KEY_W) || IsKeyDown(KEY_A) || IsKeyDown(KEY_S) || IsKeyDown(KEY_D)) {
            float panSpeed = 300.0f * dt * (fmax(50.0f, camera.position.y) / 50.0f); 
            Vector3 forward = Vector3Subtract(camera.target, camera.position); forward.y = 0; forward = Vector3Normalize(forward);
            Vector3 right = Vector3CrossProduct(forward, camera.up);
            Vector3 move = {0,0,0};

            if (IsKeyDown(KEY_W)) move = Vector3Add(move, forward);
            if (IsKeyDown(KEY_S)) move = Vector3Subtract(move, forward);
            if (IsKeyDown(KEY_D)) move = Vector3Add(move, right);
            if (IsKeyDown(KEY_A)) move = Vector3Subtract(move, right);
            move = Vector3Scale(Vector3Normalize(move), panSpeed);
            camera.position = Vector3Add(camera.position, move);
            camera.target = Vector3Add(camera.target, move);
        }
        
        // Simulation Update
        if (!isPaused) {
            // Apply scale to DT for simulation, but clamp to avoid gigantic steps glitches (though 1000x needs big steps)
            // We reduced sim movement speed, so large steps are "safer" but physics might tunnel. 
            // For now we use direct scaling.
            float simDt = dt * timeScale;
            Systems::UpdateStateMachine(registry, simDt);
            Systems::UpdateMovement(registry, simDt);
            Systems::UpdateStats(registry, simDt);
        }

        BeginDrawing();
        ClearBackground(SKYBLUE); // Better background color

        BeginMode3D(camera); 
            // Ground Plane lower to avoid Z-fighting
            DrawPlane({0,-0.5f,0}, {10000.0f, 10000.0f}, (Color){40, 100, 40, 255}); 

            // Draw Highways (Solid Cubes for visibility)
            for(const auto& road : highways) {
                // Assuming horizontal or vertical for now based on grid generation, but let's be generic
                Vector3 mid = Vector3Scale(Vector3Add(road.start, road.end), 0.5f);
                Vector3 dir = Vector3Subtract(road.end, road.start);
                float len = Vector3Length(dir);
                float angle = atan2f(dir.x, dir.z) * RAD2DEG;
                
                mid.y = 0.4f; // Raised SIGNIFICANTLY
                
                // DrawCubeWires doesn't support rotation easily without matrix. 
                // But highways are axis-aligned in our generator (Grid).
                if (fabs(dir.z) > fabs(dir.x)) {
                     // Vertical Highway (along Z)
                     DrawCube(mid, road.width, 0.4f, len, DARKGRAY);
                } else {
                     // Horizontal Highway (along X)
                     DrawCube(mid, len, 0.4f, road.width, DARKGRAY);
                }
            }

            // Draw Cities
            for(const auto& city : nation) {
                // Draw Highway Ring
                for(const auto& r : city.highwayRing) {
                     Vector3 center = Vector3Scale(Vector3Add(r.start, r.end), 0.5f);
                     float lenX = fabs(r.end.x - r.start.x) + r.width;
                     float lenZ = fabs(r.end.z - r.start.z) + r.width;
                     center.y = 0.3f; // Raised
                     DrawCube(center, lenX, 0.4f, lenZ, DARKGRAY); 
                }

                // Draw Local Roads (Grid)
                for(const auto& r : city.localRoads) {
                     Vector3 center = Vector3Scale(Vector3Add(r.start, r.end), 0.5f);
                     float lenX = fabs(r.end.x - r.start.x) + r.width;
                     float lenZ = fabs(r.end.z - r.start.z) + r.width;
                     center.y = 0.2f; // Raised
                     DrawCube(center, lenX, 0.4f, lenZ, GRAY); // Thicker (0.4 height)
                }

                if (Vector3Distance(city.center, camera.position) < 3500.0f) {
                    for(const auto& b : city.buildings) {
                        float dist = Vector3Distance(b.position, camera.position);
                        if (dist < 2000.0f) {
                            Color bCol = b.color; // Use pre-assigned color

                            // Buildings
                            // Position is now centralized logic-side, so we draw directly at b.position
                            DrawCube(b.position, b.width, b.height, b.length, bCol);
                            DrawCubeWires(b.position, b.width, b.height, b.length, BLACK);

                            // Draw Parking (for Non-Houses)
                            if (b.type != Building::HOUSE) {
                                // Draw Parking Ring (4 Strips)
                                // Distance 28.0f covers the logical slots
                                float stripLen = 48.0f; // Slightly longer than 45 logic
                                float stripThick = 8.0f; // Car depth
                                float dist = 28.0f;
                                Vector3 basePos = {b.position.x, 0.25f, b.position.z};

                                // North Strip
                                DrawCube(Vector3Add(basePos, {0, 0, -dist}), stripLen, 0.1f, stripThick, DARKGRAY);
                                // East Strip
                                DrawCube(Vector3Add(basePos, {dist, 0, 0}), stripThick, 0.1f, stripLen, DARKGRAY);
                                // South Strip
                                DrawCube(Vector3Add(basePos, {0, 0, dist}), stripLen, 0.1f, stripThick, DARKGRAY);
                                // West Strip
                                DrawCube(Vector3Add(basePos, {-dist, 0, 0}), stripThick, 0.1f, stripLen, DARKGRAY);

                                // Draw Parking Lines (White Lines Logic)
                                // We iterate 15 slots for each side
                                for(int i=0; i<15; i++) {
                                    float off = (i - 7.0f) * 3.0f;
                                    float lineHalfLen = 3.5f; // Visual length of line

                                    // North (-dist) -> Lines along Z
                                    Vector3 nC = Vector3Add(basePos, {off, 0.05f, -dist});
                                    DrawLine3D(Vector3Add(nC, {0,0,-lineHalfLen}), Vector3Add(nC, {0,0,lineHalfLen}), WHITE);

                                    // South (dist) -> Lines along Z
                                    Vector3 sC = Vector3Add(basePos, {-off, 0.05f, dist}); // Inverted X to match logic
                                    DrawLine3D(Vector3Add(sC, {0,0,-lineHalfLen}), Vector3Add(sC, {0,0,lineHalfLen}), WHITE);

                                    // East (dist) -> Lines along X
                                    Vector3 eC = Vector3Add(basePos, {dist, 0.05f, off});
                                    DrawLine3D(Vector3Add(eC, {-lineHalfLen,0,0}), Vector3Add(eC, {lineHalfLen,0,0}), WHITE);

                                    // West (-dist) -> Lines along X
                                    Vector3 wC = Vector3Add(basePos, {-dist, 0.05f, -off});
                                    DrawLine3D(Vector3Add(wC, {-lineHalfLen,0,0}), Vector3Add(wC, {lineHalfLen,0,0}), WHITE);
                                }
                                
                            } else {
                                // Home Parking
                                Vector3 parkPos = Vector3Add(b.position, {b.width/2.0f + 5.0f, -b.height/2.0f + 0.2f, 0});
                                DrawCube(parkPos, 6.0f, 0.1f, 10.0f, DARKGRAY); 
                                for(int i=0; i<4; i++) {
                                     DrawLine3D(Vector3Add(parkPos, {-3.0f, 0.05f, (i-1.5f)*4.0f}), Vector3Add(parkPos, {3.0f, 0.05f, (i-1.5f)*4.0f}), WHITE);
                                }
                            }
                            }
                        }
                    }
                }

            Systems::RenderSims(registry, camera.position);

        EndMode3D();

        // Count for UI (Actually recalculate them here or inside systems)
        countDriving = 0; countWorking = 0; countSleeping = 0;
        auto view = registry.view<StateComponent>();
        view.each([](auto& state) {
            if (state.currentState == SimState::DRIVING_TO_WORK || state.currentState == SimState::DRIVING_HOME) countDriving++;
            else if (state.currentState == SimState::WORKING) countWorking++;
            else if (state.currentState == SimState::SLEEPING) countSleeping++;
        });

        // UI
        if (showStats) {
            // Background for Stats
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.85f));
            
            float yPos = 20.0f - statsScroll;
            float leftMargin = 30.0f;
            
            DrawText("--- NATION STATISTICS (Detailed) ---", leftMargin, yPos, 30, YELLOW);
            yPos += 40;

            // 1. City Inhabitants Count
            DrawText("City Populations (Sims currently within perimeter):", leftMargin, yPos, 20, GREEN);
            yPos += 30;

            // Count sims in each city
            // This is O(N * C), expensive but okay for debug UI
            std::vector<int> cityPop(nation.size(), 0);
            auto transView = registry.view<TransformComponent>();
            
            for (auto [entity, trans] : transView.each()) {
                for (size_t i = 0; i < nation.size(); i++) {
                    if (CheckCollisionPointCircle({trans.position.x, trans.position.z}, {nation[i].center.x, nation[i].center.z}, nation[i].radius)) {
                        cityPop[i]++;
                        break; // Count in first matching city
                    }
                }
            }

            for (size_t i = 0; i < nation.size(); i++) {
                DrawText(TextFormat("City %d (%s): %d Inhabitants", i, nation[i].name.c_str(), cityPop[i]), leftMargin + 20, yPos, 20, WHITE);
                yPos += 25;
            }
            yPos += 20;

            // 2. Sim List
            DrawText("Individual Sim Data:", leftMargin, yPos, 20, GREEN);
            yPos += 30;
            
            auto fullView = registry.view<StateComponent, SimStatsComponent, HomeReferenceComponent, WorkReferenceComponent, IdentityComponent>();
            int simIndex = 0;
            for (auto entity : fullView) {
                // Optimization: Don't render text if outside screen
                if (yPos > -50 && yPos < GetScreenHeight() + 50) {
                    auto& state = fullView.get<StateComponent>(entity);
                    auto& stats = fullView.get<SimStatsComponent>(entity);
                    auto& home = fullView.get<HomeReferenceComponent>(entity);
                    auto& work = fullView.get<WorkReferenceComponent>(entity);
                    auto& id = fullView.get<IdentityComponent>(entity);
                    
                    const char* stateStr = "Unknown";
                    if (state.currentState == SimState::SLEEPING) stateStr = "Sleeping";
                    if (state.currentState == SimState::WORKING) stateStr = "Working"; // Or Studying
                    if (state.currentState == SimState::DRIVING_TO_WORK) stateStr = "Driving Work";
                    if (state.currentState == SimState::DRIVING_HOME) stateStr = "Driving Home";
                    
                    DrawText(TextFormat("%s %s (Age %d) | Money: $%.0f | Energy: %.0f | %s", 
                        id.firstName.c_str(), id.lastName.c_str(), stats.age, stats.money, stats.sleep, stateStr), 
                        leftMargin + 20, yPos, 20, LIGHTGRAY);
                }
                
                yPos += 25;
                simIndex++;
            }
        } 
        else {
            // Standard HUD
            DrawRectangle(0, 0, 300, 260, Fade(BLACK, 0.7f)); 
            DrawFPS(10, 10);
            long mem = GetMemoryUsage();
            DrawText(TextFormat("Memory: %.2f MB", mem / 1024.0f / 1024.0f), 10, 40, 20, WHITE);
            
            DrawText("HUGE NATION SIMULATOR", 10, 70, 20, YELLOW);
            
            // Clock UI
            int hours = (int)(gameTime / 3600.0f);
            int minutes = (int)((gameTime - hours*3600.0f) / 60.0f);
            DrawText(TextFormat("Day %d | %02d:%02d", dayCounter, hours, minutes), 10, 100, 30, WHITE);
            
            // Speed UI
            Color speedColor = GREEN;
            if (isPaused) { DrawText("PAUSED (Press P)", 10, 135, 20, RED); }
            else { DrawText(TextFormat("Speed: %.0fx (Keys 1-4)", timeScale), 10, 135, 20, speedColor); }

            DrawText(TextFormat("Cities: %lu | Sims: %d", nation.size(), numSims), 10, 160, 10, WHITE);
            
            DrawText(TextFormat("- Driving: %d", countDriving), 20, 180, 20, GREEN);
            DrawText(TextFormat("- Working: %d", countWorking), 20, 200, 20, BLUE);
            DrawText(TextFormat("- Sleeping: %d", countSleeping), 20, 220, 20, ORANGE);

            DrawText("[H] for Detailed Statistics", 10, 240, 10, GRAY);
            
            // Selected Sim Panel
            if (selectedSim != entt::null && registry.valid(selectedSim)) {
                 if (registry.all_of<StateComponent, SimStatsComponent, IdentityComponent>(selectedSim)) {
                      auto& st = registry.get<StateComponent>(selectedSim);
                      auto& stat = registry.get<SimStatsComponent>(selectedSim);
                      auto& id = registry.get<IdentityComponent>(selectedSim);
                      
                      Vector2 mousePos = GetMousePosition();
                      float pX = mousePos.x + 15;
                      float pY = mousePos.y + 15;
                      
                      DrawRectangle(pX, pY, 220, 140, Fade(BLUE, 0.8f));
                      DrawRectangleLines(pX, pY, 220, 140, WHITE);
                      
                      const char* stateStr = "Idle";
                      if (st.currentState == SimState::WORKING) stateStr = "Working/Studying";
                      if (st.currentState == SimState::SLEEPING) stateStr = "Sleeping";
                      if (st.currentState == SimState::DRIVING_TO_WORK) stateStr = "Commuting";
                      if (st.currentState == SimState::DRIVING_HOME) stateStr = "Going Home";
                      
                      DrawText("SELECTED SIM", pX + 10, pY + 10, 10, YELLOW);
                      DrawText(TextFormat("%s %s", id.firstName.c_str(), id.lastName.c_str()), pX + 10, pY + 30, 20, WHITE);
                      DrawText(TextFormat("Age: %d", stat.age), pX + 10, pY + 55, 10, WHITE);
                      DrawText(TextFormat("State: %s", stateStr), pX + 10, pY + 70, 10, WHITE);
                      DrawText(TextFormat("Money: $%.0f", stat.money), pX + 10, pY + 85, 10, WHITE);
                      DrawText(TextFormat("Energy: %.1f", stat.sleep), pX + 10, pY + 100, 10, WHITE);
                 }
            }
        } // Close else for HUD

        EndDrawing();
    } // Close while loop

    CloseWindow();
    return 0;
} // Close main
