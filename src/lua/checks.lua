-- self checks

local jot = require "jotlib"


jot.log_info("Checking jot.basename()")
assert(jot.basename("") == "")
assert(jot.basename(".") == ".")
assert(jot.basename("..") == "..")
assert(jot.basename("/") == ""); -- POSIX: "/"
assert(jot.basename("\\") == "");
assert(jot.basename("///") == "")
assert(jot.basename("file.txt") == "file.txt")
assert(jot.basename("/foo/bar") == "bar");
assert(jot.basename("\\foo\\bar") == "bar");
assert(jot.basename("/foo\\bar") == "bar");
assert(jot.basename("\\foo/bar") == "bar");
assert(jot.basename("foo/bar/") == ""); -- POSIX: "bar" (GNU: empty)


jot.log_info("Checking jot.dirname()")
assert(jot.dirname("") == ".")
assert(jot.dirname(".") == ".")
assert(jot.dirname("..") == ".")
assert(jot.dirname("/") == "/")
assert(jot.dirname("./") == ".")
assert(jot.dirname("file.txt") == ".")
assert(jot.dirname("/foo/bar") == "/foo")
assert(jot.dirname("foo/bar") == "foo")
assert(jot.dirname("/foo/") == "/foo")
assert(jot.dirname("/foo") == "/")
assert(jot.dirname("foo/") == "foo")


jot.log_info("Checking jot.splitpath()")
iter = jot.splitpath("foo/bar//baz/.")
assert(iter() == "foo")
assert(iter() == "bar")
assert(iter() == "baz")
assert(iter() == ".")
assert(iter() == nil)
assert(iter() == nil) -- idempotent at end
iter = jot.splitpath("")
assert(iter() == nil)
iter = jot.splitpath("///")
assert(iter() == "/")
assert(iter() == nil)
iter = jot.splitpath("./foo///bar//..//baz//")
assert(iter() == ".")
assert(iter() == "foo")
assert(iter() == "bar")
assert(iter() == "..")
assert(iter() == "baz")
assert(iter() == nil)


jot.log_info("Checking jot.joinpath()")
assert(jot.joinpath() == ".")
assert(jot.joinpath("") == ".")
assert(jot.joinpath("foo") == "foo")
assert(jot.joinpath("foo", "bar") == "foo/bar")
assert(jot.joinpath("foo/", "bar") == "foo/bar")
assert(jot.joinpath("foo", "/bar") == "foo/bar")
assert(jot.joinpath("foo/", "/bar") == "foo/bar")
assert(jot.joinpath("/", "/", "/") == "/")
assert(jot.joinpath("/foo/", "..", ".", "/bar/") == "/foo/.././bar")
assert(jot.joinpath("", "foo", "") == "foo/")
assert(jot.joinpath("inner", "", "empty", "", "args") == "inner/empty/args")


jot.log_info("Checking jot.normpath()")
assert(jot.normpath("foo") == "foo")
assert(jot.normpath("/foo") == "/foo")
assert(jot.normpath("//foo//bar//") == "/foo/bar")
assert(jot.normpath("foo/..") == ".")
assert(jot.normpath("..///.././..//foo//./bar/../bazaar/.") == "foo/bazaar")
assert(jot.normpath(".") == ".")
assert(jot.normpath("..") == ".")
assert(jot.normpath("...") == "...")
assert(jot.normpath("/") == "/")
assert(jot.normpath("/////") == "/")
assert(jot.normpath("") == ".")


jot.log_info("Checking miscellaneous functions")
assert(jot.isdir("."))
assert(jot.isdir(".."))
assert(not jot.isdir(EXEPATH))
assert(jot.isdir(jot.dirname(EXEPATH)))
local s = jot.getcwd()
assert(type(s) == "string")
assert(#s > 0)


jot.log_info("OK");
