
local lustache = require "lustache"

return function(infn, outfn, partials)
  if infn and infn ~= "-" then
    io.input(assert(io.open(infn, "r")))
  end
  if outfn and outfn ~= "-" then
    io.output(assert(io.open(outfn, "w")))
  end

  local tmpl = io.read("a")
  local s = lustache:render(tmpl, _G, partials)

  io.write(s)
  io.flush()
end
