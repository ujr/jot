-- self checks

local jot = require "jotlib"
local path = jot.path
local log = jot.log


log.info("Checking path.basename()")
assert(path.basename("") == "")
assert(path.basename(".") == ".")
assert(path.basename("..") == "..")
assert(path.basename("/") == ""); -- POSIX: "/"
assert(path.basename("\\") == "");
assert(path.basename("///") == "")
assert(path.basename("file.txt") == "file.txt")
assert(path.basename("/foo/bar") == "bar");
assert(path.basename("\\foo\\bar") == "bar");
assert(path.basename("/foo\\bar") == "bar");
assert(path.basename("\\foo/bar") == "bar");
assert(path.basename("foo/bar/") == ""); -- POSIX: "bar" (GNU: empty)


log.info("Checking path.dirname()")
assert(path.dirname("") == ".")
assert(path.dirname(".") == ".")
assert(path.dirname("..") == ".")
assert(path.dirname("./") == ".")
assert(path.dirname("/") == "/")
assert(path.dirname("file.txt") == ".")
assert(path.dirname("/foo/bar") == "/foo")
assert(path.dirname("foo/bar") == "foo")
assert(path.dirname("/foo/") == "/foo")
assert(path.dirname("/foo") == "/")
assert(path.dirname("foo/") == "foo")


log.info("Checking path.split()")
iter = path.split("foo/bar//baz/.")
assert(iter() == "foo")
assert(iter() == "bar")
assert(iter() == "baz")
assert(iter() == ".")
assert(iter() == nil)
assert(iter() == nil) -- idempotent at end
iter = path.split("")
assert(iter() == nil)
iter = path.split("///")
assert(iter() == "/")
assert(iter() == nil)
iter = path.split("./foo///bar//..//baz//")
assert(iter() == ".")
assert(iter() == "foo")
assert(iter() == "bar")
assert(iter() == "..")
assert(iter() == "baz")
assert(iter() == nil)


log.info("Checking path.join()")
assert(path.join() == ".")
assert(path.join("") == ".")
assert(path.join("foo") == "foo")
assert(path.join("foo", "bar") == "foo/bar")
assert(path.join("foo/", "bar") == "foo/bar")
assert(path.join("foo", "/bar") == "foo/bar")
assert(path.join("foo/", "/bar") == "foo/bar")
assert(path.join("/", "/", "/") == "/")
assert(path.join("/foo/", "..", ".", "/bar/") == "/foo/.././bar")
assert(path.join("", "foo", "") == "foo/")
assert(path.join("inner", "", "empty", "", "args") == "inner/empty/args")


log.info("Checking path.norm()")
assert(path.norm("foo") == "foo")
assert(path.norm("/foo") == "/foo")
assert(path.norm("//foo//bar//") == "/foo/bar")
assert(path.norm("foo/..") == ".")
assert(path.norm("..///.././..//foo//./bar/../bazaar/.") == "foo/bazaar")
assert(path.norm(".") == ".")
assert(path.norm("..") == ".")
assert(path.norm("...") == "...")
assert(path.norm("/") == "/")
assert(path.norm("/////") == "/")
assert(path.norm("") == ".")


log.info("Checking miscellaneous functions")
assert(jot.isdir("."))
assert(jot.isdir(".."))
assert(not jot.isdir(EXEPATH))
assert(jot.isdir(jot.dirname(EXEPATH)))
local s = jot.getcwd()
assert(type(s) == "string")
assert(#s > 0)


log.info("OK");
