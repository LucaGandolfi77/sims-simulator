#pragma once
#include <entt/entt.hpp>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h" // Needed for rlPushMatrix etc
#include "Components.hpp"
#include "World.hpp"
#include <vector>
#include <cmath>

namespace Systems {

    // --- Logic Systems ---

    void UpdateStats(entt::registry& registry, float dt) {
        registry.view<SimStatsComponent, StateComponent>().each([dt](auto& stats, auto& state) {
            if (state.currentState == SimState::SLEEPING) {
                stats.sleep += 15.0f * dt; // Sleep recovers faster
                if (stats.sleep > 100.0f) stats.sleep = 100.0f;
            } else {
                stats.sleep -= 0.8f * dt; // Gets tired
                if (stats.sleep < 0.0f) stats.sleep = 0.0f;
            }

            // Normal Salary
            if (state.currentState == SimState::WORKING) {
                // Students (19-25) at Uni don't earn money usually, but maybe small amount?
                // Kids (1-18) at school don't earn.
                // Adults (26-70) earn.
                if (stats.age >= 26 && stats.age <= 70) {
                     stats.money += 10.0f * dt;
                }
            }
            
            // Pension for Seniors (>70)
            // They earn passively without working
            if (stats.age > 70) {
                 stats.money += 5.0f * dt; 
            }
        });
    }

    void UpdateMovement(entt::registry& registry, float dt) {
        auto view = registry.view<TransformComponent, MovementComponent>();
        
        view.each([dt](auto& trans, auto& move) {
            if (move.isMoving && move.pathLength > 0 && move.pathIndex < move.pathLength) {
                
                Vector3 target = move.path[move.pathIndex];
                Vector3 dir = Vector3Subtract(target, trans.position);
                float dist = Vector3Length(dir);
                float step = move.speed * dt;
                
                // Check if we reach the target in this frame
                if (dist > step) {
                    dir = Vector3Normalize(dir);
                    Vector3 displacement = Vector3Scale(dir, step);
                    trans.position = Vector3Add(trans.position, displacement);
                    
                    // Update Rotation (Y-axis)
                    // atan2(x, z) gives angle from Z-axis in radians. 
                    // Raylib standard: 0 is usually ? We need to check coordinate system.
                    // Raylib 3D: +Y up, +Z is towards camera? No, Right-Handed usually.
                    // Let's assume standard math.
                    
                    trans.rotation = atan2f(dir.x, dir.z) * RAD2DEG;
                    
                } else {
                    // Reached waypoint
                    trans.position = target;
                    move.pathIndex++;
                    
                    // If finished path
                    if (move.pathIndex >= move.pathLength) {
                        move.isMoving = false;
                    }
                }
            } else {
                move.isMoving = false;
            }
        });
    }

    // Helper to push a point to path
    void AddWaypoint(MovementComponent& move, Vector3 p) {
        if (move.pathLength < 16) {
            move.path[move.pathLength++] = p;
        }
    }

