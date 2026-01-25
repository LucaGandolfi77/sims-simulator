// World Data Structures (Global)

const BuildingType = {
    HOUSE: 0,
    OFFICE: 1,
    SCHOOL: 2,
    UNIVERSITY: 3,
    PARK: 4
};

class Building {
    constructor(id, cityId, type, pos, w, h, l, color) {
        this.id = id;
        this.cityId = cityId;
        this.type = type;
        this.position = pos; // THREE.Vector3
        this.width = w;
        this.height = h;
        this.length = l;
        this.color = new THREE.Color(color);
    }
}

class Road {
    constructor(start, end, width) {
        this.start = start; // THREE.Vector3
        this.end = end;
        this.width = width;
    }
}

class City {
    constructor(name, center, radius) {
        this.name = name;
        this.center = center;
        this.radius = radius;
        this.buildings = [];
        this.localRoads = [];
        // Exits
        this.exitNorth = new THREE.Vector3();
        this.exitSouth = new THREE.Vector3();
        this.exitEast = new THREE.Vector3();
        this.exitWest = new THREE.Vector3();
    }
}

// Global World Data
const nation = [];
const highways = [];
let allHomes = [];
let allOffices = [];
let allSchools = [];
let allUniversities = [];
let allParks = [];
const homeOccupancy = {}; 
const workOccupancy = {};

function randFloat(min, max) {
    return Math.random() * (max - min) + min;
}

