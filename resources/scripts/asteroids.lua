-- resources/scripts/game.lua

-- Resolution
local WIDTH, HEIGHT = 1280, 720

-- Gameboy palette
local COLOR_PLAYER   = {149/255, 188/255, 15/255, 1.0}   -- #9bbc0f
local COLOR_LASER    = {139/255, 172/255, 15/255, 1.0}   -- #8bac0f
local COLOR_ASTEROID = {48/255,  98/255,  48/255, 1.0}   -- #306230
local COLOR_BG       = {15/255,  56/255,  15/255, 1.0}   -- #0f380f

-- Ship
local ship = {
    x = WIDTH / 2,
    y = HEIGHT / 2,
    angle = 0,
    radius = 24,
    speed = 0,
    vx = 0,
    vy = 0,
    thrust = 400,
    rotate_speed = math.rad(180),
    max_speed = 400,
    fire_delay = 0.2,
    fire_timer = 0
}

-- Laser
local lasers = {}
local LASER_SPEED = 700
local LASER_RADIUS = 4

-- Asteroids
local asteroids = {}
local ASTEROID_MIN_RADIUS = 36
local ASTEROID_MAX_RADIUS = 72
local ASTEROID_MIN_SPEED = 40
local ASTEROID_MAX_SPEED = 140
local ASTEROID_COUNT = 5

local function reset_ship()
    ship.x = WIDTH / 2
    ship.y = HEIGHT / 2
    ship.angle = 0
    ship.vx = 0
    ship.vy = 0
    ship.speed = 0
end

local function spawn_asteroids()
    asteroids = {}
    for i=1,ASTEROID_COUNT do
        local radius = math.random(ASTEROID_MIN_RADIUS, ASTEROID_MAX_RADIUS)
        local angle = math.random() * math.pi * 2
        local speed = math.random(ASTEROID_MIN_SPEED, ASTEROID_MAX_SPEED)
        local x, y
        repeat
            x = math.random(radius, WIDTH-radius)
            y = math.random(radius, HEIGHT-radius)
        until math.abs(x - ship.x) > 200 and math.abs(y - ship.y) > 200
        table.insert(asteroids, {
            x = x,
            y = y,
            radius = radius,
            angle = angle,
            speed = speed,
        })
    end
end

local function shoot_laser()
    local angle = ship.angle
    table.insert(lasers, {
        x = ship.x + math.cos(angle) * ship.radius,
        y = ship.y + math.sin(angle) * ship.radius,
        vx = math.cos(angle) * LASER_SPEED + ship.vx,
        vy = math.sin(angle) * LASER_SPEED + ship.vy,
        ttl = 1.0
    })
end

local function wrap_position(obj, radius)
    if obj.x < -radius then obj.x = WIDTH + radius end
    if obj.x > WIDTH + radius then obj.x = -radius end
    if obj.y < -radius then obj.y = HEIGHT + radius end
    if obj.y > HEIGHT + radius then obj.y = -radius end
end

local function dist(x1,y1,x2,y2)
    local dx, dy = x2-x1, y2-y1
    return math.sqrt(dx*dx + dy*dy)
end

local cam = nil

function leo.init()
    cam = leo.camera.camera_create(1280, 720)
    reset_ship()
    spawn_asteroids()
    lasers = {}
end

