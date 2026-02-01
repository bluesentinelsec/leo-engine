# Lua Validation Checklist

Numbered checklist of every Lua API function and symbol exposed by Leo Engine.

1. `leo` (global table)
1. `leo.load` (lifecycle callback)
1. `leo.update` (lifecycle callback)
1. `leo.draw` (lifecycle callback)
1. `leo.shutdown` (lifecycle callback)
1. `leo.quit`

1. `leo.graphics` (module table)
1. `leo.graphics.newImage`
1. `leo.graphics.draw`
1. `leo.graphics.setColor`
1. `leo.graphics.clear`
1. `leo.graphics.getSize`
1. `leo.graphics.beginViewport`
1. `leo.graphics.endViewport`
1. `leo.graphics.drawPixel`
1. `leo.graphics.drawLine`
1. `leo.graphics.drawGrid`
1. `leo.graphics.drawCircleFilled`
1. `leo.graphics.drawCircleOutline`
1. `leo.graphics.drawRectangleFilled`
1. `leo.graphics.drawRectangleOutline`
1. `leo.graphics.drawRectangleRoundedFilled`
1. `leo.graphics.drawRectangleRoundedOutline`
1. `leo.graphics.drawTriangleFilled`
1. `leo.graphics.drawTriangleOutline`
1. `leo.graphics.drawPolyFilled`
1. `leo.graphics.drawPolyOutline`
1. `leo.graphics.beginCamera`
1. `leo.graphics.endCamera`

1. `Texture` userdata (returned by `leo.graphics.newImage`)
1. `texture:getSize`

1. `leo.window` (module table)
1. `leo.window.setSize`
1. `leo.window.setMode`
1. `leo.window.getSize`

1. `leo.font` (module table)
1. `leo.font.new`
1. `leo.font.set`
1. `leo.font.print`

1. `Font` userdata (returned by `leo.font.new`)
1. `font:print`
1. `font:getLineHeight`

1. `leo.audio` (module table)
1. `leo.audio.newSound`
1. `leo.audio.newMusic`

1. `Sound` userdata (returned by `leo.audio.newSound`)
1. `sound:play`
1. `sound:pause`
1. `sound:stop`
1. `sound:isPlaying`
1. `sound:setLooping`
1. `sound:setVolume`
1. `sound:setPitch`

1. `Music` userdata (returned by `leo.audio.newMusic`)
1. `music:play`
1. `music:pause`
1. `music:stop`
1. `music:isPlaying`
1. `music:setLooping`
1. `music:setVolume`
1. `music:setPitch`

1. `leo.time` (module table)
1. `leo.time.ticks`
1. `leo.time.tickDelta`

1. `leo.math` (module table)
1. `leo.math.clamp`
1. `leo.math.clamp01`

1. `leo.fs` (module table)
1. `leo.fs.read`
1. `leo.fs.readWriteDir`
1. `leo.fs.write`
1. `leo.fs.getWriteDir`
1. `leo.fs.listWriteDir`
1. `leo.fs.listWriteDirFiles`
1. `leo.fs.deleteFile`
1. `leo.fs.deleteDir`

1. `leo.tiled` (module table)
1. `leo.tiled.load`

1. `TiledMap` userdata (returned by `leo.tiled.load`)
1. `map:draw`
1. `map:drawLayer`
1. `map:getSize`
1. `map:getTileSize`
1. `map:getPixelSize`
1. `map:getLayerCount`
1. `map:getLayerName`

1. `leo.camera` (module table)
1. `leo.camera.new`

1. `Camera` userdata (returned by `leo.camera.new`)
1. `camera:setTarget`
1. `camera:setPosition`
1. `camera:setOffset`
1. `camera:setDeadzone`
1. `camera:setSmoothTime`
1. `camera:setBounds`
1. `camera:setClamp`
1. `camera:setZoom`
1. `camera:setRotation`
1. `camera:update`
1. `camera:worldToScreen`
1. `camera:screenToWorld`

1. `leo.collision` (module table)
1. `leo.collision.checkRecs`
1. `leo.collision.checkCircles`
1. `leo.collision.checkCircleRec`
1. `leo.collision.checkCircleLine`
1. `leo.collision.checkPointRec`
1. `leo.collision.checkPointCircle`
1. `leo.collision.checkPointTriangle`
1. `leo.collision.checkPointLine`
1. `leo.collision.checkPointPoly`
1. `leo.collision.checkLines`

1. `input` (table passed to `leo.update`)
1. `input.quit`
1. `input.frame`
1. `input.keyboard`
1. `input.keyboard.isDown`
1. `input.keyboard.isPressed`
1. `input.keyboard.isReleased`
1. `input.mouse`
1. `input.mouse.isDown`
1. `input.mouse.isPressed`
1. `input.mouse.isReleased`
1. `input.mouse.setCursorVisible`
1. `input.mouse.x`
1. `input.mouse.y`
1. `input.mouse.dx`
1. `input.mouse.dy`
1. `input.mouse.wheelX`
1. `input.mouse.wheelY`

1. `leo.mouse` (module table)
1. `leo.mouse.setCursorVisible`
1. `input.gamepads`
1. `input.gamepads[1]`
1. `input.gamepads[2]`
1. `gamepad:isConnected`
1. `gamepad:isDown`
1. `gamepad:isPressed`
1. `gamepad:isReleased`
1. `gamepad:axis`
1. `gamepad:isAxisDown`
1. `gamepad:isAxisPressed`
1. `gamepad:isAxisReleased`
