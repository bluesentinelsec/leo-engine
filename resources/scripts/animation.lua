-- Sprite sheet animation demo for Leo Engine.

local graphics = leo.graphics
local animation = leo.animation

local anim = nil
local x = 640
local y = 360
local ox = 32
local oy = 32

function leo.load()
  anim = animation.newSheet({
    path = "resources/images/animation_test.png",
    frame_w = 64,
    frame_h = 64,
    frame_count = 3,
    frame_time = 0.45,
    looping = true,
    playing = true,
  })
end

function leo.update(dt, input)
    anim:update(dt)
end

function leo.draw()
  graphics.clear(18, 18, 22, 255)

  anim:draw({
    x = x,
    y = y,
    ox = ox,
    oy = oy,
    sx = 1.0,
    sy = 1.0,
    angle = 0,
    flipX = false,
    flipY = false,
    r = 255,
    g = 255,
    b = 255,
    a = 255,
  })
end

function leo.shutdown()
end
