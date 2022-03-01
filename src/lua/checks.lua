-- self checks

local jot = require "jotlib"
local path = jot.path
local log = jot.log


--log.info("Checking blob functions")
--assert(jot.checkblob(true))


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
assert(path.join({}) == ".");
assert(path.join({"foo", "bar", "baz"}) == "foo/bar/baz")
assert(path.join({"foo/", "//bar", ""}) == "foo/bar/");
assert(path.join({"/", "foo", "/"}) == "/foo/")


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
assert(path.match("*", "f.x"))
assert(not path.match("*", "d/f.x"))
assert(path.match("**", "f.x"))
assert(path.match("**", "d/f.x"))
assert(path.match("**", "d/e/f.x"))
assert(path.match("**/*.x", "f.x"))
assert(path.match("**/*.x", "d/f.x"))
assert(path.match("**/*.x", "d/e/f.x"))
assert(not path.match("**/*.x", "foo/"))
assert(path.match("**/a*", "a/b/ab"))
assert(path.match("a*/**/a*", "a/b/ab"))
assert(path.match("**/a*/**/b*", "b/a/b/a/b"))
assert(not path.match("a/**/*/**/b", "a/b"))
assert(path.match("a/**/*/**/b", "a//b"))
assert(not path.match("**/", "foo"))
assert(path.match("**/", "bar/"))
assert(path.match("**/", "bar/baz/"))


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
--exercise globbing:
local t = jot.glob({}, path.join(dir, "**", "*[ae]*"))
table.sort(t)
--for i,v in ipairs(t) do print(v) end
assert(t[1] == path.join(dir, "bar"))
assert(t[2] == path.join(dir, "baz"))
assert(t[3] == path.join(dir, "sub", "ham"))
assert(t[4] == path.join(dir, "sub", "spam"))
assert(t[5] == path.join(dir, "sub", "subsub", "deeply"))
assert(t[6] == path.join(dir, "sub", "subsub", "nested"))
assert(t[7] == nil)
--globbing a single file:
t = jot.glob({}, path.join(dir, "sub", "spam"))
assert(t[1] == path.join(dir, "sub", "spam"))
assert(t[2] == nil)
--and recursively delete by a post-order walk (DP not D):
for path, type in jot.walkdir(dir) do
  if type=="F" or type=="DP" or type=="SL" then
    assert(jot.remove(path))
  end
end
assert(not jot.exists(dir))


log.info("Checking Markdown rendering");
mkdn = [[# Title
Paragraph text with
a [link](/url) to **nowhere**.]]
html = jot.markdown(mkdn)
assert(html == [[<h1>Title</h1>
<p>Paragraph text with
a <a href="/url">link</a> to <strong>nowhere</strong>.</p>
]])

mkdnskip = {
  [204]="leave precedence of duplicate link defs undefined",
  [206]="will not case-fold non-ASCII",
  [353]="ignorant of Unicode categories",
  [539]="will not case-fold non-ASCII",
  [543]="leave precedence of duplicate link defs undefined",
  [618]="be laxer on html tag syntax",
  [620]="be laxer on html tag syntax",
  [621]="be laxer on html tag syntax",
  [625]="be laxer on html comments; CM is XML strict",
  [626]="be laxer on html comments; CM is XML strict",
  --
  [4] = "lazyness (low priority)",
  [5] = "lazyness (low priority)",
  [6] = "lazyness (low priority)",
  [7] = "lazyness (low priority)",
  [9] = "lazyness (low priority)",
  [93] = "illogical (low priority)",
  [212] = "known bug (medium priority)",
  [213] = "known bug (medium priority)",
  [218] = "known bug (medium priority)",
  [237] = "illogical (but also a bug)",
  [238] = "illogical (low priority)",
  [312] = "illogical (low priority)",
  [318] = "known bug (low priority)",
  --
  [999]="reason"
}
numfail = 0
first = nil
last = nil
function Test(t)
  if first and t.number < first then return end
  if last and t.number > last then return end
  if mkdnskip[t.number] then
    log.debug(string.format("mkdn SKIP test #%d (%s)", t.number, mkdnskip[t.number]))
    return
  end
  local r = jot.markdown(t.input, 256)
  if r ~= t.result then
    numfail = numfail + 1
    log.error(string.format("mkdn FAIL test #%d on %s", t.number, t.section))
    log.trace("Input: " .. t.input)
    log.trace("Output: " .. r)
    log.trace("Expected: " .. t.result)
  else
    log.debug(string.format("mkdn pass test #%d on %s", t.number, t.section))
  end
end
env = { Test = Test }
loadfile("../test/spec.lua", "t", env)()
assert(numfail == 0, "Failed CommonMark test(s): " .. numfail)


log.info("Checking Pikchr rendering")
pik = [[line "Test"]]
svg = jot.pikchr(pik)
assert(svg:sub(1,5) == "<svg ")
assert(svg:sub(-7) == "</svg>\n")


log.info("OK");
