#pragma once
#include <entt/entt.hpp>
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h" // Needed for rlPushMatrix etc
#include "Components.hpp"
#include "World.hpp"
#include <vector>
#include <cmath>
#include <cstdlib>

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
            auto GeneratePath = [&](Vector3 startPos, int startCityId, Vector3 endPos, int endCityId, float startHead, float endHead) {
                move.pathLength = 0; move.pathIndex = 0;
                
                float blockSize = 80.0f; // Must match main.cpp
                Vector3 rawCursor = startPos;
                
                // Helper to add lane-corrected path points
                auto AddLanePoint = [&](Vector3 targetPos) {
                     // Check direction
                     Vector3 dir = Vector3Subtract(targetPos, rawCursor);
                     Vector3 offset = {0,0,0};
                     
                     // 3.0f offset for local lanes
                     if (fabs(dir.z) > fabs(dir.x)) { // Vertical Movement
                         if (dir.z > 0) offset.x = -3.0f; // Moving South (Down), Right is X-
                         else offset.x = 3.0f;           // Moving North (Up), Right is X+
                     } else { // Horizontal Movement
                         if (dir.x > 0) offset.z = 3.0f; // Moving East (Right), Right is Z+
                         else offset.z = -3.0f;          // Moving West (Left), Right is Z-
                     }
                     
                     AddWaypoint(move, Vector3Add(targetPos, offset));
                     rawCursor = targetPos;
                };

                // Helper: Snap position to nearest Road Grid Lines relative to a specific City Center
                // Returns the point on the road network to enter/exit
                auto GetGridEntry = [&](Vector3 pos, Vector3 cityCenter) -> Vector3 {
                     float rx = pos.x - cityCenter.x;
                     float rz = pos.z - cityCenter.z;
                     
                     // Roads are at center +/- k*80
                     float gridX = round(rx / blockSize) * blockSize + cityCenter.x;
                     float gridZ = round(rz / blockSize) * blockSize + cityCenter.z;
                     
                     // Find which road is closer
                     if (fabs(pos.x - gridX) < fabs(pos.z - gridZ)) {
                         return { gridX, 0, pos.z }; // Snap X (Vertical Road)
                     } else {
                         return { pos.x, 0, gridZ }; // Snap Z (Horizontal Road)
                     }
                };

                // 1. Exit Parking Logic
                // Aisle Point (Away from building)
                float radStart = startHead * DEG2RAD;
                Vector3 startForward = { sinf(radStart), 0, cosf(radStart) };
                Vector3 startAisle = Vector3Subtract(startPos, Vector3Scale(startForward, 20.0f)); 
                
                AddWaypoint(move, startPos); rawCursor = startPos;
                AddWaypoint(move, startAisle); rawCursor = startAisle;

                // Find Nearest Road Entry
                Vector3 startCityCenter = (startCityId < nation.size()) ? nation[startCityId].center : startPos;
                Vector3 roadEntry = GetGridEntry(startAisle, startCityCenter);
                
                // For road entry, we assume we just merge
                AddLanePoint(roadEntry);

                // Drive from Entry to Hub (Manhattan on Grid)
                Vector3 startHub = startCityCenter;
                float jx = (float)((rand() % 16) - 8); 
                float jz = (float)((rand() % 16) - 8);
                Vector3 hubTarget = Vector3Add(startHub, {jx, 0, jz});

                bool onVertical = (fabs(roadEntry.x - (round((roadEntry.x - startCityCenter.x)/blockSize)*blockSize + startCityCenter.x)) < 1.0f);
                
                if (onVertical) {
                    // We are on a Vertical Road (Fixed X). Drive Z.
                    AddLanePoint({ roadEntry.x, 0, hubTarget.z });
                    AddLanePoint({ hubTarget.x, 0, hubTarget.z });
                } else {
                    // We are on a Horizontal Road (Fixed Z). Drive X.
                    AddLanePoint({ hubTarget.x, 0, roadEntry.z });
                    AddLanePoint({ hubTarget.x, 0, hubTarget.z });
                }

                // 2. Inter-City Pathfinding (BFS)
                if (startCityId != endCityId) {
                    // ... BFS ...
                    int rows = 3; int cols = 4;
                    std::vector<int> parent(12, -1);
                    std::vector<bool> visited(12, false);
                    std::vector<int> q; q.push_back(startCityId); visited[startCityId] = true;
                    bool found = false; int head = 0;
                    while(head < q.size()) {
                        int u = q[head++]; if (u == endCityId) { found=true; break; }
                        int r = u/cols, c = u%cols;
                        int nbs[4], nC=0;
                        if(c<cols-1) nbs[nC++]=u+1; if(c>0) nbs[nC++]=u-1;
                        if(r<rows-1) nbs[nC++]=u+cols; if(r>0) nbs[nC++]=u-cols;
                        for(int i=0;i<nC;i++) if(!visited[nbs[i]]) { visited[nbs[i]]=true; parent[nbs[i]]=u; q.push_back(nbs[i]); }
                    }
                    
                    if(found) {
                        std::vector<int> pathStack; int curr = endCityId;
                        while(curr!=startCityId) { pathStack.push_back(curr); curr=parent[curr]; }
                        int currentCity = startCityId;
                        
                        for(int i=pathStack.size()-1; i>=0; i--) {
                            int nextCity = pathStack[i];
                            int cols = 4;
                            // Add Highway Exits using Lanes
                            if (nextCity == currentCity + 1) { 
                                AddLanePoint(nation[currentCity].exitEast); AddLanePoint(nation[nextCity].exitWest);
                            } else if (nextCity == currentCity - 1) {
                                AddLanePoint(nation[currentCity].exitWest); AddLanePoint(nation[nextCity].exitEast);
                            } else if (nextCity == currentCity + cols) {
                                AddLanePoint(nation[currentCity].exitSouth); AddLanePoint(nation[nextCity].exitNorth);
                            } else if (nextCity == currentCity - cols) {
                                AddLanePoint(nation[currentCity].exitNorth); AddLanePoint(nation[nextCity].exitSouth);
                            }
                            // To Next Hub
                            float njx = (float)((rand() % 16) - 8); float njz = (float)((rand() % 16) - 8);
                            AddLanePoint(Vector3Add(nation[nextCity].center, {njx, 0, njz}));
                            currentCity = nextCity;
                        }
                    } else {
                         // Fallback
                         Vector3 endHubFallback = (endCityId < nation.size()) ? nation[endCityId].center : endPos;
                         AddLanePoint(endHubFallback);
                    }
                }

                // 3. Final Leg: From Current Location (End Hub) to End Parking
                Vector3 endCityCenter = (endCityId < nation.size()) ? nation[endCityId].center : endPos;
                
                // End Aisle
                float radEnd = endHead * DEG2RAD;
                Vector3 endForward = { sinf(radEnd), 0, cosf(radEnd) };
                Vector3 endAisle = Vector3Subtract(endPos, Vector3Scale(endForward, 20.0f));
                
                // Snap End Aisle to Road
                Vector3 roadExit = GetGridEntry(endAisle, endCityCenter);
                
                 bool onVerticalExit = (fabs(roadExit.x - (round((roadExit.x - endCityCenter.x)/blockSize)*blockSize + endCityCenter.x)) < 1.0f);
                 Vector3 lastPos = rawCursor; // Use tracked raw
                 
                 if (onVerticalExit) {
                     AddLanePoint({ roadExit.x, 0, lastPos.z });
                     AddLanePoint({ roadExit.x, 0, roadExit.z });
                 } else {
                     AddLanePoint({ lastPos.x, 0, roadExit.z });
                     AddLanePoint({ roadExit.x, 0, roadExit.z });
                 }
                 
                 // From roadExit to Aisle we drive straight
                 AddWaypoint(move, endAisle);
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
                            // Add Waypoint for "walking to car" - just direct for now
                            AddWaypoint(move, home.parkingPosition);
                            move.speed = 4.0f; // Slower walking
                            move.isMoving = true;
                        }
                    }
                    break;

                case SimState::WALKING_TO_CAR:
                    if (!move.isMoving) { 
                        state.currentState = SimState::DRIVING_TO_WORK;
                        
                        // SNAP ROTATION TO PARKING HEADING (Unparking)
                        trans.rotation = home.parkingHeading;

                        GeneratePath(home.parkingPosition, home.cityId, work.parkingPosition, work.cityId, home.parkingHeading, work.parkingHeading);
                        move.speed = 60.0f; 
                        move.isMoving = true;
                    }
                    break;

                case SimState::DRIVING_TO_WORK:
                    if (!move.isMoving) {
                        state.currentState = SimState::WALKING_FROM_WORK_CAR;
                        
                        // Arrived at Parking -> Snap Rotation
                        trans.rotation = work.parkingHeading;

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
                        // Ensure Parked Car (visualized later) has correct heading
                        // We don't store "visual car entity", just this Sim. 
                        // The car is drawn at work.parkingPosition. 
                        // RenderSims uses work.parkingHeading.
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
                         trans.rotation = work.parkingHeading; // Snap rotation

                         GeneratePath(work.parkingPosition, work.cityId, home.parkingPosition, home.cityId, work.parkingHeading, home.parkingHeading);
                         move.speed = 60.0f;
                         move.isMoving = true;
                    }
                    break;

                 case SimState::DRIVING_HOME:
                    if (!move.isMoving) {
                        state.currentState = SimState::WALKING_TO_HOUSE;
                        
                        // Arrived Home Parking -> Snap Rotation
                        trans.rotation = home.parkingHeading;

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
    void RenderSims(entt::registry& registry, Vector3 cameraPos, entt::entity selectedSim) {
        auto view = registry.view<TransformComponent, StateComponent, VisualComponent, HomeReferenceComponent, WorkReferenceComponent>();
        
        view.each([cameraPos, selectedSim, &registry](auto entity, auto& trans, auto& state, auto& vis, auto& home, auto& work) {
            float dist = Vector3Distance(trans.position, cameraPos);
            
            bool isSelected = (entity == selectedSim);
            Color drawColor = vis.color;
            if (isSelected) drawColor = GREEN; // Highlight selected SIM

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
                 
                 // Highlight logic for Car
                 Color carCol = isSelected ? GREEN : vis.carColor;

                 // Main Body
                 // Centered at 0,0,0 local
                 DrawCube({0,0,0}, size, size/2, size*2, carCol);
                 
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
                         DrawSphere(pedPos, 1.0f, drawColor);
                         
                         // Selection Ring
                         if (isSelected) {
                             DrawCylinderWires(pedPos, 1.5f, 1.5f, 3.0f, 10, GREEN);
                         }
                     } else if (dist < 2000.0f) {
                         DrawPoint3D(trans.position, drawColor); 
                     }
                 }

                 // 3. Draw Parked Car (Persistent)
                 Vector3 parkPos = {0,0,0};
                 float parkHeading = 0.0f;
                 bool drawParked = false;

                 // Car is at WORK
                 if (state.currentState == SimState::WORKING || state.currentState == SimState::WALKING_FROM_WORK_CAR) {
                     parkPos = work.parkingPosition;
                     parkHeading = work.parkingHeading;
                     drawParked = true;
                 } 
                 // Car is at HOME
                 else if (state.currentState == SimState::SLEEPING || state.currentState == SimState::WALKING_TO_CAR || state.currentState == SimState::WALKING_TO_HOUSE) {
                     parkPos = home.parkingPosition;
                     parkHeading = home.parkingHeading;
                     drawParked = true;
                 }

                 if (drawParked) {
                     float parkDist = Vector3Distance(parkPos, cameraPos);
                     if (parkDist < 3000.0f) { // Render distance for parked cars
                         float size = 3.0f;
                         parkPos.y = 1.5f; 
                         
                         Color carCol = isSelected ? GREEN : vis.carColor;

                         rlPushMatrix();
                         rlTranslatef(parkPos.x, parkPos.y, parkPos.z);
                         rlRotatef(parkHeading, 0, 1, 0); 
                         
                         DrawCube({0,0,0}, size, size/2, size*2, carCol);
                         DrawCube({0, 0.2f, size}, size*0.8f, 0.4f, 0.5f, YELLOW); 
                         DrawCube({0, 0.2f, -size}, size*0.8f, 0.4f, 0.5f, RED);   
                         
                         rlPopMatrix();
                     }
                 }
            }
        });
    }
}

