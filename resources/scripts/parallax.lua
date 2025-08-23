-- Parallax scrolling demo for Leo Engine

local VRES_W, VRES_H = 1280, 720
local FLOOR_TILE_W = 64
local FLOOR_TILE_H = 32
local FLOOR_COLS = 40
local FLOOR_Y = VRES_H - FLOOR_TILE_H

local PLAYER_RADIUS = 24
local PLAYER_COLOR = {48/255, 98/255, 48/255, 1}
local FLOOR_COLOR = {15/255, 56/255, 15/255, 1}

local BG_IMG_PATH = "resources/images/parallax_background_1920x1080.png"

local player = {
    x = VRES_W // 2,
    y = FLOOR_Y - PLAYER_RADIUS,
    vx = 0,
    speed = 300
}
local cam = nil
local bg_tex = nil

function leo.init()
    bg_tex = leo.load_image(BG_IMG_PATH)
    -- Start player somewhere left of center to see parallax
    player.x = VRES_W // 2
    player.y = FLOOR_Y - PLAYER_RADIUS

    cam = leo.camera.camera_create(VRES_W, VRES_H)
    leo.camera.set_active_camera(cam)
    leo.camera.camera_set_position(cam, player.x, VRES_H // 2)
end

function leo.update(dt)
    player.vx = 0
    if leo.is_key_down("left") then
        player.vx = -player.speed
    elseif leo.is_key_down("right") then
        player.vx = player.speed
    end
    player.x = player.x + player.vx * dt

    if player.x < PLAYER_RADIUS then player.x = PLAYER_RADIUS end
    if player.x > FLOOR_COLS * FLOOR_TILE_W - PLAYER_RADIUS then
        player.x = FLOOR_COLS * FLOOR_TILE_W - PLAYER_RADIUS
    end

    leo.camera.camera_set_position(cam, player.x, VRES_H // 2)
end

function leo.render()
    -- Draw Parallax Background
    local cam_x = player.x
    local bg_w, bg_h = 1920, 1080
    local parallax_ratio = 0.5
    local bg_x = -(cam_x * parallax_ratio) + (VRES_W / 2) - (bg_w / 2)
    local bg_y = (VRES_H / 2) - (bg_h / 2)
    leo.render_image(bg_tex, bg_x, bg_y)

    -- Draw floor tiles
    for col = 0, FLOOR_COLS-1 do
        local tile_x = col * FLOOR_TILE_W
        leo.draw_filled_rect(
            tile_x, FLOOR_Y,
            FLOOR_TILE_W, FLOOR_TILE_H,
            table.unpack(FLOOR_COLOR)
        )
    end

    -- Draw player
    leo.draw_filled_circle(
        player.x, player.y, PLAYER_RADIUS,
        table.unpack(PLAYER_COLOR)
    )
end

function leo.exit()
    leo.free_image(bg_tex)
end

