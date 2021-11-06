print("Jot version: " .. JOT_VERSION)
print("Lua version: " .. LUA_VERSION)
print("Executable:  " .. EXEPATH)
print("package.path:  " .. package.path)
print("package.cpath: " .. package.cpath)

local jot = require "jotlib"
local log, path = jot.log, jot.path

print("jot.VERSION = " .. jot.VERSION)

log.trace("sample trace msg from Lua")
log.debug("sample debug msg from Lua")
log.info("sample info msg from Lua")
log.warn("sample warn msg from Lua")
log.error("sample error msg from Lua")
log.panic("sample panic msg from Lua")

local svg = jot.pikchr("arrow; box \"Hello\" \"World\"", true);

--print(svg)

local lustache = require "lustache"
f = function () return 2 + 3 * 4 end
o = { a = "A", b = "B", c = "<br/>", "one", "two", "three" }
musketeers = { "Athos", "Aramis", "Porthos", "D'Artagnan" }
wrap = function (self) return "<" .. self .. ">" end

s = lustache:render([[
This is Jot {{JOT_VERSION}}, f() = {{f}}, {{o.a}}{{o.b}}{{&o.c}}
{{#musketeers}}
- {{{wrap}}}
{{/musketeers}}
]], _ENV)
print(s)

for _, dir in ipairs(jot.listdir "/home/ujr/doc/src/jot/src") do
  print("  " .. dir)
end

print("cwd: " .. jot.getcwd())
print(path.norm(path.join("./", "good", ".", "bye", "/.")))
--log.panic = "log is a readonly table"
--error("oops, an error")

assert(jot.touch("./DROPME.tmp"))
assert(jot.mkdir("./tempdir"))
assert(jot.rename("./DROPME.tmp", "./tempdir/testfile"))
assert(jot.remove("./tempdir/testfile"))
assert(jot.rmdir("./tempdir"))
print("cwd is " .. jot.getcwd())
print("$HOME is " .. jot.getenv("HOME"))

print("===jot.walkdir(.)===")
print("TYPE", "BYTES", "MODIFIED", "PATH")
for path, type, size, mtime in jot.walkdir(".", 0) do
  print(type, size, mtime, path)
end
