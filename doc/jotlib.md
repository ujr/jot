# Jotlib Manual

Jotlib is the set of functions exposed to Lua scripts.

To make use of jotlib, require it as shown below.
Some related functions are grouped into their own table
for convenience.

```Lua
local jot = require "jotlib"
local path = jot.path
local log = jot.log
```

## Rendering

```Lua
jot.markdown(str, opts)  -- render Markdown in str to HTML
jot.pikchr(str, opts)    -- render Pikchr in str to SVG
```

The **Markdown** renderer aims to be largely but not entirely
conformant to [CommonMark](https://spec.commonmark.org). If new to
Markdown, read a [Markdown tutorial](https://commonmark.org/help).
Options: a number; 0 is for default rendering, 256 requests
rendering as in the CommonMark samples/tests.

**Pikchr** is a new implementation of Kernighan's PIC language
by the SQLite author D. Richard Hipp. To include Pikchr in
Markdown, use a fenced code block with an info string that
reads `pikchr`. About the pikchr language, consult the
[pikchr.org](https://pikchr.org) web page.
Options: a number; 0 is for default rendering, 1 is for dark mode
(meaning inverted colors).

## Logging

Log a message (a string) at one of the given log levels.

```Lua
log.trace(msg)
log.debug(msg)
log.info(msg)
log.warn(msg)
log.error(msg)
log.panic(msg)
```

## File system paths

```Lua
path.basename(path)  --get filename part of path
path.dirname(path)   --get directory part of path
path.split(path)     --iterator over all path components
path.join(...)       --combine args into a path (string)
path.join(table)     --combine table entries into a path
path.norm(path)      --resolve . and .. and // in path
path.match(pat,path) --wildcard path matching (return boolean)
```

The **norm** function does path normalization by resolving
any `.` and `..` in the path, and reducing sequences of
directory separators into just one.

The **match** function knows about the wildcards `?`, `[...]`,
`*` and `**`. The first three are standard, but note that they
do not match the directory separator character (`/`).
The last one matches any number of directories.

## File system operations

Lua restricts itself to what is available with ANSI C, which
does not contain any directory operations. Jotlib adds the
following directory (and file) operations (at the price of
a dependency on POSIX).

```Lua
fs.getcwd()       -- get current working directory
fs.mkdir(path)    -- create directory (parents must exist)
fs.rmdir(path)    -- remove directory (must be empty)
fs.listdir(path)  -- return all directory entries (names)
fs.touch(path, ttime)   -- ensure file exists, update times
fs.remove(path)         -- delete the file at path
fs.rename(old, new)     -- rename and/or move a file
fs.exists(path, type)   -- return true iff path exists
fs.getinfo(path, tab)   -- get info on file at path
fs.walkdir(path, flags) -- file tree iterator (see below)
fs.tempdir(template)    -- create temporary directory
fs.glob(table, pat...)  -- find paths matching pat (see below)
```

The functions that modify the file system return `true` on
success, and `nil` plus an error message (string) on error.

The **touch** updates the modification and access times
of the given file to the given time (or the current time
if `ttime` is missing or `0`). If the file at *path* does
not exist, an empty file is created. (Creating a missing
file and setting its timestamps is *not* atomic.)

The **exists** function checks if the file at *path* exists
and is of the given *type*. If the *type* argument is missing,
it checks if the file at *path* exists regardless of its type.
Valid types are:
`"regular"` or `"file"` for a regular file,
`"directory"` or `"dir"` for a directory,
`"symlink"` for a symbolic link.

The convenient functions `isdir(path)` and `isfile(path)` can
easily be implemented as `exists(path, "directory")` and
`exists(path, "regular")`.

The **getinfo** function returns a table with information
on the file at the given *path*. If the second argument is
a table, this table will be reused and returned. Three
fields are added to the returned table: `type`, which is
one of `"file"`, `"directory"`, `"symlink"`, `"other"`,
`size`, which is file size in bytes, and `mtime` which
is the time of last modification in seconds since the epoch.

The **walkdir** function returns an iterator over the file
tree starting at the given root path. See *walkdir.h* for
details.

The **tempdir** function creates an empty directory with
a randomly chosen name and returns that name.
If no argument is given, the directory will be created in
`$TMPDIR` or */tmp*. If the *template* argument is given,
it must be a string that ends in `XXXXXX` and is a valid
file path; the `X` will be replaced with random letters.

The **glob** function performs “globbing” or finding path
names that match the given pattern. Path names are appended
to the given table, and the given table is also the return
value on success. On failure, return `nil` and a message.
(Implemented on top of `path.match()` and `fs.walkdir()`,
which was conveninent but probably not the most efficient.)
