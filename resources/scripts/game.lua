-- Leo Engine adventure demo (simple NES-style dungeon).

local graphics = leo.graphics
local font_api = leo.font
local collision = leo.collision
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local font = nil

local room_cols = 2
local room_rows = 2
local room_w = screen_w
local room_h = screen_h

local door_w = 180
local door_margin = (room_w - door_w) * 0.5

local player = {
  x = 0,
  y = 0,
  w = 26,
  h = 30,
  speed = 220,
  hp = 3,
  invuln = 0,
  face_x = 0,
  face_y = 1,
}

local sword = {
  active = false,
  timer = 0,
  duration = 0.18,
  hit = false,
}

local rooms = {}
local room_x = 0
local room_y = 0

local fireballs = {}

local game_over = false
local win = false

local function room_index(rx, ry)
  return ry * room_cols + rx + 1
end

local function current_room()
  return rooms[room_index(room_x, room_y)]
end

local function reset_player()
  player.x = room_w * 0.5 - player.w * 0.5
  player.y = room_h * 0.6
  player.invuln = 0
  player.face_x = 0
  player.face_y = 1
end

local function spawn_enemy(x, y)
  return {
    x = x,
    y = y,
    w = 28,
    h = 28,
    hp = 2,
    speed = 90,
    alive = true,
  }
end

local function spawn_boss(x, y)
  return {
    x = x,
    y = y,
    w = 80,
    h = 60,
    hp = 8,
    fire_timer = 0,
    fire_delay = 1.2,
    alive = true,
  }
end

local function build_rooms()
  rooms = {}
  for ry = 0, room_rows - 1 do
    for rx = 0, room_cols - 1 do
      rooms[#rooms + 1] = {
        enemies = {},
        boss = nil,
        cleared = false,
      }
    end
  end

  rooms[room_index(0, 0)].enemies[1] = spawn_enemy(420, 360)
  rooms[room_index(1, 0)].enemies[1] = spawn_enemy(760, 320)
  rooms[room_index(0, 1)].enemies[1] = spawn_enemy(520, 420)
  rooms[room_index(1, 1)].boss = spawn_boss(860, 300)
end

local function reset_game()
  game_over = false
  win = false
  player.hp = 3
  room_x = 0
  room_y = 0
  sword.active = false
  sword.timer = 0
  fireballs = {}
  build_rooms()
  reset_player()
end

local function rect_overlap(a, b)
  return collision.checkRecs({ a = a, b = b })
end

local function move_player(dt, input)
  local move_x = 0
  local move_y = 0

  if input.keyboard:isDown("left") or input.keyboard:isDown("a") then
    move_x = move_x - 1
  end
  if input.keyboard:isDown("right") or input.keyboard:isDown("d") then
    move_x = move_x + 1
  end
  if input.keyboard:isDown("up") or input.keyboard:isDown("w") then
    move_y = move_y - 1
  end
  if input.keyboard:isDown("down") or input.keyboard:isDown("s") then
    move_y = move_y + 1
  end

  local pad = input.gamepads[1]
  if pad and pad:isConnected() then
    local ax = pad:axis("leftx")
    local ay = pad:axis("lefty")
    if math.abs(ax) > 0.2 then
      move_x = move_x + ax
    end
    if math.abs(ay) > 0.2 then
      move_y = move_y + ay
    end
    if pad:isDown("dpadleft") then
      move_x = move_x - 1
    elseif pad:isDown("dpadright") then
      move_x = move_x + 1
    end
    if pad:isDown("dpadup") then
      move_y = move_y - 1
    elseif pad:isDown("dpaddown") then
      move_y = move_y + 1
    end
  end

  if move_x ~= 0 or move_y ~= 0 then
    local len = math.sqrt(move_x * move_x + move_y * move_y)
    if len > 0 then
      move_x = move_x / len
      move_y = move_y / len
    end
    player.face_x = move_x
    player.face_y = move_y
  end

  player.x = player.x + move_x * player.speed * dt
  player.y = player.y + move_y * player.speed * dt
end

local function resolve_room_bounds()
  local door_left = room_x > 0
  local door_right = room_x < room_cols - 1
  local door_up = room_y > 0
  local door_down = room_y < room_rows - 1

  local door_min = door_margin
  local door_max = door_margin + door_w

  if player.x < 0 then
    if door_left and player.y + player.h * 0.5 >= door_min and player.y + player.h * 0.5 <= door_max then
      room_x = room_x - 1
      player.x = room_w - player.w - 2
    else
      player.x = 0
    end
  elseif player.x + player.w > room_w then
    if door_right and player.y + player.h * 0.5 >= door_min and player.y + player.h * 0.5 <= door_max then
      room_x = room_x + 1
      player.x = 2
    else
      player.x = room_w - player.w
    end
  end

  if player.y < 0 then
    if door_up and player.x + player.w * 0.5 >= door_min and player.x + player.w * 0.5 <= door_max then
      room_y = room_y - 1
      player.y = room_h - player.h - 2
    else
      player.y = 0
    end
  elseif player.y + player.h > room_h then
    if door_down and player.x + player.w * 0.5 >= door_min and player.x + player.w * 0.5 <= door_max then
      room_y = room_y + 1
      player.y = 2
    else
      player.y = room_h - player.h
    end
  end
