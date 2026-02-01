-- Leo Engine Pac-Man demo.

local graphics = leo.graphics
local font_api = leo.font
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local tile_size = 28

local map_data = {
  "############################",
  "#............##............#",
  "#.####.#####.##.#####.####.#",
  "#o####.#####.##.#####.####o#",
  "#..........................#",
  "#.####.##.########.##.####.#",
  "#......##....##....##......#",
  "######.#####.##.#####.######",
  "#............##............#",
  "#.####.#####.##.#####.####.#",
  "#o..##................##..o#",
  "#.####.##.########.##.####.#",
  "#......##....##....##......#",
  "######.#####.##.#####.######",
  "#............##............#",
  "#.####.#####.##.#####.####.#",
  "#o####.#####.##.#####.####o#",
  "#..........................#",
  "############################",
}

local map_rows = #map_data
local map_cols = #map_data[1]
local map_px_w = map_cols * tile_size
local map_px_h = map_rows * tile_size
local map_origin_x = (screen_w - map_px_w) * 0.5
local map_origin_y = (screen_h - map_px_h) * 0.5

local grid = {}
local pellet_count = 0

local score = 0
local lives = 3
local power_timer = 0
local ready_timer = 0
local game_over = false
local win = false

local font = nil

local player = {
  x = 0,
  y = 0,
  radius = 12,
  speed = 150,
  dir = { dx = 0, dy = 0 },
  next_dir = { dx = 0, dy = 0 },
}

local ghosts = {}
local ghost_colors = {
  { 230, 80, 90 },
  { 90, 210, 230 },
  { 230, 160, 90 },
  { 200, 120, 230 },
}

local function tile_center(col, row)
  local x = map_origin_x + (col - 0.5) * tile_size
  local y = map_origin_y + (row - 0.5) * tile_size
  return x, y
end

local function world_to_tile(x, y)
  local col = math.floor((x - map_origin_x) / tile_size) + 1
  local row = math.floor((y - map_origin_y) / tile_size) + 1
  return col, row
end

local function is_wall(col, row)
  if col < 1 or col > map_cols or row < 1 or row > map_rows then
    return true
  end
  return grid[row][col] == 1
end

local function parse_map()
  grid = {}
  pellet_count = 0

  for row, line in ipairs(map_data) do
    grid[row] = {}
    for col = 1, #line do
      local ch = line:sub(col, col)
      if ch == "#" then
        grid[row][col] = 1
      elseif ch == "." then
        grid[row][col] = 2
        pellet_count = pellet_count + 1
      elseif ch == "o" then
        grid[row][col] = 3
        pellet_count = pellet_count + 1
      else
        grid[row][col] = 0
      end
    end
  end
end

local function consume_pellet(col, row)
  local cell = grid[row] and grid[row][col] or 0
  if cell == 2 or cell == 3 then
    grid[row][col] = 0
    pellet_count = pellet_count - 1
    if cell == 3 then
      score = score + 50
      power_timer = 6.5
    else
      score = score + 10
    end

    if pellet_count <= 0 then
      game_over = true
      win = true
    end
  end
end

local function can_move(col, row, dir)
  if dir.dx == 0 and dir.dy == 0 then
    return false
  end
  return not is_wall(col + dir.dx, row + dir.dy)
end

local function reset_positions()
  local px, py = tile_center(13, 15)
  player.x = px
  player.y = py
  player.dir = { dx = 0, dy = 0 }
  player.next_dir = { dx = 0, dy = 0 }

  ghosts = {}
  local ghost_spawns = {
    { col = 13, row = 9 },
    { col = 16, row = 9 },
    { col = 13, row = 11 },
    { col = 16, row = 11 },
  }

  for i, spawn in ipairs(ghost_spawns) do
    local gx, gy = tile_center(spawn.col, spawn.row)
    ghosts[i] = {
      x = gx,
      y = gy,
      radius = 12,
      speed = 120,
      dir = { dx = 0, dy = 1 },
      spawn = { col = spawn.col, row = spawn.row },
      respawn_timer = 0,
      color = ghost_colors[i],
    }
  end

  ready_timer = 1.2
