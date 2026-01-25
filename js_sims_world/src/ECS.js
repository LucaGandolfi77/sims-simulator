// Simple ECS Registry (Global Scope)
class Registry {
    constructor() {
        this.nextId = 0;
        this.entities = new Set();
        // Storage: Map<ComponentName, Map<EntityId, ComponentData>>
        this.components = {};
    }

    create() {
        const id = this.nextId++;
        this.entities.add(id);
        return id;
    }

    destroy(entity) {
        this.entities.delete(entity);
        for (const key in this.components) {
            this.components[key].delete(entity);
        }
    }

    // Add component
    emplace(entity, ComponentClass, ...args) {
        const name = ComponentClass.name;
        if (!this.components[name]) {
            this.components[name] = new Map();
        }
        
        // Instantiate
        const data = new ComponentClass(...args);
        this.components[name].set(entity, data);
        return data;
    }

    // Get component data
    get(entity, ComponentClass) {
        const name = ComponentClass.name;
        if (!this.components[name]) return null;
        return this.components[name].get(entity);
    }
    
    // Check if entity has component
    has(entity, ComponentClass) {
        const name = ComponentClass.name;
        return this.components[name] && this.components[name].has(entity);
    }

    // View iterator
    view(...componentClasses) {
        const names = componentClasses.map(c => c.name);
        
        let smallestMap = null;
        let minSize = Infinity;
        
        for (const name of names) {
            if (!this.components[name]) {
                return { each: () => {} };
            }
            const size = this.components[name].size;
            if (size < minSize) {
                minSize = size;
                smallestMap = this.components[name];
            }
        }

        const self = this;
        return {
            each: (callback) => {
                for (const entity of smallestMap.keys()) {
                    let hasAll = true;
                    const args = [entity];
                    
                    for (const name of names) {
                        if (!self.components[name].has(entity)) {
                            hasAll = false;
                            break;
                        }
                        args.push(self.components[name].get(entity));
                    }

                    if (hasAll) {
                        callback(...args);
                    }
                }
            }
        };
    }
}