function generateWorld(numSims, registry) {
    nation.length = 0;
    highways.length = 0;
    allHomes.length = 0;
    allOffices.length = 0;
    allSchools.length = 0;
    allUniversities.length = 0;
    allParks.length = 0;
    
    // Grid Generation (3 Rows x 4 Cols)
    const rows = 3;
    const cols = 4;
    const cityDist = 2000.0;
    
    let cityCounter = 0;
    let currentBuildId = 0;
    
    const cityNames = ["Neo Tokyo", "New York", "London", "Paris", "Berlin", "Rome", "Madrid", "Moscow", "Beijing", "Sydney", "Cairo", "Rio"];

    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            const centerX = c * cityDist;
            const centerZ = r * cityDist;
            
            const city = new City(
                cityNames[cityCounter % cityNames.length] + " " + (cityCounter+1),
                new THREE.Vector3(centerX, 0, centerZ),
                800.0
            );
            
            const blockS = 80.0;
            const gridR = 10;
            const gridC = 10;
            const startX = centerX - (gridC * blockS) / 2;
            const startZ = centerZ - (gridR * blockS) / 2;
            
            const blockMap = Array(gridR).fill().map(() => Array(gridC).fill(-1));
            
            for (let gr = 0; gr < gridR; gr++) {
                for (let gc = 0; gc < gridC; gc++) {
                    if (blockMap[gr][gc] !== -1) continue;
                    
                    const tryWorkplace = Math.random() < 0.30;
                    let placed = false;
                    
                    if (tryWorkplace) {
                        const canExpand = (gr + 1 < gridR) && (gc + 1 < gridC) &&
                                          (blockMap[gr][gc+1] === -1) &&
                                          (blockMap[gr+1][gc] === -1) &&
                                          (blockMap[gr+1][gc+1] === -1);
                        
                        if (canExpand) {
                            currentBuildId++;
                            blockMap[gr][gc] = currentBuildId;
                            blockMap[gr][gc+1] = currentBuildId;
                            blockMap[gr+1][gc] = currentBuildId;
                            blockMap[gr+1][gc+1] = currentBuildId;
                            
                            let type = BuildingType.OFFICE;
                            let color = 0x00008B;
                            let h = randFloat(30, 80);
                            
                            const pick = Math.random();
                            if (pick < 0.15) { type = BuildingType.SCHOOL; color = 0xFFFF00; h = 30; }
                            else if (pick < 0.25) { type = BuildingType.UNIVERSITY; color = 0x800080; h = 45; }
                            else if (pick < 0.35) { type = BuildingType.PARK; color = 0x00FF00; h = 1; }
                            
                            const bX = startX + gc * blockS + blockS;
                            const bZ = startZ + gr * blockS + blockS;
                            
                            city.buildings.push(new Building(currentBuildId, cityCounter, type, new THREE.Vector3(bX, h/2, bZ), 24, h, 24, color));
                            placed = true;
                        }
                    }
                    
                    if (!placed) {
                        currentBuildId++;
                        blockMap[gr][gc] = currentBuildId;
                        const h = randFloat(8, 15);
                        const bX = startX + gc * blockS + blockS/2;
                        const bZ = startZ + gr * blockS + blockS/2;
                        
                        city.buildings.push(new Building(currentBuildId, cityCounter, BuildingType.HOUSE, new THREE.Vector3(bX, h/2, bZ), 12, h, 12, 0x8B4513));
                    }
                }
            }
            
            // Roads
            for (let gr = 0; gr <= gridR; gr++) {
                for (let gc = 0; gc < gridC; gc++) {
                     let skip = false;
                     if (gr > 0 && gr < gridR) {
                         if (blockMap[gr-1][gc] === blockMap[gr][gc] && blockMap[gr][gc] !== -1) skip = true;
                     }
                     if (!skip) {
                         const zPos = startZ + gr * blockS;
                         city.localRoads.push(new Road(
                             new THREE.Vector3(startX + gc*blockS, 0.05, zPos),
                             new THREE.Vector3(startX + (gc+1)*blockS, 0.05, zPos),
                             12.0
                         ));
                     }
                }
            }
            for (let gc = 0; gc <= gridC; gc++) {
                for (let gr = 0; gr < gridR; gr++) {
                    let skip = false;
                    if (gc > 0 && gc < gridC) {
                        if (blockMap[gr][gc-1] === blockMap[gr][gc] && blockMap[gr][gc] !== -1) skip = true;
                    }
                    if (!skip) {
                        const xPos = startX + gc * blockS;
                        city.localRoads.push(new Road(
                            new THREE.Vector3(xPos, 0.05, startZ + gr*blockS),
                            new THREE.Vector3(xPos, 0.05, startZ + (gr+1)*blockS),
                            12.0
                        ));
                    }
                }
            }

            // Exits
            const ringMargin = 60.0;
            const rLeft = startX - ringMargin;
            const rRight = startX + (gridC * blockS) + ringMargin;
            const rTop = startZ - ringMargin;
            const rBottom = startZ + (gridR * blockS) + ringMargin;
            
            city.exitNorth.set((rLeft + rRight)/2, 0.1, rTop);
            city.exitEast.set(rRight, 0.1, (rTop + rBottom)/2);
            city.exitSouth.set((rLeft + rRight)/2, 0.1, rBottom);
            city.exitWest.set(rLeft, 0.1, (rTop + rBottom)/2);
            
             city.localRoads.push(new Road(new THREE.Vector3(city.exitNorth.x, 0.05, startZ), city.exitNorth.clone(), 15.0));
             city.localRoads.push(new Road(new THREE.Vector3(city.exitSouth.x, 0.05, startZ + gridR*blockS), city.exitSouth.clone(), 15.0));
             city.localRoads.push(new Road(new THREE.Vector3(startX + gridC*blockS, 0.05, city.exitEast.z), city.exitEast.clone(), 15.0));
             city.localRoads.push(new Road(new THREE.Vector3(startX, 0.05, city.exitWest.z), city.exitWest.clone(), 15.0));

            nation.push(city);
            cityCounter++;
        }
    }
    
    // Highways
    for (let r = 0; r < rows; r++) {
        for (let c = 0; c < cols; c++) {
            const currIdx = r * cols + c;
            
            if (c < cols - 1) {
                const rightIdx = r * cols + (c+1);
                highways.push(new Road(nation[currIdx].exitEast.clone(), nation[rightIdx].exitWest.clone(), 25.0));
            }
            if (r < rows - 1) {
                const downIdx = (r+1) * cols + c;
                highways.push(new Road(nation[currIdx].exitSouth.clone(), nation[downIdx].exitNorth.clone(), 25.0));
            }
        }
    }
    
    // Cache
    nation.forEach(city => {
        city.buildings.forEach(b => {
            if (b.type === BuildingType.HOUSE) allHomes.push(b);
            else if (b.type === BuildingType.OFFICE) allOffices.push(b);
            else if (b.type === BuildingType.SCHOOL) allSchools.push(b);
            else if (b.type === BuildingType.UNIVERSITY) allUniversities.push(b);
            else if (b.type === BuildingType.PARK) allParks.push(b);
            
            if (b.type === BuildingType.HOUSE) homeOccupancy[b.id] = 0;
            else workOccupancy[b.id] = 0;
        });
    });
}

