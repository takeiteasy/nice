local ecs = require "ecs"
local m = require "module"

print("=== ECS Lua Test Script ===")

-- Test the existing Position components
local ents = ecs.bulk_new(m.Position, 10)

for i = 1, #ents do
    ecs.set(ents[i], m.Position, { x = i * 2, y = i * 3})
end

print("Created", #ents, "entities with Position components")

-- Try different ways to get the LuaRenderable component
local renderable_comp = ecs.lookup("LuaRenderable")
print("LuaRenderable component ID (lookup):", renderable_comp)

-- Try looking up by module name
local renderable_module = ecs.lookup("Renderable")
print("Renderable module ID:", renderable_module)

-- If direct lookup fails, try to get it through the module
if not renderable_comp or renderable_comp == 0 then
    print("Direct lookup failed, trying alternative methods...")
    
    -- Try to find all available components
    print("\n=== Available Components ===")
    
    -- Create a test entity and try to add components we know exist
    local test_entity = ecs.new("test_entity")
    
    -- Try Position component as a test
    if m.Position then
        ecs.set(test_entity, m.Position, { x = 100, y = 200 })
        local pos_data = ecs.get(test_entity, m.Position)
        print("Position component works - x:", pos_data.x, "y:", pos_data.y)
    end
    
    -- Since we can't find LuaRenderable by lookup, let's create our own simple test
    print("\n=== Simple Entity Test (without LuaRenderable) ===")
    
    local sprite1 = ecs.new("background_sprite")
    local sprite2 = ecs.new("player_sprite") 
    local sprite3 = ecs.new("ui_element")
    
    print("Created entities:")
    print("  Background sprite ID:", sprite1, "Name:", ecs.name(sprite1))
    print("  Player sprite ID:", sprite2, "Name:", ecs.name(sprite2))
    print("  UI element ID:", sprite3, "Name:", ecs.name(sprite3))
    
    -- Test entity operations
    ecs.add_name(sprite1, "renamed_background")
    print("Renamed sprite1 to:", ecs.name(sprite1))
    
    -- Test entity deletion
    ecs.delete(test_entity)
    print("Deleted test entity")
    
else
    print("Found LuaRenderable component, running full test...")
    
    print("\n=== Creating LuaRenderable Entities ===")
    
    -- Create some renderable entities with different layers
    local sprite1 = ecs.new("background_sprite")
    ecs.set(sprite1, renderable_comp, { 
        visible = true, 
        render_layer = 0
    })
    print("Created background sprite (layer 0):", sprite1)
    
    local sprite2 = ecs.new("player_sprite")
    ecs.set(sprite2, renderable_comp, { 
        visible = true, 
        render_layer = 2
    })
    print("Created player sprite (layer 2):", sprite2)
    
    local sprite3 = ecs.new("ui_element")
    ecs.set(sprite3, renderable_comp, { 
        visible = true, 
        render_layer = 5
    })
    print("Created UI element (layer 5):", sprite3)
    
    print("\n=== Reading LuaRenderable Components ===")
    
    local bg_data = ecs.get(sprite1, renderable_comp)
    print("Background sprite - visible:", bg_data.visible, "layer:", bg_data.render_layer)
    
    local player_data = ecs.get(sprite2, renderable_comp)
    print("Player sprite - visible:", player_data.visible, "layer:", player_data.render_layer)
    
    local ui_data = ecs.get(sprite3, renderable_comp)
    print("UI element - visible:", ui_data.visible, "layer:", ui_data.render_layer)
    
    print("\n=== Component Operations ===")
    
    -- Test has component
    print("Background sprite has LuaRenderable:", ecs.has(sprite1, renderable_comp))
    
    -- Test remove component
    ecs.remove(sprite1, renderable_comp)
    print("Removed LuaRenderable from background sprite")
    print("Background sprite still has LuaRenderable:", ecs.has(sprite1, renderable_comp))
end

print("\n=== Entity Count Test ===")
local total_entities = 0
-- This is a simple way to count entities without complex queries
for i = 1, 1000 do
    if ecs.is_alive(i) then
        total_entities = total_entities + 1
    end
end
print("Approximate total entities:", total_entities)

print("\nTest script complete!")
