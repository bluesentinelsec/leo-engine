-- Leo Engine Pong demo.

local graphics = leo.graphics
local font_api = leo.font
local collision = leo.collision
local math_api = leo.math

local screen_w = 1280
local screen_h = 720

local font = nil

local paddle = {
  w = 24,
  h = 140,
  speed = 520,
}

local player = {
  x = 60,
  y = 0,
  score = 0,
}

local opponent = {
  x = 0,
  y = 0,
  score = 0,
  speed = 460,
}

local ball = {
  x = 0,
  y = 0,
  r = 10,
  vx = 0,
  vy = 0,
  speed = 520,
}

local serve_timer = 0
local serve_delay = 1.0
local serve_dir = 1

local game_over = false
local winner_text = ""

local function reset_positions()
  player.y = (screen_h - paddle.h) * 0.5
  opponent.y = (screen_h - paddle.h) * 0.5
  ball.x = screen_w * 0.5
  ball.y = screen_h * 0.5
  ball.vx = 0
  ball.vy = 0
end

local function start_serve(dir)
  serve_timer = serve_delay
  serve_dir = dir
  ball.x = screen_w * 0.5
  ball.y = screen_h * 0.5
  ball.vx = 0
  ball.vy = 0
end

local function launch_ball()
  local angle = (math.random() * 0.6 - 0.3)
  local speed = ball.speed
  ball.vx = speed * serve_dir
  ball.vy = speed * angle
end

local function clamp_paddle(p)
  p.y = math_api.clamp(p.y, 0, screen_h - paddle.h)
end

local function move_player(dt, input)
  local move = 0
  if input.keyboard:isDown("w") or input.keyboard:isDown("up") then
    move = move - 1
  end
  if input.keyboard:isDown("s") or input.keyboard:isDown("down") then
    move = move + 1
  end

  local pad = input.gamepads[1]
  if pad and pad:isConnected() then
    local axis = pad:axis("lefty")
    if math.abs(axis) > 0.15 then
      move = move + axis
    end
    if pad:isDown("dpadup") then
      move = move - 1
    end
    if pad:isDown("dpaddown") then
      move = move + 1
    end
  end

  if move ~= 0 then
    player.y = player.y + move * paddle.speed * dt
    clamp_paddle(player)
  end
end

local function move_opponent(dt)
  local target_y = ball.y - paddle.h * 0.5
  local center_y = opponent.y + paddle.h * 0.5
  local diff = target_y - center_y

  local max_step = opponent.speed * dt
  local step = math_api.clamp(diff, -max_step, max_step)
  opponent.y = opponent.y + step
  clamp_paddle(opponent)
end

local function check_paddle_hit(p, dir)
  local rect = { x = p.x, y = p.y, w = paddle.w, h = paddle.h }
  local hit = collision.checkCircleRec({
    center = { x = ball.x, y = ball.y },
    radius = ball.r,
    rect = rect,
  })

  if hit then
    local relative = ((ball.y - p.y) / paddle.h) * 2 - 1
    relative = math_api.clamp(relative, -1, 1)
    local speed = ball.speed
    ball.vx = speed * dir
    ball.vy = speed * relative * 0.9

    if dir < 0 then
      ball.x = p.x - ball.r
    else
      ball.x = p.x + paddle.w + ball.r
    end
  end
end

local function update_ball(dt)
  if serve_timer > 0 then
    serve_timer = serve_timer - dt
    if serve_timer <= 0 then
      launch_ball()
    end
    return
  end

  ball.x = ball.x + ball.vx * dt
  ball.y = ball.y + ball.vy * dt

  if ball.y - ball.r <= 0 then
    ball.y = ball.r
    ball.vy = -ball.vy
  elseif ball.y + ball.r >= screen_h then
    ball.y = screen_h - ball.r
    ball.vy = -ball.vy
  end

  if ball.vx < 0 then
    check_paddle_hit(player, 1)
  else
    check_paddle_hit(opponent, -1)
  end

  if ball.x + ball.r < 0 then
    opponent.score = opponent.score + 1
    start_serve(1)
  elseif ball.x - ball.r > screen_w then
    player.score = player.score + 1
    start_serve(-1)
  end
end

local function check_win()
  if player.score >= 10 then
    game_over = true
    winner_text = "Player wins!"
  elseif opponent.score >= 10 then
    game_over = true
    winner_text = "Opponent wins!"
  end
end

local function reset_match()
  player.score = 0
  opponent.score = 0
  game_over = false
  winner_text = ""
  reset_positions()
  start_serve(math.random() < 0.5 and -1 or 1)
end

function leo.load()
  math.randomseed(os.time())
  font = font_api.new("resources/fonts/font.ttf", 28)

  opponent.x = screen_w - 60 - paddle.w
  reset_match()
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
      reset_match()
      return
    end
    if quit then
      leo.quit()
      return
    end

    return
  end

  move_player(dt, input)
  move_opponent(dt)
  update_ball(dt)
  check_win()
end

local function draw_net()
  local seg_h = 24
  local gap = 14
  local x = screen_w * 0.5 - 3
  local y = 20
  while y < screen_h - 20 do
    graphics.drawRectangleFilled({
      x = x,
      y = y,
      w = 6,
      h = seg_h,
      r = 60,
      g = 70,
      b = 90,
      a = 200,
    })
    y = y + seg_h + gap
  end
end

function leo.draw()
  graphics.clear(16, 16, 22, 255)

  draw_net()

  graphics.drawRectangleFilled({
    x = player.x,
    y = player.y,
    w = paddle.w,
    h = paddle.h,
    r = 200,
    g = 220,
    b = 240,
    a = 255,
  })

  graphics.drawRectangleFilled({
    x = opponent.x,
    y = opponent.y,
    w = paddle.w,
    h = paddle.h,
    r = 200,
    g = 220,
    b = 240,
    a = 255,
  })

  graphics.drawCircleFilled({
    x = ball.x,
    y = ball.y,
    radius = ball.r,
    r = 255,
    g = 200,
    b = 120,
    a = 255,
  })

  if font then
    font_api.print({
      font = font,
      text = tostring(player.score),
      x = screen_w * 0.5 - 80,
      y = 30,
      size = 36,
      r = 230,
      g = 230,
      b = 240,
      a = 255,
    })
    font_api.print({
      font = font,
      text = tostring(opponent.score),
      x = screen_w * 0.5 + 50,
      y = 30,
      size = 36,
      r = 230,
      g = 230,
      b = 240,
      a = 255,
    })

    if serve_timer > 0 and not game_over then
      font_api.print({
        font = font,
        text = "Serve...",
        x = screen_w * 0.5 - 70,
        y = screen_h * 0.5 - 20,
        size = 22,
        r = 180,
        g = 200,
        b = 220,
        a = 255,
      })
    end

    if game_over then
      font_api.print({
        font = font,
        text = winner_text,
        x = screen_w * 0.5 - 140,
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
end

function leo.shutdown()
end
