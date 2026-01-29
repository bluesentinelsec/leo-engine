# Lua API

This document describes the Love2D-style Lua API for Leo Engine. It is focused on
the caller experience and aligns with Leo's deterministic, fixed-timestep
simulation model.

## Goals

- Familiar Love2D-like entry points and module naming.
- Deterministic input, audio, and timing through the engine tick.
- VFS-first asset loading (no raw filesystem reads from Lua by default).
- Small, predictable surface area that mirrors engine C++ systems.

## Top-Level Lifecycle

Leo uses a fixed-timestep simulation. The Lua entry points mirror that:

```lua
function leo.load()
  -- one-time setup
end

function leo.update(dt, input)
  -- deterministic tick update (dt = fixed tick seconds)
end

function leo.draw()
  -- render current state
end

function leo.shutdown()
  -- optional cleanup
end
```

Notes:
- `dt` is the fixed tick duration (ex: 1/60).
- `input` is the normalized per-tick input frame.
- `leo.draw()` must not mutate deterministic gameplay state.

## Modules

### leo.graphics
Rendering and textures, similar to Love2D's drawing API.

```lua
local tex = leo.graphics.newImage("resources/images/character_64x64.png")
leo.graphics.draw(tex, x, y, angle, sx, sy)
leo.graphics.setColor(255, 255, 255, 255)
leo.graphics.drawRectangleFilled(20, 20, 80, 40, 255, 64, 64, 200)
leo.graphics.drawCircleOutline(160, 90, 20, 64, 200, 255, 255)
leo.graphics.drawPolyFilled({20, 140, 80, 160, 60, 200, 10, 180}, 255, 220, 80, 255)
```

Primitive helpers (RGBA per call):
`drawPixel`, `drawLine`, `drawCircleFilled`, `drawCircleOutline`,
`drawRectangleFilled`, `drawRectangleOutline`, `drawRectangleRoundedFilled`,
`drawRectangleRoundedOutline`, `drawTriangleFilled`, `drawTriangleOutline`,
`drawPolyFilled`, `drawPolyOutline`.

Camera helpers:
`beginCamera(camera)`, `endCamera()`.

### leo.window
Window sizing and mode helpers.

```lua
leo.window.setMode("borderless")
leo.window.setSize(1280, 720)
local w, h = leo.window.getSize()
```

### leo.camera
2D camera with Zelda-style follow, using logical coordinates.

```lua
local cam = leo.camera.new()
cam:setTarget(player_x, player_y)
cam:setDeadzone(64, 48)
cam:setSmoothTime(0.12)
cam:update(dt)

leo.graphics.beginCamera(cam)
-- draw world here
leo.graphics.endCamera()
```

### leo.collision
Collision checks return boolean values.

```lua
local hit = leo.collision.checkRecs(10, 10, 32, 32, 20, 20, 10, 10)
local inside = leo.collision.checkPointCircle(50, 50, 60, 60, 12)
local poly_hit = leo.collision.checkPointPoly(40, 40, {20, 20, 80, 20, 70, 60, 30, 70})
```

### leo.font
Font loading and text rendering.

```lua
local font = leo.font.new("resources/font/font.ttf", 24)
leo.font.set(font)
leo.font.print("FPS: 60", 16, 16)
```

### leo.audio
Sound and music, SFML-style behavior.

```lua
local sfx = leo.audio.newSound("resources/sound/coin.wav")
local music = leo.audio.newMusic("resources/music/music.wav")
music:setLooping(true)
music:play()
```

### Input Frame
Deterministic per-tick input helpers passed into `leo.update`.

```lua
if input.keyboard:isPressed("space") then
  -- jump
end

if input.gamepads[1]:isAxisDown("leftx", 0.35, "negative") then
  -- move left
end

if input.mouse:isPressed("left") then
  -- shoot
end
```

### leo.time
Time helpers derived from the fixed tick and frame clock.

```lua
local t = leo.time.ticks()       -- deterministic tick count
local dt = leo.time.tickDelta()  -- fixed dt
```

### leo.fs
VFS helpers (read-only by default).

```lua
local data = leo.fs.read("resources/maps/map.json")
```

### leo.tiled
Load and draw Tiled (.tmj/.tmx) maps via tmxlite.

```lua
local map = leo.tiled.load("resources/maps/map.tmx")
local tiles_w, tiles_h = map:getSize()
local tile_w, tile_h = map:getTileSize()
local pixel_w, pixel_h = map:getPixelSize()

leo.graphics.beginCamera(cam)
map:draw(0, 0)
leo.graphics.endCamera()
```

Map methods:
- `map:draw(x, y)` -> draw all tile layers at offset
- `map:drawLayer(index, x, y)` -> draw a single tile layer (1-based index)
- `map:getSize()` -> tile dimensions (width, height)
- `map:getTileSize()` -> tile size in pixels (width, height)
- `map:getPixelSize()` -> map size in pixels (width, height)
- `map:getLayerCount()` -> number of tile layers
- `map:getLayerName(index)` -> layer name or nil

## Input Frame Shape (Lua)

The Lua `input` object mirrors the C++ `InputFrame`:

```lua
input = {
  keyboard = {
    isDown = function(key) end,
    isPressed = function(key) end,
    isReleased = function(key) end,
  },
  mouse = {
    isDown = function(button) end,
    isPressed = function(button) end,
    isReleased = function(button) end,
    x = 0, y = 0,
    dx = 0, dy = 0,
    wheelX = 0, wheelY = 0,
  },
  gamepads = {
    [1] = {
      isConnected = function() end,
      isDown = function(button) end,
      isPressed = function(button) end,
      isReleased = function(button) end,
      axis = function(axis) end,
      isAxisDown = function(axis, threshold, direction) end,
      isAxisPressed = function(axis, threshold, direction) end,
      isAxisReleased = function(axis, threshold, direction) end,
    },
    [2] = { ... }
  }
}
```

Key/button names are stable strings mapped from engine enums (e.g. `"space"`,
`"left"`, `"south"`, `"start"`, `"leftx"`).

## Recording/Playback Plan

The Lua API stays deterministic by consuming a serialized `InputFrame` each
tick. Recording and playback happen outside Lua:

- The engine records per-tick input frames as compact bitsets/arrays.
- Playback injects frames into `leo.update()` verbatim.
- Lua sees identical input values for a recorded session.

## Error Handling

Lua API calls that fail (missing assets, invalid parameters) throw Lua errors
with a clear message. Asset-loading errors should surface in `leo.load()` to
fail fast.
