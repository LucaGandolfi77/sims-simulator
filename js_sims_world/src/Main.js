// Main.js (Global Scope, No Modules)

// ---- Config ----
const SIM_COUNT = 2000;
let timeMultiplier = 30.0;
let isPaused = false;

// ---- Three.js Setup ----
const scene = new THREE.Scene();
scene.background = new THREE.Color(0xefefef);
const camera = new THREE.PerspectiveCamera(60, window.innerWidth / window.innerHeight, 0.1, 5000);
camera.position.set(0, 300, 300);

const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.shadowMap.enabled = true;
document.body.appendChild(renderer.domElement);

const controls = new THREE.OrbitControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.screenSpacePanning = false;
controls.minDistance = 10;
controls.maxDistance = 2000;
controls.maxPolarAngle = Math.PI / 2 - 0.1;

// Lights
const ambientLight = new THREE.AmbientLight(0xFFFFFF, 0.6);
scene.add(ambientLight);

const dirLight = new THREE.DirectionalLight(0xFFFFFF, 0.8);
dirLight.position.set(500, 1000, 500);
dirLight.castShadow = true;
dirLight.shadow.camera.top = 2000;
dirLight.shadow.camera.bottom = -2000;
dirLight.shadow.camera.left = -2000;
dirLight.shadow.camera.right = 2000;
dirLight.shadow.bias = -0.0005;
dirLight.shadow.mapSize.width = 2048;
dirLight.shadow.mapSize.height = 2048;
scene.add(dirLight);

// ---- ECS Setup ----
const registry = new Registry();

// ---- World Generation ----
console.log("Generating World...");
generateWorld(SIM_COUNT, registry);
populateSims(registry, SIM_COUNT);
console.log("World Generated.");

// ---- Rendering Setup (Static) ----
const buildingGeo = new THREE.BoxGeometry(1, 1, 1);
const roadMat = new THREE.MeshLambertMaterial({ color: 0x333333 });

const planeGeo = new THREE.PlaneGeometry(10000, 10000);
const plane = new THREE.Mesh(planeGeo, new THREE.MeshBasicMaterial({ color: 0x1a1a1a }));
plane.rotation.x = -Math.PI / 2;
plane.position.y = -0.1;
scene.add(plane);

function createBox(pos, w, h, l, color) {
    const mesh = new THREE.Mesh(buildingGeo, new THREE.MeshLambertMaterial({ color: color }));
    mesh.position.copy(pos);
    mesh.scale.set(w, h, l);
    mesh.castShadow = true;
    mesh.receiveShadow = true;
    scene.add(mesh);
    return mesh;
}

// Render Static
nation.forEach(city => {
    city.buildings.forEach(b => {
        createBox(b.position, b.width, b.height, b.length, b.color);
    });
    
    city.localRoads.forEach(r => {
        const len = r.start.distanceTo(r.end);
        const center = r.start.clone().add(r.end).multiplyScalar(0.5);
        const dir = r.end.clone().sub(r.start).normalize();
        const angle = Math.atan2(dir.x, dir.z);
        
        const mesh = new THREE.Mesh(buildingGeo, roadMat);
        mesh.position.copy(center);
        mesh.scale.set(r.width, 0.1, len);
        mesh.rotation.y = angle;
        scene.add(mesh);
    });
});

highways.forEach(r => {
    const len = r.start.distanceTo(r.end);
    const center = r.start.clone().add(r.end).multiplyScalar(0.5);
    const dir = r.end.clone().sub(r.start).normalize();
    const angle = Math.atan2(dir.x, dir.z);
    
    const mesh = new THREE.Mesh(buildingGeo, roadMat);
    mesh.position.copy(center);
    mesh.scale.set(r.width, 0.1, len);
    mesh.rotation.y = angle;
    scene.add(mesh);
});

// ---- Sim Rendering ----
const simMeshes = new Map();
// Simple geometries
const simGeo = new THREE.BoxGeometry(0.5, 1.8, 0.5); // Fallback geometry since capsules didn't exist in r126?
// Actually r126 has CapsuleGeometry? No, added in r137. Use cylinder/sphere or custom.
// Using Cylinder for r126 compat.
const personGeo = new THREE.CylinderGeometry(0.3, 0.3, 1.8, 8);
const carGeo = new THREE.BoxGeometry(2.0, 1.0, 4.0);

