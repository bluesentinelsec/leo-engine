-- resources/scripts/game.lua

-- Game constants
local WIDTH = 160
local HEIGHT = 144
local PADDLE_WIDTH = 2
local PADDLE_HEIGHT = 16
local BALL_RADIUS = 1
local PADDLE_SPEED = 100
local BALL_SPEED = 120
local SPEED_BOOST = 1.0

-- Gameboy colors (converted to 0-1 range)
local COLORS = {
    {149/255, 188/255, 15/255},  -- #9bbc0f background
    {139/255, 172/255, 15/255},  -- #8bac0f (unused here, but available)
    {48/255, 98/255, 48/255},    -- #306230 player & CPU
    {15/255, 56/255, 15/255}     -- #0f380f ball
}

-- Game state
local player = {x = 8, y = HEIGHT/2 - PADDLE_HEIGHT/2}
local cpu = {x = WIDTH - 16, y = HEIGHT/2 - PADDLE_HEIGHT/2}
local ball = {x = WIDTH/2, y = HEIGHT/2, vx = 0, vy = 0, active = true}
local ball_timer = nil
local cpu_pursuing = true
local cpu_switch_timer = nil

-- Camera
local cam = nil

function leo.init()
    cam = leo.camera.camera_create(WIDTH, HEIGHT)
    reset_ball()
end

function reset_ball()
    if ball_timer then
        leo.timer_cancel(ball_timer)
        ball_timer = nil
    end

    ball.x = WIDTH / 2
    ball.y = HEIGHT / 2
    ball.active = true

    local angle
    if math.random() < 0.5 then
        angle = math.random() * math.pi/6 + math.pi/6         -- 30° to 60°
    else
        angle = math.random() * math.pi/6 + 7*math.pi/6       -- 210° to 240°
    end

    ball.vx = math.cos(angle) * BALL_SPEED
    ball.vy = math.sin(angle) * BALL_SPEED
end

function leo.update(dt)

    if leo.is_key_pressed_once("escape") then
        os.exit()
    end

    -- Player movement
    if leo.is_key_down("up") then
        player.y = math.max(0, player.y - PADDLE_SPEED * dt)
    end
    if leo.is_key_down("down") then
        player.y = math.min(HEIGHT - PADDLE_HEIGHT, player.y + PADDLE_SPEED * dt)
    end

    -- CPU movement logic
    if cpu_pursuing then
        if ball.y < cpu.y + PADDLE_HEIGHT/2 then
            cpu.y = math.max(0, cpu.y - PADDLE_SPEED * dt)
        elseif ball.y > cpu.y + PADDLE_HEIGHT/2 then
            cpu.y = math.min(HEIGHT - PADDLE_HEIGHT, cpu.y + PADDLE_SPEED * dt)
        end
    else
        -- Intentionally move away from the ball
        if ball.y < cpu.y + PADDLE_HEIGHT/2 then
            cpu.y = math.min(HEIGHT - PADDLE_HEIGHT, cpu.y + PADDLE_SPEED * dt)
        elseif ball.y > cpu.y + PADDLE_HEIGHT/2 then
            cpu.y = math.max(0, cpu.y - PADDLE_SPEED * dt)
        end
    end

    -- Switch CPU behavior every 3 seconds
    if not cpu_switch_timer then
        cpu_switch_timer = leo.timer_after(3000, function()
            cpu_pursuing = not cpu_pursuing
            cpu_switch_timer = nil
        end)
    end

    -- Ball logic
    if ball.active then
        ball.x = ball.x + ball.vx * dt
        ball.y = ball.y + ball.vy * dt

        -- Bounce off top/bottom walls
        if ball.y <= BALL_RADIUS or ball.y >= HEIGHT - BALL_RADIUS then
            ball.vy = -ball.vy * SPEED_BOOST
            ball.y = math.max(BALL_RADIUS, math.min(HEIGHT - BALL_RADIUS, ball.y))
        end

        -- Bounce off player paddle
        if ball.x <= player.x + PADDLE_WIDTH + BALL_RADIUS and
           ball.y >= player.y and ball.y <= player.y + PADDLE_HEIGHT then
            ball.vx = math.abs(ball.vx) * SPEED_BOOST
        end

        -- Bounce off CPU paddle
        if ball.x >= cpu.x - BALL_RADIUS and
           ball.y >= cpu.y and ball.y <= cpu.y + PADDLE_HEIGHT then
            ball.vx = -math.abs(ball.vx) * SPEED_BOOST
        end

        -- Ball goes offscreen → deactivate and reset after delay
        if ball.x < -BALL_RADIUS or ball.x > WIDTH + BALL_RADIUS then
            ball.active = false
            ball.vx = 0
            ball.vy = 0
            if not ball_timer then
                ball_timer = leo.timer_after(2000, function()
                    reset_ball()
                end)
            end
        end
    end
end

function leo.render()
    -- Background
    leo.draw_filled_rect(0, 0, WIDTH, HEIGHT, table.unpack(COLORS[1]))

    -- Player and CPU
    leo.draw_filled_rect(player.x, player.y, PADDLE_WIDTH, PADDLE_HEIGHT, table.unpack(COLORS[3]))
    leo.draw_filled_rect(cpu.x, cpu.y, PADDLE_WIDTH, PADDLE_HEIGHT, table.unpack(COLORS[3]))

    -- Ball (only if active)
    if ball.active then
        leo.draw_filled_circle(ball.x, ball.y, BALL_RADIUS, table.unpack(COLORS[4]))
    end
end

function leo.exit()
    if cam then leo.camera.camera_free(cam) end
    if ball_timer then leo.timer_cancel(ball_timer) end
    if cpu_switch_timer then leo.timer_cancel(cpu_switch_timer) end
end

