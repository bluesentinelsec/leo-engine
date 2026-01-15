local graphics = leo.graphics
local audio = leo.audio
local font = leo.font

local background = nil
local character = nil
local main_font = nil
local coin_sfx = nil
local ogre_sfx = nil
local music = nil

local bg_w = 0
local bg_h = 0
local char_w = 0
local char_h = 0

local frame_index = 0
local fps_timer = 0
local fps_frames = 0
local fps_text = "FPS: 0.0"
local sfx_ticks = 0

leo.load = function()
  background = graphics.newImage("images/background_1920x1080.png")
  character = graphics.newImage("images/character_64x64.png")
  bg_w, bg_h = background:getSize()
  char_w, char_h = character:getSize()

  main_font = font.new("font/font.ttf", 24)
  font.set(main_font)

  coin_sfx = audio.newSound("sound/coin.wav")
  ogre_sfx = audio.newSound("sound/ogre3.wav")
  music = audio.newMusic("music/music.wav")
  music:setLooping(true)
  music:play()
  coin_sfx:play()
end

leo.update = function(dt, input)
  frame_index = input.frame

  if dt > 0 then
    fps_timer = fps_timer + dt
    fps_frames = fps_frames + 1
    if fps_timer >= 1.0 then
      local fps = fps_frames / fps_timer
      fps_text = string.format("FPS: %.1f", fps)
      fps_timer = 0
      fps_frames = 0
    end
  end

  sfx_ticks = sfx_ticks + 1
  if sfx_ticks % 120 == 0 then
    coin_sfx:play()
  end
  if sfx_ticks % 300 == 0 then
    ogre_sfx:play()
  end
end

leo.draw = function()
  graphics.clear(0, 0, 0, 255)
  graphics.setColor(255, 255, 255, 255)

  local screen_w, screen_h = graphics.getSize()
  if background and bg_w > 0 and bg_h > 0 then
    local sx = screen_w / bg_w
    local sy = screen_h / bg_h
    graphics.draw(background, 0, 0, 0, sx, sy, 0, 0)
  end

  if character and char_w > 0 and char_h > 0 then
    local cx = screen_w * 0.5
    local cy = screen_h * 0.5
    local angle = (frame_index % 360) * (math.pi / 180)
    graphics.draw(character, cx, cy, angle, 1, 1, char_w * 0.5, char_h * 0.5)
  end

  font.print(fps_text, 16, 16)
end

leo.shutdown = function()
  if music then
    music:stop()
  end
end
