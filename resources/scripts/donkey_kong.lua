-- Leo Engine Donkey Kong (simple) demo.

local graphics = leo.graphics
local font_api = leo.font
local collision = leo.collision
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local font = nil

local floor_w = 980
local floor_h = 16
local floor_x = (screen_w - floor_w) * 0.5

local floors = {
  { x1 = floor_x, y1 = 120, x2 = floor_x + floor_w, y2 = 160, thickness = floor_h },
  { x1 = floor_x, y1 = 280, x2 = floor_x + floor_w, y2 = 240, thickness = floor_h },
  { x1 = floor_x, y1 = 440, x2 = floor_x + floor_w, y2 = 480, thickness = floor_h },
  { x1 = floor_x, y1 = 600, x2 = floor_x + floor_w, y2 = 560, thickness = floor_h },
}

local function init_floors()
  for _, floor in ipairs(floors) do
    floor.minx = math.min(floor.x1, floor.x2)
    floor.maxx = math.max(floor.x1, floor.x2)
    floor.descend_dir = (floor.y2 > floor.y1) and 1 or -1
  end
end

init_floors()

local function floor_y_at(floor, x)
  local t = (x - floor.x1) / (floor.x2 - floor.x1)
  t = math_api.clamp(t, 0, 1)
  return floor.y1 + (floor.y2 - floor.y1) * t
end

local function floor_in_range(floor, x)
  return x >= floor.minx and x <= floor.maxx
end

local ladders = {}
local ladder_w = 26

local function add_ladder(x, from_floor)
  local top = floors[from_floor]
  local bottom = floors[from_floor + 1]
  if not bottom then
    return
  end

  local top_y = floor_y_at(top, x) - 6
  local bottom_y = floor_y_at(bottom, x) + bottom.thickness + 6

  ladders[#ladders + 1] = {
    x = x,
    y = top_y,
    w = ladder_w,
    h = bottom_y - top_y,
  }
end

add_ladder(floor_x + 220, 1)
add_ladder(floor_x + 720, 1)
add_ladder(floor_x + 320, 2)
add_ladder(floor_x + 820, 2)
add_ladder(floor_x + 180, 3)
add_ladder(floor_x + 620, 3)

local player = {
  x = 0,
  y = 0,
  w = 22,
  h = 32,
  vx = 0,
  vy = 0,
  speed = 220,
  climb_speed = 150,
  jump_speed = 430,
  on_ground = false,
  on_ladder = false,
}

local donkey = {
  x = floor_x + 30,
  y = 0,
  w = 64,
  h = 64,
}

local goal = {
  x = floor_x + floor_w - 110,
  y = 0,
  w = 70,
  h = 44,
}

local barrels = {}
local barrel_timer = 0
local barrel_delay = 2.2
local barrel_speed = 140
local barrel_max = 8
local barrel_radius = 12

local gravity = 900

local score = 0
local lives = 3
local game_over = false
local win = false
local ready_timer = 0

local function place_donkey_and_goal()
  donkey.y = floor_y_at(floors[1], donkey.x) - donkey.h - 6
  goal.y = floor_y_at(floors[1], goal.x) - goal.h - 6
end

