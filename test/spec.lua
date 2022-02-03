-- Generated from CommonMark's spec.txt
-- Introduction
-- What is Markdown?
-- Why is a spec needed?
-- About this document
-- Preliminaries
-- Characters and lines
-- Tabs
Test{
  number = 1,
  section = "Tabs",
  input = [==[	foo	baz		bim
]==],
  result = [==[<pre><code>foo	baz		bim
</code></pre>
]==]
}

Test{
  number = 2,
  section = "Tabs",
  input = [==[  	foo	baz		bim
]==],
  result = [==[<pre><code>foo	baz		bim
</code></pre>
]==]
}

Test{
  number = 3,
  section = "Tabs",
  input = [==[    a	a
    ὐ	a
]==],
  result = [==[<pre><code>a	a
ὐ	a
</code></pre>
]==]
}

Test{
  number = 4,
  section = "Tabs",
  input = [==[  - foo

	bar
]==],
  result = [==[<ul>
<li>
<p>foo</p>
<p>bar</p>
</li>
</ul>
]==]
}

Test{
  number = 5,
  section = "Tabs",
  input = [==[- foo

		bar
]==],
  result = [==[<ul>
<li>
<p>foo</p>
<pre><code>  bar
</code></pre>
</li>
</ul>
]==]
}

Test{
  number = 6,
  section = "Tabs",
  input = [==[>		foo
]==],
  result = [==[<blockquote>
<pre><code>  foo
</code></pre>
</blockquote>
]==]
}

Test{
  number = 7,
  section = "Tabs",
  input = [==[-		foo
]==],
  result = [==[<ul>
<li>
<pre><code>  foo
</code></pre>
</li>
</ul>
]==]
}

Test{
  number = 8,
  section = "Tabs",
  input = [==[    foo
	bar
]==],
  result = [==[<pre><code>foo
bar
</code></pre>
]==]
}

Test{
  number = 9,
  section = "Tabs",
  input = [==[ - foo
   - bar
	 - baz
]==],
  result = [==[<ul>
<li>foo
<ul>
<li>bar
<ul>
<li>baz</li>
</ul>
</li>
</ul>
</li>
</ul>
]==]
}

Test{
  number = 10,
  section = "Tabs",
  input = [==[#	Foo
]==],
  result = [==[<h1>Foo</h1>
]==]
}

Test{
  number = 11,
  section = "Tabs",
  input = [==[*	*	*	
]==],
  result = [==[<hr />
]==]
}

-- Insecure characters
-- Backslash escapes
Test{
  number = 12,
  section = "Backslash escapes",
  input = [==[\!\"\#\$\%\&\'\(\)\*\+\,\-\.\/\:\;\<\=\>\?\@\[\\\]\^\_\`\{\|\}\~
]==],
  result = [==[<p>!&quot;#$%&amp;'()*+,-./:;&lt;=&gt;?@[\]^_`{|}~</p>
]==]
}

Test{
  number = 13,
  section = "Backslash escapes",
  input = [==[\	\A\a\ \3\φ\«
]==],
  result = [==[<p>\	\A\a\ \3\φ\«</p>
]==]
}

Test{
  number = 14,
  section = "Backslash escapes",
  input = [==[\*not emphasized*
\<br/> not a tag
\[not a link](/foo)
\`not code`
1\. not a list
\* not a list
\# not a heading
\[foo]: /url "not a reference"
\&ouml; not a character entity
]==],
  result = [==[<p>*not emphasized*
&lt;br/&gt; not a tag
[not a link](/foo)
`not code`
1. not a list
* not a list
# not a heading
[foo]: /url &quot;not a reference&quot;
&amp;ouml; not a character entity</p>
]==]
}

Test{
  number = 15,
  section = "Backslash escapes",
  input = [==[\\*emphasis*
]==],
  result = [==[<p>\<em>emphasis</em></p>
]==]
}

Test{
  number = 16,
  section = "Backslash escapes",
  input = [==[foo\
bar
]==],
  result = [==[<p>foo<br />
bar</p>
]==]
}

