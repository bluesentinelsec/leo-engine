-- resources/scripts/game.lua

-- Configuration
local FONT_PATH   = "resources/font/font.ttf"
local WORLD_W     = 800
local WORLD_H     = 1000
local VIEW_W, VIEW_H = 800, 600
local SCROLL_SPEED  = 50  -- world units per second

-- State
local fontLarge, fontSmall
local cam, camX, camY
local lines = {}

for i = 1, 30 do
  lines[i] = string.format("This is demo line #%02d", i)
end

function leo.init()
  -- load two sizes of the same font
  fontLarge = leo.font.load(FONT_PATH, 48)
  fontSmall = leo.font.load(FONT_PATH, 24)

  -- create & activate a camera that views VIEW_W×VIEW_H in world coords
  cam = leo.camera.camera_create(VIEW_W, VIEW_H)
  leo.camera.set_active_camera(cam)

  -- start camera at the top of the world
  camX = VIEW_W / 2
  camY = VIEW_H / 2
  leo.camera.camera_set_position(cam, camX, camY)
end

function leo.update(dt)
  -- scroll the camera down
  camY = camY + SCROLL_SPEED * dt
  -- wrap back to top when reaching the bottom
  if camY > (WORLD_H - VIEW_H/2) then
    camY = VIEW_H / 2
  end
  leo.camera.camera_set_position(cam, camX, camY)

  -- exit on Escape
  if leo.is_key_pressed_once("escape") then
    leo.exit()
  end
end

function leo.render()
  -- background fill
  leo.draw_filled_rect(0, 0, WORLD_W, WORLD_H, 0.05, 0.05, 0.10, 1)

  -- Title banner
  leo.draw_filled_rect(20, camY - VIEW_H/2 + 20, 360, 60, 0, 0, 0, 0.6)
  leo.font.draw(
    fontLarge,
    "FONT DEMO",
    30, camY - VIEW_H/2 + 30,
    1, 1, 0.5, 1
  )

  -- Scrolling demo lines
  for i, text in ipairs(lines) do
    local y = 100 + (i-1)*30
    leo.font.draw(fontSmall, text, 30, y, 1, 1, 1, 1)
  end

  -- Footer instruction
  leo.font.draw(
    fontSmall,
    "Press ESC to quit",
    30, WORLD_H - 40,
    1, 0.6, 0.6, 1
  )
end

function leo.exit()
  if fontSmall then
    leo.font.free(fontSmall)
    fontSmall = nil
  end
  if fontLarge then
    leo.font.free(fontLarge)
    fontLarge = nil
  end
end

