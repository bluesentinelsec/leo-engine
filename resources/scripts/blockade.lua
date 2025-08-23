-- resources/scripts/game.lua

-- Config
local LOGICAL_W, LOGICAL_H = 640, 480
local CELL_SIZE = 16  -- block size for grid
local GRID_W, GRID_H = LOGICAL_W // CELL_SIZE, LOGICAL_H // CELL_SIZE

local DIRS = {
    up    = {x =  0, y = -1},
    down  = {x =  0, y =  1},
    left  = {x = -1, y =  0},
    right = {x =  1, y =  0},
}

-- Colors (r,g,b,a)
local COLORS = {
    bg = {0.12, 0.12, 0.12, 1.0},
    p1 = {0.2, 1.0, 0.3, 1.0},
    p2 = {0.2, 0.4, 1.0, 1.0},
}

-- Game state
local camera = nil
local grid = nil  -- 2D array: 0=empty, 1=P1, 2=P2
local players = {}

local function reset_game()
    -- Clear grid
    grid = {}
    for y=1,GRID_H do
        grid[y] = {}
        for x=1,GRID_W do
            grid[y][x] = 0
        end
    end

    -- Player 1: left
    players[1] = {
        x = 3, y = GRID_H // 2, dir = "right", next_dir = "right", alive = true, color = COLORS.p1
    }
    -- Player 2: right
    players[2] = {
        x = GRID_W-2, y = GRID_H // 2, dir = "left", next_dir = "left", alive = true, color = COLORS.p2
    }
    -- Fill grid with starting positions
    grid[players[1].y][players[1].x] = 1
    grid[players[2].y][players[2].x] = 2
end

function leo.init()
    camera = leo.camera.camera_create(LOGICAL_W, LOGICAL_H)
    reset_game()
    -- Control vars
    move_timer = 0.0
    MOVE_DELAY = 0.09  -- time between moves (speed)
    winner = nil
end

local function update_direction(idx, up, down, left, right)
    local p = players[idx]
    if leo.is_key_down(up)    and p.dir ~= "down"  then p.next_dir = "up"    end
    if leo.is_key_down(down)  and p.dir ~= "up"    then p.next_dir = "down"  end
    if leo.is_key_down(left)  and p.dir ~= "right" then p.next_dir = "left"  end
    if leo.is_key_down(right) and p.dir ~= "left"  then p.next_dir = "right" end
end

function leo.update(dt)
    if leo.is_key_pressed_once("escape") then
        leo.exit()
    end
    if winner then
        if leo.is_key_pressed_once("space") then
            winner = nil
            reset_game()
        end
        return
    end
    -- Input
    update_direction(1, "w", "s", "a", "d")
    update_direction(2, "up", "down", "left", "right")

    move_timer = move_timer + dt
    if move_timer >= MOVE_DELAY then
        move_timer = move_timer - MOVE_DELAY
        for idx, p in ipairs(players) do
            if p.alive then
                -- Turn
                p.dir = p.next_dir
                local dir = DIRS[p.dir]
                local nx, ny = p.x + dir.x, p.y + dir.y
                -- Check collision
                if nx < 1 or nx > GRID_W or ny < 1 or ny > GRID_H or grid[ny][nx] ~= 0 then
                    p.alive = false
                else
                    p.x, p.y = nx, ny
                    grid[ny][nx] = idx
                end
            end
        end
        -- Check win/lose
        local alive_count = 0
        local last_alive = nil
        for i, p in ipairs(players) do
            if p.alive then
                alive_count = alive_count + 1
                last_alive = i
            end
        end
        if alive_count < 2 then
            winner = (alive_count == 1) and last_alive or 0 -- 0=draw
        end
    end
end

function leo.render()
    -- Set camera for scaling
    leo.camera.camera_set_position(camera, LOGICAL_W/2, LOGICAL_H/2)
    -- BG
    leo.draw_filled_rect(0, 0, LOGICAL_W, LOGICAL_H, table.unpack(COLORS.bg))
    -- Draw grid
    for y=1,GRID_H do
        for x=1,GRID_W do
            if grid[y][x] == 1 then
                leo.draw_filled_rect((x-1)*CELL_SIZE, (y-1)*CELL_SIZE, CELL_SIZE, CELL_SIZE, table.unpack(COLORS.p1))
            elseif grid[y][x] == 2 then
                leo.draw_filled_rect((x-1)*CELL_SIZE, (y-1)*CELL_SIZE, CELL_SIZE, CELL_SIZE, table.unpack(COLORS.p2))
            end
        end
    end
    -- Winner text
    if winner then
        local msg = winner == 0 and "Draw!" or ("Player "..winner.." wins!")
        -- Large white text at center (you can substitute with your text drawing if available)
        leo.draw_filled_rect(LOGICAL_W/2 - 80, LOGICAL_H/2 - 24, 160, 48, 0,0,0,0.7)
        -- (Replace with leo.draw_text if available)
    end
end

function leo.exit()
    if camera then
        leo.camera.camera_free(camera)
    end
end

