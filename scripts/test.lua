local ecs = require "ecs"
local m = require "module"


local renderable_comp = ecs.lookup("LuaRenderable")
print("LuaRenderable component ID (lookup):", renderable_comp)

local chunk_comp = ecs.lookup("LuaRenderable")
print("ChunkComp component ID (lookup):", chunk_comp)

local renderable_module = ecs.lookup("Renderable")
print("Renderable module ID:", renderable_module)

local test = ecs.new("Test")
print("Test entity:", test)
ecs.set(test, renderable_comp,  {x = 50, y = 20})
local data = ecs.get(test, renderable_comp)
print("Test Renderable component data:", data.x, data.y)
local chunk = ecs.get(test, chunk_comp)
chunk.x = 10
chunk.y = 20
print("Test ChunkComp component data:", chunk.x, chunk.y)
