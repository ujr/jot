-- self checks

local jot = require "jotlib"
local log = jot.log
local path = jot.path
local fs = jot.fs


--log.info("Checking blob functions")
--assert(jot.checkblob(true))  -- makes Valgrind fail


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


log.info("Checking path.parts()")
local iter = path.parts("foo/bar//baz/.")
assert(iter() == "foo")
assert(iter() == "bar")
assert(iter() == "baz")
assert(iter() == ".")
assert(iter() == nil)
assert(iter() == nil) -- idempotent at end
iter = path.parts("")
assert(iter() == nil)
iter = path.parts("///")
assert(iter() == "/")
assert(iter() == nil)
iter = path.parts("./foo///bar//..//baz//")
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


log.info("Checking path.split()")
local iter = path.split("a/b;a/c")
assert(iter() == "a/b")
assert(iter() == "a/c")
assert(iter() == nil)
iter = path.split(";a/b;;c;")
assert(iter() == "a/b")
assert(iter() == "c")
assert(iter() == nil)


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
assert(path.match("**/*.md", "./foo/bar.md"))
assert(path.match("**/*.md", "../foo/bar.md"))


log.info("Checking jot.split()")
local iter = jot.split("foo", "-")
assert(iter() == "foo")
assert(iter() == nil)
iter = jot.split(" foo- bar -baz ", "-")
assert(iter() == " foo")
assert(iter() == " bar ")
assert(iter() == "baz ")
assert(iter() == nil)
iter = jot.split(" foo- bar -baz ", "-", 'trim')
assert(iter() == "foo")
assert(iter() == "bar")
assert(iter() == "baz")
assert(iter() == nil)
iter = jot.split("foo--bar- -baz", "-", 'notrim', 'dropempty')
assert(iter() == "foo")
assert(iter() == "bar")
assert(iter() == " ")
assert(iter() == "baz")
assert(iter() == nil)
iter = jot.split("foo--bar- -baz", "-", 'trim', 'dropempty')
assert(iter() == "foo")
assert(iter() == "bar")
assert(iter() == "baz")
assert(iter() == nil)
iter = jot.split("foo,,bar,,baz", ",", 'dropempty', 4)
assert(iter() == "foo")
assert(iter() == "bar")
-- dropped empty part counts against max!
assert(iter() == ",baz")
assert(iter() == nil)


log.info("Checking file system operation: getcwd, exists, mkdir, rmdir")
assert(fs.exists(".", "directory") == true)
assert(fs.exists("..", "file") == false)
assert(fs.exists("..", "any") == true)
assert(fs.exists(EXEPATH))
assert(fs.exists(EXEPATH, "file"))
assert(not fs.exists(EXEPATH, "directory"))
assert(fs.exists(path.dirname(EXEPATH), "directory"))
local s = fs.getcwd()
assert(type(s) == "string")
assert(#s > 0)
local name = "/tmp/unlikelybutriskyname"
assert(fs.mkdir(name))
assert(fs.exists(name, "directory"))
local info = assert(fs.getinfo(name))
assert(type(info) == "table")
assert(info.type == "directory")
log.debug(string.format("type %s, size %s, mtime %s",
  info.type, info.size, os.date("%Y-%m-%d %H:%M:%S", info.mtime)))
--print(info.type, info.size, os.date("%Y-%m-%d %H:%M:%S", info.mtime))
assert(fs.rmdir(name))


log.info("Checking file system operations: touch, glob, walkdir, remove")
local dir = assert(fs.tempdir())
assert(fs.touch(path.join(dir, "foo")))
assert(fs.touch(path.join(dir, "bar")))
assert(fs.touch(path.join(dir, "baz")))
assert(fs.mkdir(path.join(dir, "sub")))
assert(fs.touch(path.join(dir, "sub", "spam")))
assert(fs.touch(path.join(dir, "sub", "ham")))
assert(fs.mkdir(path.join(dir, "sub", "subsub")))
assert(fs.touch(path.join(dir, "sub", "subsub", "deeply")))
assert(fs.touch(path.join(dir, "sub", "subsub", "nested")))
--check stuff exists:
assert(fs.exists(dir), "directory")
assert(fs.exists(path.join(dir, "sub/subsub/nested"), "file"))
assert(fs.exists(path.join(dir, "sub/subsub"), "directory"))
--exercise globbing:
local t = fs.glob({}, path.join(dir, "**", "*[ae]*"))
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
t = fs.glob({}, path.join(dir, "sub", "spam"))
assert(t[1] == path.join(dir, "sub", "spam"))
assert(t[2] == nil)
--and recursively delete by a post-order walk (DP not D):
for path, type in fs.walkdir(dir) do
  if type=="F" or type=="DP" or type=="SL" then
    assert(fs.remove(path))
  end
end
assert(not fs.exists(dir))


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
