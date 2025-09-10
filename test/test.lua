local ecs = require "ecs"

local hand_texture = get_texture("test/hand")
local robot_texture = get_texture("test/robot")

-- Camera state variables
local camera_dragging = false
hide_cursor()

-- Mouse event handlers for camera control
local function handle_mouse_down(event)
    if event.button == MouseButton.LEFT then
        if not camera_dragging then
            camera_dragging = true
        end
    end
end

local function handle_mouse_up(event)
    if event.button == MouseButton.LEFT then
        if camera_dragging then
            camera_dragging = false
        end
    end
end

local function handle_mouse_move(event)
    if camera_dragging then
        local current_zoom = 1.0 / camera_zoom()
        camera_move(-event.position.dx * current_zoom, -event.position.dy * current_zoom)
    end
end

local function handle_mouse_scroll(event)
    camera_zoom_by(event.scroll.y * frame_duration())
end

-- Register event callbacks using the new system
register_event_callback(EventType.mouse_down, handle_mouse_down)
register_event_callback(EventType.mouse_up, handle_mouse_up)
register_event_callback(EventType.mouse_move, handle_mouse_move)
register_event_callback(EventType.mouse_scroll, handle_mouse_scroll)

-- Simple ImGui test window
function draw_imgui_test(it)
    local show_window = true
    show_window = imgui.begin_window("Test Window", show_window)
    if show_window then
        imgui.text("Hello from Lua!")
        imgui.text("Frame size: " .. framebuffer_width() .. "x" .. framebuffer_height())
        imgui.text("Frame duration: " .. string.format("%.3f", frame_duration()) .. "ms")
        
        if imgui.button("Test Button") then
            print("Button clicked from Lua!")
        end
        
        imgui.separator()
        imgui.text("Texture ID: " .. test_texture)
        
        imgui.separator()
        imgui.text("Camera Controls:")
        imgui.text("Drag: " .. (camera_dragging and "true" or "false"))
        
        local cam_x, cam_y = camera_position()
        imgui.text("Camera Position: (" .. string.format("%.2f", cam_x) .. ", " .. string.format("%.2f", cam_y) .. ")")
        imgui.text("Camera Zoom: " .. string.format("%.3f", camera_zoom()))
        
        local mouse_x, mouse_y = mouse_position()
        imgui.text("Mouse: (" .. string.format("%.2f", mouse_x) .. ", " .. string.format("%.2f", mouse_y) .. ")")
    end
    imgui.end_window()
end

-- Updated chunk event callbacks using new callback system
local function on_chunk_created(x, y)
    print("(LUA) Chunk created at (" .. x .. ", " .. y .. ")")
    if x == 0 and y == 0 then
        local test = ecs.new("Test", ChunkEntity)
        ecs.set(test, ChunkEntity, {
            texture_id = hand_texture,
            x = 5,
            y = 5,
            clip_width = 16,
            clip_height = 16,
            width = 16,
            height = 16
        })
        set_entity_target(test, 10, 10)
    end
end

local function on_chunk_deleted(x, y)
    print("(LUA) Chunk deleted at (" .. x .. ", " .. y .. ")")
end

local function on_chunk_visibility_changed(x, y, old_vis, new_vis)
    local visibility_names = {
        [ChunkVisibility.out_of_sign] = "OutOfSign",
        [ChunkVisibility.visible] = "Visible", 
        [ChunkVisibility.occluded] = "Occluded"
    }
    local old_name = visibility_names[old_vis] or tostring(old_vis)
    local new_name = visibility_names[new_vis] or tostring(new_vis)
    print("(LUA) Chunk at (" .. x .. ", " .. y .. ") visibility changed from " .. old_name .. " to " .. new_name)
end

-- Register chunk callbacks using the new enum-based system
register_chunk_callback(ChunkEventType.created, on_chunk_created)
register_chunk_callback(ChunkEventType.deleted, on_chunk_deleted)
register_chunk_callback(ChunkEventType.visibility_changed, on_chunk_visibility_changed)

-- Register the function to be called each frame
-- ecs.system(draw_imgui_test, "ImGuiTestSystem", ecs.OnUpdate)