Test{
  number = 17,
  section = "Backslash escapes",
  input = [==[`` \[\` ``
]==],
  result = [==[<p><code>\[\`</code></p>
]==]
}

Test{
  number = 18,
  section = "Backslash escapes",
  input = [==[    \[\]
]==],
  result = [==[<pre><code>\[\]
</code></pre>
]==]
}

Test{
  number = 19,
  section = "Backslash escapes",
  input = [==[~~~
\[\]
~~~
]==],
  result = [==[<pre><code>\[\]
</code></pre>
]==]
}

Test{
  number = 20,
  section = "Backslash escapes",
  input = [==[<http://example.com?find=\*>
]==],
  result = [==[<p><a href="http://example.com?find=%5C*">http://example.com?find=\*</a></p>
]==]
}

Test{
  number = 21,
  section = "Backslash escapes",
  input = [==[<a href="/bar\/)">
]==],
  result = [==[<a href="/bar\/)">
]==]
}

Test{
  number = 22,
  section = "Backslash escapes",
  input = [==[[foo](/bar\* "ti\*tle")
]==],
  result = [==[<p><a href="/bar*" title="ti*tle">foo</a></p>
]==]
}

Test{
  number = 23,
  section = "Backslash escapes",
  input = [==[[foo]

[foo]: /bar\* "ti\*tle"
]==],
  result = [==[<p><a href="/bar*" title="ti*tle">foo</a></p>
]==]
}

Test{
  number = 24,
  section = "Backslash escapes",
  input = [==[``` foo\+bar
foo
```
]==],
  result = [==[<pre><code class="language-foo+bar">foo
</code></pre>
]==]
}

-- Entity and numeric character references
Test{
  number = 25,
  section = "Entity and numeric character references",
  input = [==[&nbsp; &amp; &copy; &AElig; &Dcaron;
&frac34; &HilbertSpace; &DifferentialD;
&ClockwiseContourIntegral; &ngE;
]==],
  result = [==[<p>  &amp; © Æ Ď
¾ ℋ ⅆ
∲ ≧̸</p>
]==]
}

Test{
  number = 26,
  section = "Entity and numeric character references",
  input = [==[&#35; &#1234; &#992; &#0;
]==],
  result = [==[<p># Ӓ Ϡ �</p>
]==]
}

Test{
  number = 27,
  section = "Entity and numeric character references",
  input = [==[&#X22; &#XD06; &#xcab;
]==],
  result = [==[<p>&quot; ആ ಫ</p>
]==]
}

Test{
  number = 28,
  section = "Entity and numeric character references",
  input = [==[&nbsp &x; &#; &#x;
&#87654321;
&#abcdef0;
&ThisIsNotDefined; &hi?;
]==],
  result = [==[<p>&amp;nbsp &amp;x; &amp;#; &amp;#x;
&amp;#87654321;
&amp;#abcdef0;
&amp;ThisIsNotDefined; &amp;hi?;</p>
]==]
}

Test{
  number = 29,
  section = "Entity and numeric character references",
  input = [==[&copy
]==],
  result = [==[<p>&amp;copy</p>
]==]
}

Test{
  number = 30,
  section = "Entity and numeric character references",
  input = [==[&MadeUpEntity;
]==],
  result = [==[<p>&amp;MadeUpEntity;</p>
]==]
}

Test{
  number = 31,
  section = "Entity and numeric character references",
  input = [==[<a href="&ouml;&ouml;.html">
]==],
  result = [==[<a href="&ouml;&ouml;.html">
]==]
}

Test{
  number = 32,
  section = "Entity and numeric character references",
  input = [==[[foo](/f&ouml;&ouml; "f&ouml;&ouml;")
]==],
  result = [==[<p><a href="/f%C3%B6%C3%B6" title="föö">foo</a></p>
]==]
}

Test{
  number = 33,
  section = "Entity and numeric character references",
  input = [==[[foo]

[foo]: /f&ouml;&ouml; "f&ouml;&ouml;"
]==],
  result = [==[<p><a href="/f%C3%B6%C3%B6" title="föö">foo</a></p>
]==]
}

Test{
  number = 34,
  section = "Entity and numeric character references",
  input = [==[``` f&ouml;&ouml;
foo
```
]==],
  result = [==[<pre><code class="language-föö">foo
</code></pre>
]==]
}

Test{
  number = 35,
  section = "Entity and numeric character references",
  input = [==[`f&ouml;&ouml;`
]==],
  result = [==[<p><code>f&amp;ouml;&amp;ouml;</code></p>
]==]
}

Test{
  number = 36,
  section = "Entity and numeric character references",
  input = [==[    f&ouml;f&ouml;
]==],
  result = [==[<pre><code>f&amp;ouml;f&amp;ouml;
</code></pre>
]==]
}

Test{
  number = 37,
  section = "Entity and numeric character references",
  input = [==[&#42;foo&#42;
*foo*
]==],
  result = [==[<p>*foo*
<em>foo</em></p>
]==]
}

Test{
  number = 38,
  section = "Entity and numeric character references",
  input = [==[&#42; foo

* foo
]==],
  result = [==[<p>* foo</p>
<ul>
<li>foo</li>
</ul>
]==]
}

Test{
  number = 39,
  section = "Entity and numeric character references",
  input = [==[foo&#10;&#10;bar
]==],
  result = [==[<p>foo

bar</p>
]==]
}

Test{
  number = 40,
  section = "Entity and numeric character references",
  input = [==[&#9;foo
]==],
  result = [==[<p>	foo</p>
]==]
}

Test{
  number = 41,
  section = "Entity and numeric character references",
  input = [==[[a](url &quot;tit&quot;)
]==],
  result = [==[<p>[a](url &quot;tit&quot;)</p>
]==]
}

-- Blocks and inlines
-- Precedence
Test{
  number = 42,
  section = "Precedence",
  input = [==[- `one
- two`
]==],
  result = [==[<ul>
<li>`one</li>
<li>two`</li>
</ul>
]==]
}

-- Container blocks and leaf blocks
-- Leaf blocks
-- Thematic breaks
Test{
  number = 43,
  section = "Thematic breaks",
  input = [==[***
---
___
]==],
  result = [==[<hr />
<hr />
<hr />
]==]
}

Test{
  number = 44,
  section = "Thematic breaks",
  input = [==[+++
]==],
  result = [==[<p>+++</p>
]==]
}

Test{
  number = 45,
  section = "Thematic breaks",
  input = [==[===
]==],
  result = [==[<p>===</p>
]==]
}

Test{
  number = 46,
  section = "Thematic breaks",
  input = [==[--
**
__
]==],
  result = [==[<p>--
**
__</p>
]==]
}

Test{
  number = 47,
  section = "Thematic breaks",
  input = [==[ ***
  ***
   ***
]==],
  result = [==[<hr />
<hr />
<hr />
]==]
}

Test{
  number = 48,
  section = "Thematic breaks",
  input = [==[    ***
]==],
  result = [==[<pre><code>***
</code></pre>
]==]
}

Test{
  number = 49,
  section = "Thematic breaks",
  input = [==[Foo
    ***
]==],
  result = [==[<p>Foo
***</p>
]==]
}

Test{
  number = 50,
  section = "Thematic breaks",
  input = [==[_____________________________________
]==],
  result = [==[<hr />
]==]
}

Test{
  number = 51,
  section = "Thematic breaks",
  input = [==[ - - -
]==],
  result = [==[<hr />
]==]
}

Test{
  number = 52,
  section = "Thematic breaks",
  input = [==[ **  * ** * ** * **
]==],
  result = [==[<hr />
]==]
}

Test{
  number = 53,
  section = "Thematic breaks",
  input = [==[-     -      -      -
]==],
  result = [==[<hr />
]==]
}

Test{
  number = 54,
  section = "Thematic breaks",
  input = [==[- - - -    
]==],
  result = [==[<hr />
]==]
}

Test{
  number = 55,
  section = "Thematic breaks",
  input = [==[_ _ _ _ a

a------

---a---
]==],
  result = [==[<p>_ _ _ _ a</p>
<p>a------</p>
<p>---a---</p>
]==]
}

Test{
  number = 56,
  section = "Thematic breaks",
  input = [==[ *-*
]==],
  result = [==[<p><em>-</em></p>
]==]
}

Test{
  number = 57,
  section = "Thematic breaks",
  input = [==[- foo
***
- bar
]==],
  result = [==[<ul>
<li>foo</li>
</ul>
<hr />
<ul>
<li>bar</li>
</ul>
]==]
}

Test{
  number = 58,
  section = "Thematic breaks",
  input = [==[Foo
***
bar
]==],
  result = [==[<p>Foo</p>
<hr />
<p>bar</p>
]==]
}

Test{
  number = 59,
  section = "Thematic breaks",
  input = [==[Foo
---
bar
]==],
  result = [==[<h2>Foo</h2>
<p>bar</p>
]==]
}

Test{
  number = 60,
  section = "Thematic breaks",
  input = [==[* Foo
* * *
* Bar
]==],
  result = [==[<ul>
<li>Foo</li>
</ul>
<hr />
<ul>
<li>Bar</li>
</ul>
]==]
}

Test{
  number = 61,
  section = "Thematic breaks",
  input = [==[- Foo
- * * *
]==],
  result = [==[<ul>
<li>Foo</li>
<li>
<hr />
</li>
</ul>
]==]
}

-- ATX headings
Test{
  number = 62,
  section = "ATX headings",
  input = [==[# foo
## foo
### foo
#### foo
##### foo
###### foo
]==],
  result = [==[<h1>foo</h1>
<h2>foo</h2>
<h3>foo</h3>
<h4>foo</h4>
<h5>foo</h5>
<h6>foo</h6>
]==]
}

Test{
  number = 63,
  section = "ATX headings",
  input = [==[####### foo
]==],
  result = [==[<p>####### foo</p>
]==]
}

Test{
  number = 64,
  section = "ATX headings",
  input = [==[#5 bolt

#hashtag
]==],
  result = [==[<p>#5 bolt</p>
<p>#hashtag</p>
]==]
}

Test{
  number = 65,
  section = "ATX headings",
  input = [==[\## foo
]==],
  result = [==[<p>## foo</p>
]==]
}

Test{
  number = 66,
  section = "ATX headings",
  input = [==[# foo *bar* \*baz\*
]==],
  result = [==[<h1>foo <em>bar</em> *baz*</h1>
]==]
}

Test{
  number = 67,
  section = "ATX headings",
  input = [==[#                  foo                     
]==],
  result = [==[<h1>foo</h1>
]==]
}

Test{
  number = 68,
  section = "ATX headings",
  input = [==[ ### foo
  ## foo
   # foo
]==],
  result = [==[<h3>foo</h3>
<h2>foo</h2>
<h1>foo</h1>
]==]
}

Test{
  number = 69,
  section = "ATX headings",
  input = [==[    # foo
]==],
  result = [==[<pre><code># foo
</code></pre>
]==]
}

Test{
  number = 70,
  section = "ATX headings",
  input = [==[foo
    # bar
]==],
  result = [==[<p>foo
# bar</p>
]==]
}

Test{
  number = 71,
  section = "ATX headings",
  input = [==[## foo ##
  ###   bar    ###
]==],
  result = [==[<h2>foo</h2>
<h3>bar</h3>
]==]
}

Test{
  number = 72,
  section = "ATX headings",
  input = [==[# foo ##################################
##### foo ##
]==],
  result = [==[<h1>foo</h1>
<h5>foo</h5>
]==]
}

Test{
  number = 73,
  section = "ATX headings",
  input = [==[### foo ###     
]==],
  result = [==[<h3>foo</h3>
]==]
}

Test{
  number = 74,
  section = "ATX headings",
  input = [==[### foo ### b
]==],
  result = [==[<h3>foo ### b</h3>
]==]
}

Test{
  number = 75,
  section = "ATX headings",
  input = [==[# foo#
]==],
  result = [==[<h1>foo#</h1>
]==]
}

Test{
  number = 76,
  section = "ATX headings",
  input = [==[### foo \###
## foo #\##
# foo \#
]==],
  result = [==[<h3>foo ###</h3>
<h2>foo ###</h2>
<h1>foo #</h1>
]==]
}

Test{
  number = 77,
  section = "ATX headings",
  input = [==[****
## foo
****
]==],
  result = [==[<hr />
<h2>foo</h2>
<hr />
]==]
}

Test{
  number = 78,
  section = "ATX headings",
  input = [==[Foo bar
# baz
Bar foo
]==],
  result = [==[<p>Foo bar</p>
<h1>baz</h1>
<p>Bar foo</p>
]==]
}

Test{
  number = 79,
  section = "ATX headings",
  input = [==[## 
#
### ###
]==],
  result = [==[<h2></h2>
<h1></h1>
<h3></h3>
]==]
}

-- Setext headings
Test{
  number = 80,
  section = "Setext headings",
  input = [==[Foo *bar*
=========

Foo *bar*
---------
]==],
  result = [==[<h1>Foo <em>bar</em></h1>
<h2>Foo <em>bar</em></h2>
]==]
}

Test{
  number = 81,
  section = "Setext headings",
  input = [==[Foo *bar
baz*
====
]==],
  result = [==[<h1>Foo <em>bar
baz</em></h1>
]==]
}

Test{
  number = 82,
  section = "Setext headings",
  input = [==[  Foo *bar
baz*	
====
]==],
  result = [==[<h1>Foo <em>bar
baz</em></h1>
]==]
}

Test{
  number = 83,
  section = "Setext headings",
  input = [==[Foo
-------------------------

Foo
=
]==],
  result = [==[<h2>Foo</h2>
<h1>Foo</h1>
]==]
}

Test{
  number = 84,
  section = "Setext headings",
  input = [==[   Foo
---

  Foo
-----

  Foo
  ===
]==],
  result = [==[<h2>Foo</h2>
<h2>Foo</h2>
<h1>Foo</h1>
]==]
}

Test{
  number = 85,
  section = "Setext headings",
  input = [==[    Foo
    ---

    Foo
---
]==],
  result = [==[<pre><code>Foo
---

Foo
</code></pre>
<hr />
]==]
}

Test{
  number = 86,
  section = "Setext headings",
  input = [==[Foo
   ----      
]==],
  result = [==[<h2>Foo</h2>
]==]
}

Test{
  number = 87,
  section = "Setext headings",
  input = [==[Foo
    ---
]==],
  result = [==[<p>Foo
---</p>
]==]
}

Test{
  number = 88,
  section = "Setext headings",
  input = [==[Foo
= =

Foo
--- -
]==],
  result = [==[<p>Foo
= =</p>
<p>Foo</p>
<hr />
]==]
}

Test{
  number = 89,
  section = "Setext headings",
  input = [==[Foo  
-----
]==],
  result = [==[<h2>Foo</h2>
]==]
}

Test{
  number = 90,
  section = "Setext headings",
  input = [==[Foo\
----
]==],
  result = [==[<h2>Foo\</h2>
]==]
}

Test{
  number = 91,
  section = "Setext headings",
  input = [==[`Foo
----
`

<a title="a lot
---
of dashes"/>
]==],
  result = [==[<h2>`Foo</h2>
<p>`</p>
<h2>&lt;a title=&quot;a lot</h2>
<p>of dashes&quot;/&gt;</p>
]==]
}

Test{
  number = 92,
  section = "Setext headings",
  input = [==[> Foo
---
]==],
  result = [==[<blockquote>
<p>Foo</p>
</blockquote>
<hr />
]==]
}

Test{
  number = 93,
  section = "Setext headings",
  input = [==[> foo
bar
===
]==],
  result = [==[<blockquote>
<p>foo
bar
===</p>
</blockquote>
]==]
}

Test{
  number = 94,
  section = "Setext headings",
  input = [==[- Foo
---
]==],
  result = [==[<ul>
<li>Foo</li>
</ul>
<hr />
]==]
}

Test{
  number = 95,
  section = "Setext headings",
  input = [==[Foo
Bar
---
]==],
  result = [==[<h2>Foo
Bar</h2>
]==]
}

Test{
  number = 96,
  section = "Setext headings",
  input = [==[---
Foo
---
Bar
---
Baz
]==],
  result = [==[<hr />
<h2>Foo</h2>
<h2>Bar</h2>
<p>Baz</p>
]==]
}

Test{
  number = 97,
  section = "Setext headings",
  input = [==[
====
]==],
  result = [==[<p>====</p>
]==]
}

Test{
  number = 98,
  section = "Setext headings",
  input = [==[---
---
]==],
  result = [==[<hr />
<hr />
]==]
}

Test{
  number = 99,
  section = "Setext headings",
  input = [==[- foo
-----
]==],
  result = [==[<ul>
<li>foo</li>
</ul>
<hr />
]==]
}

Test{
  number = 100,
  section = "Setext headings",
  input = [==[    foo
---
]==],
  result = [==[<pre><code>foo
</code></pre>
<hr />
]==]
}

Test{
  number = 101,
  section = "Setext headings",
  input = [==[> foo
-----
]==],
  result = [==[<blockquote>
<p>foo</p>
</blockquote>
<hr />
]==]
}

Test{
  number = 102,
  section = "Setext headings",
  input = [==[\> foo
------
]==],
  result = [==[<h2>&gt; foo</h2>
]==]
}

Test{
  number = 103,
  section = "Setext headings",
  input = [==[Foo

bar
---
baz
]==],
  result = [==[<p>Foo</p>
<h2>bar</h2>
<p>baz</p>
]==]
}

Test{
  number = 104,
  section = "Setext headings",
  input = [==[Foo
bar

---

baz
]==],
  result = [==[<p>Foo
bar</p>
<hr />
<p>baz</p>
]==]
}

Test{
  number = 105,
  section = "Setext headings",
  input = [==[Foo
bar
* * *
baz
]==],
  result = [==[<p>Foo
bar</p>
<hr />
<p>baz</p>
]==]
}

Test{
  number = 106,
  section = "Setext headings",
  input = [==[Foo
bar
\---
baz
]==],
  result = [==[<p>Foo
bar
---
baz</p>
]==]
}

-- Indented code blocks
Test{
  number = 107,
  section = "Indented code blocks",
  input = [==[    a simple
      indented code block
]==],
  result = [==[<pre><code>a simple
  indented code block
</code></pre>
]==]
}

Test{
  number = 108,
  section = "Indented code blocks",
  input = [==[  - foo

    bar
]==],
  result = [==[<ul>
<li>
<p>foo</p>
<p>bar</p>
</li>
</ul>
]==]
}

Test{
  number = 109,
  section = "Indented code blocks",
  input = [==[1.  foo

    - bar
]==],
  result = [==[<ol>
<li>
<p>foo</p>
<ul>
<li>bar</li>
</ul>
</li>
</ol>
]==]
}

Test{
  number = 110,
  section = "Indented code blocks",
  input = [==[    <a/>
    *hi*

    - one
]==],
  result = [==[<pre><code>&lt;a/&gt;
*hi*

- one
</code></pre>
]==]
}

Test{
  number = 111,
  section = "Indented code blocks",
  input = [==[    chunk1

    chunk2
  
 
 
    chunk3
]==],
  result = [==[<pre><code>chunk1

chunk2



chunk3
</code></pre>
]==]
}

Test{
  number = 112,
  section = "Indented code blocks",
  input = [==[    chunk1
      
      chunk2
]==],
  result = [==[<pre><code>chunk1
  
  chunk2
</code></pre>
]==]
}

Test{
  number = 113,
  section = "Indented code blocks",
  input = [==[Foo
    bar

]==],
  result = [==[<p>Foo
bar</p>
]==]
}

Test{
  number = 114,
  section = "Indented code blocks",
  input = [==[    foo
bar
]==],
  result = [==[<pre><code>foo
</code></pre>
<p>bar</p>
]==]
}

Test{
  number = 115,
  section = "Indented code blocks",
  input = [==[# Heading
    foo
Heading
------
    foo
----
]==],
  result = [==[<h1>Heading</h1>
<pre><code>foo
</code></pre>
<h2>Heading</h2>
<pre><code>foo
</code></pre>
<hr />
]==]
}

Test{
  number = 116,
  section = "Indented code blocks",
  input = [==[        foo
    bar
]==],
  result = [==[<pre><code>    foo
bar
</code></pre>
]==]
}

Test{
  number = 117,
  section = "Indented code blocks",
  input = [==[
    
    foo
    

]==],
  result = [==[<pre><code>foo
</code></pre>
]==]
}

Test{
  number = 118,
  section = "Indented code blocks",
  input = [==[    foo  
]==],
  result = [==[<pre><code>foo  
</code></pre>
]==]
}

-- Fenced code blocks
Test{
  number = 119,
  section = "Fenced code blocks",
  input = [==[```
<
 >
```
]==],
  result = [==[<pre><code>&lt;
 &gt;
</code></pre>
]==]
}

Test{
  number = 120,
  section = "Fenced code blocks",
  input = [==[~~~
<
 >
~~~
]==],
  result = [==[<pre><code>&lt;
 &gt;
</code></pre>
]==]
}

Test{
  number = 121,
  section = "Fenced code blocks",
  input = [==[``
foo
``
]==],
  result = [==[<p><code>foo</code></p>
]==]
}

Test{
  number = 122,
  section = "Fenced code blocks",
  input = [==[```
aaa
~~~
```
]==],
  result = [==[<pre><code>aaa
~~~
</code></pre>
]==]
}

Test{
  number = 123,
  section = "Fenced code blocks",
  input = [==[~~~
aaa
```
~~~
]==],
  result = [==[<pre><code>aaa
```
</code></pre>
]==]
}

Test{
  number = 124,
  section = "Fenced code blocks",
  input = [==[````
aaa
```
``````
]==],
  result = [==[<pre><code>aaa
```
</code></pre>
]==]
}

Test{
  number = 125,
  section = "Fenced code blocks",
  input = [==[~~~~
aaa
~~~
~~~~
]==],
  result = [==[<pre><code>aaa
~~~
</code></pre>
]==]
}

Test{
  number = 126,
  section = "Fenced code blocks",
  input = [==[```
]==],
  result = [==[<pre><code></code></pre>
]==]
}

Test{
  number = 127,
  section = "Fenced code blocks",
  input = [==[`````

```
aaa
]==],
  result = [==[<pre><code>
```
aaa
</code></pre>
]==]
}

Test{
  number = 128,
  section = "Fenced code blocks",
  input = [==[> ```
> aaa

bbb
]==],
  result = [==[<blockquote>
<pre><code>aaa
</code></pre>
</blockquote>
<p>bbb</p>
]==]
}

Test{
  number = 129,
  section = "Fenced code blocks",
  input = [==[```

  
```
]==],
  result = [==[<pre><code>
  
</code></pre>
]==]
}

Test{
  number = 130,
  section = "Fenced code blocks",
  input = [==[```
```
]==],
  result = [==[<pre><code></code></pre>
]==]
}

Test{
  number = 131,
  section = "Fenced code blocks",
  input = [==[ ```
 aaa
aaa
```
]==],
  result = [==[<pre><code>aaa
aaa
</code></pre>
]==]
}

Test{
  number = 132,
  section = "Fenced code blocks",
  input = [==[  ```
aaa
  aaa
aaa
  ```
]==],
  result = [==[<pre><code>aaa
aaa
aaa
</code></pre>
]==]
}

Test{
  number = 133,
  section = "Fenced code blocks",
  input = [==[   ```
   aaa
    aaa
  aaa
   ```
]==],
  result = [==[<pre><code>aaa
 aaa
aaa
</code></pre>
]==]
}

Test{
  number = 134,
  section = "Fenced code blocks",
  input = [==[    ```
    aaa
    ```
]==],
  result = [==[<pre><code>```
aaa
```
</code></pre>
]==]
}

Test{
  number = 135,
  section = "Fenced code blocks",
  input = [==[```
aaa
  ```
]==],
  result = [==[<pre><code>aaa
</code></pre>
]==]
}

Test{
  number = 136,
  section = "Fenced code blocks",
  input = [==[   ```
aaa
  ```
]==],
  result = [==[<pre><code>aaa
</code></pre>
]==]
}

Test{
  number = 137,
  section = "Fenced code blocks",
  input = [==[```
aaa
    ```
]==],
  result = [==[<pre><code>aaa
    ```
</code></pre>
]==]
}

Test{
  number = 138,
  section = "Fenced code blocks",
  input = [==[``` ```
aaa
]==],
  result = [==[<p><code> </code>
aaa</p>
]==]
}

Test{
  number = 139,
  section = "Fenced code blocks",
  input = [==[~~~~~~
aaa
~~~ ~~
]==],
  result = [==[<pre><code>aaa
~~~ ~~
</code></pre>
]==]
}

Test{
  number = 140,
  section = "Fenced code blocks",
  input = [==[foo
```
bar
```
baz
]==],
  result = [==[<p>foo</p>
<pre><code>bar
</code></pre>
<p>baz</p>
]==]
}

Test{
  number = 141,
  section = "Fenced code blocks",
  input = [==[foo
---
~~~
bar
~~~
# baz
]==],
  result = [==[<h2>foo</h2>
<pre><code>bar
</code></pre>
<h1>baz</h1>
]==]
}

Test{
  number = 142,
  section = "Fenced code blocks",
  input = [==[```ruby
def foo(x)
  return 3
end
```
]==],
  result = [==[<pre><code class="language-ruby">def foo(x)
  return 3
end
</code></pre>
]==]
}

Test{
  number = 143,
  section = "Fenced code blocks",
  input = [==[~~~~    ruby startline=3 $%@#$
def foo(x)
  return 3
end
~~~~~~~
]==],
  result = [==[<pre><code class="language-ruby">def foo(x)
  return 3
end
</code></pre>
]==]
}

Test{
  number = 144,
  section = "Fenced code blocks",
  input = [==[````;
````
]==],
  result = [==[<pre><code class="language-;"></code></pre>
]==]
}

Test{
  number = 145,
  section = "Fenced code blocks",
  input = [==[``` aa ```
foo
]==],
  result = [==[<p><code>aa</code>
foo</p>
]==]
}

Test{
  number = 146,
  section = "Fenced code blocks",
  input = [==[~~~ aa ``` ~~~
foo
~~~
]==],
  result = [==[<pre><code class="language-aa">foo
</code></pre>
]==]
}

Test{
  number = 147,
  section = "Fenced code blocks",
  input = [==[```
``` aaa
```
]==],
  result = [==[<pre><code>``` aaa
</code></pre>
]==]
}

-- HTML blocks
Test{
  number = 148,
  section = "HTML blocks",
  input = [==[<table><tr><td>
<pre>
**Hello**,

_world_.
</pre>
</td></tr></table>
]==],
  result = [==[<table><tr><td>
<pre>
**Hello**,
<p><em>world</em>.
</pre></p>
</td></tr></table>
]==]
}

Test{
  number = 149,
  section = "HTML blocks",
  input = [==[<table>
  <tr>
    <td>
           hi
    </td>
  </tr>
</table>

okay.
]==],
  result = [==[<table>
  <tr>
    <td>
           hi
    </td>
  </tr>
</table>
<p>okay.</p>
]==]
}

Test{
  number = 150,
  section = "HTML blocks",
  input = [==[ <div>
  *hello*
         <foo><a>
]==],
  result = [==[ <div>
  *hello*
         <foo><a>
]==]
}

Test{
  number = 151,
  section = "HTML blocks",
  input = [==[</div>
*foo*
]==],
  result = [==[</div>
*foo*
]==]
}

Test{
  number = 152,
  section = "HTML blocks",
  input = [==[<DIV CLASS="foo">

*Markdown*

</DIV>
]==],
  result = [==[<DIV CLASS="foo">
<p><em>Markdown</em></p>
</DIV>
]==]
}

Test{
  number = 153,
  section = "HTML blocks",
  input = [==[<div id="foo"
  class="bar">
</div>
]==],
  result = [==[<div id="foo"
  class="bar">
</div>
]==]
}

Test{
  number = 154,
  section = "HTML blocks",
  input = [==[<div id="foo" class="bar
  baz">
</div>
]==],
  result = [==[<div id="foo" class="bar
  baz">
</div>
]==]
}

Test{
  number = 155,
  section = "HTML blocks",
  input = [==[<div>
*foo*

*bar*
]==],
  result = [==[<div>
*foo*
<p><em>bar</em></p>
]==]
}

Test{
  number = 156,
  section = "HTML blocks",
  input = [==[<div id="foo"
*hi*
]==],
  result = [==[<div id="foo"
*hi*
]==]
}

Test{
  number = 157,
  section = "HTML blocks",
  input = [==[<div class
foo
]==],
  result = [==[<div class
foo
]==]
}

Test{
  number = 158,
  section = "HTML blocks",
  input = [==[<div *???-&&&-<---
*foo*
]==],
  result = [==[<div *???-&&&-<---
*foo*
]==]
}

Test{
  number = 159,
  section = "HTML blocks",
  input = [==[<div><a href="bar">*foo*</a></div>
]==],
  result = [==[<div><a href="bar">*foo*</a></div>
]==]
}

Test{
  number = 160,
  section = "HTML blocks",
  input = [==[<table><tr><td>
foo
</td></tr></table>
]==],
  result = [==[<table><tr><td>
foo
</td></tr></table>
]==]
}

Test{
  number = 161,
  section = "HTML blocks",
  input = [==[<div></div>
``` c
int x = 33;
```
]==],
  result = [==[<div></div>
``` c
int x = 33;
```
]==]
}

Test{
  number = 162,
  section = "HTML blocks",
  input = [==[<a href="foo">
*bar*
</a>
]==],
  result = [==[<a href="foo">
*bar*
</a>
]==]
}

Test{
  number = 163,
  section = "HTML blocks",
  input = [==[<Warning>
*bar*
</Warning>
]==],
  result = [==[<Warning>
*bar*
</Warning>
]==]
}

Test{
  number = 164,
  section = "HTML blocks",
  input = [==[<i class="foo">
*bar*
</i>
]==],
  result = [==[<i class="foo">
*bar*
</i>
]==]
}

Test{
  number = 165,
  section = "HTML blocks",
  input = [==[</ins>
*bar*
]==],
  result = [==[</ins>
*bar*
]==]
}

Test{
  number = 166,
  section = "HTML blocks",
  input = [==[<del>
*foo*
</del>
]==],
  result = [==[<del>
*foo*
</del>
]==]
}

Test{
  number = 167,
  section = "HTML blocks",
  input = [==[<del>

*foo*

</del>
]==],
  result = [==[<del>
<p><em>foo</em></p>
</del>
]==]
}

Test{
  number = 168,
  section = "HTML blocks",
  input = [==[<del>*foo*</del>
]==],
  result = [==[<p><del><em>foo</em></del></p>
]==]
}

Test{
  number = 169,
  section = "HTML blocks",
  input = [==[<pre language="haskell"><code>
import Text.HTML.TagSoup

main :: IO ()
main = print $ parseTags tags
</code></pre>
okay
]==],
  result = [==[<pre language="haskell"><code>
import Text.HTML.TagSoup

main :: IO ()
main = print $ parseTags tags
</code></pre>
<p>okay</p>
]==]
}

Test{
  number = 170,
  section = "HTML blocks",
  input = [==[<script type="text/javascript">
// JavaScript example

document.getElementById("demo").innerHTML = "Hello JavaScript!";
</script>
okay
]==],
  result = [==[<script type="text/javascript">
// JavaScript example

document.getElementById("demo").innerHTML = "Hello JavaScript!";
</script>
<p>okay</p>
]==]
}

Test{
  number = 171,
  section = "HTML blocks",
  input = [==[<textarea>

*foo*

_bar_

</textarea>
]==],
  result = [==[<textarea>

*foo*

_bar_

</textarea>
]==]
}

Test{
  number = 172,
  section = "HTML blocks",
  input = [==[<style
  type="text/css">
h1 {color:red;}

p {color:blue;}
</style>
okay
]==],
  result = [==[<style
  type="text/css">
h1 {color:red;}

p {color:blue;}
</style>
<p>okay</p>
]==]
}

Test{
  number = 173,
  section = "HTML blocks",
  input = [==[<style
  type="text/css">

foo
]==],
  result = [==[<style
  type="text/css">

foo
]==]
}

Test{
  number = 174,
  section = "HTML blocks",
  input = [==[> <div>
> foo

bar
]==],
  result = [==[<blockquote>
<div>
foo
</blockquote>
<p>bar</p>
]==]
}

Test{
  number = 175,
  section = "HTML blocks",
  input = [==[- <div>
- foo
]==],
  result = [==[<ul>
<li><div></li>
<li>foo</li>
</ul>
]==]
}

Test{
  number = 176,
  section = "HTML blocks",
  input = [==[<style>p{color:red;}</style>
*foo*
]==],
  result = [==[<style>p{color:red;}</style>
<p><em>foo</em></p>
]==]
}

Test{
  number = 177,
  section = "HTML blocks",
  input = [==[<!-- foo -->*bar*
*baz*
]==],
  result = [==[<!-- foo -->*bar*
<p><em>baz</em></p>
]==]
}

Test{
  number = 178,
  section = "HTML blocks",
  input = [==[<script>
foo
</script>1. *bar*
]==],
  result = [==[<script>
foo
</script>1. *bar*
]==]
}

Test{
  number = 179,
  section = "HTML blocks",
  input = [==[<!-- Foo

bar
   baz -->
okay
]==],
  result = [==[<!-- Foo

bar
   baz -->
<p>okay</p>
]==]
}

Test{
  number = 180,
  section = "HTML blocks",
  input = [==[<?php

  echo '>';

?>
okay
]==],
  result = [==[<?php

  echo '>';

?>
<p>okay</p>
]==]
}

Test{
  number = 181,
  section = "HTML blocks",
  input = [==[<!DOCTYPE html>
]==],
  result = [==[<!DOCTYPE html>
]==]
}

Test{
  number = 182,
  section = "HTML blocks",
  input = [==[<![CDATA[
function matchwo(a,b)
{
  if (a < b && a < 0) then {
    return 1;

  } else {

    return 0;
  }
}
]]>
okay
]==],
  result = [==[<![CDATA[
function matchwo(a,b)
{
  if (a < b && a < 0) then {
    return 1;

  } else {

    return 0;
  }
}
]]>
<p>okay</p>
]==]
}

Test{
  number = 183,
  section = "HTML blocks",
  input = [==[  <!-- foo -->

    <!-- foo -->
]==],
  result = [==[  <!-- foo -->
<pre><code>&lt;!-- foo --&gt;
</code></pre>
]==]
}

Test{
  number = 184,
  section = "HTML blocks",
  input = [==[  <div>

    <div>
]==],
  result = [==[  <div>
<pre><code>&lt;div&gt;
</code></pre>
]==]
}

Test{
  number = 185,
  section = "HTML blocks",
  input = [==[Foo
<div>
bar
</div>
]==],
  result = [==[<p>Foo</p>
<div>
bar
</div>
]==]
}

Test{
  number = 186,
  section = "HTML blocks",
  input = [==[<div>
bar
</div>
*foo*
]==],
  result = [==[<div>
bar
</div>
*foo*
]==]
}

Test{
  number = 187,
  section = "HTML blocks",
  input = [==[Foo
<a href="bar">
baz
]==],
  result = [==[<p>Foo
<a href="bar">
baz</p>
]==]
}

Test{
  number = 188,
  section = "HTML blocks",
  input = [==[<div>

*Emphasized* text.

</div>
]==],
  result = [==[<div>
<p><em>Emphasized</em> text.</p>
</div>
]==]
}

Test{
  number = 189,
  section = "HTML blocks",
  input = [==[<div>
*Emphasized* text.
</div>
]==],
  result = [==[<div>
*Emphasized* text.
</div>
]==]
}

Test{
  number = 190,
  section = "HTML blocks",
  input = [==[<table>

<tr>

<td>
Hi
</td>

</tr>

</table>
]==],
  result = [==[<table>
<tr>
<td>
Hi
</td>
</tr>
</table>
]==]
}

Test{
  number = 191,
  section = "HTML blocks",
  input = [==[<table>

  <tr>

    <td>
      Hi
    </td>

  </tr>

</table>
]==],
  result = [==[<table>
  <tr>
<pre><code>&lt;td&gt;
  Hi
&lt;/td&gt;
</code></pre>
  </tr>
</table>
]==]
}

-- Link reference definitions
Test{
  number = 192,
  section = "Link reference definitions",
  input = [==[[foo]: /url "title"

[foo]
]==],
  result = [==[<p><a href="/url" title="title">foo</a></p>
]==]
}

Test{
  number = 193,
  section = "Link reference definitions",
  input = [==[   [foo]: 
      /url  
           'the title'  

[foo]
]==],
  result = [==[<p><a href="/url" title="the title">foo</a></p>
]==]
}

Test{
  number = 194,
  section = "Link reference definitions",
  input = [==[[Foo*bar\]]:my_(url) 'title (with parens)'

[Foo*bar\]]
]==],
  result = [==[<p><a href="my_(url)" title="title (with parens)">Foo*bar]</a></p>
]==]
}

