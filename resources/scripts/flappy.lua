-- resources/scripts/game.lua

-- Config
local LOGICAL_W, LOGICAL_H = 480, 640
local BIRD_RADIUS = 20
local PIPE_W = 64
local GAP_H = 180
local PIPE_SPEED = 160
local PIPE_INTERVAL = 1.4 -- seconds

-- Colors
local COLORS = {
    bg   = {0.8, 0.9, 1.0, 1.0},
    bird = {1.0, 0.9, 0.2, 1.0},
    pipe = {0.2, 0.8, 0.3, 1.0},
    ground = {0.5, 0.3, 0.1, 1.0},
}

-- Game state
local camera = nil
local bird = {}
local pipes = {}
local timer = 0
local pipe_timer = 0
local game_over = false
local score = 0

local function reset()
    bird.x = LOGICAL_W // 3
    bird.y = LOGICAL_H // 2
    bird.vy = 0
    pipes = {}
    pipe_timer = 0
    timer = 0
    score = 0
    game_over = false
end

function leo.init()
    camera = leo.camera.camera_create(LOGICAL_W, LOGICAL_H)
    reset()
end

function spawn_pipe()
    local min_y = 80
    local max_y = LOGICAL_H - GAP_H - 120
    local gap_y = math.random(min_y, max_y)
    table.insert(pipes, {
        x = LOGICAL_W + PIPE_W,
        gap_y = gap_y,
        passed = false,
    })
end

function leo.update(dt)
    if leo.is_key_pressed_once("escape") then leo.exit() end

    if game_over then
        if leo.is_key_pressed_once("space") or leo.is_key_pressed_once("return") then
            reset()
        end
        return
    end

    timer = timer + dt
    pipe_timer = pipe_timer + dt

    -- Bird physics
    bird.vy = bird.vy + 700 * dt
    bird.y = bird.y + bird.vy * dt

    -- Flap
    if leo.is_key_pressed_once("space") or leo.is_key_pressed_once("up") then
        bird.vy = -280
    end

    -- Pipes
    if pipe_timer >= PIPE_INTERVAL then
        pipe_timer = pipe_timer - PIPE_INTERVAL
        spawn_pipe()
    end

    for i = #pipes, 1, -1 do
        local pipe = pipes[i]
        pipe.x = pipe.x - PIPE_SPEED * dt

        -- Scoring: pass pipe
        if not pipe.passed and pipe.x + PIPE_W < bird.x then
            pipe.passed = true
            score = score + 1
        end

        -- Remove offscreen pipes
        if pipe.x + PIPE_W < 0 then
            table.remove(pipes, i)
        end
    end

    -- Collision detection
    for _, pipe in ipairs(pipes) do
        -- Top pipe
        if bird.x + BIRD_RADIUS > pipe.x and bird.x - BIRD_RADIUS < pipe.x + PIPE_W then
            if bird.y - BIRD_RADIUS < pipe.gap_y or bird.y + BIRD_RADIUS > pipe.gap_y + GAP_H then
                game_over = true
                break
            end
        end
    end

    -- Floor/Ceiling collision
    if bird.y + BIRD_RADIUS > LOGICAL_H - 30 or bird.y - BIRD_RADIUS < 0 then
        game_over = true
    end
end

function leo.render()
    -- Set camera for scaling
    leo.camera.camera_set_position(camera, LOGICAL_W / 2, LOGICAL_H / 2)

    -- Background
    leo.draw_filled_rect(0, 0, LOGICAL_W, LOGICAL_H, table.unpack(COLORS.bg))

    -- Ground
    leo.draw_filled_rect(0, LOGICAL_H - 30, LOGICAL_W, 30, table.unpack(COLORS.ground))

    -- Pipes
    for _, pipe in ipairs(pipes) do
        -- Top pipe
        leo.draw_filled_rect(pipe.x, 0, PIPE_W, pipe.gap_y, table.unpack(COLORS.pipe))
        -- Bottom pipe
        leo.draw_filled_rect(pipe.x, pipe.gap_y + GAP_H, PIPE_W, LOGICAL_H - (pipe.gap_y + GAP_H + 30), table.unpack(COLORS.pipe))
    end

    -- Bird
    leo.draw_filled_circle(bird.x, bird.y, BIRD_RADIUS, table.unpack(COLORS.bird))

    -- Score
    -- Replace with leo.draw_text if you have it, or draw rectangles as placeholders
    -- leo.draw_text("Score: "..score, 8, 8, ...)
    -- For now, just show a block if you like

    -- Game over
    if game_over then
        leo.draw_filled_rect(LOGICAL_W/2-110, LOGICAL_H/2-32, 220, 64, 0,0,0,0.7)
        -- Draw "Game Over" if you have text, or nothing
    end
end

function leo.exit()
    if camera then leo.camera.camera_free(camera) end
end

