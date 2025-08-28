local ecs = require "ecs"
local m = require "module"

local ents = ecs.bulk_new(m.Position, 10)

for i = 1, #ents do
    ecs.set(ents[i], m.Position, { x = i * 2, y = i * 3})
end

local renderable = ecs.lookup("Renderable")
print("renderable ID:", renderable)
if renderable then
    local no_id_comp = ecs.new("no_id", renderable)
    assert(ecs.name(no_id_comp) == "no_id")
    assert(ecs.has(no_id_comp, renderable))
    print("Successfully created entity with renderable")
    
    -- Get the current component value
    local current_value = ecs.get(no_id_comp, renderable)
    print("Current renderable value:")
    print("  dummy:", current_value.dummy)
    
    -- Modify the component
    current_value.dummy = false
    ecs.set(no_id_comp, renderable, current_value)
    print("Changed dummy to false")
    
    -- Verify the change
    local updated_value = ecs.get(no_id_comp, renderable)
    print("Updated renderable value:")
    print("  dummy:", updated_value.dummy)
    
    -- Change it back
    ecs.set(no_id_comp, renderable, { dummy = true })
    print("Changed dummy back to true using direct table")
    
    -- Final verification
    local final_value = ecs.get(no_id_comp, renderable)
    print("Final renderable value:")
    print("  dummy:", final_value.dummy)
else
    print("Failed to lookup renderable")
end