Test{
  number = 195,
  section = "Link reference definitions",
  input = [==[[Foo bar]:
<my url>
'title'

[Foo bar]
]==],
  result = [==[<p><a href="my%20url" title="title">Foo bar</a></p>
]==]
}

Test{
  number = 196,
  section = "Link reference definitions",
  input = [==[[foo]: /url '
title
line1
line2
'

[foo]
]==],
  result = [==[<p><a href="/url" title="
title
line1
line2
">foo</a></p>
]==]
}

Test{
  number = 197,
  section = "Link reference definitions",
  input = [==[[foo]: /url 'title

with blank line'

[foo]
]==],
  result = [==[<p>[foo]: /url 'title</p>
<p>with blank line'</p>
<p>[foo]</p>
]==]
}

Test{
  number = 198,
  section = "Link reference definitions",
  input = [==[[foo]:
/url

[foo]
]==],
  result = [==[<p><a href="/url">foo</a></p>
]==]
}

Test{
  number = 199,
  section = "Link reference definitions",
  input = [==[[foo]:

[foo]
]==],
  result = [==[<p>[foo]:</p>
<p>[foo]</p>
]==]
}

Test{
  number = 200,
  section = "Link reference definitions",
  input = [==[[foo]: <>

[foo]
]==],
  result = [==[<p><a href="">foo</a></p>
]==]
}

Test{
  number = 201,
  section = "Link reference definitions",
  input = [==[[foo]: <bar>(baz)

[foo]
]==],
  result = [==[<p>[foo]: <bar>(baz)</p>
<p>[foo]</p>
]==]
}

Test{
  number = 202,
  section = "Link reference definitions",
  input = [==[[foo]: /url\bar\*baz "foo\"bar\baz"

[foo]
]==],
  result = [==[<p><a href="/url%5Cbar*baz" title="foo&quot;bar\baz">foo</a></p>
]==]
}

Test{
  number = 203,
  section = "Link reference definitions",
  input = [==[[foo]

[foo]: url
]==],
  result = [==[<p><a href="url">foo</a></p>
]==]
}

Test{
  number = 204,
  section = "Link reference definitions",
  input = [==[[foo]

[foo]: first
[foo]: second
]==],
  result = [==[<p><a href="first">foo</a></p>
]==]
}

Test{
  number = 205,
  section = "Link reference definitions",
  input = [==[[FOO]: /url

[Foo]
]==],
  result = [==[<p><a href="/url">Foo</a></p>
]==]
}

Test{
  number = 206,
  section = "Link reference definitions",
  input = [==[[ΑΓΩ]: /φου

[αγω]
]==],
  result = [==[<p><a href="/%CF%86%CE%BF%CF%85">αγω</a></p>
]==]
}

Test{
  number = 207,
  section = "Link reference definitions",
  input = [==[[foo]: /url
]==],
  result = [==[]==]
}

Test{
  number = 208,
  section = "Link reference definitions",
  input = [==[[
foo
]: /url
bar
]==],
  result = [==[<p>bar</p>
]==]
}

Test{
  number = 209,
  section = "Link reference definitions",
  input = [==[[foo]: /url "title" ok
]==],
  result = [==[<p>[foo]: /url &quot;title&quot; ok</p>
]==]
}

Test{
  number = 210,
  section = "Link reference definitions",
  input = [==[[foo]: /url
"title" ok
]==],
  result = [==[<p>&quot;title&quot; ok</p>
]==]
}

Test{
  number = 211,
  section = "Link reference definitions",
  input = [==[    [foo]: /url "title"

[foo]
]==],
  result = [==[<pre><code>[foo]: /url &quot;title&quot;
</code></pre>
<p>[foo]</p>
]==]
}

Test{
  number = 212,
  section = "Link reference definitions",
  input = [==[```
[foo]: /url
```

[foo]
]==],
  result = [==[<pre><code>[foo]: /url
</code></pre>
<p>[foo]</p>
]==]
}

Test{
  number = 213,
  section = "Link reference definitions",
  input = [==[Foo
[bar]: /baz

[bar]
]==],
  result = [==[<p>Foo
[bar]: /baz</p>
<p>[bar]</p>
]==]
}

Test{
  number = 214,
  section = "Link reference definitions",
  input = [==[# [Foo]
[foo]: /url
> bar
]==],
  result = [==[<h1><a href="/url">Foo</a></h1>
<blockquote>
<p>bar</p>
</blockquote>
]==]
}

Test{
  number = 215,
  section = "Link reference definitions",
  input = [==[[foo]: /url
bar
===
[foo]
]==],
  result = [==[<h1>bar</h1>
<p><a href="/url">foo</a></p>
]==]
}

Test{
  number = 216,
  section = "Link reference definitions",
  input = [==[[foo]: /url
===
[foo]
]==],
  result = [==[<p>===
<a href="/url">foo</a></p>
]==]
}

Test{
  number = 217,
  section = "Link reference definitions",
  input = [==[[foo]: /foo-url "foo"
[bar]: /bar-url
  "bar"
[baz]: /baz-url

[foo],
[bar],
[baz]
]==],
  result = [==[<p><a href="/foo-url" title="foo">foo</a>,
<a href="/bar-url" title="bar">bar</a>,
<a href="/baz-url">baz</a></p>
]==]
}

Test{
  number = 218,
  section = "Link reference definitions",
  input = [==[[foo]

> [foo]: /url
]==],
  result = [==[<p><a href="/url">foo</a></p>
<blockquote>
</blockquote>
]==]
}

-- Paragraphs
Test{
  number = 219,
  section = "Paragraphs",
  input = [==[aaa

bbb
]==],
  result = [==[<p>aaa</p>
<p>bbb</p>
]==]
}

Test{
  number = 220,
  section = "Paragraphs",
  input = [==[aaa
bbb

ccc
ddd
]==],
  result = [==[<p>aaa
bbb</p>
<p>ccc
ddd</p>
]==]
}

Test{
  number = 221,
  section = "Paragraphs",
  input = [==[aaa


bbb
]==],
  result = [==[<p>aaa</p>
<p>bbb</p>
]==]
}

Test{
  number = 222,
  section = "Paragraphs",
  input = [==[  aaa
 bbb
]==],
  result = [==[<p>aaa
bbb</p>
]==]
}

Test{
  number = 223,
  section = "Paragraphs",
  input = [==[aaa
             bbb
                                       ccc
]==],
  result = [==[<p>aaa
bbb
ccc</p>
]==]
}

Test{
  number = 224,
  section = "Paragraphs",
  input = [==[   aaa
bbb
]==],
  result = [==[<p>aaa
bbb</p>
]==]
}

Test{
  number = 225,
  section = "Paragraphs",
  input = [==[    aaa
bbb
]==],
  result = [==[<pre><code>aaa
</code></pre>
<p>bbb</p>
]==]
}

Test{
  number = 226,
  section = "Paragraphs",
  input = [==[aaa     
bbb     
]==],
  result = [==[<p>aaa<br />
bbb</p>
]==]
}

-- Blank lines
Test{
  number = 227,
  section = "Blank lines",
  input = [==[  

aaa
  

# aaa

  
]==],
  result = [==[<p>aaa</p>
<h1>aaa</h1>
]==]
}

-- Container blocks
-- Block quotes
Test{
  number = 228,
  section = "Block quotes",
  input = [==[> # Foo
> bar
> baz
]==],
  result = [==[<blockquote>
<h1>Foo</h1>
<p>bar
baz</p>
</blockquote>
]==]
}

Test{
  number = 229,
  section = "Block quotes",
  input = [==[># Foo
>bar
> baz
]==],
  result = [==[<blockquote>
<h1>Foo</h1>
<p>bar
baz</p>
</blockquote>
]==]
}

Test{
  number = 230,
  section = "Block quotes",
  input = [==[   > # Foo
   > bar
 > baz
]==],
  result = [==[<blockquote>
<h1>Foo</h1>
<p>bar
baz</p>
</blockquote>
]==]
}

Test{
  number = 231,
  section = "Block quotes",
  input = [==[    > # Foo
    > bar
    > baz
]==],
  result = [==[<pre><code>&gt; # Foo
&gt; bar
&gt; baz
</code></pre>
]==]
}

Test{
  number = 232,
  section = "Block quotes",
  input = [==[> # Foo
> bar
baz
]==],
  result = [==[<blockquote>
<h1>Foo</h1>
<p>bar
baz</p>
</blockquote>
]==]
}

Test{
  number = 233,
  section = "Block quotes",
  input = [==[> bar
baz
> foo
]==],
  result = [==[<blockquote>
<p>bar
baz
foo</p>
</blockquote>
]==]
}

Test{
  number = 234,
  section = "Block quotes",
  input = [==[> foo
---
]==],
  result = [==[<blockquote>
<p>foo</p>
</blockquote>
<hr />
]==]
}

Test{
  number = 235,
  section = "Block quotes",
  input = [==[> - foo
- bar
]==],
  result = [==[<blockquote>
<ul>
<li>foo</li>
</ul>
</blockquote>
<ul>
<li>bar</li>
</ul>
]==]
}

Test{
  number = 236,
  section = "Block quotes",
  input = [==[>     foo
    bar
]==],
  result = [==[<blockquote>
<pre><code>foo
</code></pre>
</blockquote>
<pre><code>bar
</code></pre>
]==]
}

Test{
  number = 237,
  section = "Block quotes",
  input = [==[> ```
foo
```
]==],
  result = [==[<blockquote>
<pre><code></code></pre>
</blockquote>
<p>foo</p>
<pre><code></code></pre>
]==]
}

Test{
  number = 238,
  section = "Block quotes",
  input = [==[> foo
    - bar
]==],
  result = [==[<blockquote>
<p>foo
- bar</p>
</blockquote>
]==]
}

Test{
  number = 239,
  section = "Block quotes",
  input = [==[>
]==],
  result = [==[<blockquote>
</blockquote>
]==]
}

Test{
  number = 240,
  section = "Block quotes",
  input = [==[>
>  
> 
]==],
  result = [==[<blockquote>
</blockquote>
]==]
}

Test{
  number = 241,
  section = "Block quotes",
  input = [==[>
> foo
>  
]==],
  result = [==[<blockquote>
<p>foo</p>
</blockquote>
]==]
}

Test{
  number = 242,
  section = "Block quotes",
  input = [==[> foo

> bar
]==],
  result = [==[<blockquote>
<p>foo</p>
</blockquote>
<blockquote>
<p>bar</p>
</blockquote>
]==]
}

