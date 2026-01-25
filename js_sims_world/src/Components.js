// Components.js - Data Structures (Global Scope)

class TransformComponent {
    constructor(pos = {x:0, y:0, z:0}, rot = 0) {
        this.position = new THREE.Vector3(pos.x, pos.y, pos.z);
        this.rotation = rot; // Degrees Y-axis
        this.dirty = true;
    }
}

class MovementComponent {
    constructor() {
        this.path = []; // Queue of Vector3
        this.pathIndex = 0;
        this.pathLength = 0;
        this.speed = 0.0;
        this.isMoving = false;
        // Navigation targets
        this.target = new THREE.Vector3();
    }
}

const SimState = {
    SLEEPING: 0,
    WALKING_TO_CAR: 1,
    DRIVING_TO_WORK: 2,
    WORKING: 3,
    WALKING_FROM_WORK_CAR: 4,
    DRIVING_HOME: 5,
    WALKING_TO_HOUSE: 6,
    WALKING_TO_CAR_WORK: 7, // Added for completeness in logic
    WALKING_FROM_CAR: 8     // Added
};

class StateComponent {
    constructor(state = SimState.SLEEPING, timer = 0) {
        this.state = state;
        this.timer = timer;
    }
}

class SimStatsComponent {
    constructor(id = 0, age = 20, money = 100, energy = 100) {
        this.id = id;
        this.age = age;
        this.money = money;
        this.energy = energy;
    }
}

class IdentityComponent {
    constructor(first = "John", last = "Doe") {
        this.firstName = first;
        this.lastName = last;
    }
}

class VisualComponent {
    constructor(color = 0xFFFFFF, carColor = 0xFF0000) {
        this.bodyColor = color;
        this.carColor = carColor;
    }
}

// Relation Components
class HomeReferenceComponent {
    constructor(bId, cId, doorPos, parkPos, parkHead) {
        this.buildingId = bId;
        this.cityId = cId;
        this.doorPos = doorPos ? doorPos.clone() : new THREE.Vector3();
        this.parkingPos = parkPos ? parkPos.clone() : new THREE.Vector3();
        this.parkingHeading = parkHead;
    }
}

class WorkReferenceComponent {
    constructor(bId, cId, doorPos, parkPos, parkHead) {
        this.buildingId = bId;
        this.cityId = cId;
        this.doorPos = doorPos ? doorPos.clone() : new THREE.Vector3();
        this.parkingPos = parkPos ? parkPos.clone() : new THREE.Vector3();
        this.parkingHeading = parkHead;
    }
}
