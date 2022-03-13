
local jot = require "jotlib"
local log = jot.log
local path = jot.path
local fs = jot.fs
local lustache = require "lustache"


-- proc: processor (or renderer), a function (infn, outfn, ctx)
-- that reads infn and creates or overwrites outfn, optionally
-- using the given context information (a table)

local function markdown_proc(infile, ctx, outfile)
  -- read src|render markdown|expand mustache|layout|write dst
  local t = infile:read("a")
  t = jot.markdown(t)
  t = lustache:render(t, ctx.model, ctx.partials)
  --if ctx.layout then TODO end
  if not outfile then outfile = io.output() end
  outfile:write(t)
end


local function mustache_proc(infile, ctx, outfile)
  local t = infile:read("a")
  t = lustache:render(t, ctx.model, ctx.partials)
  if not outfile then outfile = io.output() end
  outfile:write(t)
end


local function verbatim_proc(infile, ctx, outfile)
  -- read src | write dst
  local t = infile:read("a")
  if not outfile then outfile = io.output() end
  outfile:write(t)
end


local function getproc(fn)
  -- TODO search processor table for first match, if none found, proceed as here:
  if path.match("**/*.md", fn) then
    log.debug("using markdown processor")
    return markdown_proc
  end
  if path.match("**/*.tmpl", fn) then
    log.debug("using mustache processor")
    return mustache_proc
  end
  log.debug("using default (verbatim) processor for " .. fn)
  return verbatim_proc
end


return function(infn, outfn, luafiles, partials, args)
  log.debug(string.format(
    "render(infn=%q, outfn=%q, luafiles=%s, partials=%s)",
    infn, outfn, luafiles, partials))

  -- run all Lua initialization files:
  for fn in path.split(luafiles) do
    assert(loadfile(fn))()
  end

  -- load partials for mustache rendering:
  local t = {}
  for pat in path.split(partials) do
    fs.glob(t, pat)
  end

  partials = {}  -- reuse
  for i, fn in ipairs(t) do
    log.debug("loading partial: " .. fn)
    local f = assert(io.open(fn, "r"))
    local t = f:read("*a")  f:close()  -- TODO consider: t = fs.readfile(fn)
    partials[path.basename(fn)] = t    -- TODO or just path.norm(fn)? cut ext?
  end

  local proc = getproc(infn)

  local ctx = {}
  ctx.infn = infn
  ctx.outfn = outfn
  ctx.layout = nil
  ctx.partials = partials
  ctx.model = _G
  ARGS = args

  if infn and infn ~= "-" then
    io.input(assert(io.open(infn, "r")))
  end

  if outfn and outfn ~= "-" then
    io.output(assert(io.open(outfn, "w")))
  end

  proc(io.input(), ctx, io.output())

  --local tmpl = io.read("a")
  --local s = lustache:render(tmpl, _G, partials)
  --io.write(s)

  io.flush()
  io.input():close()
  io.output():close()
end