end

local function start_swing(input)
  local pressed = input.keyboard:isPressed("space") or input.keyboard:isPressed("enter")
  local pad = input.gamepads[1]
  if pad and pad:isConnected() and pad:isPressed("south") then
    pressed = true
  end
  if pressed and not sword.active then
    sword.active = true
    sword.timer = sword.duration
    sword.hit = false
  end
end

local function update_sword(dt)
  if sword.active then
    sword.timer = sword.timer - dt
    if sword.timer <= 0 then
      sword.active = false
    end
  end
end

local function sword_hitbox()
  if not sword.active then
    return nil
  end

  local size_w = 36
  local size_h = 20
  local offset = 20
  local dx = player.face_x
  local dy = player.face_y

  if math.abs(dx) < 0.1 and math.abs(dy) < 0.1 then
    dy = 1
  end

  if math.abs(dx) > math.abs(dy) then
    if dx > 0 then
      return { x = player.x + player.w + offset - 4, y = player.y + 6, w = size_w, h = size_h }
    else
      return { x = player.x - offset - size_w + 4, y = player.y + 6, w = size_w, h = size_h }
    end
  else
    if dy > 0 then
      return { x = player.x + 4, y = player.y + player.h + offset - 4, w = size_h, h = size_w }
    else
      return { x = player.x + 4, y = player.y - offset - size_w + 4, w = size_h, h = size_w }
    end
  end
end

local function update_enemies(dt)
  local room = current_room()
  for _, enemy in ipairs(room.enemies) do
    if enemy.alive then
      local dx = player.x - enemy.x
      local dy = player.y - enemy.y
      local len = math.sqrt(dx * dx + dy * dy)
      if len > 0 then
        enemy.x = enemy.x + (dx / len) * enemy.speed * dt
        enemy.y = enemy.y + (dy / len) * enemy.speed * dt
      end
    end
  end
end

local function update_boss(dt)
  local room = current_room()
  local boss = room.boss
  if not boss or not boss.alive then
    return
  end

  boss.fire_timer = boss.fire_timer - dt
  if boss.fire_timer <= 0 then
    boss.fire_timer = boss.fire_delay
    local dx = (player.x + player.w * 0.5) - (boss.x + boss.w * 0.5)
    local dy = (player.y + player.h * 0.5) - (boss.y + boss.h * 0.5)
    local len = math.sqrt(dx * dx + dy * dy)
    if len < 1 then
      len = 1
    end
    fireballs[#fireballs + 1] = {
      x = boss.x + boss.w * 0.5,
      y = boss.y + boss.h * 0.5,
      vx = dx / len * 160,
      vy = dy / len * 160,
      radius = 6,
    }
  end
end

local function update_fireballs(dt)
  for i = #fireballs, 1, -1 do
    local f = fireballs[i]
    f.x = f.x + f.vx * dt
    f.y = f.y + f.vy * dt
    if f.x < -40 or f.x > room_w + 40 or f.y < -40 or f.y > room_h + 40 then
      table.remove(fireballs, i)
    end
  end
end

local function apply_sword_hits()
  if not sword.active or sword.hit then
    return
  end

  local hitbox = sword_hitbox()
  if not hitbox then
    return
  end

  local room = current_room()
  for _, enemy in ipairs(room.enemies) do
    if enemy.alive and rect_overlap(hitbox, enemy) then
      enemy.hp = enemy.hp - 1
      sword.hit = true
      if enemy.hp <= 0 then
        enemy.alive = false
      end
      return
    end
  end

  local boss = room.boss
  if boss and boss.alive and rect_overlap(hitbox, boss) then
    boss.hp = boss.hp - 1
    sword.hit = true
    if boss.hp <= 0 then
      boss.alive = false
      win = true
      game_over = true
    end
  end
end

local function player_take_hit()
  if player.invuln > 0 then
    return
  end
  player.hp = player.hp - 1
  player.invuln = 0.9
  if player.hp <= 0 then
    game_over = true
    win = false
  end
end

local function check_enemy_collisions()
  local room = current_room()
  local prect = { x = player.x, y = player.y, w = player.w, h = player.h }

  for _, enemy in ipairs(room.enemies) do
    if enemy.alive and rect_overlap(prect, enemy) then
      player_take_hit()
      return
    end
  end

  local boss = room.boss
  if boss and boss.alive and rect_overlap(prect, boss) then
    player_take_hit()
  end

  for i = #fireballs, 1, -1 do
    local f = fireballs[i]
    local hit = collision.checkCircleRec({
      center = { x = f.x, y = f.y },
      radius = f.radius,
      rect = prect,
    })
    if hit then
      table.remove(fireballs, i)
      player_take_hit()
      return
    end
  end
end

