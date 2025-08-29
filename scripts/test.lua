local ecs = require "ecs"
local m = require "module"

local test_texture = register_texture("hand.png")
local EcsRenderable = ecs.lookup("LuaRenderable")

local test = ecs.new("Test", EcsRenderable)
ecs.set(test, EcsRenderable, {
    texture_id = test_texture,
    x = 100,
    y = 100
})