Test{
  number = 243,
  section = "Block quotes",
  input = [==[> foo
> bar
]==],
  result = [==[<blockquote>
<p>foo
bar</p>
</blockquote>
]==]
}

Test{
  number = 244,
  section = "Block quotes",
  input = [==[> foo
>
> bar
]==],
  result = [==[<blockquote>
<p>foo</p>
<p>bar</p>
</blockquote>
]==]
}

Test{
  number = 245,
  section = "Block quotes",
  input = [==[foo
> bar
]==],
  result = [==[<p>foo</p>
<blockquote>
<p>bar</p>
</blockquote>
]==]
}

Test{
  number = 246,
  section = "Block quotes",
  input = [==[> aaa
***
> bbb
]==],
  result = [==[<blockquote>
<p>aaa</p>
</blockquote>
<hr />
<blockquote>
<p>bbb</p>
</blockquote>
]==]
}

Test{
  number = 247,
  section = "Block quotes",
  input = [==[> bar
baz
]==],
  result = [==[<blockquote>
<p>bar
baz</p>
</blockquote>
]==]
}

Test{
  number = 248,
  section = "Block quotes",
  input = [==[> bar

baz
]==],
  result = [==[<blockquote>
<p>bar</p>
</blockquote>
<p>baz</p>
]==]
}

Test{
  number = 249,
  section = "Block quotes",
  input = [==[> bar
>
baz
]==],
  result = [==[<blockquote>
<p>bar</p>
</blockquote>
<p>baz</p>
]==]
}

Test{
  number = 250,
  section = "Block quotes",
  input = [==[> > > foo
bar
]==],
  result = [==[<blockquote>
<blockquote>
<blockquote>
<p>foo
bar</p>
</blockquote>
</blockquote>
</blockquote>
]==]
}

Test{
  number = 251,
  section = "Block quotes",
  input = [==[>>> foo
> bar
>>baz
]==],
  result = [==[<blockquote>
<blockquote>
<blockquote>
<p>foo
bar
baz</p>
</blockquote>
</blockquote>
</blockquote>
]==]
}

Test{
  number = 252,
  section = "Block quotes",
  input = [==[>     code

>    not code
]==],
  result = [==[<blockquote>
<pre><code>code
</code></pre>
</blockquote>
<blockquote>
<p>not code</p>
</blockquote>
]==]
}

-- List items
Test{
  number = 253,
  section = "List items",
  input = [==[A paragraph
with two lines.

    indented code

> A block quote.
]==],
  result = [==[<p>A paragraph
with two lines.</p>
<pre><code>indented code
</code></pre>
<blockquote>
<p>A block quote.</p>
</blockquote>
]==]
}

Test{
  number = 254,
  section = "List items",
  input = [==[1.  A paragraph
    with two lines.

        indented code

    > A block quote.
]==],
  result = [==[<ol>
<li>
<p>A paragraph
with two lines.</p>
<pre><code>indented code
</code></pre>
<blockquote>
<p>A block quote.</p>
</blockquote>
</li>
</ol>
]==]
}

Test{
  number = 255,
  section = "List items",
  input = [==[- one

 two
]==],
  result = [==[<ul>
<li>one</li>
</ul>
<p>two</p>
]==]
}

Test{
  number = 256,
  section = "List items",
  input = [==[- one

  two
]==],
  result = [==[<ul>
<li>
<p>one</p>
<p>two</p>
</li>
</ul>
]==]
}

Test{
  number = 257,
  section = "List items",
  input = [==[ -    one

     two
]==],
  result = [==[<ul>
<li>one</li>
</ul>
<pre><code> two
</code></pre>
]==]
}

Test{
  number = 258,
  section = "List items",
  input = [==[ -    one

      two
]==],
  result = [==[<ul>
<li>
<p>one</p>
<p>two</p>
</li>
</ul>
]==]
}

Test{
  number = 259,
  section = "List items",
  input = [==[   > > 1.  one
>>
>>     two
]==],
  result = [==[<blockquote>
<blockquote>
<ol>
<li>
<p>one</p>
<p>two</p>
</li>
</ol>
</blockquote>
</blockquote>
]==]
}

Test{
  number = 260,
  section = "List items",
  input = [==[>>- one
>>
  >  > two
]==],
  result = [==[<blockquote>
<blockquote>
<ul>
<li>one</li>
</ul>
<p>two</p>
</blockquote>
</blockquote>
]==]
}

Test{
  number = 261,
  section = "List items",
  input = [==[-one

2.two
]==],
  result = [==[<p>-one</p>
<p>2.two</p>
]==]
}

Test{
  number = 262,
  section = "List items",
  input = [==[- foo


  bar
]==],
  result = [==[<ul>
<li>
<p>foo</p>
<p>bar</p>
</li>
</ul>
]==]
}

Test{
  number = 263,
  section = "List items",
  input = [==[1.  foo

    ```
    bar
    ```

    baz

    > bam
]==],
  result = [==[<ol>
<li>
<p>foo</p>
<pre><code>bar
</code></pre>
<p>baz</p>
<blockquote>
<p>bam</p>
</blockquote>
</li>
</ol>
]==]
}

Test{
  number = 264,
  section = "List items",
  input = [==[- Foo

      bar


      baz
]==],
  result = [==[<ul>
<li>
<p>Foo</p>
<pre><code>bar


baz
</code></pre>
</li>
</ul>
]==]
}

Test{
  number = 265,
  section = "List items",
  input = [==[123456789. ok
]==],
  result = [==[<ol start="123456789">
<li>ok</li>
</ol>
]==]
}

Test{
  number = 266,
  section = "List items",
  input = [==[1234567890. not ok
]==],
  result = [==[<p>1234567890. not ok</p>
]==]
}

Test{
  number = 267,
  section = "List items",
  input = [==[0. ok
]==],
  result = [==[<ol start="0">
<li>ok</li>
</ol>
]==]
}

Test{
  number = 268,
  section = "List items",
  input = [==[003. ok
]==],
  result = [==[<ol start="3">
<li>ok</li>
</ol>
]==]
}

Test{
  number = 269,
  section = "List items",
  input = [==[-1. not ok
]==],
  result = [==[<p>-1. not ok</p>
]==]
}

Test{
  number = 270,
  section = "List items",
  input = [==[- foo

      bar
]==],
  result = [==[<ul>
<li>
<p>foo</p>
<pre><code>bar
</code></pre>
</li>
</ul>
]==]
}

Test{
  number = 271,
  section = "List items",
  input = [==[  10.  foo

           bar
]==],
  result = [==[<ol start="10">
<li>
<p>foo</p>
<pre><code>bar
</code></pre>
</li>
</ol>
]==]
}

Test{
  number = 272,
  section = "List items",
  input = [==[    indented code

paragraph

    more code
]==],
  result = [==[<pre><code>indented code
</code></pre>
<p>paragraph</p>
<pre><code>more code
</code></pre>
]==]
}

Test{
  number = 273,
  section = "List items",
  input = [==[1.     indented code

   paragraph

       more code
]==],
  result = [==[<ol>
<li>
<pre><code>indented code
</code></pre>
<p>paragraph</p>
<pre><code>more code
</code></pre>
</li>
</ol>
]==]
}

Test{
  number = 274,
  section = "List items",
  input = [==[1.      indented code

   paragraph

       more code
]==],
  result = [==[<ol>
<li>
<pre><code> indented code
</code></pre>
<p>paragraph</p>
<pre><code>more code
</code></pre>
</li>
</ol>
]==]
}

Test{
  number = 275,
  section = "List items",
  input = [==[   foo

bar
]==],
  result = [==[<p>foo</p>
<p>bar</p>
]==]
}

Test{
  number = 276,
  section = "List items",
  input = [==[-    foo

  bar
]==],
  result = [==[<ul>
<li>foo</li>
</ul>
<p>bar</p>
]==]
}

Test{
  number = 277,
  section = "List items",
  input = [==[-  foo

   bar
]==],
  result = [==[<ul>
<li>
<p>foo</p>
<p>bar</p>
</li>
</ul>
]==]
}

Test{
  number = 278,
  section = "List items",
  input = [==[-
  foo
-
  ```
  bar
  ```
-
      baz
]==],
  result = [==[<ul>
<li>foo</li>
<li>
<pre><code>bar
</code></pre>
</li>
<li>
<pre><code>baz
</code></pre>
</li>
</ul>
]==]
}

Test{
  number = 279,
  section = "List items",
  input = [==[-   
  foo
]==],
  result = [==[<ul>
<li>foo</li>
</ul>
]==]
}

Test{
  number = 280,
  section = "List items",
  input = [==[-

  foo
]==],
  result = [==[<ul>
<li></li>
</ul>
<p>foo</p>
]==]
}

Test{
  number = 281,
  section = "List items",
  input = [==[- foo
-
- bar
]==],
  result = [==[<ul>
<li>foo</li>
<li></li>
<li>bar</li>
</ul>
]==]
}

Test{
  number = 282,
  section = "List items",
  input = [==[- foo
-   
- bar
]==],
  result = [==[<ul>
<li>foo</li>
<li></li>
<li>bar</li>
</ul>
]==]
}

Test{
  number = 283,
  section = "List items",
  input = [==[1. foo
2.
3. bar
]==],
  result = [==[<ol>
<li>foo</li>
<li></li>
<li>bar</li>
</ol>
]==]
}

Test{
  number = 284,
  section = "List items",
  input = [==[*
]==],
  result = [==[<ul>
<li></li>
</ul>
]==]
}

Test{
  number = 285,
  section = "List items",
  input = [==[foo
*

foo
1.
]==],
  result = [==[<p>foo
*</p>
<p>foo
1.</p>
]==]
}

Test{
  number = 286,
  section = "List items",
  input = [==[ 1.  A paragraph
     with two lines.

         indented code

     > A block quote.
]==],
  result = [==[<ol>
<li>
<p>A paragraph
with two lines.</p>
<pre><code>indented code
</code></pre>
<blockquote>
<p>A block quote.</p>
</blockquote>
</li>
</ol>
]==]
}

Test{
  number = 287,
  section = "List items",
  input = [==[  1.  A paragraph
      with two lines.

          indented code

      > A block quote.
]==],
  result = [==[<ol>
<li>
<p>A paragraph
with two lines.</p>
<pre><code>indented code
</code></pre>
<blockquote>
<p>A block quote.</p>
</blockquote>
</li>
</ol>
]==]
}

Test{
  number = 288,
  section = "List items",
  input = [==[   1.  A paragraph
       with two lines.

           indented code

       > A block quote.
]==],
  result = [==[<ol>
<li>
<p>A paragraph
with two lines.</p>
<pre><code>indented code
</code></pre>
<blockquote>
<p>A block quote.</p>
</blockquote>
</li>
</ol>
]==]
}

Test{
  number = 289,
  section = "List items",
  input = [==[    1.  A paragraph
        with two lines.

            indented code

        > A block quote.
]==],
  result = [==[<pre><code>1.  A paragraph
    with two lines.

        indented code

    &gt; A block quote.
</code></pre>
]==]
}

Test{
  number = 290,
  section = "List items",
  input = [==[  1.  A paragraph
with two lines.

          indented code

      > A block quote.
]==],
  result = [==[<ol>
<li>
<p>A paragraph
with two lines.</p>
<pre><code>indented code
</code></pre>
<blockquote>
<p>A block quote.</p>
</blockquote>
</li>
</ol>
]==]
}

Test{
  number = 291,
  section = "List items",
  input = [==[  1.  A paragraph
    with two lines.
]==],
  result = [==[<ol>
<li>A paragraph
with two lines.</li>
</ol>
]==]
}

Test{
  number = 292,
  section = "List items",
  input = [==[> 1. > Blockquote
continued here.
]==],
  result = [==[<blockquote>
<ol>
<li>
<blockquote>
<p>Blockquote
continued here.</p>
</blockquote>
</li>
</ol>
</blockquote>
]==]
}

Test{
  number = 293,
  section = "List items",
  input = [==[> 1. > Blockquote
> continued here.
]==],
  result = [==[<blockquote>
<ol>
<li>
<blockquote>
<p>Blockquote
continued here.</p>
</blockquote>
</li>
</ol>
</blockquote>
]==]
}

Test{
  number = 294,
  section = "List items",
  input = [==[- foo
  - bar
    - baz
      - boo
]==],
  result = [==[<ul>
<li>foo
<ul>
<li>bar
<ul>
<li>baz
<ul>
<li>boo</li>
</ul>
</li>
</ul>
</li>
</ul>
</li>
</ul>
]==]
}

Test{
  number = 295,
  section = "List items",
  input = [==[- foo
 - bar
  - baz
   - boo
]==],
  result = [==[<ul>
<li>foo</li>
<li>bar</li>
<li>baz</li>
<li>boo</li>
</ul>
]==]
}

Test{
  number = 296,
  section = "List items",
  input = [==[10) foo
    - bar
]==],
  result = [==[<ol start="10">
<li>foo
<ul>
<li>bar</li>
</ul>
</li>
</ol>
]==]
}

Test{
  number = 297,
  section = "List items",
  input = [==[10) foo
   - bar
]==],
  result = [==[<ol start="10">
<li>foo</li>
</ol>
<ul>
<li>bar</li>
</ul>
]==]
}

Test{
  number = 298,
  section = "List items",
  input = [==[- - foo
]==],
  result = [==[<ul>
<li>
<ul>
<li>foo</li>
</ul>
</li>
</ul>
]==]
}

Test{
  number = 299,
  section = "List items",
  input = [==[1. - 2. foo
]==],
  result = [==[<ol>
<li>
<ul>
<li>
<ol start="2">
<li>foo</li>
</ol>
</li>
</ul>
</li>
</ol>
]==]
}

Test{
  number = 300,
  section = "List items",
  input = [==[- # Foo
- Bar
  ---
  baz
]==],
  result = [==[<ul>
<li>
<h1>Foo</h1>
</li>
<li>
<h2>Bar</h2>
baz</li>
</ul>
]==]
}

-- Motivation
-- Lists
Test{
  number = 301,
  section = "Lists",
  input = [==[- foo
- bar
+ baz
]==],
  result = [==[<ul>
<li>foo</li>
<li>bar</li>
</ul>
<ul>
<li>baz</li>
</ul>
]==]
}

Test{
  number = 302,
  section = "Lists",
  input = [==[1. foo
2. bar
3) baz
]==],
  result = [==[<ol>
<li>foo</li>
<li>bar</li>
</ol>
<ol start="3">
<li>baz</li>
</ol>
]==]
}

Test{
  number = 303,
  section = "Lists",
  input = [==[Foo
- bar
- baz
]==],
  result = [==[<p>Foo</p>
<ul>
<li>bar</li>
<li>baz</li>
</ul>
]==]
}

Test{
  number = 304,
  section = "Lists",
  input = [==[The number of windows in my house is
14.  The number of doors is 6.
]==],
  result = [==[<p>The number of windows in my house is
14.  The number of doors is 6.</p>
]==]
}

Test{
  number = 305,
  section = "Lists",
  input = [==[The number of windows in my house is
1.  The number of doors is 6.
]==],
  result = [==[<p>The number of windows in my house is</p>
<ol>
<li>The number of doors is 6.</li>
</ol>
]==]
}

Test{
  number = 306,
  section = "Lists",
  input = [==[- foo

- bar


- baz
]==],
  result = [==[<ul>
<li>
<p>foo</p>
</li>
<li>
<p>bar</p>
</li>
<li>
<p>baz</p>
</li>
</ul>
]==]
}

Test{
  number = 307,
  section = "Lists",
  input = [==[- foo
  - bar
    - baz


      bim
]==],
  result = [==[<ul>
<li>foo
<ul>
<li>bar
<ul>
<li>
<p>baz</p>
<p>bim</p>
</li>
</ul>
</li>
</ul>
</li>
</ul>
]==]
}

Test{
  number = 308,
  section = "Lists",
  input = [==[- foo
- bar

<!-- -->

- baz
- bim
]==],
  result = [==[<ul>
<li>foo</li>
<li>bar</li>
</ul>
<!-- -->
<ul>
<li>baz</li>
<li>bim</li>
</ul>
]==]
}

Test{
  number = 309,
  section = "Lists",
  input = [==[-   foo

    notcode

-   foo

<!-- -->

    code
]==],
  result = [==[<ul>
<li>
<p>foo</p>
<p>notcode</p>
</li>
<li>
<p>foo</p>
</li>
</ul>
<!-- -->
<pre><code>code
</code></pre>
]==]
}

Test{
  number = 310,
  section = "Lists",
  input = [==[- a
 - b
  - c
   - d
  - e
 - f
- g
]==],
  result = [==[<ul>
<li>a</li>
<li>b</li>
<li>c</li>
<li>d</li>
<li>e</li>
<li>f</li>
<li>g</li>
</ul>
]==]
}

Test{
  number = 311,
  section = "Lists",
  input = [==[1. a

  2. b

   3. c
]==],
  result = [==[<ol>
<li>
<p>a</p>
</li>
<li>
<p>b</p>
</li>
<li>
<p>c</p>
</li>
</ol>
]==]
}