    void UpdateStateMachine(entt::registry& registry, float dt) {
        auto view = registry.view<StateComponent, SimStatsComponent, MovementComponent, TransformComponent, HomeReferenceComponent, WorkReferenceComponent>();

        view.each([dt](auto& state, auto& stats, auto& move, auto& trans, auto& home, auto& work) {
            
            // Helper lambda to generate grid-like path with BFS for inter-city travel
            auto GeneratePath = [&](Vector3 startPos, int startCityId, Vector3 endPos, int endCityId) {
                move.pathLength = 0; move.pathIndex = 0;
                
                // 1. Exit Parking to Start Hub
                // Move from parking slot to the city center (Hub) of the current city
                // We add a waypoint that is "street-like" to avoid clipping through buildings if possible,
                // but for now, straight to Hub is the "local road" abstraction.
                Vector3 startHub = (startCityId < nation.size()) ? nation[startCityId].center : startPos;
                
                // If we are already at the hub/city, no need to add startHub if it's too close to startPos?
                // Just add it to ensure connectivity.
                AddWaypoint(move, startHub);

                // 2. Inter-City Pathfinding (BFS)
                if (startCityId != endCityId) {
                    // We need a path of Cities from Start to End.
                    // Map is 3 rows, 4 cols. IDs 0..11.
                    // Adjacency: 
                    //   Right: +1 (if c < 3)
                    //   Left: -1 (if c > 0)
                    //   Down: +4 (if r < 2)
                    //   Up: -4 (if r > 0)
                    
                    int rows = 3; int cols = 4;
                    
                    std::vector<int> parent(12, -1);
                    std::vector<bool> visited(12, false);
                    std::vector<int> q;
                    
                    q.push_back(startCityId);
                    visited[startCityId] = true;
                    
                    bool found = false;
                    int head = 0;
                    while(head < q.size()) {
                        int u = q[head++];
                        if (u == endCityId) { found=true; break; }
                        
                        int r = u / cols;
                        int c = u % cols;
                        
                        int neighbors[4];
                        int nCheck = 0;
                        
                        if (c < cols - 1) neighbors[nCheck++] = u + 1; // Right
                        if (c > 0) neighbors[nCheck++] = u - 1;        // Left
                        if (r < rows - 1) neighbors[nCheck++] = u + cols; // Down
                        if (r > 0) neighbors[nCheck++] = u - cols;     // Up
                        
                        for(int i=0; i<nCheck; i++) {
                            int v = neighbors[i];
                            if (!visited[v]) {
                                visited[v] = true;
                                parent[v] = u;
                                q.push_back(v);
                            }
                        }
                    }
                    
                    if (found) {
                        // Reconstruct path
                        std::vector<int> pathStack;
                        int curr = endCityId;
                        while(curr != startCityId) {
                            pathStack.push_back(curr);
                            curr = parent[curr];
                        }
                        // PathStack has End -> ... -> Next to Start.
                        // We need to add them in reverse order (Start -> Next -> ... -> End)
                        
                        int currentCity = startCityId;
                        for(int i = pathStack.size() - 1; i >= 0; i--) {
                            int nextCity = pathStack[i];
                            
                            // Determine Direction
                            // Cols = 4 (hardcoded in main, hope it matches here or we pass it?)
                            // Note: 'cols' should be accessible. It was 4 in main. 
                            // Systems.hpp doesn't know 'cols', but we used 4 in BFS above.
                            int cols = 4; 
                            
                            // Check adjacency
                            if (nextCity == currentCity + 1) { // Right
                                AddWaypoint(move, nation[currentCity].exitEast);
                                AddWaypoint(move, nation[nextCity].exitWest);
                            } 
                            else if (nextCity == currentCity - 1) { // Left
                                AddWaypoint(move, nation[currentCity].exitWest);
                                AddWaypoint(move, nation[nextCity].exitEast);
                            }
                            else if (nextCity == currentCity + cols) { // Down
                                AddWaypoint(move, nation[currentCity].exitSouth);
                                AddWaypoint(move, nation[nextCity].exitNorth);
                            }
                            else if (nextCity == currentCity - cols) { // Up
                                AddWaypoint(move, nation[currentCity].exitNorth);
                                AddWaypoint(move, nation[nextCity].exitSouth);
                            }
                            
                            // Drive to center of next city (Hub) to connect to next leg or final dest
                            AddWaypoint(move, nation[nextCity].center);
                            
                            currentCity = nextCity;
                        }
                    } else {
                        // Fallback implies unconnected graph (unlikely here)
                        Vector3 endHub = (endCityId < nation.size()) ? nation[endCityId].center : endPos;
                        AddWaypoint(move, endHub);
                    }
                } else {
                     // Same city, logic handled by skipping BFS
                }

                // 3. Final Leg: Hub to Parking
                AddWaypoint(move, endPos);
            };

            switch (state.currentState) {
                case SimState::SLEEPING:
                    if (stats.sleep >= 100.0f) {
                        // Retired Logic: Stay home or wander? 
                        // For simplicity, they just stay "Sleeping" (Idle at home) or we could add IDLE state.
                        // Implemented as: Sleep 100 -> Wait a bit -> Sleep drops -> Sleep again.
                        // To avoid infinite loop of waking up: ensure they only wake up if they have somewhere to go.
                        
                        if (stats.age > 70) {
                             // Retired: Go to PARK if assigned (simulate as "State=WORKING" but logic will differ)
                             // If work is -1 (no park assigned), they stay home.
                             if (work.buildingId != -1) {
                                 state.currentState = SimState::WALKING_TO_CAR;
                                 move.pathLength = 0; move.pathIndex = 0;
                                 AddWaypoint(move, home.parkingPosition);
                                 move.speed = 4.0f; // Slower walking
                                 move.isMoving = true;
                             } else {
                                state.currentState = SimState::WALKING_TO_HOUSE; 
                             }
                        } else {
                            // Workers/Students go to Work
                            state.currentState = SimState::WALKING_TO_CAR;
                            move.pathLength = 0; move.pathIndex = 0;
                            AddWaypoint(move, home.parkingPosition);
                            move.speed = 4.0f; // Slower walking
                            move.isMoving = true;
                        }
                    }
                    break;

                case SimState::WALKING_TO_CAR:
                    if (!move.isMoving) { 
                        state.currentState = SimState::DRIVING_TO_WORK;
                        GeneratePath(home.parkingPosition, home.cityId, work.parkingPosition, work.cityId);
                        move.speed = 60.0f; // Slower driving (was 150)
                        move.isMoving = true;
                    }
                    break;

                case SimState::DRIVING_TO_WORK:
                    if (!move.isMoving) {
                        state.currentState = SimState::WALKING_FROM_WORK_CAR;
                        move.pathLength = 0; move.pathIndex = 0;
                        AddWaypoint(move, work.doorPosition);
                        
                        move.speed = 4.0f;
                        move.isMoving = true;
                    }
                    break;
                
                case SimState::WALKING_FROM_WORK_CAR:
                    if (!move.isMoving) {
                        state.currentState = SimState::WORKING;
                        state.actionTimer = 30.0f; // Work duration
                    }
                    break;

                case SimState::WORKING:
                    state.actionTimer -= dt;
                    
                    // PARK LOGIC: Walk in circles if at Park (Age > 70)
                    // We need to know if current location is a Park. 
                    // Best guess: check Age > 70 usually implies Park if at "Work"
                    if (stats.age > 70) {
                         // Park Behavior: Walk in circles around current parking? 
                         // Or proper park center. 
                         // "walk in circles and sit"
                         // Simple circle around work.doorPosition (Building Center)
                         
                         float angle = GetTime() * 0.5f + stats.id; // Corrected: simId -> id
                         float radius = 15.0f;
                         
                         // Occasional Sit (Pause movement)
                         int mod = (int)GetTime() % 20;
                         if (mod < 10) {
                              // Walking
                              Vector3 center = work.doorPosition; // Actually door is offset, let's use door as center pivot
                              // Re-calculate local pos
                              trans.position.x = center.x + cosf(angle) * radius;
                              trans.position.z = center.z + sinf(angle) * radius;
                              // Update rotation
                              // Tangent to circle (-sin, cos)
                              float dx = -sinf(angle); float dz = cosf(angle);
                              trans.rotation = atan2f(dx, dz) * RAD2DEG;
                         } else {
                              // Sitting (Idle)
                              // Just stop updating pos/rot
                         }
                    }
                    
                    if (state.actionTimer <= 0 || stats.sleep < 20.0f) {
                         state.currentState = SimState::DRIVING_HOME;
                         trans.position = work.parkingPosition; // Teleport to car
                         GeneratePath(work.parkingPosition, work.cityId, home.parkingPosition, home.cityId);
                         move.speed = 60.0f;
                         move.isMoving = true;
                    }
                    break;

                 case SimState::DRIVING_HOME:
                    if (!move.isMoving) {
                        state.currentState = SimState::WALKING_TO_HOUSE;
                        move.pathLength = 0; move.pathIndex = 0;
                        AddWaypoint(move, home.doorPosition);
                        move.speed = 4.0f;
                        move.isMoving = true;
                    }
                    break;

                 case SimState::WALKING_TO_HOUSE:
                    if (!move.isMoving) {
                        state.currentState = SimState::SLEEPING;
                    }
                    break;
            }
        });
    }

