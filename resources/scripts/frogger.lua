-- Leo Engine Frogger (simple) demo.

local graphics = leo.graphics
local font_api = leo.font
local collision = leo.collision
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local font = nil

local lane_h = 60
local road_lanes = 4
local river_lanes = 4
local total_rows = river_lanes + road_lanes + 3
local playfield_y = (screen_h - total_rows * lane_h) * 0.5

local goal_row = 1
local river_start_row = 2
local middle_bank_row = river_start_row + river_lanes
local road_start_row = middle_bank_row + 1
local start_row = road_start_row + road_lanes

local step_x = 64
local move_repeat = 0

local lanes = {}

local player = {
  x = 0,
  y = 0,
  w = 26,
  h = 26,
  speed = 280,
}

local player_row = start_row

local logs = {}
local cars = {}
local goals = {}

local score = 0
local lives = 3
local game_over = false
local win = false
local round_cooldown = 0

local function row_top(row)
  return playfield_y + (row - 1) * lane_h
end

local function row_from_y(y)
  local center_y = y + player.h * 0.5
  local row = math.floor((center_y - playfield_y) / lane_h) + 1
  return math_api.clamp(row, 1, total_rows)
end

local function make_lane(y, count, speed, size, gap, dir, is_log)
  lanes[#lanes + 1] = {
    y = y,
    count = count,
    speed = speed,
    size = size,
    gap = gap,
    dir = dir,
    is_log = is_log,
  }
end

local function init_lanes()
  lanes = {}

  for i = 1, river_lanes do
    local y = row_top(river_start_row + (i - 1))
    local dir = (i % 2 == 0) and -1 or 1
    make_lane(y, 3 + i % 2, 70 + i * 18, 120 + i * 8, 160, dir, true)
  end

  for i = 1, road_lanes do
    local y = row_top(road_start_row + (i - 1))
    local dir = (i % 2 == 0) and 1 or -1
    make_lane(y, 4 + (i % 2), 120 + i * 22, 90 + i * 6, 140, dir, false)
  end
end

local function create_goal_slots()
  goals = {}
  local goal_y = row_top(goal_row) + (lane_h - 36) * 0.5
  local count = 5
  local spacing = screen_w / (count + 1)
  for i = 1, count do
    goals[#goals + 1] = {
      x = spacing * i - 20,
      y = goal_y,
      w = 40,
      h = 36,
      filled = false,
    }
  end
end

local function snap_x(x)
  local center = x + player.w * 0.5
  local snapped = math.floor(center / step_x + 0.5) * step_x
  return snapped - player.w * 0.5
end

local function reset_player()
  player.x = snap_x(screen_w * 0.5 - player.w * 0.5)
  player_row = start_row
  player.y = row_top(player_row) + (lane_h - player.h) * 0.5
end

local function reset_round()
  logs = {}
  cars = {}
  for _, lane in ipairs(lanes) do
    local list = lane.is_log and logs or cars
    local offset = math.random() * lane.gap
    for i = 1, lane.count do
      list[#list + 1] = {
        x = offset + (i - 1) * lane.gap,
        y = lane.y,
        w = lane.size,
        h = lane_h - 12,
        dir = lane.dir,
        speed = lane.speed,
      }
    end
  end
  reset_player()
  round_cooldown = 0.6
end

local function reset_game()
  score = 0
  lives = 3
  game_over = false
  win = false
  init_lanes()
  create_goal_slots()
  reset_round()
end

local function wrap_x(obj)
  if obj.x > screen_w + 120 then
    obj.x = -obj.w - 120
  elseif obj.x + obj.w < -120 then
    obj.x = screen_w + 120
  end
end

local function update_movers(dt)
  for _, log in ipairs(logs) do
    log.x = log.x + log.speed * log.dir * dt
    wrap_x(log)
  end
  for _, car in ipairs(cars) do
    car.x = car.x + car.speed * car.dir * dt
    wrap_x(car)
  end
end