end

local function reset_game()
  score = 0
  lives = 3
  power_timer = 0
  game_over = false
  win = false
  parse_map()
  reset_positions()
end

local function update_player_input(input)
  local desired_dx = 0
  local desired_dy = 0

  if input.keyboard:isDown("left") or input.keyboard:isDown("a") then
    desired_dx = -1
  elseif input.keyboard:isDown("right") or input.keyboard:isDown("d") then
    desired_dx = 1
  elseif input.keyboard:isDown("up") or input.keyboard:isDown("w") then
    desired_dy = -1
  elseif input.keyboard:isDown("down") or input.keyboard:isDown("s") then
    desired_dy = 1
  end

  local pad = input.gamepads[1]
  if pad and pad:isConnected() then
    local ax = pad:axis("leftx")
    local ay = pad:axis("lefty")
    if math.abs(ax) > 0.3 or math.abs(ay) > 0.3 then
      if math.abs(ax) > math.abs(ay) then
        desired_dx = ax > 0 and 1 or -1
        desired_dy = 0
      else
        desired_dy = ay > 0 and 1 or -1
        desired_dx = 0
      end
    end
    if pad:isDown("dpadleft") then
      desired_dx = -1
      desired_dy = 0
    elseif pad:isDown("dpadright") then
      desired_dx = 1
      desired_dy = 0
    elseif pad:isDown("dpadup") then
      desired_dx = 0
      desired_dy = -1
    elseif pad:isDown("dpaddown") then
      desired_dx = 0
      desired_dy = 1
    end
  end

  if desired_dx ~= 0 or desired_dy ~= 0 then
    player.next_dir = { dx = desired_dx, dy = desired_dy }
  end
end

local function update_player(dt, input)
  update_player_input(input)

  local col, row = world_to_tile(player.x, player.y)
  local center_x, center_y = tile_center(col, row)
  local at_center = math.abs(player.x - center_x) < 1.5 and math.abs(player.y - center_y) < 1.5

  if at_center then
    player.x = center_x
    player.y = center_y

    if can_move(col, row, player.next_dir) then
      player.dir = { dx = player.next_dir.dx, dy = player.next_dir.dy }
    end

    if not can_move(col, row, player.dir) then
      player.dir = { dx = 0, dy = 0 }
    end

    consume_pellet(col, row)
  end

  player.x = player.x + player.dir.dx * player.speed * dt
  player.y = player.y + player.dir.dy * player.speed * dt
end