function leo.update(dt)
    -- Controls
    if leo.is_key_down("left")  then
        ship.angle = ship.angle - ship.rotate_speed * dt
    end
    if leo.is_key_down("right") then
        ship.angle = ship.angle + ship.rotate_speed * dt
    end
    if leo.is_key_down("up") then
        ship.vx = ship.vx + math.cos(ship.angle) * ship.thrust * dt
        ship.vy = ship.vy + math.sin(ship.angle) * ship.thrust * dt
        -- Limit speed
        local speed = math.sqrt(ship.vx*ship.vx + ship.vy*ship.vy)
        if speed > ship.max_speed then
            local scale = ship.max_speed / speed
            ship.vx = ship.vx * scale
            ship.vy = ship.vy * scale
        end
    end

    -- Shoot laser
    ship.fire_timer = ship.fire_timer - dt
    if leo.is_key_pressed_once("space") and ship.fire_timer <= 0 then
        shoot_laser()
        ship.fire_timer = ship.fire_delay
    end

    -- Move ship
    ship.x = ship.x + ship.vx * dt
    ship.y = ship.y + ship.vy * dt
    wrap_position(ship, ship.radius)

    -- Update lasers
    for i=#lasers,1,-1 do
        local laser = lasers[i]
        laser.x = laser.x + laser.vx * dt
        laser.y = laser.y + laser.vy * dt
        laser.ttl = laser.ttl - dt
        wrap_position(laser, LASER_RADIUS)
        if laser.ttl <= 0 then
            table.remove(lasers, i)
        end
    end

    -- Update asteroids
    for _, ast in ipairs(asteroids) do
        ast.x = ast.x + math.cos(ast.angle) * ast.speed * dt
        ast.y = ast.y + math.sin(ast.angle) * ast.speed * dt
        wrap_position(ast, ast.radius)
    end

    -- Laser <-> asteroid collision
    for li=#lasers,1,-1 do
        local laser = lasers[li]
        for ai=#asteroids,1,-1 do
            local ast = asteroids[ai]
            if dist(laser.x, laser.y, ast.x, ast.y) < ast.radius then
                table.remove(asteroids, ai)
                table.remove(lasers, li)
                -- Respawn if all gone
                if #asteroids == 0 then
                    spawn_asteroids()
                end
                break
            end
        end
    end

    -- Ship <-> asteroid collision
    for _, ast in ipairs(asteroids) do
        if dist(ship.x, ship.y, ast.x, ast.y) < ast.radius + ship.radius*0.7 then
            reset_ship()
            break
        end
    end

    -- Exit on ESC
    if leo.is_key_pressed_once("escape") then
        leo.exit()
    end
end

function leo.render()
    -- Background
    leo.draw_filled_rect(0, 0, WIDTH, HEIGHT, table.unpack(COLOR_BG))

    -- Draw asteroids (as circles)
    for _, ast in ipairs(asteroids) do
        leo.draw_filled_circle(ast.x, ast.y, ast.radius, table.unpack(COLOR_ASTEROID))
    end

    -- Draw lasers (as small circles)
    for _, laser in ipairs(lasers) do
        leo.draw_filled_circle(laser.x, laser.y, LASER_RADIUS, table.unpack(COLOR_LASER))
    end

    -- Draw ship (triangle)
    local ang = ship.angle
    local r = ship.radius
    local px = ship.x
    local py = ship.y
    local function vertex(theta)
        return px + math.cos(theta) * r, py + math.sin(theta) * r
    end
    local x1, y1 = vertex(ang)
    local x2, y2 = vertex(ang + 2.5)
    local x3, y3 = vertex(ang - 2.5)
    leo.draw_filled_circle(x1, y1, 4, table.unpack(COLOR_PLAYER))
    leo.draw_filled_circle(x2, y2, 4, table.unpack(COLOR_PLAYER))
    leo.draw_filled_circle(x3, y3, 4, table.unpack(COLOR_PLAYER))
    -- Draw lines to make ship "triangle"
    leo.draw_filled_rect((x1+x2)/2-2, (y1+y2)/2-2, 4, 4, table.unpack(COLOR_PLAYER))
    leo.draw_filled_rect((x1+x3)/2-2, (y1+y3)/2-2, 4, 4, table.unpack(COLOR_PLAYER))
    leo.draw_filled_rect((x2+x3)/2-2, (y2+y3)/2-2, 4, 4, table.unpack(COLOR_PLAYER))
end

function leo.exit()
    -- nothing to free
end