local function consume_grid_input(input, dt)
  local dx, dy = 0, 0
  if input.keyboard:isPressed("left") or input.keyboard:isPressed("a") then
    dx = -1
  elseif input.keyboard:isPressed("right") or input.keyboard:isPressed("d") then
    dx = 1
  elseif input.keyboard:isPressed("up") or input.keyboard:isPressed("w") then
    dy = -1
  elseif input.keyboard:isPressed("down") or input.keyboard:isPressed("s") then
    dy = 1
  end

  local pad = input.gamepads[1]
  if pad and pad:isConnected() then
    if pad:isPressed("dpadleft") then
      dx = -1
    elseif pad:isPressed("dpadright") then
      dx = 1
    elseif pad:isPressed("dpadup") then
      dy = -1
    elseif pad:isPressed("dpaddown") then
      dy = 1
    end
  end

  if dx == 0 and dy == 0 then
    move_repeat = math.max(0, move_repeat - dt)
    if pad and pad:isConnected() and move_repeat <= 0 then
      local ax = pad:axis("leftx")
      local ay = pad:axis("lefty")
      if math.abs(ax) > 0.6 or math.abs(ay) > 0.6 then
        if math.abs(ax) > math.abs(ay) then
          dx = ax > 0 and 1 or -1
        else
          dy = ay > 0 and 1 or -1
        end
        move_repeat = 0.18
      end
    end
  else
    move_repeat = 0.12
  end

  if dx ~= 0 and dy ~= 0 then
    dx = 0
  end

  return dx, dy
end

local function handle_input(input, dt)
  local dx, dy = consume_grid_input(input, dt)

  if dx ~= 0 then
    player.x = snap_x(player.x)
    player.x = player.x + dx * step_x
    player.x = math_api.clamp(player.x, 8, screen_w - player.w - 8)
  end

  if dy ~= 0 then
    player_row = math_api.clamp(player_row + dy, 1, total_rows)
    player.y = row_top(player_row) + (lane_h - player.h) * 0.5
  end
end

local function check_collision_rect(a, b)
  return collision.checkRecs({ a = a, b = b })
end

local function update_player_state(dt)
  local inset = 4
  local rect = {
    x = player.x + inset,
    y = player.y + inset,
    w = player.w - inset * 2,
    h = player.h - inset * 2,
  }

  local row = row_from_y(player.y)
  player_row = row

  if row >= road_start_row and row < road_start_row + road_lanes then
    for _, car in ipairs(cars) do
      local car_rect = { x = car.x + 6, y = car.y + 8, w = car.w - 12, h = car.h - 16 }
      if check_collision_rect(rect, car_rect) then
        lives = lives - 1
        if lives <= 0 then
          game_over = true
          win = false
        else
          reset_round()
        end
        return
      end
    end
  elseif row >= river_start_row and row < river_start_row + river_lanes then
    local on_log = false
    for _, log in ipairs(logs) do
      local log_rect = { x = log.x + 8, y = log.y + 8, w = log.w - 16, h = log.h - 16 }
      if check_collision_rect(rect, log_rect) then
        on_log = true
        player.x = player.x + log.speed * log.dir * dt
        player.x = math_api.clamp(player.x, 0, screen_w - player.w)
        break
      end
    end
    if not on_log then
      lives = lives - 1
      if lives <= 0 then
        game_over = true
        win = false
      else
        reset_round()
      end
      return
    end
  end

  for _, goal in ipairs(goals) do
    if not goal.filled and check_collision_rect(rect, goal) then
      goal.filled = true
      score = score + 100
      reset_round()
      break
    end
  end

  local all_filled = true
  for _, goal in ipairs(goals) do
    if not goal.filled then
      all_filled = false
      break
    end
  end
  if all_filled then
    game_over = true
    win = true
  end
end

local function update_game(dt, input)
  if round_cooldown > 0 then
    round_cooldown = round_cooldown - dt
    return
  end

  handle_input(input, dt)
  update_movers(dt)
  update_player_state(dt)
end