    // --- Rendering System ---
    void RenderSims(entt::registry& registry, Vector3 cameraPos) {
        auto view = registry.view<TransformComponent, StateComponent, VisualComponent, HomeReferenceComponent, WorkReferenceComponent>();
        
        view.each([cameraPos](auto& trans, auto& state, auto& vis, auto& home, auto& work) {
            float dist = Vector3Distance(trans.position, cameraPos);

            // 1. Draw Active Car (Driving)
            if (state.currentState == SimState::DRIVING_TO_WORK || state.currentState == SimState::DRIVING_HOME) {
                 if (dist > 3000.0f) return;
                 float size = 3.0f;
                 // Raise car slightly to avoid z-fighting with roads
                 Vector3 carPos = trans.position;
                 carPos.y = 1.5f; 
                 
                 // Draw Rotated Car
                 rlPushMatrix();
                 rlTranslatef(carPos.x, carPos.y, carPos.z);
                 rlRotatef(trans.rotation, 0, 1, 0); // Rotate Y
                 
                 // Main Body
                 // Centered at 0,0,0 local
                 DrawCube({0,0,0}, size, size/2, size*2, vis.carColor);
                 
                 // Front Headlights (Yellow) -> Assuming +Z is forward or +X? 
                 // Based on rotation math atan2(x, z), +Z should be forward direction?
                 // Let's test visualize. if wrong, rotate 90.
                 DrawCube({0, 0.2f, size}, size*0.8f, 0.4f, 0.5f, YELLOW); // Front lights
                 DrawCube({0, 0.2f, -size}, size*0.8f, 0.4f, 0.5f, RED);   // Back lights
                 
                 rlPopMatrix();
            } 
            else {
                 // 2. Draw Sim (Person)
                 if (dist < 3000.0f) {
                     if (dist < 800.0f) {
                         // Draw person slightly raised
                         Vector3 pedPos = trans.position;
                         pedPos.y = 1.0f;
                         DrawSphere(pedPos, 1.0f, vis.color);
                     } else if (dist < 2000.0f) {
                         DrawPoint3D(trans.position, vis.color); 
                     }
                 }

                 // 3. Draw Parked Car (Persistent)
                 Vector3 parkPos = {0,0,0};
                 bool drawParked = false;

                 // Car is at WORK
                 if (state.currentState == SimState::WORKING || state.currentState == SimState::WALKING_FROM_WORK_CAR) {
                     parkPos = work.parkingPosition;
                     drawParked = true;
                 } 
                 // Car is at HOME
                 else if (state.currentState == SimState::SLEEPING || state.currentState == SimState::WALKING_TO_CAR || state.currentState == SimState::WALKING_TO_HOUSE) {
                     parkPos = home.parkingPosition;
                     drawParked = true;
                 }

                 if (drawParked) {
                     float parkDist = Vector3Distance(parkPos, cameraPos);
                     if (parkDist < 3000.0f) { // Render distance for parked cars
                         float size = 3.0f;
                         parkPos.y = 1.5f; 
                         DrawCube(parkPos, size, size/2, size*2, vis.carColor);
                     }
                 }
            }
        });
    }
}

