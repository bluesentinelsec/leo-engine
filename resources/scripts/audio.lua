-- Leo Engine audio demo: music + sound effects with simple controls.

local graphics = leo.graphics
local font_api = leo.font
local audio = leo.audio

local font = nil
local base_size = 26
local line_height = 0

local music = nil
local music_volume = 70
local music_looping = true

local sfx_list = {}
local sfx_names = {}
local sfx_index = 1
local sfx_timer = 0
local sfx_interval = 3.0
local sfx_volume = 80
local last_sfx = "(none yet)"

local function clamp(value, min_value, max_value)
  if value < min_value then
    return min_value
  end
  if value > max_value then
    return max_value
  end
  return value
end

local function advance_for(size)
  if line_height == 0 then
    return size
  end
  return line_height * (size / base_size)
end

local function set_music_volume(value)
  music_volume = clamp(value, 0, 100)
  if music then
    music:setVolume(music_volume)
  end
end

local function set_sfx_volume(value)
  sfx_volume = clamp(value, 0, 100)
  for _, sfx in ipairs(sfx_list) do
    sfx:setVolume(sfx_volume)
  end
end

local function play_next_sfx()
  if #sfx_list == 0 then
    return
  end
  local sfx = sfx_list[sfx_index]
  if sfx then
    sfx:play()
  end
  last_sfx = sfx_names[sfx_index] or "(unknown)"
  sfx_index = sfx_index + 1
  if sfx_index > #sfx_list then
    sfx_index = 1
  end
end

function leo.load()
  font = font_api.new("resources/fonts/font.ttf", base_size)
  line_height = font:getLineHeight()

  music = audio.newMusic({
    path = "resources/music/music.wav",
    looping = music_looping,
    volume = music_volume,
    play = true,
  })

  sfx_list = {
    audio.newSound({ path = "resources/sound/coin.wav", volume = sfx_volume }),
    audio.newSound({ path = "resources/sound/ogre3.wav", volume = sfx_volume }),
  }
  sfx_names = { "coin.wav", "ogre3.wav" }
end

function leo.update(dt, input)
  sfx_timer = sfx_timer + dt
  while sfx_timer >= sfx_interval do
    sfx_timer = sfx_timer - sfx_interval
    play_next_sfx()
  end

  local keyboard = input.keyboard
  if not keyboard then
    return
  end

  if keyboard:isPressed("space") then
    if music:isPlaying() then
      music:pause()
    else
      music:play()
    end
  end

  if keyboard:isPressed("s") then
    music:stop()
  end

  if keyboard:isPressed("r") then
    music:stop()
    music:play()
  end

  if keyboard:isPressed("l") then
    music_looping = not music_looping
    music:setLooping(music_looping)
  end

  if keyboard:isPressed("up") then
    set_music_volume(music_volume + 5)
  end

  if keyboard:isPressed("down") then
    set_music_volume(music_volume - 5)
  end

  if keyboard:isPressed("right") then
    set_sfx_volume(sfx_volume + 5)
  end

  if keyboard:isPressed("left") then
    set_sfx_volume(sfx_volume - 5)
  end

  if keyboard:isPressed("f") then
    play_next_sfx()
  end
end

function leo.draw()
  graphics.clear(16, 18, 24, 255)

  local x = 40
  local y = 40

  font_api.print({
    font = font,
    text = "Audio Demo",
    x = x,
    y = y,
    size = 32,
    r = 230,
    g = 230,
    b = 245,
    a = 255,
  })
  y = y + advance_for(32) + 12

  local music_state = "paused"
  if music and music:isPlaying() then
    music_state = "playing"
  end

  font_api.print({
    font = font,
    text = string.format("Music: %s  |  looping: %s", music_state, music_looping and "on" or "off"),
    x = x,
    y = y,
    size = 22,
    r = 180,
    g = 210,
    b = 240,
    a = 255,
  })
  y = y + advance_for(22) + 6

  font_api.print({
    font = font,
    text = string.format("Music volume: %d", music_volume),
    x = x,
    y = y,
    size = 20,
    r = 160,
    g = 200,
    b = 220,
    a = 255,
  })
  y = y + advance_for(20) + 6

  font_api.print({
    font = font,
    text = string.format("SFX volume: %d  |  last: %s", sfx_volume, last_sfx),
    x = x,
    y = y,
    size = 20,
    r = 160,
    g = 200,
    b = 220,
    a = 255,
  })
  y = y + advance_for(20) + 6

  local next_sfx = sfx_interval - sfx_timer
  if next_sfx < 0 then
    next_sfx = 0
  end
  font_api.print({
    font = font,
    text = string.format("Next auto SFX in: %.1fs", next_sfx),
    x = x,
    y = y,
    size = 20,
    r = 170,
    g = 200,
    b = 210,
    a = 255,
  })
  y = y + advance_for(20) + 14

  font_api.print({
    font = font,
    text = "Controls",
    x = x,
    y = y,
    size = 22,
    r = 210,
    g = 220,
    b = 230,
    a = 255,
  })
  y = y + advance_for(22) + 4

  font_api.print({
    font = font,
    text = "Space: Play/Pause  |  S: Stop  |  R: Restart  |  L: Toggle loop",
    x = x,
    y = y,
    size = 18,
    r = 150,
    g = 180,
    b = 200,
    a = 255,
  })
  y = y + advance_for(18) + 4

  font_api.print({
    font = font,
    text = "Up/Down: Music volume  |  Left/Right: SFX volume  |  F: Play SFX",
    x = x,
    y = y,
    size = 18,
    r = 150,
    g = 180,
    b = 200,
    a = 255,
  })
end

function leo.shutdown()
end
