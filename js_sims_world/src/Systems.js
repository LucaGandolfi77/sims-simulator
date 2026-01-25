// Systems.js (Global)

function UpdateStats(registry, dt) {
    registry.view(SimStatsComponent, StateComponent).each((entity, stats, state) => {
        if (state.state === SimState.SLEEPING) {
            stats.energy += 15.0 * dt;
            if (stats.energy > 100) stats.energy = 100;
        } else {
            stats.energy -= 1.5 * dt;
            if (stats.energy < 0) stats.energy = 0;
        }
        
        if (state.state === SimState.WORKING) {
            stats.money += 10.0 * dt;
        }
    });
}

function UpdateStateMachine(registry, dt) {
    const timeMult = 1.0; 
    
    registry.view(StateComponent, SimStatsComponent, MovementComponent, TransformComponent, HomeReferenceComponent, WorkReferenceComponent).each((entity, state, stats, move, trans, homeRef, workRef) => {
        
        const goTo = (newState, destPos, speed, isDriving) => {
            state.state = newState;
            move.target.copy(destPos);
            move.path.length = 0;
            move.path.push(destPos.clone());
            move.speed = speed;
            move.isMoving = true;
            state.timer = 0;
        };

        switch (state.state) {
            case SimState.SLEEPING:
                if (stats.energy >= 100) {
                    goTo(SimState.WALKING_TO_CAR, homeRef.parkingPos, 5.0, false);
                }
                break;
                
            case SimState.WALKING_TO_CAR:
                if (!move.isMoving) {
                    goTo(SimState.DRIVING_TO_WORK, workRef.parkingPos, 40.0, true);
                }
                break;
                
            case SimState.DRIVING_TO_WORK:
                if (!move.isMoving) {
                    goTo(SimState.WALKING_FROM_CAR, workRef.doorPos, 5.0, false);
                }
                break;
                
            case SimState.WALKING_FROM_CAR: // Using explicit state value
                if (!move.isMoving) {
                    state.state = SimState.WORKING;
                    state.timer = 20.0;
                }
                break;
                
            case SimState.WORKING:
                state.timer -= dt * timeMult;
                if (state.timer <= 0 || stats.energy < 10) {
                    goTo(SimState.WALKING_TO_CAR_WORK, workRef.parkingPos, 5.0, false);
                }
                break;

            case SimState.WALKING_TO_CAR_WORK:
                 if (!move.isMoving) {
                    goTo(SimState.DRIVING_HOME, homeRef.parkingPos, 40.0, true);
                 }
                 break;
                 
            case SimState.DRIVING_HOME:
                 if (!move.isMoving) {
                    goTo(SimState.WALKING_TO_HOUSE, homeRef.doorPos, 5.0, false);
                 }
                 break;
                 
            case SimState.WALKING_TO_HOUSE:
                if (!move.isMoving) {
                    state.state = SimState.SLEEPING;
                }
                break;
        }
    });
}

function UpdateMovement(registry, dt) {
    registry.view(MovementComponent, TransformComponent, StateComponent).each((entity, move, trans, state) => {
        if (!move.isMoving) return;
        
        let target = move.target;
        const isDriving = (state.state === SimState.DRIVING_TO_WORK || state.state === SimState.DRIVING_HOME);
        
        if (isDriving) {
            const cx = trans.position.x;
            const cz = trans.position.z;
            const tx = target.x;
            const tz = target.z;
            
            const dx = Math.abs(tx - cx);
            const dz = Math.abs(tz - cz);
            
            const turnThreshold = 1.0; 
            let steerTarget = new THREE.Vector3();
            
            if (dx > turnThreshold) {
                steerTarget.set(tx, trans.position.y, cz); 
            } else {
                steerTarget.set(cx, trans.position.y, tz);
            }
            
            const dir = new THREE.Vector3().subVectors(steerTarget, trans.position);
            dir.y = 0;
            const dist = dir.length();
            
            if (dist > 0.5) {
                dir.normalize();
                const angle = Math.atan2(dir.x, dir.z);
                trans.rotation = angle;
                trans.position.add(dir.multiplyScalar(move.speed * dt));
            } else {
                if (trans.position.distanceTo(target) < 1.0) {
                    move.isMoving = false;
                    trans.position.copy(target);
                } else {
                    trans.position.x = tx; 
                }
            }

        } else {
            const dir = new THREE.Vector3().subVectors(target, trans.position);
            const dist = dir.length();
            
            if (dist > 0.5) {
                dir.normalize();
                trans.rotation = Math.atan2(dir.x, dir.z);
                trans.position.add(dir.multiplyScalar(move.speed * dt));
            } else {
                move.isMoving = false;
                trans.position.copy(target);
            }
        }
    });
}