local function reset_player()
  local spawn_x = floors[#floors].x1 + 70
  local spawn_y = floor_y_at(floors[#floors], spawn_x)
  player.x = spawn_x
  player.y = spawn_y - player.h
  player.vx = 0
  player.vy = 0
  player.on_ground = true
  player.on_ladder = false
end

local function reset_barrels()
  barrels = {}
  barrel_timer = 0
end

local function reset_game()
  score = 0
  lives = 3
  game_over = false
  win = false
  ready_timer = 1.0
  place_donkey_and_goal()
  reset_player()
  reset_barrels()
end

local function spawn_barrel()
  if #barrels >= barrel_max then
    return
  end
  local floor = floors[1]
  local start_x = donkey.x + donkey.w + barrel_radius
  barrels[#barrels + 1] = {
    x = start_x,
    y = floor_y_at(floor, start_x) - barrel_radius,
    vx = barrel_speed * floor.descend_dir,
    vy = 0,
    dir = floor.descend_dir,
    falling = false,
    floor_index = 1,
  }
end

local function rects_overlap(a, b)
  return collision.checkRecs({ a = a, b = b })
end

local function get_ladder_overlap(x, y, w, h)
  local rect = { x = x, y = y, w = w, h = h }
  for _, ladder in ipairs(ladders) do
    if rects_overlap(rect, ladder) then
      return ladder
    end
  end
  return nil
end

local function find_floor_under(x, feet_y)
  local best = nil
  local best_y = nil
  local best_dist = nil

  for _, floor in ipairs(floors) do
    if floor_in_range(floor, x) then
      local fy = floor_y_at(floor, x)
      local dist = feet_y - fy
      if dist >= -6 and dist <= 24 then
        if not best_dist or dist < best_dist then
          best = floor
          best_y = fy
          best_dist = dist
        end
      end
    end
  end

  return best, best_y
end

local function handle_floor_collision(prev_y)
  player.on_ground = false
  local foot_x = player.x + player.w * 0.5
  local prev_feet = prev_y + player.h
  local curr_feet = player.y + player.h

  for _, floor in ipairs(floors) do
    if floor_in_range(floor, foot_x) then
      local fy = floor_y_at(floor, foot_x)
      if prev_feet <= fy and curr_feet >= fy - 1 then
        player.y = fy - player.h
        player.vy = 0
        player.on_ground = true
        break
      end
    end
  end
end

local function update_player(dt, input)
  local move = 0
  if input.keyboard:isDown("left") or input.keyboard:isDown("a") then
    move = move - 1
  end
  if input.keyboard:isDown("right") or input.keyboard:isDown("d") then
    move = move + 1
  end

  local climb = 0
  if input.keyboard:isDown("up") or input.keyboard:isDown("w") then
    climb = climb - 1
  elseif input.keyboard:isDown("down") or input.keyboard:isDown("s") then
    climb = climb + 1
  end

  local pad = input.gamepads[1]
  if pad and pad:isConnected() then
    local axis = pad:axis("leftx")
    if math.abs(axis) > 0.2 then
      move = move + axis
    end
    local ay = pad:axis("lefty")
    if math.abs(ay) > 0.2 then
      climb = ay > 0 and 1 or -1
    end
    if pad:isDown("dpadleft") then
      move = move - 1
    elseif pad:isDown("dpadright") then
      move = move + 1
    end
    if pad:isDown("dpadup") then
      climb = -1
    elseif pad:isDown("dpaddown") then
      climb = 1
    end
  end

  local jump_pressed = input.keyboard:isPressed("space") or input.keyboard:isPressed("enter")
  if pad and pad:isConnected() and pad:isPressed("south") then
    jump_pressed = true
  end

  player.vx = move * player.speed

  local ladder = get_ladder_overlap(player.x, player.y, player.w, player.h)
  local wants_climb = climb ~= 0
  if ladder and (wants_climb or player.on_ladder) then
    player.on_ladder = true
  else
    player.on_ladder = false
  end

  if player.on_ladder then
    player.vy = climb * player.climb_speed
    if jump_pressed then
      player.on_ladder = false
      player.vy = -player.jump_speed
    end
  else
    if jump_pressed and player.on_ground then
      player.vy = -player.jump_speed
      player.on_ground = false
    end
    player.vy = player.vy + gravity * dt
  end

  local prev_y = player.y
  player.x = player.x + player.vx * dt
  player.y = player.y + player.vy * dt

  local min_x = floor_x + 6
  local max_x = floor_x + floor_w - player.w - 6
  player.x = math_api.clamp(player.x, min_x, max_x)

  if not (player.on_ladder and wants_climb) then
    handle_floor_collision(prev_y)

    if player.vy >= 0 then
      local foot_x = player.x + player.w * 0.5
      local floor, fy = find_floor_under(foot_x, player.y + player.h)
      if floor and fy then
        player.y = fy - player.h
        player.vy = 0
        player.on_ground = true
      end
    end
  end

  local ladder_after = get_ladder_overlap(player.x, player.y, player.w, player.h)
  if not ladder_after then
    player.on_ladder = false
  end
end

local function update_barrels(dt)
  barrel_timer = barrel_timer + dt
  if barrel_timer >= barrel_delay then
    barrel_timer = barrel_timer - barrel_delay
    spawn_barrel()
  end

  for i = #barrels, 1, -1 do
    local b = barrels[i]
    local prev_y = b.y

    if b.falling then
      b.vy = b.vy + gravity * dt
      b.y = b.y + b.vy * dt

      local floor = floors[b.floor_index]
      if floor and floor_in_range(floor, b.x) then
        local fy = floor_y_at(floor, b.x)
        if prev_y + barrel_radius <= fy and b.y + barrel_radius >= fy then
          b.y = fy - barrel_radius
          b.vy = 0
          b.falling = false
          b.dir = floor.descend_dir
          b.vx = b.dir * barrel_speed
        end
      else
        if b.y - barrel_radius > screen_h + 60 then
          table.remove(barrels, i)
          score = score + 100
        end
      end
    else
      local floor = floors[b.floor_index]
      if not floor then
        table.remove(barrels, i)
      else
        b.dir = floor.descend_dir
        b.vx = b.dir * barrel_speed
        b.x = b.x + b.vx * dt
        b.y = floor_y_at(floor, b.x) - barrel_radius

        local end_x = (b.dir == 1) and floor.maxx or floor.minx
        if (b.dir == 1 and b.x + barrel_radius >= end_x) or (b.dir == -1 and b.x - barrel_radius <= end_x) then
          b.x = end_x - b.dir * barrel_radius
          b.falling = true
          b.vx = 0
          b.vy = 0
          b.floor_index = b.floor_index + 1
        else
          local ladder = get_ladder_overlap(b.x - barrel_radius, b.y - barrel_radius, barrel_radius * 2, barrel_radius * 2)
          if ladder and b.floor_index < #floors and math.random() < 0.004 then
            b.falling = true
            b.vx = 0
            b.vy = 0
            b.floor_index = b.floor_index + 1
          end
        end
      end
    end
  end
end

local function check_barrel_hits()
  local player_rect = { x = player.x, y = player.y, w = player.w, h = player.h }
  for _, b in ipairs(barrels) do
    local hit = collision.checkCircleRec({
      center = { x = b.x, y = b.y },
      radius = barrel_radius,
      rect = player_rect,
    })
    if hit then
      lives = lives - 1
      if lives <= 0 then
        game_over = true
        win = false
      else
        ready_timer = 1.0
        reset_player()
        reset_barrels()
      end
      return
    end
  end
end

local function check_goal()
  local player_rect = { x = player.x, y = player.y, w = player.w, h = player.h }
  if rects_overlap(player_rect, goal) then
    game_over = true
    win = true
  end
end

local function update_game(dt, input)
  if ready_timer > 0 then
    ready_timer = ready_timer - dt
    if ready_timer < 0 then
      ready_timer = 0
    end
  end

  if ready_timer == 0 then
    update_player(dt, input)
    update_barrels(dt)
    check_barrel_hits()
    check_goal()
  end
end

local function draw_floors()
  for _, floor in ipairs(floors) do
    graphics.drawPolyFilled({
      points = {
        floor.x1,
        floor.y1,
        floor.x2,
        floor.y2,
        floor.x2,
        floor.y2 + floor.thickness,
        floor.x1,
        floor.y1 + floor.thickness,
      },
      r = 160,
      g = 80,
      b = 40,
      a = 255,
    })
    graphics.drawLine({
      x1 = floor.x1,
      y1 = floor.y1,
      x2 = floor.x2,
      y2 = floor.y2,
      r = 210,
      g = 130,
      b = 90,
      a = 220,
    })
  end
end

local function draw_ladders()
  for _, ladder in ipairs(ladders) do
    graphics.drawRectangleOutline({
      x = ladder.x,
      y = ladder.y,
      w = ladder.w,
      h = ladder.h,
      r = 210,
      g = 200,
      b = 140,
      a = 220,
    })

    local rung_y = ladder.y + 8
    while rung_y < ladder.y + ladder.h - 6 do
      graphics.drawLine({
        x1 = ladder.x + 4,
        y1 = rung_y,
        x2 = ladder.x + ladder.w - 4,
        y2 = rung_y,
        r = 210,
        g = 200,
        b = 140,
        a = 200,
      })
      rung_y = rung_y + 12
    end
  end
end

local function draw_donkey()
  graphics.drawRectangleFilled({
    x = donkey.x,
    y = donkey.y,
    w = donkey.w,
    h = donkey.h,
    r = 120,
    g = 80,
    b = 50,
    a = 255,
  })
  graphics.drawRectangleFilled({
    x = donkey.x + 10,
    y = donkey.y + 12,
    w = donkey.w - 20,
    h = donkey.h - 24,
    r = 160,
    g = 100,
    b = 70,
    a = 255,
  })
end

local function draw_goal()
  graphics.drawRectangleFilled({
    x = goal.x,
    y = goal.y,
    w = goal.w,
    h = goal.h,
    r = 230,
    g = 120,
    b = 160,
    a = 255,
  })
end

local function draw_barrels()
  for _, b in ipairs(barrels) do
    graphics.drawCircleFilled({
      x = b.x,
      y = b.y,
      radius = barrel_radius,
      r = 180,
      g = 120,
      b = 70,
      a = 255,
    })
    graphics.drawCircleOutline({
      x = b.x,
      y = b.y,
      radius = barrel_radius,
      r = 230,
      g = 190,
      b = 130,
      a = 220,
    })
  end
end

local function draw_player()
  graphics.drawRectangleFilled({
    x = player.x,
    y = player.y,
    w = player.w,
    h = player.h,
    r = 80,
    g = 160,
    b = 230,
    a = 255,
  })
  graphics.drawRectangleFilled({
    x = player.x + 4,
    y = player.y + 6,
    w = player.w - 8,
    h = 10,
    r = 230,
    g = 230,
    b = 230,
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
    x = screen_w - 150,
    y = 24,
    size = 22,
    r = 230,
    g = 230,
    b = 230,
    a = 255,
  })

  font_api.print({
    font = font,
    text = "Move: WASD/Arrows  Jump: Space/Enter  Climb: Up/Down",
    x = 220,
    y = screen_h - 36,
    size = 18,
    r = 190,
    g = 200,
    b = 220,
    a = 200,
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
      g = 220,
      b = 140,
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
  graphics.clear(16, 16, 22, 255)

  draw_floors()
  draw_ladders()
  draw_goal()
  draw_donkey()
  draw_barrels()
  draw_player()
  draw_ui()
end

function leo.shutdown()
end
