
local args = ...
print("Jot version:   " .. JOT_VERSION)
print("Lua version:   " .. LUA_VERSION)
print("Executable:    " .. EXEPATH)
print("package.path:  " .. package.path)
print("package.cpath: " .. package.cpath)
print("args:          " .. table.concat(args, ", "))

local jot = require "jotlib"
local log, path, fs = jot.log, jot.path, jot.fs

print("jot.VERSION = " .. jot.VERSION)

log.trace("sample trace msg from Lua")
log.debug("sample debug msg from Lua")
log.info("sample info msg from Lua")
log.warn("sample warn msg from Lua")
log.error("sample error msg from Lua")
log.panic("sample panic msg from Lua")

--error("oopsy")

for s in jot.split("foo,  bar  , ,baz,bazaar,,,x", ",", "trim", "dropempty", 3, "notrim", 'trim') do
  print("!"..s.."!")
end

local t = fs.glob({"foo", "bar"}, "../src/**/", "../**/*.md", "jot.h")
for i, p in ipairs(t) do
  print(p)
end

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

for _, dir in ipairs(fs.listdir "/home/ujr/doc/src/jot/src") do
  print("  " .. dir)
end

print("cwd: " .. fs.getcwd())
print(path.norm(path.join("./", "good", ".", "bye", "/.")))
--log.panic = "log is a readonly table"
--error("oops, an error")

assert(fs.touch("./DROPME.tmp"))
assert(fs.mkdir("./tempdir"))
assert(fs.rename("./DROPME.tmp", "./tempdir/testfile"))
assert(fs.remove("./tempdir/testfile"))
assert(fs.rmdir("./tempdir"))
print("cwd is " .. fs.getcwd())
print("$HOME is " .. jot.getenv("HOME"))

print("===jot.walkdir(.)===")
print("TYPE", "BYTES", "MODIFIED", "PATH")
for path, type, size, mtime in fs.walkdir(".", 64) do
  print(type, size, mtime, path)
end

mkdn = [[# Title
Paragraph text
on two lines.
- item
- item
  1. first
  2. second
- back to outer list
with *lazy* continuation
---
Paragraph text again]]
print(jot.markdown(mkdn))
