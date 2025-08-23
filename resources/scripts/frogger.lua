-- resources/scripts/game.lua

-- Virtual resolution
local WIDTH, HEIGHT = 320, 180
local GRID = 16

-- Game Boy Colors
local GB_COLOR_BG    = {149/255, 188/255, 15/255, 1.0}   -- #9bbc0f
local GB_COLOR_LANE  = {139/255, 172/255, 15/255, 1.0}   -- #8bac0f
local GB_COLOR_FROG  = {48/255,  98/255,  48/255,  1.0}  -- #306230
local GB_COLOR_CAR   = {15/255,  56/255,  15/255,  1.0}  -- #0f380f

-- Compute grid-aligned center for frog start
local function align_grid(val)
    return math.floor(val / GRID) * GRID
end

-- Frog properties (always aligned to grid)
local frog = {
    x = align_grid(WIDTH // 2),
    y = align_grid(HEIGHT - GRID),
    w = GRID,
    h = GRID,
    color = GB_COLOR_FROG,
}

-- Lane and car properties
local LANES = {
    { y = align_grid(60),  speed = 40, count = 3, dir = 1 },
    { y = align_grid(90),  speed = 60, count = 2, dir = -1 },
    { y = align_grid(120), speed = 80, count = 2, dir = 1 },
}
local cars = {}
local car_w, car_h = 32, 16

local function reset_frog()
    frog.x = align_grid(WIDTH // 2)
    frog.y = align_grid(HEIGHT - GRID)
end

local function spawn_cars()
    cars = {}
    for _, lane in ipairs(LANES) do
        for i=1, lane.count do
            local offset = (i-1)*(WIDTH // lane.count)
            table.insert(cars, {
                x = (lane.dir == 1) and (offset) or (WIDTH - offset - car_w),
                y = lane.y,
                w = car_w,
                h = car_h,
                speed = lane.speed,
                dir = lane.dir
            })
        end
    end
end

local function collides(a, b)
    return not (a.x + a.w <= b.x or b.x + b.w <= a.x or a.y + a.h <= b.y or b.y + b.h <= a.y)
end

function leo.init()
    reset_frog()
    spawn_cars()
end

function leo.update(dt)
    -- Move frog one grid cell at a time, and always align to grid
    if leo.is_key_pressed_once("up") then
        frog.y = frog.y - GRID
    elseif leo.is_key_pressed_once("down") then
        frog.y = frog.y + GRID
    elseif leo.is_key_pressed_once("left") then
        frog.x = frog.x - GRID
    elseif leo.is_key_pressed_once("right") then
        frog.x = frog.x + GRID
    end

    -- Clamp frog to screen and realign to grid
    frog.x = math.max(0, math.min(frog.x, WIDTH - frog.w))
    frog.y = math.max(0, math.min(frog.y, HEIGHT - frog.h))
    frog.x = align_grid(frog.x)
    frog.y = align_grid(frog.y)

    -- Move cars
    for _, car in ipairs(cars) do
        car.x = car.x + car.speed * dt * car.dir
        -- Wrap cars around screen
        if car.dir == 1 and car.x > WIDTH then car.x = -car_w end
        if car.dir == -1 and car.x < -car_w then car.x = WIDTH end
        -- Collision with frog
        if collides(frog, car) then
            reset_frog()
        end
    end

    -- Win condition: reach the top
    if frog.y <= 0 then
        reset_frog()
    end

    -- Exit on ESCAPE
    if leo.is_key_pressed_once("escape") then
        leo.exit()
    end
end

function leo.render()
    -- Background
    leo.draw_filled_rect(0, 0, WIDTH, HEIGHT, table.unpack(GB_COLOR_BG))

    -- Lanes
    for _, lane in ipairs(LANES) do
        leo.draw_filled_rect(0, lane.y, WIDTH, car_h, table.unpack(GB_COLOR_LANE))
    end

    -- Cars
    for _, car in ipairs(cars) do
        leo.draw_filled_rect(car.x, car.y, car.w, car.h, table.unpack(GB_COLOR_CAR))
    end

    -- Frog
    leo.draw_filled_rect(frog.x, frog.y, frog.w, frog.h, table.unpack(frog.color))
end

function leo.exit()
    -- nothing to free in this simple example
end