function updateSimVis() {
    registry.view(TransformComponent, VisualComponent, StateComponent).each((entity, trans, vis, state) => {
        let mesh = simMeshes.get(entity);
        const isDriving = (state.state === SimState.DRIVING_TO_WORK || state.state === SimState.DRIVING_HOME);
        
        if (!mesh) {
            const mat = new THREE.MeshLambertMaterial({ color: vis.bodyColor });
            mesh = new THREE.Mesh(personGeo, mat);
            scene.add(mesh);
            simMeshes.set(entity, mesh);
        }
        
        if (isDriving && !mesh.userData.isCar) {
            mesh.geometry = carGeo;
            mesh.position.y = 0.5;
            mesh.material.color.setHex(vis.carColor);
            mesh.userData.isCar = true;
        } else if (!isDriving && (mesh.userData.isCar || mesh.userData.isCar === undefined)) {
            mesh.geometry = personGeo;
            mesh.position.y = 1.0;
            mesh.material.color.setHex(vis.bodyColor);
            mesh.userData.isCar = false;
        }
        
        mesh.position.x = trans.position.x;
        mesh.position.z = trans.position.z;
        mesh.position.y = isDriving ? 0.5 : 0.9;
        mesh.rotation.y = trans.rotation;
        
        if (state.state === SimState.SLEEPING || state.state === SimState.WORKING) {
            mesh.visible = false;
        } else {
            mesh.visible = true;
        }
    });
}

// ---- Raycasting ----
const raycaster = new THREE.Raycaster();
const mouse = new THREE.Vector2();
let selectedEntity = null;

function onMouseClick(event) {
    if (event.target.tagName !== 'CANVAS') return; // Ignore clicks on UI
    mouse.x = (event.clientX / window.innerWidth) * 2 - 1;
    mouse.y = -(event.clientY / window.innerHeight) * 2 + 1;
    
    raycaster.setFromCamera(mouse, camera);
    
    const meshes = Array.from(simMeshes.values()).filter(m => m.visible);
    const intersects = raycaster.intersectObjects(meshes);
    
    if (intersects.length > 0) {
        const mesh = intersects[0].object;
        for (const [ent, m] of simMeshes.entries()) {
            if (m === mesh) {
                selectedEntity = ent;
                document.getElementById('selection-panel').style.display = 'block';
                updateSelectionUI();
                break;
            }
        }
    } else {
        selectedEntity = null;
        document.getElementById('selection-panel').style.display = 'none';
    }
}
window.addEventListener('click', onMouseClick);
window.addEventListener('resize', () => {
    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();
    renderer.setSize(window.innerWidth, window.innerHeight);
});

function updateSelectionUI() {
    if (selectedEntity === null) return;
    
    const iden = registry.get(selectedEntity, IdentityComponent);
    const stats = registry.get(selectedEntity, SimStatsComponent);
    const state = registry.get(selectedEntity, StateComponent);
    
    if (iden) document.getElementById('sel-name').innerText = `${iden.firstName} ${iden.lastName}`;
    if (stats) {
        document.getElementById('sel-age').innerText = `Age: ${stats.age}`;
        document.getElementById('sel-money').innerText = `Money: $${Math.floor(stats.money)}`;
    }
    if (state) {
        const stateNames = ["Sleeping", "Walking to Car", "Driving to Work", "Working", "Walking to Car (Work)", "Driving Home", "Walking Home", "Walking to Car (Work)", "Walking from Car"];
        document.getElementById('sel-state').innerText = `State: ${stateNames[state.state] || 'Unknown ('+state.state+')'}`;
    }
}

// ---- UI Listeners ----
// We must wait for DOM or just ensure these exist (they do in index.html)
// Just separate init? No, script is at end of body.
document.getElementById('btn-pause').onclick = function() { isPaused = !isPaused; this.innerText = isPaused ? "Resume" : "Pause"; };
document.getElementById('btn-speed-1').onclick = () => { timeMultiplier = 1.0; };
document.getElementById('btn-speed-30').onclick = () => { timeMultiplier = 30.0; };
document.getElementById('btn-speed-100').onclick = () => { timeMultiplier = 100.0; };

// ---- Main Loop ----
const clock = new THREE.Clock();
let fpsTime = 0;
let frames = 0;

function animate() {
    requestAnimationFrame(animate);
    
    const rawDt = clock.getDelta();
    const dt = isPaused ? 0 : rawDt * timeMultiplier;
    
    if (!isPaused) {
        UpdateStats(registry, dt);
        UpdateStateMachine(registry, dt);
        UpdateMovement(registry, dt);
    }
    
    updateSimVis();
    controls.update();
    renderer.render(scene, camera);
    
    if (selectedEntity !== null) updateSelectionUI();
    
    fpsTime += rawDt;
    frames++;
    if (fpsTime >= 0.5) {
        document.getElementById('fps-counter').innerText = `FPS: ${Math.round(frames / fpsTime)}`;
        
        let sSleep = 0, sWork = 0, sDrive = 0;
        registry.view(StateComponent).each((e, state) => {
            if (state.state === SimState.SLEEPING) sSleep++;
            else if (state.state === SimState.WORKING) sWork++;
            else if (state.state === SimState.DRIVING_TO_WORK || state.state === SimState.DRIVING_HOME) sDrive++;
        });
        document.getElementById('cnt-driving').innerText = sDrive;
        document.getElementById('cnt-working').innerText = sWork;
        document.getElementById('cnt-sleeping').innerText = sSleep;
        
        frames = 0;
        fpsTime = 0;
    }
}

animate();
