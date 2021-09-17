
local lustache = require "lustache"
local jot = require "jotlib"
local gmatch = string.gmatch

return function(infn, outfn, partials)
  jot.log_debug(string.format("render(infn=%q, outfn=%q, partials=%s)", infn, outfn, partials))
  if infn and infn ~= "-" then
    io.input(assert(io.open(infn, "r")))
  end
  if outfn and outfn ~= "-" then
    io.output(assert(io.open(outfn, "w")))
  end
  if partials and type(partials) == "table" then
    for _, fn in ipairs(partials) do
      jot.log_debug("loading partial: " .. fn)
      local f = assert(io.open(fn, "r"))
      local t = f:read("*a"); f:close()
      partials[jot.basename(fn)] = t
    end
  end

  local tmpl = io.read("a")
  local s = lustache:render(tmpl, _G, partials)

  io.write(s)
  io.flush()
end
