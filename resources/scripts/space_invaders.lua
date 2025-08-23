local cam = nil

-- Game state
local player = {}
local enemies = {}
local lasers = {}
local game_paused = false
local pause_timer = nil
local enemy_fire_enabled = false

-- Constants
local SCREEN_W, SCREEN_H = 1280, 720
local PLAYER_W, PLAYER_H = 40, 20
local ENEMY_W, ENEMY_H = 30, 20
local LASER_W, LASER_H = 4, 10

local PLAYER_SPEED = 300
local LASER_SPEED = 500
local ENEMY_ROWS = 3
local ENEMY_COLS = 8
local ENEMY_SPACING = 60

-- Enemy movement state
local enemy_direction = 1     -- 1 = right, -1 = left
local enemy_dx = 10           -- pixels per step
local enemy_dy = 20           -- pixels to descend on direction change
local enemy_move_timer = 0
local enemy_move_interval = 0.5  -- seconds between moves

function reset_game()
    -- Reset player
    player.x = SCREEN_W / 2 - PLAYER_W / 2
    player.y = SCREEN_H - 60
    player.w = PLAYER_W
    player.h = PLAYER_H
    player.alive = true

    -- Reset enemies (centered)
    enemies = {}
    local total_width = (ENEMY_COLS - 1) * ENEMY_SPACING
    local start_x = (SCREEN_W - total_width) / 2

    for row = 1, ENEMY_ROWS do
        for col = 1, ENEMY_COLS do
            local enemy = {
                x = start_x + (col - 1) * ENEMY_SPACING,
                y = 100 + (row - 1) * ENEMY_SPACING,
                w = ENEMY_W,
                h = ENEMY_H,
                alive = true
            }
            table.insert(enemies, enemy)
        end
    end

    lasers = {}
    game_paused = false
    enemy_fire_enabled = false
    enemy_direction = 1
    enemy_move_timer = 0

    leo.timer_after(2000, function()
        enemy_fire_enabled = true
    end)
end

function pause_and_reset(delay_ms)
    game_paused = true
    pause_timer = leo.timer_after(delay_ms, function()
        reset_game()
    end)
end

function fire_laser(x, y, dy)
    table.insert(lasers, {
        x = x,
        y = y,
        w = LASER_W,
        h = LASER_H,
        dy = dy
    })
end

function leo.init()
    cam = leo.camera.camera_create(SCREEN_W, SCREEN_H)
    reset_game()
end

function leo.update(dt)
    if game_paused then return end

    -- Player movement
    if leo.is_key_down("left") then
        player.x = player.x - PLAYER_SPEED * dt
    elseif leo.is_key_down("right") then
        player.x = player.x + PLAYER_SPEED * dt
    end

    -- Player shoot
    if leo.is_key_pressed_once("space") then
        fire_laser(player.x + player.w / 2 - LASER_W / 2, player.y, -LASER_SPEED)
    end

    -- Move lasers
    for i = #lasers, 1, -1 do
        local laser = lasers[i]
        laser.y = laser.y + laser.dy * dt
        if laser.y < -LASER_H or laser.y > SCREEN_H + LASER_H then
            table.remove(lasers, i)
        end
    end

    -- Laser collisions
    for i = #lasers, 1, -1 do
        local laser = lasers[i]

        if laser.dy < 0 then
            for _, enemy in ipairs(enemies) do
                if enemy.alive and rects_overlap(laser, enemy) then
                    enemy.alive = false
                    table.remove(lasers, i)
                    break
                end
            end
        else
            if player.alive and rects_overlap(laser, player) then
                player.alive = false
                table.remove(lasers, i)
                pause_and_reset(1000)
                return
            end
        end
    end

    -- Enemy fire
    if enemy_fire_enabled then
        for _, enemy in ipairs(enemies) do
            if enemy.alive and math.random() < dt * 0.1 then
                fire_laser(enemy.x + enemy.w / 2 - LASER_W / 2, enemy.y + enemy.h, LASER_SPEED)
            end
        end
    end

    -- Enemy movement (step every interval)
    enemy_move_timer = enemy_move_timer + dt
    if enemy_move_timer >= enemy_move_interval then
        enemy_move_timer = 0

        -- Determine if enemies hit screen edge
        local shift_down = false
        for _, enemy in ipairs(enemies) do
            if enemy.alive then
                local next_x = enemy.x + enemy_dx * enemy_direction
                if next_x < 0 or next_x + ENEMY_W > SCREEN_W then
                    shift_down = true
                    enemy_direction = -enemy_direction
                    break
                end
            end
        end

        for _, enemy in ipairs(enemies) do
            if enemy.alive then
                if shift_down then
                    enemy.y = enemy.y + enemy_dy
                else
                    enemy.x = enemy.x + enemy_dx * enemy_direction
                end
            end
        end
    end

    -- Win check
    local any_alive = false
    for _, enemy in ipairs(enemies) do
        if enemy.alive then
            any_alive = true
            break
        end
    end

    if not any_alive then
        pause_and_reset(3000)
    end
end

function leo.render()
    leo.draw_filled_rect(0, 0, SCREEN_W, SCREEN_H, 0.607, 0.737, 0.059, 1.0) -- #9bbc0f

    for _, enemy in ipairs(enemies) do
        if enemy.alive then
            leo.draw_filled_rect(enemy.x, enemy.y, enemy.w, enemy.h, 0.188, 0.384, 0.188, 1.0) -- #306230
        end
    end

    if player.alive then
        leo.draw_filled_rect(player.x, player.y, player.w, player.h, 0.188, 0.384, 0.188, 1.0) -- #306230
    end

    for _, laser in ipairs(lasers) do
        leo.draw_filled_rect(laser.x, laser.y, laser.w, laser.h, 0.059, 0.22, 0.059, 1.0) -- #0f380f
    end
end

function leo.exit()
end

function rects_overlap(a, b)
    return a.x < b.x + b.w and
           a.x + a.w > b.x and
           a.y < b.y + b.h and
           a.y + a.h > b.y
end

