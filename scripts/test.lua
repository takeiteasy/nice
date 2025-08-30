local ecs = require "ecs"
local m = require "module"

local test_texture = register_texture("hand.png")
-- local EcsRenderable = ecs.lookup("LuaRenderable")

local test = ecs.new("Test", EcsRenderable)
ecs.set(test, EcsRenderable, {
    texture_id = test_texture,
    x = 100,
    y = 100
})

-- Simple ImGui test window
function draw_imgui_test(it)
    local show_window = true
    show_window = imgui.begin_window("Test Window", show_window)
    if show_window then
        imgui.text("Hello from Lua!")
        imgui.text("Frame size: " .. framebuffer_width() .. "x" .. framebuffer_height())
        
        if imgui.button("Test Button") then
            print("Button clicked from Lua!")
        end
        
        imgui.separator()
        imgui.text("Texture ID: " .. test_texture)
    end
    imgui.end_window()
end

-- Register the function to be called each frame
ecs.system(draw_imgui_test, "ImGuiTestSystem", ecs.OnUpdate)