local function update_game(dt, input)
  if player.invuln > 0 then
    player.invuln = math.max(0, player.invuln - dt)
  end

  move_player(dt, input)
  resolve_room_bounds()
  start_swing(input)
  update_sword(dt)
  update_enemies(dt)
  update_boss(dt)
  update_fireballs(dt)
  apply_sword_hits()
  check_enemy_collisions()

  local room = current_room()
  local cleared = true
  for _, enemy in ipairs(room.enemies) do
    if enemy.alive then
      cleared = false
      break
    end
  end
  if room.boss and room.boss.alive then
    cleared = false
  end
  room.cleared = cleared
end

local function draw_room()
  graphics.drawRectangleFilled({
    x = 0,
    y = 0,
    w = room_w,
    h = room_h,
    r = 30,
    g = 34,
    b = 44,
    a = 255,
  })

  local wall = 22
  graphics.drawRectangleFilled({
    x = 0,
    y = 0,
    w = room_w,
    h = wall,
    r = 70,
    g = 80,
    b = 110,
    a = 255,
  })
  graphics.drawRectangleFilled({
    x = 0,
    y = room_h - wall,
    w = room_w,
    h = wall,
    r = 70,
    g = 80,
    b = 110,
    a = 255,
  })
  graphics.drawRectangleFilled({
    x = 0,
    y = 0,
    w = wall,
    h = room_h,
    r = 70,
    g = 80,
    b = 110,
    a = 255,
  })
  graphics.drawRectangleFilled({
    x = room_w - wall,
    y = 0,
    w = wall,
    h = room_h,
    r = 70,
    g = 80,
    b = 110,
    a = 255,
  })

  local door = current_room()
  local door_min = door_margin
  local door_max = door_margin + door_w

  if room_y > 0 then
    graphics.drawRectangleFilled({
      x = door_min,
      y = 0,
      w = door_w,
      h = wall,
      r = 30,
      g = 34,
      b = 44,
      a = 255,
    })
  end
  if room_y < room_rows - 1 then
    graphics.drawRectangleFilled({
      x = door_min,
      y = room_h - wall,
      w = door_w,
      h = wall,
      r = 30,
      g = 34,
      b = 44,
      a = 255,
    })
  end
  if room_x > 0 then
    graphics.drawRectangleFilled({
      x = 0,
      y = door_min,
      w = wall,
      h = door_w,
      r = 30,
      g = 34,
      b = 44,
      a = 255,
    })
  end
  if room_x < room_cols - 1 then
    graphics.drawRectangleFilled({
      x = room_w - wall,
      y = door_min,
      w = wall,
      h = door_w,
      r = 30,
      g = 34,
      b = 44,
      a = 255,
    })
  end
end

local function draw_entities()
  local room = current_room()

  for _, enemy in ipairs(room.enemies) do
    if enemy.alive then
      graphics.drawRectangleFilled({
        x = enemy.x,
        y = enemy.y,
        w = enemy.w,
        h = enemy.h,
        r = 200,
        g = 120,
        b = 80,
        a = 255,
      })
    end
  end

  if room.boss and room.boss.alive then
    graphics.drawRectangleFilled({
      x = room.boss.x,
      y = room.boss.y,
      w = room.boss.w,
      h = room.boss.h,
      r = 160,
      g = 80,
      b = 160,
      a = 255,
    })
    graphics.drawRectangleFilled({
      x = room.boss.x + 10,
      y = room.boss.y + 12,
      w = room.boss.w - 20,
      h = 10,
      r = 220,
      g = 140,
      b = 220,
      a = 255,
    })
  end

  for _, f in ipairs(fireballs) do
    graphics.drawCircleFilled({
      x = f.x,
      y = f.y,
      radius = f.radius,
      r = 240,
      g = 120,
      b = 90,
      a = 240,
    })
  end

  if player.invuln <= 0 or math.floor(player.invuln * 12) % 2 == 0 then
    graphics.drawRectangleFilled({
      x = player.x,
      y = player.y,
      w = player.w,
      h = player.h,
      r = 100,
      g = 200,
      b = 130,
      a = 255,
    })
  end

  if sword.active then
    local hb = sword_hitbox()
    if hb then
      graphics.drawRectangleFilled({
        x = hb.x,
        y = hb.y,
        w = hb.w,
        h = hb.h,
        r = 200,
        g = 200,
        b = 240,
        a = 180,
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
    text = string.format("HP: %d", player.hp),
    x = 20,
    y = 18,
    size = 20,
    r = 220,
    g = 220,
    b = 230,
    a = 255,
  })

  local room_label = string.format("Room %d/%d", room_index(room_x, room_y), room_cols * room_rows)
  font_api.print({
    font = font,
    text = room_label,
    x = 20,
    y = 42,
    size = 18,
    r = 180,
    g = 190,
    b = 210,
    a = 220,
  })

  if game_over then
    local title = win and "Boss Defeated!" or "Game Over"
    font_api.print({
      font = font,
      text = title,
      x = screen_w * 0.5 - 120,
      y = screen_h * 0.5 - 40,
      size = 30,
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
  font = font_api.new("resources/fonts/font.ttf", 20)
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
  draw_room()
  draw_entities()
  draw_ui()
end

function leo.shutdown()
end
