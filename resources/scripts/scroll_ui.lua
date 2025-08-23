local cam = nil
local bg = nil
local cam_speed = 200   -- pixels per second

function leo.init()
    cam = leo.camera.camera_create(1280, 720)
    bg = leo.load_image("resources/images/background_1920x1080.png")
end

function leo.update(dt)
    local dx, dy = 0, 0
    if leo.is_key_down("left") then  dx = dx - cam_speed * dt end
    if leo.is_key_down("right") then dx = dx + cam_speed * dt end
    if leo.is_key_down("up") then    dy = dy - cam_speed * dt end
    if leo.is_key_down("down") then  dy = dy + cam_speed * dt end

    if dx ~= 0 or dy ~= 0 then
        leo.camera.camera_move(cam, dx, dy)
    end
end

function leo.render()
    -- Draw world layer (affected by camera)
    leo.render_image(bg, 0, 0)

    -- Draw fixed UI elements (not affected by camera)
    local previous = leo.camera.push_no_camera()
    leo.draw_filled_circle(20, 20, 10, 1.0, 1.0, 1.0, 1.0) -- White circle at (20, 20)
    leo.camera.pop_camera(previous)
end

function leo.exit()
    leo.free_image(bg)
    leo.camera.camera_free(cam)
end