Test{
  number = 312,
  section = "Lists",
  input = [==[- a
 - b
  - c
   - d
    - e
]==],
  result = [==[<ul>
<li>a</li>
<li>b</li>
<li>c</li>
<li>d
- e</li>
</ul>
]==]
}

Test{
  number = 313,
  section = "Lists",
  input = [==[1. a

  2. b

    3. c
]==],
  result = [==[<ol>
<li>
<p>a</p>
</li>
<li>
<p>b</p>
</li>
</ol>
<pre><code>3. c
</code></pre>
]==]
}

Test{
  number = 314,
  section = "Lists",
  input = [==[- a
- b

- c
]==],
  result = [==[<ul>
<li>
<p>a</p>
</li>
<li>
<p>b</p>
</li>
<li>
<p>c</p>
</li>
</ul>
]==]
}

Test{
  number = 315,
  section = "Lists",
  input = [==[* a
*

* c
]==],
  result = [==[<ul>
<li>
<p>a</p>
</li>
<li></li>
<li>
<p>c</p>
</li>
</ul>
]==]
}

Test{
  number = 316,
  section = "Lists",
  input = [==[- a
- b

  c
- d
]==],
  result = [==[<ul>
<li>
<p>a</p>
</li>
<li>
<p>b</p>
<p>c</p>
</li>
<li>
<p>d</p>
</li>
</ul>
]==]
}

Test{
  number = 317,
  section = "Lists",
  input = [==[- a
- b

  [ref]: /url
- d
]==],
  result = [==[<ul>
<li>
<p>a</p>
</li>
<li>
<p>b</p>
</li>
<li>
<p>d</p>
</li>
</ul>
]==]
}

Test{
  number = 318,
  section = "Lists",
  input = [==[- a
- ```
  b


  ```
- c
]==],
  result = [==[<ul>
<li>a</li>
<li>
<pre><code>b


</code></pre>
</li>
<li>c</li>
</ul>
]==]
}

Test{
  number = 319,
  section = "Lists",
  input = [==[- a
  - b

    c
- d
]==],
  result = [==[<ul>
<li>a
<ul>
<li>
<p>b</p>
<p>c</p>
</li>
</ul>
</li>
<li>d</li>
</ul>
]==]
}

Test{
  number = 320,
  section = "Lists",
  input = [==[* a
  > b
  >
* c
]==],
  result = [==[<ul>
<li>a
<blockquote>
<p>b</p>
</blockquote>
</li>
<li>c</li>
</ul>
]==]
}

Test{
  number = 321,
  section = "Lists",
  input = [==[- a
  > b
  ```
  c
  ```
- d
]==],
  result = [==[<ul>
<li>a
<blockquote>
<p>b</p>
</blockquote>
<pre><code>c
</code></pre>
</li>
<li>d</li>
</ul>
]==]
}

Test{
  number = 322,
  section = "Lists",
  input = [==[- a
]==],
  result = [==[<ul>
<li>a</li>
</ul>
]==]
}

Test{
  number = 323,
  section = "Lists",
  input = [==[- a
  - b
]==],
  result = [==[<ul>
<li>a
<ul>
<li>b</li>
</ul>
</li>
</ul>
]==]
}

Test{
  number = 324,
  section = "Lists",
  input = [==[1. ```
   foo
   ```

   bar
]==],
  result = [==[<ol>
<li>
<pre><code>foo
</code></pre>
<p>bar</p>
</li>
</ol>
]==]
}

Test{
  number = 325,
  section = "Lists",
  input = [==[* foo
  * bar

  baz
]==],
  result = [==[<ul>
<li>
<p>foo</p>
<ul>
<li>bar</li>
</ul>
<p>baz</p>
</li>
</ul>
]==]
}

Test{
  number = 326,
  section = "Lists",
  input = [==[- a
  - b
  - c

- d
  - e
  - f
]==],
  result = [==[<ul>
<li>
<p>a</p>
<ul>
<li>b</li>
<li>c</li>
</ul>
</li>
<li>
<p>d</p>
<ul>
<li>e</li>
<li>f</li>
</ul>
</li>
</ul>
]==]
}

-- Inlines
Test{
  number = 327,
  section = "Inlines",
  input = [==[`hi`lo`
]==],
  result = [==[<p><code>hi</code>lo`</p>
]==]
}

-- Code spans
Test{
  number = 328,
  section = "Code spans",
  input = [==[`foo`
]==],
  result = [==[<p><code>foo</code></p>
]==]
}

Test{
  number = 329,
  section = "Code spans",
  input = [==[`` foo ` bar ``
]==],
  result = [==[<p><code>foo ` bar</code></p>
]==]
}

Test{
  number = 330,
  section = "Code spans",
  input = [==[` `` `
]==],
  result = [==[<p><code>``</code></p>
]==]
}

Test{
  number = 331,
  section = "Code spans",
  input = [==[`  ``  `
]==],
  result = [==[<p><code> `` </code></p>
]==]
}

Test{
  number = 332,
  section = "Code spans",
  input = [==[` a`
]==],
  result = [==[<p><code> a</code></p>
]==]
}

Test{
  number = 333,
  section = "Code spans",
  input = [==[` b `
]==],
  result = [==[<p><code> b </code></p>
]==]
}

Test{
  number = 334,
  section = "Code spans",
  input = [==[` `
`  `
]==],
  result = [==[<p><code> </code>
<code>  </code></p>
]==]
}

Test{
  number = 335,
  section = "Code spans",
  input = [==[``
foo
bar  
baz
``
]==],
  result = [==[<p><code>foo bar   baz</code></p>
]==]
}

Test{
  number = 336,
  section = "Code spans",
  input = [==[``
foo 
``
]==],
  result = [==[<p><code>foo </code></p>
]==]
}

Test{
  number = 337,
  section = "Code spans",
  input = [==[`foo   bar 
baz`
]==],
  result = [==[<p><code>foo   bar  baz</code></p>
]==]
}

Test{
  number = 338,
  section = "Code spans",
  input = [==[`foo\`bar`
]==],
  result = [==[<p><code>foo\</code>bar`</p>
]==]
}

Test{
  number = 339,
  section = "Code spans",
  input = [==[``foo`bar``
]==],
  result = [==[<p><code>foo`bar</code></p>
]==]
}

Test{
  number = 340,
  section = "Code spans",
  input = [==[` foo `` bar `
]==],
  result = [==[<p><code>foo `` bar</code></p>
]==]
}

Test{
  number = 341,
  section = "Code spans",
  input = [==[*foo`*`
]==],
  result = [==[<p>*foo<code>*</code></p>
]==]
}

Test{
  number = 342,
  section = "Code spans",
  input = [==[[not a `link](/foo`)
]==],
  result = [==[<p>[not a <code>link](/foo</code>)</p>
]==]
}

Test{
  number = 343,
  section = "Code spans",
  input = [==[`<a href="`">`
]==],
  result = [==[<p><code>&lt;a href=&quot;</code>&quot;&gt;`</p>
]==]
}

Test{
  number = 344,
  section = "Code spans",
  input = [==[<a href="`">`
]==],
  result = [==[<p><a href="`">`</p>
]==]
}

Test{
  number = 345,
  section = "Code spans",
  input = [==[`<http://foo.bar.`baz>`
]==],
  result = [==[<p><code>&lt;http://foo.bar.</code>baz&gt;`</p>
]==]
}

Test{
  number = 346,
  section = "Code spans",
  input = [==[<http://foo.bar.`baz>`
]==],
  result = [==[<p><a href="http://foo.bar.%60baz">http://foo.bar.`baz</a>`</p>
]==]
}

Test{
  number = 347,
  section = "Code spans",
  input = [==[```foo``
]==],
  result = [==[<p>```foo``</p>
]==]
}

Test{
  number = 348,
  section = "Code spans",
  input = [==[`foo
]==],
  result = [==[<p>`foo</p>
]==]
}

Test{
  number = 349,
  section = "Code spans",
  input = [==[`foo``bar``
]==],
  result = [==[<p>`foo<code>bar</code></p>
]==]
}

-- Emphasis and strong emphasis
Test{
  number = 350,
  section = "Emphasis and strong emphasis",
  input = [==[*foo bar*
]==],
  result = [==[<p><em>foo bar</em></p>
]==]
}

Test{
  number = 351,
  section = "Emphasis and strong emphasis",
  input = [==[a * foo bar*
]==],
  result = [==[<p>a * foo bar*</p>
]==]
}

Test{
  number = 352,
  section = "Emphasis and strong emphasis",
  input = [==[a*"foo"*
]==],
  result = [==[<p>a*&quot;foo&quot;*</p>
]==]
}

Test{
  number = 353,
  section = "Emphasis and strong emphasis",
  input = [==[* a *
]==],
  result = [==[<p>* a *</p>
]==]
}

Test{
  number = 354,
  section = "Emphasis and strong emphasis",
  input = [==[foo*bar*
]==],
  result = [==[<p>foo<em>bar</em></p>
]==]
}

Test{
  number = 355,
  section = "Emphasis and strong emphasis",
  input = [==[5*6*78
]==],
  result = [==[<p>5<em>6</em>78</p>
]==]
}

Test{
  number = 356,
  section = "Emphasis and strong emphasis",
  input = [==[_foo bar_
]==],
  result = [==[<p><em>foo bar</em></p>
]==]
}

Test{
  number = 357,
  section = "Emphasis and strong emphasis",
  input = [==[_ foo bar_
]==],
  result = [==[<p>_ foo bar_</p>
]==]
}

Test{
  number = 358,
  section = "Emphasis and strong emphasis",
  input = [==[a_"foo"_
]==],
  result = [==[<p>a_&quot;foo&quot;_</p>
]==]
}

Test{
  number = 359,
  section = "Emphasis and strong emphasis",
  input = [==[foo_bar_
]==],
  result = [==[<p>foo_bar_</p>
]==]
}

Test{
  number = 360,
  section = "Emphasis and strong emphasis",
  input = [==[5_6_78
]==],
  result = [==[<p>5_6_78</p>
]==]
}

Test{
  number = 361,
  section = "Emphasis and strong emphasis",
  input = [==[пристаням_стремятся_
]==],
  result = [==[<p>пристаням_стремятся_</p>
]==]
}

Test{
  number = 362,
  section = "Emphasis and strong emphasis",
  input = [==[aa_"bb"_cc
]==],
  result = [==[<p>aa_&quot;bb&quot;_cc</p>
]==]
}

Test{
  number = 363,
  section = "Emphasis and strong emphasis",
  input = [==[foo-_(bar)_
]==],
  result = [==[<p>foo-<em>(bar)</em></p>
]==]
}

Test{
  number = 364,
  section = "Emphasis and strong emphasis",
  input = [==[_foo*
]==],
  result = [==[<p>_foo*</p>
]==]
}

Test{
  number = 365,
  section = "Emphasis and strong emphasis",
  input = [==[*foo bar *
]==],
  result = [==[<p>*foo bar *</p>
]==]
}

Test{
  number = 366,
  section = "Emphasis and strong emphasis",
  input = [==[*foo bar
*
]==],
  result = [==[<p>*foo bar
*</p>
]==]
}

Test{
  number = 367,
  section = "Emphasis and strong emphasis",
  input = [==[*(*foo)
]==],
  result = [==[<p>*(*foo)</p>
]==]
}

Test{
  number = 368,
  section = "Emphasis and strong emphasis",
  input = [==[*(*foo*)*
]==],
  result = [==[<p><em>(<em>foo</em>)</em></p>
]==]
}

Test{
  number = 369,
  section = "Emphasis and strong emphasis",
  input = [==[*foo*bar
]==],
  result = [==[<p><em>foo</em>bar</p>
]==]
}

Test{
  number = 370,
  section = "Emphasis and strong emphasis",
  input = [==[_foo bar _
]==],
  result = [==[<p>_foo bar _</p>
]==]
}

Test{
  number = 371,
  section = "Emphasis and strong emphasis",
  input = [==[_(_foo)
]==],
  result = [==[<p>_(_foo)</p>
]==]
}

Test{
  number = 372,
  section = "Emphasis and strong emphasis",
  input = [==[_(_foo_)_
]==],
  result = [==[<p><em>(<em>foo</em>)</em></p>
]==]
}

Test{
  number = 373,
  section = "Emphasis and strong emphasis",
  input = [==[_foo_bar
]==],
  result = [==[<p>_foo_bar</p>
]==]
}

Test{
  number = 374,
  section = "Emphasis and strong emphasis",
  input = [==[_пристаням_стремятся
]==],
  result = [==[<p>_пристаням_стремятся</p>
]==]
}

Test{
  number = 375,
  section = "Emphasis and strong emphasis",
  input = [==[_foo_bar_baz_
]==],
  result = [==[<p><em>foo_bar_baz</em></p>
]==]
}

Test{
  number = 376,
  section = "Emphasis and strong emphasis",
  input = [==[_(bar)_.
]==],
  result = [==[<p><em>(bar)</em>.</p>
]==]
}

Test{
  number = 377,
  section = "Emphasis and strong emphasis",
  input = [==[**foo bar**
]==],
  result = [==[<p><strong>foo bar</strong></p>
]==]
}

Test{
  number = 378,
  section = "Emphasis and strong emphasis",
  input = [==[** foo bar**
]==],
  result = [==[<p>** foo bar**</p>
]==]
}

Test{
  number = 379,
  section = "Emphasis and strong emphasis",
  input = [==[a**"foo"**
]==],
  result = [==[<p>a**&quot;foo&quot;**</p>
]==]
}

Test{
  number = 380,
  section = "Emphasis and strong emphasis",
  input = [==[foo**bar**
]==],
  result = [==[<p>foo<strong>bar</strong></p>
]==]
}

Test{
  number = 381,
  section = "Emphasis and strong emphasis",
  input = [==[__foo bar__
]==],
  result = [==[<p><strong>foo bar</strong></p>
]==]
}

Test{
  number = 382,
  section = "Emphasis and strong emphasis",
  input = [==[__ foo bar__
]==],
  result = [==[<p>__ foo bar__</p>
]==]
}

Test{
  number = 383,
  section = "Emphasis and strong emphasis",
  input = [==[__
foo bar__
]==],
  result = [==[<p>__
foo bar__</p>
]==]
}

Test{
  number = 384,
  section = "Emphasis and strong emphasis",
  input = [==[a__"foo"__
]==],
  result = [==[<p>a__&quot;foo&quot;__</p>
]==]
}

Test{
  number = 385,
  section = "Emphasis and strong emphasis",
  input = [==[foo__bar__
]==],
  result = [==[<p>foo__bar__</p>
]==]
}

Test{
  number = 386,
  section = "Emphasis and strong emphasis",
  input = [==[5__6__78
]==],
  result = [==[<p>5__6__78</p>
]==]
}

Test{
  number = 387,
  section = "Emphasis and strong emphasis",
  input = [==[пристаням__стремятся__
]==],
  result = [==[<p>пристаням__стремятся__</p>
]==]
}

Test{
  number = 388,
  section = "Emphasis and strong emphasis",
  input = [==[__foo, __bar__, baz__
]==],
  result = [==[<p><strong>foo, <strong>bar</strong>, baz</strong></p>
]==]
}

Test{
  number = 389,
  section = "Emphasis and strong emphasis",
  input = [==[foo-__(bar)__
]==],
  result = [==[<p>foo-<strong>(bar)</strong></p>
]==]
}

Test{
  number = 390,
  section = "Emphasis and strong emphasis",
  input = [==[**foo bar **
]==],
  result = [==[<p>**foo bar **</p>
]==]
}

Test{
  number = 391,
  section = "Emphasis and strong emphasis",
  input = [==[**(**foo)
]==],
  result = [==[<p>**(**foo)</p>
]==]
}

Test{
  number = 392,
  section = "Emphasis and strong emphasis",
  input = [==[*(**foo**)*
]==],
  result = [==[<p><em>(<strong>foo</strong>)</em></p>
]==]
}

Test{
  number = 393,
  section = "Emphasis and strong emphasis",
  input = [==[**Gomphocarpus (*Gomphocarpus physocarpus*, syn.
*Asclepias physocarpa*)**
]==],
  result = [==[<p><strong>Gomphocarpus (<em>Gomphocarpus physocarpus</em>, syn.
<em>Asclepias physocarpa</em>)</strong></p>
]==]
}

Test{
  number = 394,
  section = "Emphasis and strong emphasis",
  input = [==[**foo "*bar*" foo**
]==],
  result = [==[<p><strong>foo &quot;<em>bar</em>&quot; foo</strong></p>
]==]
}

Test{
  number = 395,
  section = "Emphasis and strong emphasis",
  input = [==[**foo**bar
]==],
  result = [==[<p><strong>foo</strong>bar</p>
]==]
}

Test{
  number = 396,
  section = "Emphasis and strong emphasis",
  input = [==[__foo bar __
]==],
  result = [==[<p>__foo bar __</p>
]==]
}

Test{
  number = 397,
  section = "Emphasis and strong emphasis",
  input = [==[__(__foo)
]==],
  result = [==[<p>__(__foo)</p>
]==]
}

