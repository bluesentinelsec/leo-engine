-- Simple Tetris for Leo Engine
-- Drop this into resources/scripts/game.lua

-- Grid dimensions and cell size
local GRID_W, GRID_H = 10, 20
local CELL = 24
local DROP_INTERVAL = 0.5  -- seconds per drop

-- Camera handle
local cam

-- Tetromino definitions (rotation states)
local shapes = {
    -- I
    {
        { {0,1},{1,1},{2,1},{3,1} },
        { {2,0},{2,1},{2,2},{2,3} },
    },
    -- O
    {
        { {1,0},{2,0},{1,1},{2,1} },
    },
    -- T
    {
        { {1,0},{0,1},{1,1},{2,1} },
        { {1,0},{1,1},{2,1},{1,2} },
        { {0,1},{1,1},{2,1},{1,2} },
        { {1,0},{0,1},{1,1},{1,2} },
    },
    -- L
    {
        { {0,1},{1,1},{2,1},{2,0} },
        { {1,0},{1,1},{1,2},{2,2} },
        { {0,1},{0,2},{1,1},{2,1} },
        { {0,0},{1,0},{1,1},{1,2} },
    },
    -- J
    {
        { {0,0},{0,1},{1,1},{2,1} },
        { {1,0},{2,0},{1,1},{1,2} },
        { {0,1},{1,1},{2,1},{2,2} },
        { {1,0},{1,1},{1,2},{0,2} },
    },
    -- S
    {
        { {1,0},{2,0},{0,1},{1,1} },
        { {1,0},{1,1},{2,1},{2,2} },
    },
    -- Z
    {
        { {0,0},{1,0},{1,1},{2,1} },
        { {2,0},{2,1},{1,1},{1,2} },
    },
}

-- Colors for each shape (r,g,b)
local shapeColors = {
    {0,1,1},   -- I: cyan
    {1,1,0},   -- O: yellow
    {0.6,0,0.6}, -- T: purple
    {1,0.5,0}, -- L: orange
    {0,0,1},   -- J: blue
    {0,1,0},   -- S: green
    {1,0,0},   -- Z: red
}

-- Game state
local board, current, dropTimer

-- Helper: create an empty board
local function newBoard()
    local b = {}
    for y = 1, GRID_H do
        b[y] = {}
        for x = 1, GRID_W do b[y][x] = 0 end
    end
    return b
end

-- Spawn a new piece
dofile = dofile  -- workaround for syntax highlighting
local function spawnPiece()
    current = {}
    current.id = math.random(1, #shapes)
    current.rot = 1
    current.x = math.floor(GRID_W/2) - 1
    current.y = 0
end

-- Check if piece at (x,y) with rotation `r` fits
local function canMove(x, y, r)
    local states = shapes[current.id]
    local state = states[r]
    for _, b in ipairs(state) do
        local gx = x + b[1]
        local gy = y + b[2]
        if gx < 0 or gx >= GRID_W or gy < 0 or gy >= GRID_H then
            return false
        end
        if board[gy+1][gx+1] ~= 0 then
            return false
        end
    end
    return true
end

-- Initialize
function leo.init()
    math.randomseed(os.time())
    board = newBoard()
    dropTimer = 0
    spawnPiece()
    -- Create camera to view entire playfield
    cam = leo.camera.camera_create(GRID_W * CELL, GRID_H * CELL)
    -- Center camera on the playfield
    leo.camera.camera_set_position(cam, (GRID_W * CELL) / 2, (GRID_H * CELL) / 2)
end

-- Frame update
function leo.update(dt)
    -- Input: left/right
    if leo.is_key_pressed_once("left") and canMove(current.x-1, current.y, current.rot) then
        current.x = current.x - 1
    elseif leo.is_key_pressed_once("right") and canMove(current.x+1, current.y, current.rot) then
        current.x = current.x + 1
    end
    -- Rotate on up
    if leo.is_key_pressed_once("up") then
        local count = #shapes[current.id]
        local nr = current.rot % count + 1
        if canMove(current.x, current.y, nr) then
            current.rot = nr
        end
    end
    -- Soft drop
    if leo.is_key_down("down") then
        dropTimer = dropTimer + dt * 5
    end
    -- Automatic drop
    dropTimer = dropTimer + dt
    if dropTimer >= DROP_INTERVAL then
        dropTimer = dropTimer - DROP_INTERVAL
        if canMove(current.x, current.y+1, current.rot) then
            current.y = current.y + 1
        else
            -- Lock piece
            for _, b in ipairs(shapes[current.id][current.rot]) do
                local bx = current.x + b[1] + 1
                local by = current.y + b[2] + 1
                board[by][bx] = current.id
            end
            -- Clear full lines
            for y = GRID_H, 1, -1 do
                local full = true
                for x = 1, GRID_W do
                    if board[y][x] == 0 then full = false; break end
                end
                if full then
                    table.remove(board, y)
                    local row = {}
                    for i = 1, GRID_W do row[i] = 0 end
                    table.insert(board, 1, row)
                end
            end
            spawnPiece()
            -- Check game over
            if not canMove(current.x, current.y, current.rot) then
                board = newBoard()
                spawnPiece()
            end
        end
    end
    -- Exit on ESC
    if leo.is_key_pressed_once("escape") then
        leo.exit()
    end
end

-- Render frame
function leo.render()
    -- Draw through camera
    -- Background (black)
    leo.draw_filled_rect(0, 0, GRID_W * CELL, GRID_H * CELL, 0, 0, 0, 1)
    -- Draw board blocks
    for y = 1, GRID_H do
        for x = 1, GRID_W do
            local id = board[y][x]
            if id ~= 0 then
                local c = shapeColors[id]
                local px = (x-1) * CELL
                local py = (y-1) * CELL
                leo.draw_filled_rect(px, py, CELL-1, CELL-1, c[1], c[2], c[3], 1)
            end
        end
    end
    -- Draw current piece
    local c = shapeColors[current.id]
    for _, b in ipairs(shapes[current.id][current.rot]) do
        local px = (current.x + b[1]) * CELL
        local py = (current.y + b[2]) * CELL
        leo.draw_filled_rect(px, py, CELL-1, CELL-1, c[1], c[2], c[3], 1)
    end
end

-- Cleanup
function leo.exit()
    -- Free camera
    if cam then
        leo.camera.camera_free(cam)
        cam = nil
    end
end

