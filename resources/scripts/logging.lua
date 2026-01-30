-- Leo Engine logging demo.

leo.load = function()
    leo.log.debug("Lua log: debug")
    leo.log.info("Lua log: info")
    leo.log.warn("Lua log: warn")
    leo.log.error("Lua log: error")
    leo.log.fatal("Lua log: fatal")
end

leo.update = function(dt, input)
    leo.quit()
end

leo.draw = function()
end

leo.shutdown = function()
end