local function ghost_available_dirs(col, row)
  local dirs = {}
  if not is_wall(col + 1, row) then
    dirs[#dirs + 1] = { dx = 1, dy = 0 }
  end
  if not is_wall(col - 1, row) then
    dirs[#dirs + 1] = { dx = -1, dy = 0 }
  end
  if not is_wall(col, row + 1) then
    dirs[#dirs + 1] = { dx = 0, dy = 1 }
  end
  if not is_wall(col, row - 1) then
    dirs[#dirs + 1] = { dx = 0, dy = -1 }
  end
  return dirs
end

local function choose_ghost_dir(ghost, col, row, frightened)
  local options = ghost_available_dirs(col, row)
  if #options == 0 then
    return { dx = 0, dy = 0 }
  end

  if #options > 1 then
    local rev_dx = -ghost.dir.dx
    local rev_dy = -ghost.dir.dy
    for i = #options, 1, -1 do
      if options[i].dx == rev_dx and options[i].dy == rev_dy then
        table.remove(options, i)
        break
      end
    end
  end

  local player_col, player_row = world_to_tile(player.x, player.y)
  local best = options[1]
  local best_score = nil

  for _, dir in ipairs(options) do
    local nc = col + dir.dx
    local nr = row + dir.dy
    local dist = math.abs(nc - player_col) + math.abs(nr - player_row)
    local score = frightened and dist or -dist

    if best_score == nil or score > best_score then
      best_score = score
      best = dir
    elseif score == best_score and math.random() < 0.35 then
      best = dir
    end
  end

  return best
end

local function update_ghost(ghost, dt)
  if ghost.respawn_timer > 0 then
    ghost.respawn_timer = ghost.respawn_timer - dt
    if ghost.respawn_timer <= 0 then
      local gx, gy = tile_center(ghost.spawn.col, ghost.spawn.row)
      ghost.x = gx
      ghost.y = gy
      ghost.dir = { dx = 0, dy = -1 }
    end
    return
  end

  local col, row = world_to_tile(ghost.x, ghost.y)
  local center_x, center_y = tile_center(col, row)
  local at_center = math.abs(ghost.x - center_x) < 1.2 and math.abs(ghost.y - center_y) < 1.2

  if at_center then
    ghost.x = center_x
    ghost.y = center_y

    local frightened = power_timer > 0
    ghost.dir = choose_ghost_dir(ghost, col, row, frightened)
  end

  local speed = ghost.speed
  if power_timer > 0 then
    speed = ghost.speed * 0.65
  end

  ghost.x = ghost.x + ghost.dir.dx * speed * dt
  ghost.y = ghost.y + ghost.dir.dy * speed * dt
end

local function check_player_hit()
  for _, ghost in ipairs(ghosts) do
    if ghost.respawn_timer <= 0 then
      local dx = ghost.x - player.x
      local dy = ghost.y - player.y
      local dist2 = dx * dx + dy * dy
      local hit_dist = (ghost.radius + player.radius - 2)
      if dist2 <= hit_dist * hit_dist then
        if power_timer > 0 then
          score = score + 200
          ghost.respawn_timer = 2.6
          local gx, gy = tile_center(ghost.spawn.col, ghost.spawn.row)
          ghost.x = gx
          ghost.y = gy
          ghost.dir = { dx = 0, dy = 0 }
        else
          lives = lives - 1
          if lives <= 0 then
            game_over = true
            win = false
          else
            reset_positions()
          end
          return
        end
      end
    end
  end
end

local function update_game(dt, input)
  if ready_timer > 0 then
    ready_timer = ready_timer - dt
    return
  end

  update_player(dt, input)

  for _, ghost in ipairs(ghosts) do
    update_ghost(ghost, dt)
  end

  check_player_hit()

  if power_timer > 0 then
    power_timer = math.max(0, power_timer - dt)
  end
end

local function draw_map()
  for row = 1, map_rows do
    for col = 1, map_cols do
      local cell = grid[row][col]
      if cell == 1 then
        graphics.drawRectangleFilled({
          x = map_origin_x + (col - 1) * tile_size,
          y = map_origin_y + (row - 1) * tile_size,
          w = tile_size,
          h = tile_size,
          r = 18,
          g = 32,
          b = 84,
          a = 255,
        })
      elseif cell == 2 then
        local px, py = tile_center(col, row)
        graphics.drawCircleFilled({
          x = px,
          y = py,
          radius = 3,
          r = 240,
          g = 220,
          b = 120,
          a = 240,
        })
      elseif cell == 3 then
        local px, py = tile_center(col, row)
        graphics.drawCircleFilled({
          x = px,
          y = py,
          radius = 7,
          r = 240,
          g = 220,
          b = 120,
          a = 250,
        })
      end
    end
  end
end

local function draw_player()
  graphics.drawCircleFilled({
    x = player.x,
    y = player.y,
    radius = player.radius,
    r = 240,
    g = 210,
    b = 60,
    a = 255,
  })

  local mouth = 0.35 + 0.2 * math.abs(math.sin(os.clock() * 6))
  local angle = 0
  if player.dir.dx == 1 then
    angle = 0
  elseif player.dir.dx == -1 then
    angle = math.pi
  elseif player.dir.dy == -1 then
    angle = -math.pi * 0.5
  elseif player.dir.dy == 1 then
    angle = math.pi * 0.5
  end

  local a1 = angle + mouth
  local a2 = angle - mouth
  local x1 = player.x + math.cos(a1) * player.radius
  local y1 = player.y + math.sin(a1) * player.radius
  local x2 = player.x + math.cos(a2) * player.radius
  local y2 = player.y + math.sin(a2) * player.radius

  graphics.drawTriangleFilled({
    x1 = player.x,
    y1 = player.y,
    x2 = x1,
    y2 = y1,
    x3 = x2,
    y3 = y2,
    r = 10,
    g = 10,
    b = 14,
    a = 255,
  })
end

local function draw_ghosts()
  for _, ghost in ipairs(ghosts) do
    if ghost.respawn_timer <= 0 then
      local r, g, b = ghost.color[1], ghost.color[2], ghost.color[3]
      if power_timer > 0 then
        r, g, b = 80, 120, 230
      end
      graphics.drawCircleFilled({
        x = ghost.x,
        y = ghost.y,
        radius = ghost.radius,
        r = r,
        g = g,
        b = b,
        a = 255,
      })
    end
  end
end

local function draw_ui()
  if not font then
    return
  end

  font_api.print({
    font = font,
    text = string.format("Score: %d", score),
    x = 30,
    y = 24,
    size = 22,
    r = 230,
    g = 230,
    b = 230,
    a = 255,
  })

  font_api.print({
    font = font,
    text = string.format("Lives: %d", lives),
    x = screen_w - 160,
    y = 24,
    size = 22,
    r = 230,
    g = 230,
    b = 230,
    a = 255,
  })

  if ready_timer > 0 and not game_over then
    font_api.print({
      font = font,
      text = "Ready!",
      x = screen_w * 0.5 - 45,
      y = screen_h * 0.5 + 140,
      size = 22,
      r = 255,
      g = 210,
      b = 120,
      a = 255,
    })
  end

  if game_over then
    local title = win and "You Win!" or "Game Over"
    font_api.print({
      font = font,
      text = title,
      x = screen_w * 0.5 - 70,
      y = screen_h * 0.5 - 40,
      size = 32,
      r = 255,
      g = 210,
      b = 120,
      a = 255,
    })
    font_api.print({
      font = font,
      text = "Enter/Start: Play Again",
      x = screen_w * 0.5 - 170,
      y = screen_h * 0.5 + 8,
      size = 18,
      r = 200,
      g = 210,
      b = 230,
      a = 255,
    })
    font_api.print({
      font = font,
      text = "Esc/Back: Quit",
      x = screen_w * 0.5 - 110,
      y = screen_h * 0.5 + 34,
      size = 18,
      r = 200,
      g = 210,
      b = 230,
      a = 255,
    })
  end
end

function leo.load()
  math.randomseed(os.time())
  font = font_api.new("resources/fonts/font.ttf", 24)
  reset_game()
end

function leo.update(dt, input)
  if game_over then
    local restart = input.keyboard:isPressed("enter") or input.keyboard:isPressed("space")
    local quit = input.keyboard:isPressed("escape")

    local pad = input.gamepads[1]
    if pad and pad:isConnected() then
      if pad:isPressed("start") or pad:isPressed("south") then
        restart = true
      end
      if pad:isPressed("back") then
        quit = true
      end
    end

    if restart then
      reset_game()
    elseif quit then
      leo.quit()
    end
    return
  end

  update_game(dt, input)
end

function leo.draw()
  graphics.clear(10, 10, 14, 255)

  draw_map()
  draw_player()
  draw_ghosts()
  draw_ui()
end

function leo.shutdown()
end