Test{
  number = 398,
  section = "Emphasis and strong emphasis",
  input = [==[_(__foo__)_
]==],
  result = [==[<p><em>(<strong>foo</strong>)</em></p>
]==]
}

Test{
  number = 399,
  section = "Emphasis and strong emphasis",
  input = [==[__foo__bar
]==],
  result = [==[<p>__foo__bar</p>
]==]
}

Test{
  number = 400,
  section = "Emphasis and strong emphasis",
  input = [==[__пристаням__стремятся
]==],
  result = [==[<p>__пристаням__стремятся</p>
]==]
}

Test{
  number = 401,
  section = "Emphasis and strong emphasis",
  input = [==[__foo__bar__baz__
]==],
  result = [==[<p><strong>foo__bar__baz</strong></p>
]==]
}

Test{
  number = 402,
  section = "Emphasis and strong emphasis",
  input = [==[__(bar)__.
]==],
  result = [==[<p><strong>(bar)</strong>.</p>
]==]
}

Test{
  number = 403,
  section = "Emphasis and strong emphasis",
  input = [==[*foo [bar](/url)*
]==],
  result = [==[<p><em>foo <a href="/url">bar</a></em></p>
]==]
}

Test{
  number = 404,
  section = "Emphasis and strong emphasis",
  input = [==[*foo
bar*
]==],
  result = [==[<p><em>foo
bar</em></p>
]==]
}

Test{
  number = 405,
  section = "Emphasis and strong emphasis",
  input = [==[_foo __bar__ baz_
]==],
  result = [==[<p><em>foo <strong>bar</strong> baz</em></p>
]==]
}

Test{
  number = 406,
  section = "Emphasis and strong emphasis",
  input = [==[_foo _bar_ baz_
]==],
  result = [==[<p><em>foo <em>bar</em> baz</em></p>
]==]
}

Test{
  number = 407,
  section = "Emphasis and strong emphasis",
  input = [==[__foo_ bar_
]==],
  result = [==[<p><em><em>foo</em> bar</em></p>
]==]
}

Test{
  number = 408,
  section = "Emphasis and strong emphasis",
  input = [==[*foo *bar**
]==],
  result = [==[<p><em>foo <em>bar</em></em></p>
]==]
}

Test{
  number = 409,
  section = "Emphasis and strong emphasis",
  input = [==[*foo **bar** baz*
]==],
  result = [==[<p><em>foo <strong>bar</strong> baz</em></p>
]==]
}

Test{
  number = 410,
  section = "Emphasis and strong emphasis",
  input = [==[*foo**bar**baz*
]==],
  result = [==[<p><em>foo<strong>bar</strong>baz</em></p>
]==]
}

Test{
  number = 411,
  section = "Emphasis and strong emphasis",
  input = [==[*foo**bar*
]==],
  result = [==[<p><em>foo**bar</em></p>
]==]
}

Test{
  number = 412,
  section = "Emphasis and strong emphasis",
  input = [==[***foo** bar*
]==],
  result = [==[<p><em><strong>foo</strong> bar</em></p>
]==]
}

Test{
  number = 413,
  section = "Emphasis and strong emphasis",
  input = [==[*foo **bar***
]==],
  result = [==[<p><em>foo <strong>bar</strong></em></p>
]==]
}

Test{
  number = 414,
  section = "Emphasis and strong emphasis",
  input = [==[*foo**bar***
]==],
  result = [==[<p><em>foo<strong>bar</strong></em></p>
]==]
}

Test{
  number = 415,
  section = "Emphasis and strong emphasis",
  input = [==[foo***bar***baz
]==],
  result = [==[<p>foo<em><strong>bar</strong></em>baz</p>
]==]
}

Test{
  number = 416,
  section = "Emphasis and strong emphasis",
  input = [==[foo******bar*********baz
]==],
  result = [==[<p>foo<strong><strong><strong>bar</strong></strong></strong>***baz</p>
]==]
}

Test{
  number = 417,
  section = "Emphasis and strong emphasis",
  input = [==[*foo **bar *baz* bim** bop*
]==],
  result = [==[<p><em>foo <strong>bar <em>baz</em> bim</strong> bop</em></p>
]==]
}

Test{
  number = 418,
  section = "Emphasis and strong emphasis",
  input = [==[*foo [*bar*](/url)*
]==],
  result = [==[<p><em>foo <a href="/url"><em>bar</em></a></em></p>
]==]
}

Test{
  number = 419,
  section = "Emphasis and strong emphasis",
  input = [==[** is not an empty emphasis
]==],
  result = [==[<p>** is not an empty emphasis</p>
]==]
}

Test{
  number = 420,
  section = "Emphasis and strong emphasis",
  input = [==[**** is not an empty strong emphasis
]==],
  result = [==[<p>**** is not an empty strong emphasis</p>
]==]
}

Test{
  number = 421,
  section = "Emphasis and strong emphasis",
  input = [==[**foo [bar](/url)**
]==],
  result = [==[<p><strong>foo <a href="/url">bar</a></strong></p>
]==]
}

Test{
  number = 422,
  section = "Emphasis and strong emphasis",
  input = [==[**foo
bar**
]==],
  result = [==[<p><strong>foo
bar</strong></p>
]==]
}

Test{
  number = 423,
  section = "Emphasis and strong emphasis",
  input = [==[__foo _bar_ baz__
]==],
  result = [==[<p><strong>foo <em>bar</em> baz</strong></p>
]==]
}

Test{
  number = 424,
  section = "Emphasis and strong emphasis",
  input = [==[__foo __bar__ baz__
]==],
  result = [==[<p><strong>foo <strong>bar</strong> baz</strong></p>
]==]
}

Test{
  number = 425,
  section = "Emphasis and strong emphasis",
  input = [==[____foo__ bar__
]==],
  result = [==[<p><strong><strong>foo</strong> bar</strong></p>
]==]
}

Test{
  number = 426,
  section = "Emphasis and strong emphasis",
  input = [==[**foo **bar****
]==],
  result = [==[<p><strong>foo <strong>bar</strong></strong></p>
]==]
}

Test{
  number = 427,
  section = "Emphasis and strong emphasis",
  input = [==[**foo *bar* baz**
]==],
  result = [==[<p><strong>foo <em>bar</em> baz</strong></p>
]==]
}

Test{
  number = 428,
  section = "Emphasis and strong emphasis",
  input = [==[**foo*bar*baz**
]==],
  result = [==[<p><strong>foo<em>bar</em>baz</strong></p>
]==]
}

Test{
  number = 429,
  section = "Emphasis and strong emphasis",
  input = [==[***foo* bar**
]==],
  result = [==[<p><strong><em>foo</em> bar</strong></p>
]==]
}

Test{
  number = 430,
  section = "Emphasis and strong emphasis",
  input = [==[**foo *bar***
]==],
  result = [==[<p><strong>foo <em>bar</em></strong></p>
]==]
}

Test{
  number = 431,
  section = "Emphasis and strong emphasis",
  input = [==[**foo *bar **baz**
bim* bop**
]==],
  result = [==[<p><strong>foo <em>bar <strong>baz</strong>
bim</em> bop</strong></p>
]==]
}

Test{
  number = 432,
  section = "Emphasis and strong emphasis",
  input = [==[**foo [*bar*](/url)**
]==],
  result = [==[<p><strong>foo <a href="/url"><em>bar</em></a></strong></p>
]==]
}

Test{
  number = 433,
  section = "Emphasis and strong emphasis",
  input = [==[__ is not an empty emphasis
]==],
  result = [==[<p>__ is not an empty emphasis</p>
]==]
}

Test{
  number = 434,
  section = "Emphasis and strong emphasis",
  input = [==[____ is not an empty strong emphasis
]==],
  result = [==[<p>____ is not an empty strong emphasis</p>
]==]
}

Test{
  number = 435,
  section = "Emphasis and strong emphasis",
  input = [==[foo ***
]==],
  result = [==[<p>foo ***</p>
]==]
}

Test{
  number = 436,
  section = "Emphasis and strong emphasis",
  input = [==[foo *\**
]==],
  result = [==[<p>foo <em>*</em></p>
]==]
}

Test{
  number = 437,
  section = "Emphasis and strong emphasis",
  input = [==[foo *_*
]==],
  result = [==[<p>foo <em>_</em></p>
]==]
}

Test{
  number = 438,
  section = "Emphasis and strong emphasis",
  input = [==[foo *****
]==],
  result = [==[<p>foo *****</p>
]==]
}

Test{
  number = 439,
  section = "Emphasis and strong emphasis",
  input = [==[foo **\***
]==],
  result = [==[<p>foo <strong>*</strong></p>
]==]
}

Test{
  number = 440,
  section = "Emphasis and strong emphasis",
  input = [==[foo **_**
]==],
  result = [==[<p>foo <strong>_</strong></p>
]==]
}

Test{
  number = 441,
  section = "Emphasis and strong emphasis",
  input = [==[**foo*
]==],
  result = [==[<p>*<em>foo</em></p>
]==]
}

Test{
  number = 442,
  section = "Emphasis and strong emphasis",
  input = [==[*foo**
]==],
  result = [==[<p><em>foo</em>*</p>
]==]
}

Test{
  number = 443,
  section = "Emphasis and strong emphasis",
  input = [==[***foo**
]==],
  result = [==[<p>*<strong>foo</strong></p>
]==]
}

Test{
  number = 444,
  section = "Emphasis and strong emphasis",
  input = [==[****foo*
]==],
  result = [==[<p>***<em>foo</em></p>
]==]
}

Test{
  number = 445,
  section = "Emphasis and strong emphasis",
  input = [==[**foo***
]==],
  result = [==[<p><strong>foo</strong>*</p>
]==]
}

Test{
  number = 446,
  section = "Emphasis and strong emphasis",
  input = [==[*foo****
]==],
  result = [==[<p><em>foo</em>***</p>
]==]
}

Test{
  number = 447,
  section = "Emphasis and strong emphasis",
  input = [==[foo ___
]==],
  result = [==[<p>foo ___</p>
]==]
}

Test{
  number = 448,
  section = "Emphasis and strong emphasis",
  input = [==[foo _\__
]==],
  result = [==[<p>foo <em>_</em></p>
]==]
}

Test{
  number = 449,
  section = "Emphasis and strong emphasis",
  input = [==[foo _*_
]==],
  result = [==[<p>foo <em>*</em></p>
]==]
}

Test{
  number = 450,
  section = "Emphasis and strong emphasis",
  input = [==[foo _____
]==],
  result = [==[<p>foo _____</p>
]==]
}

Test{
  number = 451,
  section = "Emphasis and strong emphasis",
  input = [==[foo __\___
]==],
  result = [==[<p>foo <strong>_</strong></p>
]==]
}

Test{
  number = 452,
  section = "Emphasis and strong emphasis",
  input = [==[foo __*__
]==],
  result = [==[<p>foo <strong>*</strong></p>
]==]
}

Test{
  number = 453,
  section = "Emphasis and strong emphasis",
  input = [==[__foo_
]==],
  result = [==[<p>_<em>foo</em></p>
]==]
}

Test{
  number = 454,
  section = "Emphasis and strong emphasis",
  input = [==[_foo__
]==],
  result = [==[<p><em>foo</em>_</p>
]==]
}

Test{
  number = 455,
  section = "Emphasis and strong emphasis",
  input = [==[___foo__
]==],
  result = [==[<p>_<strong>foo</strong></p>
]==]
}

Test{
  number = 456,
  section = "Emphasis and strong emphasis",
  input = [==[____foo_
]==],
  result = [==[<p>___<em>foo</em></p>
]==]
}

Test{
  number = 457,
  section = "Emphasis and strong emphasis",
  input = [==[__foo___
]==],
  result = [==[<p><strong>foo</strong>_</p>
]==]
}

Test{
  number = 458,
  section = "Emphasis and strong emphasis",
  input = [==[_foo____
]==],
  result = [==[<p><em>foo</em>___</p>
]==]
}

Test{
  number = 459,
  section = "Emphasis and strong emphasis",
  input = [==[**foo**
]==],
  result = [==[<p><strong>foo</strong></p>
]==]
}

Test{
  number = 460,
  section = "Emphasis and strong emphasis",
  input = [==[*_foo_*
]==],
  result = [==[<p><em><em>foo</em></em></p>
]==]
}

Test{
  number = 461,
  section = "Emphasis and strong emphasis",
  input = [==[__foo__
]==],
  result = [==[<p><strong>foo</strong></p>
]==]
}

Test{
  number = 462,
  section = "Emphasis and strong emphasis",
  input = [==[_*foo*_
]==],
  result = [==[<p><em><em>foo</em></em></p>
]==]
}

Test{
  number = 463,
  section = "Emphasis and strong emphasis",
  input = [==[****foo****
]==],
  result = [==[<p><strong><strong>foo</strong></strong></p>
]==]
}

Test{
  number = 464,
  section = "Emphasis and strong emphasis",
  input = [==[____foo____
]==],
  result = [==[<p><strong><strong>foo</strong></strong></p>
]==]
}

Test{
  number = 465,
  section = "Emphasis and strong emphasis",
  input = [==[******foo******
]==],
  result = [==[<p><strong><strong><strong>foo</strong></strong></strong></p>
]==]
}

Test{
  number = 466,
  section = "Emphasis and strong emphasis",
  input = [==[***foo***
]==],
  result = [==[<p><em><strong>foo</strong></em></p>
]==]
}

Test{
  number = 467,
  section = "Emphasis and strong emphasis",
  input = [==[_____foo_____
]==],
  result = [==[<p><em><strong><strong>foo</strong></strong></em></p>
]==]
}

Test{
  number = 468,
  section = "Emphasis and strong emphasis",
  input = [==[*foo _bar* baz_
]==],
  result = [==[<p><em>foo _bar</em> baz_</p>
]==]
}

Test{
  number = 469,
  section = "Emphasis and strong emphasis",
  input = [==[*foo __bar *baz bim__ bam*
]==],
  result = [==[<p><em>foo <strong>bar *baz bim</strong> bam</em></p>
]==]
}

Test{
  number = 470,
  section = "Emphasis and strong emphasis",
  input = [==[**foo **bar baz**
]==],
  result = [==[<p>**foo <strong>bar baz</strong></p>
]==]
}

Test{
  number = 471,
  section = "Emphasis and strong emphasis",
  input = [==[*foo *bar baz*
]==],
  result = [==[<p>*foo <em>bar baz</em></p>
]==]
}

Test{
  number = 472,
  section = "Emphasis and strong emphasis",
  input = [==[*[bar*](/url)
]==],
  result = [==[<p>*<a href="/url">bar*</a></p>
]==]
}

Test{
  number = 473,
  section = "Emphasis and strong emphasis",
  input = [==[_foo [bar_](/url)
]==],
  result = [==[<p>_foo <a href="/url">bar_</a></p>
]==]
}

Test{
  number = 474,
  section = "Emphasis and strong emphasis",
  input = [==[*<img src="foo" title="*"/>
]==],
  result = [==[<p>*<img src="foo" title="*"/></p>
]==]
}

Test{
  number = 475,
  section = "Emphasis and strong emphasis",
  input = [==[**<a href="**">
]==],
  result = [==[<p>**<a href="**"></p>
]==]
}

Test{
  number = 476,
  section = "Emphasis and strong emphasis",
  input = [==[__<a href="__">
]==],
  result = [==[<p>__<a href="__"></p>
]==]
}

Test{
  number = 477,
  section = "Emphasis and strong emphasis",
  input = [==[*a `*`*
]==],
  result = [==[<p><em>a <code>*</code></em></p>
]==]
}

Test{
  number = 478,
  section = "Emphasis and strong emphasis",
  input = [==[_a `_`_
]==],
  result = [==[<p><em>a <code>_</code></em></p>
]==]
}

Test{
  number = 479,
  section = "Emphasis and strong emphasis",
  input = [==[**a<http://foo.bar/?q=**>
]==],
  result = [==[<p>**a<a href="http://foo.bar/?q=**">http://foo.bar/?q=**</a></p>
]==]
}

Test{
  number = 480,
  section = "Emphasis and strong emphasis",
  input = [==[__a<http://foo.bar/?q=__>
]==],
  result = [==[<p>__a<a href="http://foo.bar/?q=__">http://foo.bar/?q=__</a></p>
]==]
}

-- Links
Test{
  number = 481,
  section = "Links",
  input = [==[[link](/uri "title")
]==],
  result = [==[<p><a href="/uri" title="title">link</a></p>
]==]
}

Test{
  number = 482,
  section = "Links",
  input = [==[[link](/uri)
]==],
  result = [==[<p><a href="/uri">link</a></p>
]==]
}

Test{
  number = 483,
  section = "Links",
  input = [==[[](./target.md)
]==],
  result = [==[<p><a href="./target.md"></a></p>
]==]
}

Test{
  number = 484,
  section = "Links",
  input = [==[[link]()
]==],
  result = [==[<p><a href="">link</a></p>
]==]
}

Test{
  number = 485,
  section = "Links",
  input = [==[[link](<>)
]==],
  result = [==[<p><a href="">link</a></p>
]==]
}

