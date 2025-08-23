-- resources/scripts/game.lua

-- Config
local WIDTH, HEIGHT = 640, 400
local GRAVITY = 1200
local MOVE_SPEED = 220
local JUMP_VELOCITY = 420
local PLAYER_W, PLAYER_H = 32, 48

-- Level platforms: {x, y, w, h}
local platforms = {
    { 0, HEIGHT-40, WIDTH, 40 },
    { 60, 280, 160, 20 },
    { 320, 330, 120, 20 },
    { 240, 220, 120, 20 },
    { 440, 170, 140, 20 },
    { 90, 120, 120, 20 },
}

local player = {
    x = 80, y = HEIGHT-40-PLAYER_H,
    w = PLAYER_W, h = PLAYER_H,
    vx = 0, vy = 0,
    grounded = false,
    can_jump = false,
}

local camera = nil

function reset_player()
    player.x = 80
    player.y = HEIGHT-40-PLAYER_H
    player.vx = 0
    player.vy = 0
end

function leo.init()
    camera = leo.camera.camera_create(WIDTH, HEIGHT)
    reset_player()
end

-- Basic AABB collision (rect1, rect2)
local function aabb(ax, ay, aw, ah, bx, by, bw, bh)
    return ax < bx+bw and ax+aw > bx and ay < by+bh and ay+ah > by
end

function leo.update(dt)
    if leo.is_key_pressed_once("escape") then leo.exit() end
    if leo.is_key_pressed_once("r") then reset_player() end

    -- Move left/right
    local move = 0
    if leo.is_key_down("left") or leo.is_key_down("a") then move = move - 1 end
    if leo.is_key_down("right") or leo.is_key_down("d") then move = move + 1 end
    player.vx = MOVE_SPEED * move

    -- Gravity
    player.vy = player.vy + GRAVITY * dt

    -- Jumping
    if (leo.is_key_pressed_once("space") or leo.is_key_pressed_once("z") or leo.is_key_pressed_once("up") or leo.is_key_pressed_once("w"))
      and player.grounded then
        player.vy = -JUMP_VELOCITY
        player.grounded = false
    end

    -- Predict movement
    local next_x = player.x + player.vx * dt
    local next_y = player.y + player.vy * dt

    -- Horizontal collision
    local hit_x = false
    for _, plat in ipairs(platforms) do
        if aabb(next_x, player.y, player.w, player.h, plat[1], plat[2], plat[3], plat[4]) then
            hit_x = true
            break
        end
    end
    if not hit_x then
        player.x = next_x
    else
        player.vx = 0
    end

    -- Vertical collision
    player.grounded = false
    local hit_y = false
    for _, plat in ipairs(platforms) do
        if aabb(player.x, next_y, player.w, player.h, plat[1], plat[2], plat[3], plat[4]) then
            hit_y = true
            -- Land on top of the platform
            if player.vy > 0 then
                player.y = plat[2] - player.h
                player.grounded = true
            elseif player.vy < 0 then
                player.y = plat[2] + plat[4]
            end
            player.vy = 0
            break
        end
    end
    if not hit_y then
        player.y = next_y
    end

    -- Keep player in bounds (left/right/floor/ceiling)
    if player.x < 0 then player.x = 0; player.vx = 0 end
    if player.x + player.w > WIDTH then player.x = WIDTH - player.w; player.vx = 0 end
    if player.y + player.h > HEIGHT then player.y = HEIGHT - player.h; player.vy = 0; player.grounded = true end
    if player.y < 0 then player.y = 0; player.vy = 0 end

    -- Camera follows player (centered, but clamp to level bounds)
    leo.camera.camera_set_position(camera,
        math.max(WIDTH/2, math.min(player.x + player.w/2, WIDTH-WIDTH/2)),
        math.max(HEIGHT/2, math.min(player.y + player.h/2, HEIGHT-HEIGHT/2))
    )
end

function leo.render()
    leo.draw_filled_rect(0, 0, WIDTH, HEIGHT, 0.15, 0.18, 0.28, 1.0) -- bg
    -- Platforms
    for _, plat in ipairs(platforms) do
        leo.draw_filled_rect(plat[1], plat[2], plat[3], plat[4], 0.23, 0.35, 0.15, 1.0)
    end
    -- Player
    leo.draw_filled_rect(player.x, player.y, player.w, player.h, 0.93, 0.74, 0.22, 1.0)
end

function leo.exit()
    if camera then leo.camera.camera_free(camera) end
end

