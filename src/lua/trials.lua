print("Jot version: " .. JOT_VERSION)
print("Lua version: " .. LUA_VERSION)
print("Executable:  " .. EXEPATH)
print("Arguments:   " .. tostring(#ARGS))
for i, v in ipairs(ARGS) do
  print("  arg" .. tostring(i) .. ": " .. tostring(v))
end
print("package.path:  " .. package.path)
print("package.cpath: " .. package.cpath)

local jot = require "jotlib"

jot.log_trace("sample trace msg from Lua")
jot.log_debug("sample debug msg from Lua")
jot.log_info("sample info msg from Lua")
jot.log_warn("sample warn msg from Lua")
jot.log_error("sample error msg from Lua")
jot.log_panic("sample panic msg from Lua")

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