Test{
  number = 486,
  section = "Links",
  input = [==[[]()
]==],
  result = [==[<p><a href=""></a></p>
]==]
}

Test{
  number = 487,
  section = "Links",
  input = [==[[link](/my uri)
]==],
  result = [==[<p>[link](/my uri)</p>
]==]
}

Test{
  number = 488,
  section = "Links",
  input = [==[[link](</my uri>)
]==],
  result = [==[<p><a href="/my%20uri">link</a></p>
]==]
}

Test{
  number = 489,
  section = "Links",
  input = [==[[link](foo
bar)
]==],
  result = [==[<p>[link](foo
bar)</p>
]==]
}

Test{
  number = 490,
  section = "Links",
  input = [==[[link](<foo
bar>)
]==],
  result = [==[<p>[link](<foo
bar>)</p>
]==]
}

Test{
  number = 491,
  section = "Links",
  input = [==[[a](<b)c>)
]==],
  result = [==[<p><a href="b)c">a</a></p>
]==]
}

Test{
  number = 492,
  section = "Links",
  input = [==[[link](<foo\>)
]==],
  result = [==[<p>[link](&lt;foo&gt;)</p>
]==]
}

Test{
  number = 493,
  section = "Links",
  input = [==[[a](<b)c
[a](<b)c>
[a](<b>c)
]==],
  result = [==[<p>[a](&lt;b)c
[a](&lt;b)c&gt;
[a](<b>c)</p>
]==]
}

Test{
  number = 494,
  section = "Links",
  input = [==[[link](\(foo\))
]==],
  result = [==[<p><a href="(foo)">link</a></p>
]==]
}

Test{
  number = 495,
  section = "Links",
  input = [==[[link](foo(and(bar)))
]==],
  result = [==[<p><a href="foo(and(bar))">link</a></p>
]==]
}

