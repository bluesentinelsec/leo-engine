function leo.init()
  score = 0
  -- note: load_font, not load
  font = leo.font.load("resources/font/font.ttf", 32)
end

function leo.update(dt)
  score = score + math.floor(dt * 10)
end

function leo.render()
  -- draw_font, not draw
  leo.font.draw(font, "Score: "..score, 10, 100, 1,1,1,1)
end

function leo.exit()
  -- free_font, not free
  leo.font.free(font)
end

