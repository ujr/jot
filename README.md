# Jot

This is jot (working title), a self-contained tool
to generate static web sites with blog capability.
Yet another? Yes, there are so many out there that
one more does no harm.

## Usage

    jot new <path>        create initial site structure
    jot build [path]      build or rebuild site in path (or .)
    jot render [file]     render file (or stdin) to stdout
    jot help              show available commands and options

## Features

Inspired by [Jekyll][jekyll] and [Hugo][hugo], but aiming to be
much simpler, more lightweight, and free of large dependencies.
Using [Lua][lua] for scripting and [mustache][mustache] (or
something similarly simple) for templates. Will provide support
for [pikchr][pikchr] (Pic-like inline graphics).

## Notes

A jot (noun) is a small dot, from the greek letter iota,
which has no dot, but is often written as a very small
dash below a vowel (the *iota subscriptum*).
To jot something down is to quickly take notes, and
the jotter is the notebook or scratch pad where you do it.

Lua is copyright © 1994–2021 Lua.org, PUC-Rio. MIT license.  
Pikchr is copyright © 2020 by D. Richard Hipp. Zero-clause BSD license.

[jekyll]: https://jekyllrb.com/
[hugo]: https://gohugo.io/
[lua]: https://www.lua.org/
[mustache]: https://mustache.github.io/
[pikchr]: https://pikchr.org/