local function draw_background()
  graphics.clear(12, 18, 24, 255)

  -- Goal bank
  graphics.drawRectangleFilled({
    x = 0,
    y = row_top(goal_row),
    w = screen_w,
    h = lane_h,
    r = 40,
    g = 80,
    b = 40,
    a = 255,
  })

  -- River
  graphics.drawRectangleFilled({
    x = 0,
    y = row_top(river_start_row),
    w = screen_w,
    h = river_lanes * lane_h,
    r = 30,
    g = 90,
    b = 150,
    a = 255,
  })

  -- Middle bank
  graphics.drawRectangleFilled({
    x = 0,
    y = row_top(middle_bank_row),
    w = screen_w,
    h = lane_h,
    r = 40,
    g = 80,
    b = 40,
    a = 255,
  })

  -- Road
  graphics.drawRectangleFilled({
    x = 0,
    y = row_top(road_start_row),
    w = screen_w,
    h = road_lanes * lane_h,
    r = 30,
    g = 30,
    b = 40,
    a = 255,
  })

  -- Start bank
  graphics.drawRectangleFilled({
    x = 0,
    y = row_top(start_row),
    w = screen_w,
    h = lane_h,
    r = 40,
    g = 80,
    b = 40,
    a = 255,
  })
end

local function draw_logs()
  for _, log in ipairs(logs) do
    graphics.drawRectangleRoundedFilled({
      x = log.x,
      y = log.y + 6,
      w = log.w,
      h = log.h - 12,
      radius = 8,
      r = 130,
      g = 90,
      b = 60,
      a = 255,
    })
  end
end

local function draw_cars()
  for _, car in ipairs(cars) do
    graphics.drawRectangleRoundedFilled({
      x = car.x,
      y = car.y + 6,
      w = car.w,
      h = car.h - 12,
      radius = 8,
      r = 210,
      g = 80,
      b = 90,
      a = 255,
    })
  end
end

local function draw_goals()
  for _, goal in ipairs(goals) do
    graphics.drawRectangleOutline({
      x = goal.x,
      y = goal.y,
      w = goal.w,
      h = goal.h,
      r = 200,
      g = 220,
      b = 170,
      a = 220,
    })
    if goal.filled then
      graphics.drawRectangleFilled({
        x = goal.x + 6,
        y = goal.y + 6,
        w = goal.w - 12,
        h = goal.h - 12,
        r = 140,
        g = 230,
        b = 120,
        a = 255,
      })
    end
  end
end

local function draw_player()
  graphics.drawRectangleFilled({
    x = player.x,
    y = player.y,
    w = player.w,
    h = player.h,
    r = 100,
    g = 220,
    b = 120,
    a = 255,
  })
  graphics.drawRectangleFilled({
    x = player.x + 6,
    y = player.y + 6,
    w = 6,
    h = 6,
    r = 10,
    g = 20,
    b = 10,
    a = 200,
  })
  graphics.drawRectangleFilled({
    x = player.x + player.w - 12,
    y = player.y + 6,
    w = 6,
    h = 6,
    r = 10,
    g = 20,
    b = 10,
    a = 200,
  })
end

local function draw_ui()
  if not font then
    return
  end

  font_api.print({
    font = font,
    text = string.format("Score: %d", score),
    x = 20,
    y = screen_h - 30,
    size = 20,
    r = 220,
    g = 220,
    b = 230,
    a = 255,
  })
  font_api.print({
    font = font,
    text = string.format("Lives: %d", lives),
    x = screen_w - 140,
    y = screen_h - 30,
    size = 20,
    r = 220,
    g = 220,
    b = 230,
    a = 255,
  })

  if game_over then
    local title = win and "You Win!" or "Game Over"
    font_api.print({
      font = font,
      text = title,
      x = screen_w * 0.5 - 70,
      y = screen_h * 0.5 - 40,
      size = 32,
      r = 255,
      g = 220,
      b = 140,
      a = 255,
    })
    font_api.print({
      font = font,
      text = "Enter/Start: Play Again",
      x = screen_w * 0.5 - 170,
      y = screen_h * 0.5 + 10,
      size = 18,
      r = 190,
      g = 210,
      b = 230,
      a = 255,
    })
    font_api.print({
      font = font,
      text = "Esc/Back: Quit",
      x = screen_w * 0.5 - 110,
      y = screen_h * 0.5 + 36,
      size = 18,
      r = 190,
      g = 210,
      b = 230,
      a = 255,
    })
  end
end

function leo.load()
  math.randomseed(os.time())
  font = font_api.new("resources/fonts/font.ttf", 22)
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
  draw_background()
  draw_goals()
  draw_logs()
  draw_cars()
  draw_player()
  draw_ui()
end

function leo.shutdown()
end
