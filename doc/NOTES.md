# Notes

## Markdown

- Each sequence of characters is a valid (but probably ugly)
  CommonMark (Markdown) document: there is no syntax error!
- escapes: only before an ASCII punct character, otherwise literal.
- escapes: in ANY context except code span/block, autolinks, raw HTML.
- entities: in ANY context except code span/block.
- Test suites: see here (has pointers to others)
  <https://github.com/karlcow/markdown-testsuite>
- The CommonMark reference implementation, huge:
  <https://github.com/commonmark/cmark>

References

- Our parser is inspired by Natacha Porté's Markdown parser
  at <http://fossil.instinctive.eu/libsoldout>, a derivative of
  which is used in [Fossil](https://fossil-scm.org) and probably
  other places.
- However, our parser was written from scratch, and the design
  gradually moved somewhat away from its original inspiration.
- [CommonMark](https://commonmark.org) is useful not only
  as a (somewhat lengthy) specification, but also as a
  large collection of examples and test cases.
- The original Markdown description by John Gruber is
  available at <https://daringfireball.net/projects/markdown/>

## Directory Structure

- config.jot       main config, a Lua file, required (signature)
- data/            configuration data
- init/            initialization scripts (.lua)
- content/         site content (one subdir per section)
  - index.md       available through <http://root/index.html>
  - about.md       available through <http://root/about.html>
  - blog/          available through <http://root/blog/*>
  - articles/      available through <http://root/articles/*>
- drafts/          like content/, but drafts, only published on request
- layouts/         page layout templates
  - default.html   the default page layout (if nothing else specified)
- partials/        the root for partial inclusion
- public/          build target folder; to be published; exclude from SCM
- static/          assets; copied verbatim to site root; eg images, js, css

## Processing Pipelines

- content/\*.md    markdown | mustache
- content/\*.\*    mustache
- partials/\*      mustache
- static/\*        verbatim

## Site Building

- Initialize (defaults, args)
- Load `config.jot`   (args override config)
- Load `init/\*.lua`  (additional functionality)
- Load `data/\*`      (optional, avaliable in `site.data.*`)
- Scan `content/**`   (available in `site.content.section.*`)
- Scan `drafts/**`    (optional, merged with content)
- Create DestDir      (rm -rf, mkdir -p, default: public/)
- Walk SourceDir:     (default is .)
  - skip public,layouts,partials,etc.
  - skip stuff excluded by config
  - let proc = getproc(filename)  (see Processing Pipelines above)
  - if proc: proc(filename, DestDir)
  - else: copy file to DestDir (verbatim)

To render a template is to compile it and call the resulting function,
passing it a view model and an output function.

```Lua
render = function(fn, viewmodel, outfun)
  local f = assert(io.open(fn, "r"))
  jot.compile(f:read("a"))(viewmodel, outfun)
  f:close()
end
```

## Standard Options

From <http://www.tldp.org/LDP/abs/html/standard-options.html>

```text
-h  --help       Give usage message and exit
-v  --version    Show program version and exit

-a  --all        Show all information / operate on all arguments
-l  --list       List files or arguments (no other action)
-o               Output file name
-q  --quiet      Suppress stdout
-r  --recursive  Operate recursively (down directory tree)
-v  --verbose    Additional info to stdout or stderr
-z               Apply or assume compression (usually gzip)
```
