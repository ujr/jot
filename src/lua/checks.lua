-- self checks

local jot = require "jotlib"
local path = jot.path
local log = jot.log


log.info("Checking path.basename()")
path.config("/")
assert(path.basename("") == "")
assert(path.basename(".") == ".")
assert(path.basename("..") == "..")
assert(path.basename("/") == ""); -- POSIX: "/"
--assert(path.basename("\\") == "");
assert(path.basename("///") == "")
assert(path.basename("file.txt") == "file.txt")
assert(path.basename("/foo/bar") == "bar");
--assert(path.basename("\\foo\\bar") == "bar");
--assert(path.basename("/foo\\bar") == "bar");
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


log.info("Checking path.match()")
assert(path.match("", ""))
assert(path.match("*.txt", "file.txt"))
assert(not path.match("*.txt", "dir/file.txt"))
assert(path.match("**/*.txt", "dir/file.txt"))
assert(not path.match("foo/*", "foo/.hidden"))
assert(path.match("foo/.*", "foo/.hidden"))


log.info("Checking miscellaneous functions")
assert(jot.exists(".", "directory") == true)
assert(jot.exists("..", "file") == false)
assert(jot.exists("..", "any") == true)
assert(jot.exists(EXEPATH))
assert(jot.exists(EXEPATH, "file"))
assert(not jot.exists(EXEPATH, "directory"))
assert(jot.exists(path.dirname(EXEPATH), "directory"))
local s = jot.getcwd()
assert(type(s) == "string")
assert(#s > 0)
local name = "/tmp/unlikelybutriskyname"
assert(jot.mkdir(name))
assert(jot.exists(name, "directory"))
local info = assert(jot.getinfo(name))
assert(type(info) == "table")
assert(info.type == "directory")
--print(info.type, info.size, os.date("%Y-%m-%d %H:%M:%S", info.mtime))
assert(jot.rmdir(name))


log.info("Checking file system operations")
dir = assert(jot.tempdir())
assert(jot.touch(path.join(dir, "foo")))
assert(jot.touch(path.join(dir, "bar")))
assert(jot.touch(path.join(dir, "baz")))
assert(jot.mkdir(path.join(dir, "sub")))
assert(jot.touch(path.join(dir, "sub", "spam")))
assert(jot.touch(path.join(dir, "sub", "ham")))
assert(jot.mkdir(path.join(dir, "sub", "subsub")))
assert(jot.touch(path.join(dir, "sub", "subsub", "deeply")))
assert(jot.touch(path.join(dir, "sub", "subsub", "nested")))
--check stuff exists:
assert(jot.exists(dir), "directory")
assert(jot.exists(path.join(dir, "sub/subsub/nested"), "file"))
assert(jot.exists(path.join(dir, "sub/subsub"), "directory"))
--and recursively delete by a post-order walk (DP not D):
for path, type in jot.walkdir(dir) do
  if type=="F" or type=="DP" or type=="SL" then
    assert(jot.remove(path))
  end
end
assert(not jot.exists(dir))


log.info("OK");