function populateSims(registry, count) {
    const firstNames = ["James", "Mary", "John", "Patricia", "Robert", "Jennifer", "Michael", "Linda"];
    const lastNames = ["Smith", "Johnson", "Williams", "Brown", "Jones", "Garcia", "Miller"];

    for (let i = 0; i < count; i++) {
        const entity = registry.create();
        
        if (allHomes.length === 0) break;
        const home = allHomes[Math.floor(Math.random() * allHomes.length)];
        
        const r = Math.random() * 100;
        let age = 30;
        if (r < 15) age = Math.floor(randFloat(5, 18));
        else if (r < 25) age = Math.floor(randFloat(19, 25));
        else if (r < 85) age = Math.floor(randFloat(26, 70));
        else age = Math.floor(randFloat(71, 90));
        
        const initialMoney = (age < 25) ? 1000 : 100;
        
        registry.emplace(entity, SimStatsComponent, i, age, initialMoney, 100);
        registry.emplace(entity, TransformComponent, home.position, 0);
        registry.emplace(entity, MovementComponent);
        registry.emplace(entity, StateComponent, SimState.SLEEPING, 0);
        registry.emplace(entity, IdentityComponent, firstNames[i%firstNames.length], lastNames[i%lastNames.length]);
        
        let col = Math.random() * 0xFFFFFF;
        if (age > 70) col = 0xCCCCCC; 
        registry.emplace(entity, VisualComponent, col, col);
        
        let work = null;
        if (age <= 18 && allSchools.length > 0) work = allSchools[Math.floor(Math.random()*allSchools.length)];
        else if (age <= 25 && allUniversities.length > 0) work = allUniversities[Math.floor(Math.random()*allUniversities.length)];
        else if (age <= 70 && allOffices.length > 0) work = allOffices[Math.floor(Math.random()*allOffices.length)];
        else if (age > 70 && allParks.length > 0) work = allParks[Math.floor(Math.random()*allParks.length)];
        
        let hSlot = homeOccupancy[home.id]++;
        let homePark = home.position.clone();
        let homeHeading = 90.0;
        
        if (hSlot < 3) {
            let base = home.position.clone().add(new THREE.Vector3(home.width/2 + 8.0, 0, 0));
            homePark.copy(base).add(new THREE.Vector3(0, 0, (hSlot - 1.0) * 4.0));
        } else {
             homePark.copy(home.position).add(new THREE.Vector3(home.width/2 + 15.0, 0, 0));
        }
        
        const homeDoor = home.position.clone().add(new THREE.Vector3(0, -home.height/2 + 1, home.length/2 + 1));
        
        let wDoor = new THREE.Vector3();
        let wPark = new THREE.Vector3();
        let wHead = 0.0;
        let wBId = -1, wCId = home.cityId;
        
        if (work) {
            wBId = work.id;
            wCId = work.cityId;
            wDoor.copy(work.position).add(new THREE.Vector3(0, -work.height/2 + 1, work.length/2 + 1));
            
            let wSlot = workOccupancy[work.id]++;
            if (wSlot < 60) {
                 let side = Math.floor(wSlot / 15);
                 let idx = wSlot % 15;
                 let localOffset = (idx - 7.0) * 3.0;
                 let dist = 28.0;
                 let offset = new THREE.Vector3();
                 
                 if (side === 0) { offset.set(localOffset, 0, -dist); wHead = 0.0; } 
                 else if (side === 1) { offset.set(dist, 0, localOffset); wHead = 270.0; }
                 else if (side === 2) { offset.set(-localOffset, 0, dist); wHead = 180.0; }
                 else if (side === 3) { offset.set(-dist, 0, -localOffset); wHead = 90.0; }
                 
                 wPark.copy(work.position).add(offset);
            } else {
                 let angle = (wSlot * 137.5) * (Math.PI/180);
                 let rad = 45.0 + (wSlot/60) * 2.5;
                 let ox = Math.cos(angle) * rad;
                 let oz = Math.sin(angle) * rad;
                 wPark.copy(work.position).add(new THREE.Vector3(ox, 0, oz));
                 wHead = Math.atan2(-ox, -oz) * (180/Math.PI);
            }
        } else {
            wDoor.copy(homeDoor);
            wPark.copy(homePark);
            wBId = home.id;
            wHead = homeHeading;
        }
        
        registry.emplace(entity, HomeReferenceComponent, home.id, home.cityId, homeDoor, homePark, homeHeading);
        registry.emplace(entity, WorkReferenceComponent, wBId, wCId, wDoor, wPark, wHead);
    }
}