Test{
  number = 496,
  section = "Links",
  input = [==[[link](foo(and(bar))
]==],
  result = [==[<p>[link](foo(and(bar))</p>
]==]
}

Test{
  number = 497,
  section = "Links",
  input = [==[[link](foo\(and\(bar\))
]==],
  result = [==[<p><a href="foo(and(bar)">link</a></p>
]==]
}

Test{
  number = 498,
  section = "Links",
  input = [==[[link](<foo(and(bar)>)
]==],
  result = [==[<p><a href="foo(and(bar)">link</a></p>
]==]
}

Test{
  number = 499,
  section = "Links",
  input = [==[[link](foo\)\:)
]==],
  result = [==[<p><a href="foo):">link</a></p>
]==]
}

Test{
  number = 500,
  section = "Links",
  input = [==[[link](#fragment)

[link](http://example.com#fragment)

[link](http://example.com?foo=3#frag)
]==],
  result = [==[<p><a href="#fragment">link</a></p>
<p><a href="http://example.com#fragment">link</a></p>
<p><a href="http://example.com?foo=3#frag">link</a></p>
]==]
}

Test{
  number = 501,
  section = "Links",
  input = [==[[link](foo\bar)
]==],
  result = [==[<p><a href="foo%5Cbar">link</a></p>
]==]
}

Test{
  number = 502,
  section = "Links",
  input = [==[[link](foo%20b&auml;)
]==],
  result = [==[<p><a href="foo%20b%C3%A4">link</a></p>
]==]
}

Test{
  number = 503,
  section = "Links",
  input = [==[[link]("title")
]==],
  result = [==[<p><a href="%22title%22">link</a></p>
]==]
}

Test{
  number = 504,
  section = "Links",
  input = [==[[link](/url "title")
[link](/url 'title')
[link](/url (title))
]==],
  result = [==[<p><a href="/url" title="title">link</a>
<a href="/url" title="title">link</a>
<a href="/url" title="title">link</a></p>
]==]
}

Test{
  number = 505,
  section = "Links",
  input = [==[[link](/url "title \"&quot;")
]==],
  result = [==[<p><a href="/url" title="title &quot;&quot;">link</a></p>
]==]
}

Test{
  number = 506,
  section = "Links",
  input = [==[[link](/url "title")
]==],
  result = [==[<p><a href="/url%C2%A0%22title%22">link</a></p>
]==]
}

Test{
  number = 507,
  section = "Links",
  input = [==[[link](/url "title "and" title")
]==],
  result = [==[<p>[link](/url &quot;title &quot;and&quot; title&quot;)</p>
]==]
}

Test{
  number = 508,
  section = "Links",
  input = [==[[link](/url 'title "and" title')
]==],
  result = [==[<p><a href="/url" title="title &quot;and&quot; title">link</a></p>
]==]
}

Test{
  number = 509,
  section = "Links",
  input = [==[[link](   /uri
  "title"  )
]==],
  result = [==[<p><a href="/uri" title="title">link</a></p>
]==]
}

Test{
  number = 510,
  section = "Links",
  input = [==[[link] (/uri)
]==],
  result = [==[<p>[link] (/uri)</p>
]==]
}

Test{
  number = 511,
  section = "Links",
  input = [==[[link [foo [bar]]](/uri)
]==],
  result = [==[<p><a href="/uri">link [foo [bar]]</a></p>
]==]
}

Test{
  number = 512,
  section = "Links",
  input = [==[[link] bar](/uri)
]==],
  result = [==[<p>[link] bar](/uri)</p>
]==]
}

Test{
  number = 513,
  section = "Links",
  input = [==[[link [bar](/uri)
]==],
  result = [==[<p>[link <a href="/uri">bar</a></p>
]==]
}

Test{
  number = 514,
  section = "Links",
  input = [==[[link \[bar](/uri)
]==],
  result = [==[<p><a href="/uri">link [bar</a></p>
]==]
}

Test{
  number = 515,
  section = "Links",
  input = [==[[link *foo **bar** `#`*](/uri)
]==],
  result = [==[<p><a href="/uri">link <em>foo <strong>bar</strong> <code>#</code></em></a></p>
]==]
}

Test{
  number = 516,
  section = "Links",
  input = [==[[![moon](moon.jpg)](/uri)
]==],
  result = [==[<p><a href="/uri"><img src="moon.jpg" alt="moon" /></a></p>
]==]
}

Test{
  number = 517,
  section = "Links",
  input = [==[[foo [bar](/uri)](/uri)
]==],
  result = [==[<p>[foo <a href="/uri">bar</a>](/uri)</p>
]==]
}

Test{
  number = 518,
  section = "Links",
  input = [==[[foo *[bar [baz](/uri)](/uri)*](/uri)
]==],
  result = [==[<p>[foo <em>[bar <a href="/uri">baz</a>](/uri)</em>](/uri)</p>
]==]
}

Test{
  number = 519,
  section = "Links",
  input = [==[![[[foo](uri1)](uri2)](uri3)
]==],
  result = [==[<p><img src="uri3" alt="[foo](uri2)" /></p>
]==]
}

Test{
  number = 520,
  section = "Links",
  input = [==[*[foo*](/uri)
]==],
  result = [==[<p>*<a href="/uri">foo*</a></p>
]==]
}

Test{
  number = 521,
  section = "Links",
  input = [==[[foo *bar](baz*)
]==],
  result = [==[<p><a href="baz*">foo *bar</a></p>
]==]
}

Test{
  number = 522,
  section = "Links",
  input = [==[*foo [bar* baz]
]==],
  result = [==[<p><em>foo [bar</em> baz]</p>
]==]
}

Test{
  number = 523,
  section = "Links",
  input = [==[[foo <bar attr="](baz)">
]==],
  result = [==[<p>[foo <bar attr="](baz)"></p>
]==]
}

Test{
  number = 524,
  section = "Links",
  input = [==[[foo`](/uri)`
]==],
  result = [==[<p>[foo<code>](/uri)</code></p>
]==]
}

Test{
  number = 525,
  section = "Links",
  input = [==[[foo<http://example.com/?search=](uri)>
]==],
  result = [==[<p>[foo<a href="http://example.com/?search=%5D(uri)">http://example.com/?search=](uri)</a></p>
]==]
}

Test{
  number = 526,
  section = "Links",
  input = [==[[foo][bar]

[bar]: /url "title"
]==],
  result = [==[<p><a href="/url" title="title">foo</a></p>
]==]
}

Test{
  number = 527,
  section = "Links",
  input = [==[[link [foo [bar]]][ref]

[ref]: /uri
]==],
  result = [==[<p><a href="/uri">link [foo [bar]]</a></p>
]==]
}

Test{
  number = 528,
  section = "Links",
  input = [==[[link \[bar][ref]

[ref]: /uri
]==],
  result = [==[<p><a href="/uri">link [bar</a></p>
]==]
}

Test{
  number = 529,
  section = "Links",
  input = [==[[link *foo **bar** `#`*][ref]

[ref]: /uri
]==],
  result = [==[<p><a href="/uri">link <em>foo <strong>bar</strong> <code>#</code></em></a></p>
]==]
}

Test{
  number = 530,
  section = "Links",
  input = [==[[![moon](moon.jpg)][ref]

[ref]: /uri
]==],
  result = [==[<p><a href="/uri"><img src="moon.jpg" alt="moon" /></a></p>
]==]
}

Test{
  number = 531,
  section = "Links",
  input = [==[[foo [bar](/uri)][ref]

[ref]: /uri
]==],
  result = [==[<p>[foo <a href="/uri">bar</a>]<a href="/uri">ref</a></p>
]==]
}

Test{
  number = 532,
  section = "Links",
  input = [==[[foo *bar [baz][ref]*][ref]

[ref]: /uri
]==],
  result = [==[<p>[foo <em>bar <a href="/uri">baz</a></em>]<a href="/uri">ref</a></p>
]==]
}

Test{
  number = 533,
  section = "Links",
  input = [==[*[foo*][ref]

[ref]: /uri
]==],
  result = [==[<p>*<a href="/uri">foo*</a></p>
]==]
}

Test{
  number = 534,
  section = "Links",
  input = [==[[foo *bar][ref]*

[ref]: /uri
]==],
  result = [==[<p><a href="/uri">foo *bar</a>*</p>
]==]
}

Test{
  number = 535,
  section = "Links",
  input = [==[[foo <bar attr="][ref]">

[ref]: /uri
]==],
  result = [==[<p>[foo <bar attr="][ref]"></p>
]==]
}

Test{
  number = 536,
  section = "Links",
  input = [==[[foo`][ref]`

[ref]: /uri
]==],
  result = [==[<p>[foo<code>][ref]</code></p>
]==]
}

Test{
  number = 537,
  section = "Links",
  input = [==[[foo<http://example.com/?search=][ref]>

[ref]: /uri
]==],
  result = [==[<p>[foo<a href="http://example.com/?search=%5D%5Bref%5D">http://example.com/?search=][ref]</a></p>
]==]
}

Test{
  number = 538,
  section = "Links",
  input = [==[[foo][BaR]

[bar]: /url "title"
]==],
  result = [==[<p><a href="/url" title="title">foo</a></p>
]==]
}

Test{
  number = 539,
  section = "Links",
  input = [==[[ẞ]

[SS]: /url
]==],
  result = [==[<p><a href="/url">ẞ</a></p>
]==]
}

Test{
  number = 540,
  section = "Links",
  input = [==[[Foo
  bar]: /url

[Baz][Foo bar]
]==],
  result = [==[<p><a href="/url">Baz</a></p>
]==]
}

Test{
  number = 541,
  section = "Links",
  input = [==[[foo] [bar]

[bar]: /url "title"
]==],
  result = [==[<p>[foo] <a href="/url" title="title">bar</a></p>
]==]
}

Test{
  number = 542,
  section = "Links",
  input = [==[[foo]
[bar]

[bar]: /url "title"
]==],
  result = [==[<p>[foo]
<a href="/url" title="title">bar</a></p>
]==]
}

Test{
  number = 543,
  section = "Links",
  input = [==[[foo]: /url1

[foo]: /url2

[bar][foo]
]==],
  result = [==[<p><a href="/url1">bar</a></p>
]==]
}

Test{
  number = 544,
  section = "Links",
  input = [==[[bar][foo\!]

[foo!]: /url
]==],
  result = [==[<p>[bar][foo!]</p>
]==]
}

Test{
  number = 545,
  section = "Links",
  input = [==[[foo][ref[]

[ref[]: /uri
]==],
  result = [==[<p>[foo][ref[]</p>
<p>[ref[]: /uri</p>
]==]
}

Test{
  number = 546,
  section = "Links",
  input = [==[[foo][ref[bar]]

[ref[bar]]: /uri
]==],
  result = [==[<p>[foo][ref[bar]]</p>
<p>[ref[bar]]: /uri</p>
]==]
}

Test{
  number = 547,
  section = "Links",
  input = [==[[[[foo]]]

[[[foo]]]: /url
]==],
  result = [==[<p>[[[foo]]]</p>
<p>[[[foo]]]: /url</p>
]==]
}

Test{
  number = 548,
  section = "Links",
  input = [==[[foo][ref\[]

[ref\[]: /uri
]==],
  result = [==[<p><a href="/uri">foo</a></p>
]==]
}

Test{
  number = 549,
  section = "Links",
  input = [==[[bar\\]: /uri

[bar\\]
]==],
  result = [==[<p><a href="/uri">bar\</a></p>
]==]
}

Test{
  number = 550,
  section = "Links",
  input = [==[[]

[]: /uri
]==],
  result = [==[<p>[]</p>
<p>[]: /uri</p>
]==]
}

Test{
  number = 551,
  section = "Links",
  input = [==[[
 ]

[
 ]: /uri
]==],
  result = [==[<p>[
]</p>
<p>[
]: /uri</p>
]==]
}

Test{
  number = 552,
  section = "Links",
  input = [==[[foo][]

[foo]: /url "title"
]==],
  result = [==[<p><a href="/url" title="title">foo</a></p>
]==]
}

Test{
  number = 553,
  section = "Links",
  input = [==[[*foo* bar][]

[*foo* bar]: /url "title"
]==],
  result = [==[<p><a href="/url" title="title"><em>foo</em> bar</a></p>
]==]
}

Test{
  number = 554,
  section = "Links",
  input = [==[[Foo][]

[foo]: /url "title"
]==],
  result = [==[<p><a href="/url" title="title">Foo</a></p>
]==]
}

Test{
  number = 555,
  section = "Links",
  input = [==[[foo] 
[]

[foo]: /url "title"
]==],
  result = [==[<p><a href="/url" title="title">foo</a>
[]</p>
]==]
}

Test{
  number = 556,
  section = "Links",
  input = [==[[foo]

[foo]: /url "title"
]==],
  result = [==[<p><a href="/url" title="title">foo</a></p>
]==]
}

Test{
  number = 557,
  section = "Links",
  input = [==[[*foo* bar]

[*foo* bar]: /url "title"
]==],
  result = [==[<p><a href="/url" title="title"><em>foo</em> bar</a></p>
]==]
}

Test{
  number = 558,
  section = "Links",
  input = [==[[[*foo* bar]]

[*foo* bar]: /url "title"
]==],
  result = [==[<p>[<a href="/url" title="title"><em>foo</em> bar</a>]</p>
]==]
}

Test{
  number = 559,
  section = "Links",
  input = [==[[[bar [foo]

[foo]: /url
]==],
  result = [==[<p>[[bar <a href="/url">foo</a></p>
]==]
}

Test{
  number = 560,
  section = "Links",
  input = [==[[Foo]

[foo]: /url "title"
]==],
  result = [==[<p><a href="/url" title="title">Foo</a></p>
]==]
}

Test{
  number = 561,
  section = "Links",
  input = [==[[foo] bar

[foo]: /url
]==],
  result = [==[<p><a href="/url">foo</a> bar</p>
]==]
}

Test{
  number = 562,
  section = "Links",
  input = [==[\[foo]

[foo]: /url "title"
]==],
  result = [==[<p>[foo]</p>
]==]
}

Test{
  number = 563,
  section = "Links",
  input = [==[[foo*]: /url

*[foo*]
]==],
  result = [==[<p>*<a href="/url">foo*</a></p>
]==]
}

Test{
  number = 564,
  section = "Links",
  input = [==[[foo][bar]

[foo]: /url1
[bar]: /url2
]==],
  result = [==[<p><a href="/url2">foo</a></p>
]==]
}

Test{
  number = 565,
  section = "Links",
  input = [==[[foo][]

[foo]: /url1
]==],
  result = [==[<p><a href="/url1">foo</a></p>
]==]
}

Test{
  number = 566,
  section = "Links",
  input = [==[[foo]()

[foo]: /url1
]==],
  result = [==[<p><a href="">foo</a></p>
]==]
}

Test{
  number = 567,
  section = "Links",
  input = [==[[foo](not a link)

[foo]: /url1
]==],
  result = [==[<p><a href="/url1">foo</a>(not a link)</p>
]==]
}

Test{
  number = 568,
  section = "Links",
  input = [==[[foo][bar][baz]

[baz]: /url
]==],
  result = [==[<p>[foo]<a href="/url">bar</a></p>
]==]
}

Test{
  number = 569,
  section = "Links",
  input = [==[[foo][bar][baz]

[baz]: /url1
[bar]: /url2
]==],
  result = [==[<p><a href="/url2">foo</a><a href="/url1">baz</a></p>
]==]
}

Test{
  number = 570,
  section = "Links",
  input = [==[[foo][bar][baz]

[baz]: /url1
[foo]: /url2
]==],
  result = [==[<p>[foo]<a href="/url1">bar</a></p>
]==]
}

-- Images
Test{
  number = 571,
  section = "Images",
  input = [==[![foo](/url "title")
]==],
  result = [==[<p><img src="/url" alt="foo" title="title" /></p>
]==]
}

Test{
  number = 572,
  section = "Images",
  input = [==[![foo *bar*]

[foo *bar*]: train.jpg "train & tracks"
]==],
  result = [==[<p><img src="train.jpg" alt="foo bar" title="train &amp; tracks" /></p>
]==]
}

Test{
  number = 573,
  section = "Images",
  input = [==[![foo ![bar](/url)](/url2)
]==],
  result = [==[<p><img src="/url2" alt="foo bar" /></p>
]==]
}

Test{
  number = 574,
  section = "Images",
  input = [==[![foo [bar](/url)](/url2)
]==],
  result = [==[<p><img src="/url2" alt="foo bar" /></p>
]==]
}

Test{
  number = 575,
  section = "Images",
  input = [==[![foo *bar*][]

[foo *bar*]: train.jpg "train & tracks"
]==],
  result = [==[<p><img src="train.jpg" alt="foo bar" title="train &amp; tracks" /></p>
]==]
}

Test{
  number = 576,
  section = "Images",
  input = [==[![foo *bar*][foobar]

[FOOBAR]: train.jpg "train & tracks"
]==],
  result = [==[<p><img src="train.jpg" alt="foo bar" title="train &amp; tracks" /></p>
]==]
}

Test{
  number = 577,
  section = "Images",
  input = [==[![foo](train.jpg)
]==],
  result = [==[<p><img src="train.jpg" alt="foo" /></p>
]==]
}

Test{
  number = 578,
  section = "Images",
  input = [==[My ![foo bar](/path/to/train.jpg  "title"   )
]==],
  result = [==[<p>My <img src="/path/to/train.jpg" alt="foo bar" title="title" /></p>
]==]
}

Test{
  number = 579,
  section = "Images",
  input = [==[![foo](<url>)
]==],
  result = [==[<p><img src="url" alt="foo" /></p>
]==]
}

Test{
  number = 580,
  section = "Images",
  input = [==[![](/url)
]==],
  result = [==[<p><img src="/url" alt="" /></p>
]==]
}

Test{
  number = 581,
  section = "Images",
  input = [==[![foo][bar]

[bar]: /url
]==],
  result = [==[<p><img src="/url" alt="foo" /></p>
]==]
}

Test{
  number = 582,
  section = "Images",
  input = [==[![foo][bar]

[BAR]: /url
]==],
  result = [==[<p><img src="/url" alt="foo" /></p>
]==]
}

Test{
  number = 583,
  section = "Images",
  input = [==[![foo][]

[foo]: /url "title"
]==],
  result = [==[<p><img src="/url" alt="foo" title="title" /></p>
]==]
}

Test{
  number = 584,
  section = "Images",
  input = [==[![*foo* bar][]

[*foo* bar]: /url "title"
]==],
  result = [==[<p><img src="/url" alt="foo bar" title="title" /></p>
]==]
}

Test{
  number = 585,
  section = "Images",
  input = [==[![Foo][]

[foo]: /url "title"
]==],
  result = [==[<p><img src="/url" alt="Foo" title="title" /></p>
]==]
}

Test{
  number = 586,
  section = "Images",
  input = [==[![foo] 
[]

[foo]: /url "title"
]==],
  result = [==[<p><img src="/url" alt="foo" title="title" />
[]</p>
]==]
}

Test{
  number = 587,
  section = "Images",
  input = [==[![foo]

[foo]: /url "title"
]==],
  result = [==[<p><img src="/url" alt="foo" title="title" /></p>
]==]
}

Test{
  number = 588,
  section = "Images",
  input = [==[![*foo* bar]

[*foo* bar]: /url "title"
]==],
  result = [==[<p><img src="/url" alt="foo bar" title="title" /></p>
]==]
}

Test{
  number = 589,
  section = "Images",
  input = [==[![[foo]]

[[foo]]: /url "title"
]==],
  result = [==[<p>![[foo]]</p>
<p>[[foo]]: /url &quot;title&quot;</p>
]==]
}

Test{
  number = 590,
  section = "Images",
  input = [==[![Foo]

[foo]: /url "title"
]==],
  result = [==[<p><img src="/url" alt="Foo" title="title" /></p>
]==]
}

Test{
  number = 591,
  section = "Images",
  input = [==[!\[foo]

[foo]: /url "title"
]==],
  result = [==[<p>![foo]</p>
]==]
}

Test{
  number = 592,
  section = "Images",
  input = [==[\![foo]

[foo]: /url "title"
]==],
  result = [==[<p>!<a href="/url" title="title">foo</a></p>
]==]
}

-- Autolinks
Test{
  number = 593,
  section = "Autolinks",
  input = [==[<http://foo.bar.baz>
]==],
  result = [==[<p><a href="http://foo.bar.baz">http://foo.bar.baz</a></p>
]==]
}

Test{
  number = 594,
  section = "Autolinks",
  input = [==[<http://foo.bar.baz/test?q=hello&id=22&boolean>
]==],
  result = [==[<p><a href="http://foo.bar.baz/test?q=hello&amp;id=22&amp;boolean">http://foo.bar.baz/test?q=hello&amp;id=22&amp;boolean</a></p>
]==]
}

Test{
  number = 595,
  section = "Autolinks",
  input = [==[<irc://foo.bar:2233/baz>
]==],
  result = [==[<p><a href="irc://foo.bar:2233/baz">irc://foo.bar:2233/baz</a></p>
]==]
}

Test{
  number = 596,
  section = "Autolinks",
  input = [==[<MAILTO:FOO@BAR.BAZ>
]==],
  result = [==[<p><a href="MAILTO:FOO@BAR.BAZ">MAILTO:FOO@BAR.BAZ</a></p>
]==]
}

Test{
  number = 597,
  section = "Autolinks",
  input = [==[<a+b+c:d>
]==],
  result = [==[<p><a href="a+b+c:d">a+b+c:d</a></p>
]==]
}

Test{
  number = 598,
  section = "Autolinks",
  input = [==[<made-up-scheme://foo,bar>
]==],
  result = [==[<p><a href="made-up-scheme://foo,bar">made-up-scheme://foo,bar</a></p>
]==]
}

Test{
  number = 599,
  section = "Autolinks",
  input = [==[<http://../>
]==],
  result = [==[<p><a href="http://../">http://../</a></p>
]==]
}

Test{
  number = 600,
  section = "Autolinks",
  input = [==[<localhost:5001/foo>
]==],
  result = [==[<p><a href="localhost:5001/foo">localhost:5001/foo</a></p>
]==]
}

Test{
  number = 601,
  section = "Autolinks",
  input = [==[<http://foo.bar/baz bim>
]==],
  result = [==[<p>&lt;http://foo.bar/baz bim&gt;</p>
]==]
}

Test{
  number = 602,
  section = "Autolinks",
  input = [==[<http://example.com/\[\>
]==],
  result = [==[<p><a href="http://example.com/%5C%5B%5C">http://example.com/\[\</a></p>
]==]
}

Test{
  number = 603,
  section = "Autolinks",
  input = [==[<foo@bar.example.com>
]==],
  result = [==[<p><a href="mailto:foo@bar.example.com">foo@bar.example.com</a></p>
]==]
}

Test{
  number = 604,
  section = "Autolinks",
  input = [==[<foo+special@Bar.baz-bar0.com>
]==],
  result = [==[<p><a href="mailto:foo+special@Bar.baz-bar0.com">foo+special@Bar.baz-bar0.com</a></p>
]==]
}

Test{
  number = 605,
  section = "Autolinks",
  input = [==[<foo\+@bar.example.com>
]==],
  result = [==[<p>&lt;foo+@bar.example.com&gt;</p>
]==]
}

Test{
  number = 606,
  section = "Autolinks",
  input = [==[<>
]==],
  result = [==[<p>&lt;&gt;</p>
]==]
}

Test{
  number = 607,
  section = "Autolinks",
  input = [==[< http://foo.bar >
]==],
  result = [==[<p>&lt; http://foo.bar &gt;</p>
]==]
}

Test{
  number = 608,
  section = "Autolinks",
  input = [==[<m:abc>
]==],
  result = [==[<p>&lt;m:abc&gt;</p>
]==]
}

Test{
  number = 609,
  section = "Autolinks",
  input = [==[<foo.bar.baz>
]==],
  result = [==[<p>&lt;foo.bar.baz&gt;</p>
]==]
}

Test{
  number = 610,
  section = "Autolinks",
  input = [==[http://example.com
]==],
  result = [==[<p>http://example.com</p>
]==]
}

Test{
  number = 611,
  section = "Autolinks",
  input = [==[foo@bar.example.com
]==],
  result = [==[<p>foo@bar.example.com</p>
]==]
}

-- Raw HTML
Test{
  number = 612,
  section = "Raw HTML",
  input = [==[<a><bab><c2c>
]==],
  result = [==[<p><a><bab><c2c></p>
]==]
}

Test{
  number = 613,
  section = "Raw HTML",
  input = [==[<a/><b2/>
]==],
  result = [==[<p><a/><b2/></p>
]==]
}

Test{
  number = 614,
  section = "Raw HTML",
  input = [==[<a  /><b2
data="foo" >
]==],
  result = [==[<p><a  /><b2
data="foo" ></p>
]==]
}

Test{
  number = 615,
  section = "Raw HTML",
  input = [==[<a foo="bar" bam = 'baz <em>"</em>'
_boolean zoop:33=zoop:33 />
]==],
  result = [==[<p><a foo="bar" bam = 'baz <em>"</em>'
_boolean zoop:33=zoop:33 /></p>
]==]
}

Test{
  number = 616,
  section = "Raw HTML",
  input = [==[Foo <responsive-image src="foo.jpg" />
]==],
  result = [==[<p>Foo <responsive-image src="foo.jpg" /></p>
]==]
}

Test{
  number = 617,
  section = "Raw HTML",
  input = [==[<33> <__>
]==],
  result = [==[<p>&lt;33&gt; &lt;__&gt;</p>
]==]
}

Test{
  number = 618,
  section = "Raw HTML",
  input = [==[<a h*#ref="hi">
]==],
  result = [==[<p>&lt;a h*#ref=&quot;hi&quot;&gt;</p>
]==]
}

Test{
  number = 619,
  section = "Raw HTML",
  input = [==[<a href="hi'> <a href=hi'>
]==],
  result = [==[<p>&lt;a href=&quot;hi'&gt; &lt;a href=hi'&gt;</p>
]==]
}

Test{
  number = 620,
  section = "Raw HTML",
  input = [==[< a><
foo><bar/ >
<foo bar=baz
bim!bop />
]==],
  result = [==[<p>&lt; a&gt;&lt;
foo&gt;&lt;bar/ &gt;
&lt;foo bar=baz
bim!bop /&gt;</p>
]==]
}

Test{
  number = 621,
  section = "Raw HTML",
  input = [==[<a href='bar'title=title>
]==],
  result = [==[<p>&lt;a href='bar'title=title&gt;</p>
]==]
}

Test{
  number = 622,
  section = "Raw HTML",
  input = [==[</a></foo >
]==],
  result = [==[<p></a></foo ></p>
]==]
}

Test{
  number = 623,
  section = "Raw HTML",
  input = [==[</a href="foo">
]==],
  result = [==[<p>&lt;/a href=&quot;foo&quot;&gt;</p>
]==]
}

Test{
  number = 624,
  section = "Raw HTML",
  input = [==[foo <!-- this is a
comment - with hyphen -->
]==],
  result = [==[<p>foo <!-- this is a
comment - with hyphen --></p>
]==]
}

Test{
  number = 625,
  section = "Raw HTML",
  input = [==[foo <!-- not a comment -- two hyphens -->
]==],
  result = [==[<p>foo &lt;!-- not a comment -- two hyphens --&gt;</p>
]==]
}

Test{
  number = 626,
  section = "Raw HTML",
  input = [==[foo <!--> foo -->

foo <!-- foo--->
]==],
  result = [==[<p>foo &lt;!--&gt; foo --&gt;</p>
<p>foo &lt;!-- foo---&gt;</p>
]==]
}

Test{
  number = 627,
  section = "Raw HTML",
  input = [==[foo <?php echo $a; ?>
]==],
  result = [==[<p>foo <?php echo $a; ?></p>
]==]
}

Test{
  number = 628,
  section = "Raw HTML",
  input = [==[foo <!ELEMENT br EMPTY>
]==],
  result = [==[<p>foo <!ELEMENT br EMPTY></p>
]==]
}

Test{
  number = 629,
  section = "Raw HTML",
  input = [==[foo <![CDATA[>&<]]>
]==],
  result = [==[<p>foo <![CDATA[>&<]]></p>
]==]
}

Test{
  number = 630,
  section = "Raw HTML",
  input = [==[foo <a href="&ouml;">
]==],
  result = [==[<p>foo <a href="&ouml;"></p>
]==]
}

Test{
  number = 631,
  section = "Raw HTML",
  input = [==[foo <a href="\*">
]==],
  result = [==[<p>foo <a href="\*"></p>
]==]
}

Test{
  number = 632,
  section = "Raw HTML",
  input = [==[<a href="\"">
]==],
  result = [==[<p>&lt;a href=&quot;&quot;&quot;&gt;</p>
]==]
}

-- Hard line breaks
Test{
  number = 633,
  section = "Hard line breaks",
  input = [==[foo  
baz
]==],
  result = [==[<p>foo<br />
baz</p>
]==]
}

Test{
  number = 634,
  section = "Hard line breaks",
  input = [==[foo\
baz
]==],
  result = [==[<p>foo<br />
baz</p>
]==]
}

Test{
  number = 635,
  section = "Hard line breaks",
  input = [==[foo       
baz
]==],
  result = [==[<p>foo<br />
baz</p>
]==]
}

Test{
  number = 636,
  section = "Hard line breaks",
  input = [==[foo  
     bar
]==],
  result = [==[<p>foo<br />
bar</p>
]==]
}

Test{
  number = 637,
  section = "Hard line breaks",
  input = [==[foo\
     bar
]==],
  result = [==[<p>foo<br />
bar</p>
]==]
}

Test{
  number = 638,
  section = "Hard line breaks",
  input = [==[*foo  
bar*
]==],
  result = [==[<p><em>foo<br />
bar</em></p>
]==]
}

Test{
  number = 639,
  section = "Hard line breaks",
  input = [==[*foo\
bar*
]==],
  result = [==[<p><em>foo<br />
bar</em></p>
]==]
}

Test{
  number = 640,
  section = "Hard line breaks",
  input = [==[`code  
span`
]==],
  result = [==[<p><code>code   span</code></p>
]==]
}

Test{
  number = 641,
  section = "Hard line breaks",
  input = [==[`code\
span`
]==],
  result = [==[<p><code>code\ span</code></p>
]==]
}

Test{
  number = 642,
  section = "Hard line breaks",
  input = [==[<a href="foo  
bar">
]==],
  result = [==[<p><a href="foo  
bar"></p>
]==]
}

Test{
  number = 643,
  section = "Hard line breaks",
  input = [==[<a href="foo\
bar">
]==],
  result = [==[<p><a href="foo\
bar"></p>
]==]
}

Test{
  number = 644,
  section = "Hard line breaks",
  input = [==[foo\
]==],
  result = [==[<p>foo\</p>
]==]
}

Test{
  number = 645,
  section = "Hard line breaks",
  input = [==[foo  
]==],
  result = [==[<p>foo</p>
]==]
}

Test{
  number = 646,
  section = "Hard line breaks",
  input = [==[### foo\
]==],
  result = [==[<h3>foo\</h3>
]==]
}

Test{
  number = 647,
  section = "Hard line breaks",
  input = [==[### foo  
]==],
  result = [==[<h3>foo</h3>
]==]
}

-- Soft line breaks
Test{
  number = 648,
  section = "Soft line breaks",
  input = [==[foo
baz
]==],
  result = [==[<p>foo
baz</p>
]==]
}

Test{
  number = 649,
  section = "Soft line breaks",
  input = [==[foo 
 baz
]==],
  result = [==[<p>foo
baz</p>
]==]
}

-- Textual content
Test{
  number = 650,
  section = "Textual content",
  input = [==[hello $.;'there
]==],
  result = [==[<p>hello $.;'there</p>
]==]
}

Test{
  number = 651,
  section = "Textual content",
  input = [==[Foo χρῆν
]==],
  result = [==[<p>Foo χρῆν</p>
]==]
}

Test{
  number = 652,
  section = "Textual content",
  input = [==[Multiple     spaces
]==],
  result = [==[<p>Multiple     spaces</p>
]==]
}

-- Appendix: A parsing strategy
-- Overview
-- Phase 1: block structure
-- Phase 2: inline structure
-- An algorithm for parsing nested emphasis and links
-- *look for link or image*
-- *process emphasis*
