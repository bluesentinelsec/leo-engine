-- resources/scripts/game.lua

-- Gameboy Colors
local COLORS = {
    BG    = {149/255, 188/255, 15/255},  -- #9bbc0f (background)
    PATH  = {139/255, 172/255, 15/255},  -- #8bac0f (not used but available)
    PLAYER= {48/255, 98/255, 48/255},    -- #306230
    WALL  = {15/255, 56/255, 15/255},    -- #0f380f
    FINISH= {1.0, 1.0, 1.0},             -- white for finish
}

-- Maze configuration
local MAZE_W, MAZE_H = 80, 10          -- 24px per cell for 1920x200
local CELL_SIZE = 24                   -- 80*24=1920, 10*20=200
local VIRTUAL_W, VIRTUAL_H = 160, 144

-- Maze, player, finish
local maze = {}
local player = {
    x=1, y=1,          -- Grid position (cell)
    fx=1, fy=1,        -- Floating point position (for smoothness)
    speed=5            -- cells per second; tune this to change movement speed!
}
local finish = {x=MAZE_W-2, y=MAZE_H-2}

-- Camera
local cam = nil

-- Utilities
local function shuffle(t)
    for i = #t, 2, -1 do
        local j = math.random(1, i)
        t[i], t[j] = t[j], t[i]
    end
end

-- Maze Generation: Recursive backtracker
local function generate_maze()
    maze = {}
    for y=1,MAZE_H do
        maze[y] = {}
        for x=1,MAZE_W do
            maze[y][x] = 1 -- wall
        end
    end

    local function carve(x, y)
        maze[y][x] = 0 -- path
        local dirs = {{1,0},{-1,0},{0,1},{0,-1}}
        shuffle(dirs)
        for _, d in ipairs(dirs) do
            local nx, ny = x + d[1]*2, y + d[2]*2
            if nx > 1 and nx < MAZE_W and ny > 1 and ny < MAZE_H and maze[ny][nx] == 1 then
                maze[y+d[2]][x+d[1]] = 0
                carve(nx, ny)
            end
        end
    end

    carve(2,2)
    -- Set entry and exit points (guaranteed open)
    maze[2][1] = 0  -- Entry
    maze[MAZE_H-1][MAZE_W] = 0 -- Exit

    -- Place player/start
    player.x, player.y = 1, 2
    player.fx, player.fy = 1, 2
    finish.x, finish.y = MAZE_W, MAZE_H-1
end

local function can_move_to(x, y)
    return x >= 1 and x <= MAZE_W and y >= 1 and y <= MAZE_H and maze[y][x] == 0
end

-- Center camera on player, clamp to world bounds
local function update_camera()
    local cam_x = player.fx * CELL_SIZE + CELL_SIZE/2
    local cam_y = player.fy * CELL_SIZE + CELL_SIZE/2

    local world_w = MAZE_W * CELL_SIZE
    local world_h = MAZE_H * CELL_SIZE

    -- Camera center position, clamp to world edge
    local cam_half_w = VIRTUAL_W / 2
    local cam_half_h = VIRTUAL_H / 2
    cam_x = math.max(cam_half_w, math.min(cam_x, world_w - cam_half_w))
    cam_y = math.max(cam_half_h, math.min(cam_y, world_h - cam_half_h))
    leo.camera.camera_set_position(cam, cam_x, cam_y)
end

function leo.init()
    math.randomseed(os.time())
    generate_maze()
    cam = leo.camera.camera_create(VIRTUAL_W, VIRTUAL_H)
    update_camera()
end

function leo.exit()
    if cam then leo.camera.camera_free(cam) end
end

function leo.update(dt)
    -- Movement (smooth, frame-rate independent)
    local dx, dy = 0, 0
    if leo.is_key_down("left")  then dx = dx - 1 end
    if leo.is_key_down("right") then dx = dx + 1 end
    if leo.is_key_down("up")    then dy = dy - 1 end
    if leo.is_key_down("down")  then dy = dy + 1 end

    -- Normalize for diagonal movement (optional; comment to allow faster diagonal)
    if dx ~= 0 and dy ~= 0 then
        dx = dx * 0.7071
        dy = dy * 0.7071
    end

    -- Target floating position
    local target_fx = player.fx + dx * player.speed * dt
    local target_fy = player.fy + dy * player.speed * dt

    -- Collision check: round to nearest cell for the new proposed position
    local tx = math.floor(target_fx + 0.5)
    local ty = math.floor(target_fy + 0.5)
    if can_move_to(tx, ty) then
        player.fx = target_fx
        player.fy = target_fy
        -- Clamp floating point position to bounds
        player.fx = math.max(1, math.min(MAZE_W, player.fx))
        player.fy = math.max(1, math.min(MAZE_H, player.fy))
        -- Update integer grid position
        player.x = math.floor(player.fx + 0.5)
        player.y = math.floor(player.fy + 0.5)
        update_camera()
    end

    -- Win check
    if player.x == finish.x and player.y == finish.y then
        generate_maze()
        update_camera()
    end

    -- Quit on ESC
    if leo.is_key_pressed_once("escape") then
        os.exit()
    end
end

function leo.render()
    leo.camera.set_active_camera(cam)

    -- Background
    leo.draw_filled_rect(0, 0, MAZE_W*CELL_SIZE, MAZE_H*CELL_SIZE, table.unpack(COLORS.BG))

    -- Maze
    for y=1,MAZE_H do
        for x=1,MAZE_W do
            if maze[y][x] == 1 then
                leo.draw_filled_rect(
                    (x-1)*CELL_SIZE, (y-1)*CELL_SIZE, CELL_SIZE, CELL_SIZE,
                    table.unpack(COLORS.WALL)
                )
            end
        end
    end

    -- Finish (draw below player for visibility)
    leo.draw_filled_rect(
        (finish.x-1)*CELL_SIZE+CELL_SIZE*0.15, (finish.y-1)*CELL_SIZE+CELL_SIZE*0.15,
        CELL_SIZE*0.7, CELL_SIZE*0.7, table.unpack(COLORS.FINISH)
    )

    -- Player
    leo.draw_filled_rect(
        (player.fx-1)*CELL_SIZE+CELL_SIZE*0.2, (player.fy-1)*CELL_SIZE+CELL_SIZE*0.2,
        CELL_SIZE*0.6, CELL_SIZE*0.6, table.unpack(COLORS.PLAYER)
    )

    leo.camera.set_active_camera(nil) -- Reset camera for UI (optional)
end

