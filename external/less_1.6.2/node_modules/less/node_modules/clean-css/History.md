[2.0.8 / 2014-02-07](https://github.com/GoalSmashers/clean-css/compare/v2.0.7...v2.0.8)
==================

* Fixed issue [#232](https://github.com/GoalSmashers/clean-css/issues/232) - edge case in non-adjacent selectors merging.

[2.0.7 / 2014-01-16](https://github.com/GoalSmashers/clean-css/compare/v2.0.6...v2.0.7)
==================

* Fixed issue [#208](https://github.com/GoalSmashers/clean-css/issues/208) - don't swallow @page and @viewport.

[2.0.6 / 2014-01-04](https://github.com/GoalSmashers/clean-css/compare/v2.0.5...v2.0.6)
==================

* Fixed issue [#198](https://github.com/GoalSmashers/clean-css/issues/198) - process comments and `@import`s correctly.
* Fixed issue [#205](https://github.com/GoalSmashers/clean-css/issues/205) - freeze on broken @import declaration.

[2.0.5 / 2014-01-03](https://github.com/GoalSmashers/clean-css/compare/v2.0.4...v2.0.5)
==================

* Fixed issue [#199](https://github.com/GoalSmashers/clean-css/issues/199) - keep line breaks with no advanced optimizations.
* Fixed issue [#203](https://github.com/GoalSmashers/clean-css/issues/203) - Buffer as a first argument to minify method.

[2.0.4 / 2013-12-19](https://github.com/GoalSmashers/clean-css/compare/v2.0.3...v2.0.4)
==================

* Fixed issue [#193](https://github.com/GoalSmashers/clean-css/issues/193) - HSL color space normalization.

[2.0.3 / 2013-12-18](https://github.com/GoalSmashers/clean-css/compare/v2.0.2...v2.0.3)
==================

* Fixed issue [#191](https://github.com/GoalSmashers/clean-css/issues/191) - leading numbers in font/animation names.
* Fixed issue [#192](https://github.com/GoalSmashers/clean-css/issues/192) - many `@import`s inside a comment.

[2.0.2 / 2013-11-18](https://github.com/GoalSmashers/clean-css/compare/v2.0.1...v2.0.2)
==================

* Fixed issue [#177](https://github.com/GoalSmashers/clean-css/issues/177) - process broken content correctly.

[2.0.1 / 2013-11-14](https://github.com/GoalSmashers/clean-css/compare/v2.0.0...v2.0.1)
==================

* Fixed issue [#176](https://github.com/GoalSmashers/clean-css/issues/176) - hangs on `undefined` keyword.

[2.0.0 / 2013-11-04](https://github.com/GoalSmashers/clean-css/compare/v1.1.7...v2.0.0)
==================

* Adds simplified and more advanced text escaping / restoring via `EscapeStore` class.
* Adds simplified and much faster empty elements removal.
* Adds missing `@import` processing to our benchmark (run via `npm run bench`).
* Adds CSS tokenizer which will make it possible to optimize content by reordering and/or merging selectors.
* Adds basic optimizer removing duplicate selectors from a list.
* Adds merging duplicate properties within a single selector's body.
* Adds merging adjacent selectors within a scope (single and multiple ones).
* Changes behavior of `--keep-line-breaks`/`keepBreaks` option to keep breaks after trailing braces only.
* Makes all multiple selectors ordered alphabetically (aids merging).
* Adds property overriding so more coarse properties override more granular ones.
* Adds reducing non-adjacent selectors.
* Adds `--skip-advanced`/`noAdvanced` switch to disable advanced optimizations.
* Adds reducing non-adjacent selectors when overridden by more complex selectors.
* Fixed issue [#138](https://github.com/GoalSmashers/clean-css/issues/138) - makes CleanCSS interface OO.
* Fixed issue [#139](https://github.com/GoalSmashers/clean-css/issues/138) - consistent error & warning handling.
* Fixed issue [#145](https://github.com/GoalSmashers/clean-css/issues/145) - debug mode in library too.
* Fixed issue [#157](https://github.com/GoalSmashers/clean-css/issues/157) - gets rid of `removeEmpty` option.
* Fixed issue [#159](https://github.com/GoalSmashers/clean-css/issues/159) - escaped quotes inside content.
* Fixed issue [#162](https://github.com/GoalSmashers/clean-css/issues/162) - strip quotes from Base64 encoded URLs.
* Fixed issue [#166](https://github.com/GoalSmashers/clean-css/issues/166) - `debug` formatting in CLI
* Fixed issue [#167](https://github.com/GoalSmashers/clean-css/issues/167) - `background:transparent` minification.

[1.1.7 / 2013-10-28](https://github.com/GoalSmashers/clean-css/compare/v1.1.6...v1.1.7)
==================

* Fixed issue [#156](https://github.com/GoalSmashers/clean-css/issues/156) - `@import`s inside comments.

[1.1.6 / 2013-10-26](https://github.com/GoalSmashers/clean-css/compare/v1.1.5...v1.1.6)
==================

* Fixed issue [#155](https://github.com/GoalSmashers/clean-css/issues/155) - broken irregular CSS content.

[1.1.5 / 2013-10-24](https://github.com/GoalSmashers/clean-css/compare/v1.1.4...v1.1.5)
==================

* Fixed issue [#153](https://github.com/GoalSmashers/clean-css/issues/153) - keepSpecialComments 0/1 as a string.

[1.1.4 / 2013-10-23](https://github.com/GoalSmashers/clean-css/compare/v1.1.3...v1.1.4)
==================

* Fixed issue [#152](https://github.com/GoalSmashers/clean-css/issues/152) - adds an option to disable rebasing.

[1.1.3 / 2013-10-04](https://github.com/GoalSmashers/clean-css/compare/v1.1.2...v1.1.3)
==================

* Fixed issue [#150](https://github.com/GoalSmashers/clean-css/issues/150) - minifying `background:none`.

[1.1.2 / 2013-09-29](https://github.com/GoalSmashers/clean-css/compare/v1.1.1...v1.1.2)
==================

* Fixed issue [#149](https://github.com/GoalSmashers/clean-css/issues/149) - shorthand font property.

[1.1.1 / 2013-09-07](https://github.com/GoalSmashers/clean-css/compare/v1.1.0...v1.1.1)
==================

* Fixed issue [#144](https://github.com/GoalSmashers/clean-css/issues/144) - skip URLs rebasing by default.

[1.1.0 / 2013-09-06](https://github.com/GoalSmashers/clean-css/compare/v1.0.12...v1.1.0)
==================

* Renamed lib's `debug` option to `benchmark` when doing per-minification benchmarking.
* Added simplified comments processing & imports.
* Fixed issue [#43](https://github.com/GoalSmashers/clean-css/issues/43) - `--debug` switch for minification stats.
* Fixed issue [#65](https://github.com/GoalSmashers/clean-css/issues/65) - full color name / hex shortening.
* Fixed issue [#84](https://github.com/GoalSmashers/clean-css/issues/84) - support for `@import` with media queries.
* Fixed issue [#124](https://github.com/GoalSmashers/clean-css/issues/124) - raise error on broken imports.
* Fixed issue [#126](https://github.com/GoalSmashers/clean-css/issues/126) - proper CSS expressions handling.
* Fixed issue [#129](https://github.com/GoalSmashers/clean-css/issues/129) - rebasing imported URLs.
* Fixed issue [#130](https://github.com/GoalSmashers/clean-css/issues/130) - better code modularity.
* Fixed issue [#135](https://github.com/GoalSmashers/clean-css/issues/135) - require node.js 0.8+.

[1.0.12 / 2013-07-19](https://github.com/GoalSmashers/clean-css/compare/v1.0.11...v1.0.12)
===================

* Fixed issue [#121](https://github.com/GoalSmashers/clean-css/issues/121) - ability to skip `@import` processing.

[1.0.11 / 2013-07-08](https://github.com/GoalSmashers/clean-css/compare/v1.0.10...v1.0.11)
===================

* Fixed issue [#117](https://github.com/GoalSmashers/clean-css/issues/117) - line break escaping in comments.

[1.0.10 / 2013-06-13](https://github.com/GoalSmashers/clean-css/compare/v1.0.9...v1.0.10)
===================

* Fixed issue [#114](https://github.com/GoalSmashers/clean-css/issues/114) - comments in imported stylesheets.

[1.0.9 / 2013-06-11](https://github.com/GoalSmashers/clean-css/compare/v1.0.8...v1.0.9)
==================

* Fixed issue [#113](https://github.com/GoalSmashers/clean-css/issues/113) - `@import` in comments.

[1.0.8 / 2013-06-10](https://github.com/GoalSmashers/clean-css/compare/v1.0.7...v1.0.8)
==================

* Fixed issue [#112](https://github.com/GoalSmashers/clean-css/issues/112) - reducing `box-shadow` zeros.

[1.0.7 / 2013-06-05](https://github.com/GoalSmashers/clean-css/compare/v1.0.6...v1.0.7)
==================

* Support for `@import` URLs starting with `//`.
  By [@petetak](https://github.com/petetak).

[1.0.6 / 2013-06-04](https://github.com/GoalSmashers/clean-css/compare/v1.0.5...v1.0.6)
==================

* Fixed issue [#110](https://github.com/GoalSmashers/clean-css/issues/110) - data URIs in URLs.

[1.0.5 / 2013-05-26](https://github.com/GoalSmashers/clean-css/compare/v1.0.4...v1.0.5)
==================

* Fixed issue [#107](https://github.com/GoalSmashers/clean-css/issues/107) - data URIs in imported stylesheets.

1.0.4 / 2013-05-23
==================

* Rewrite relative URLs in imported stylesheets.
  By [@bluej100](https://github.com/bluej100).

1.0.3 / 2013-05-20
==================

* Support alternative `@import` syntax with file name not wrapped inside `url()` statement.
  By [@bluej100](https://github.com/bluej100).

1.0.2 / 2013-04-29
==================

* Fixed issue [#97](https://github.com/GoalSmashers/clean-css/issues/97) - `--remove-empty` & FontAwesome.

1.0.1 / 2013-04-08
==================

* Do not pick up `bench` and `test` while building `npm` package.
  By [@sindresorhus](https://https://github.com/sindresorhus).

1.0.0 / 2013-03-30
==================

* Fixed issue [#2](https://github.com/GoalSmashers/clean-css/issues/2) - resolving `@import` rules.
* Fixed issue [#44](https://github.com/GoalSmashers/clean-css/issues/44) - examples in `--help`.
* Fixed issue [#46](https://github.com/GoalSmashers/clean-css/issues/46) - preserving special characters in URLs and attributes.
* Fixed issue [#80](https://github.com/GoalSmashers/clean-css/issues/80) - quotation in multi line strings.
* Fixed issue [#83](https://github.com/GoalSmashers/clean-css/issues/83) - HSL to hex color conversions.
* Fixed issue [#86](https://github.com/GoalSmashers/clean-css/issues/86) - broken `@charset` replacing.
* Fixed issue [#88](https://github.com/GoalSmashers/clean-css/issues/88) - removes space in `! important`.
* Fixed issue [#92](https://github.com/GoalSmashers/clean-css/issues/92) - uppercase hex to short versions.

0.10.2 / 2013-03-19
===================

* Fixed issue [#79](https://github.com/GoalSmashers/clean-css/issues/79) - node.js 0.10.x compatibility.

0.10.1 / 2013-02-14
===================

* Fixed issue [#66](https://github.com/GoalSmashers/clean-css/issues/66) - line breaks without extra spaces should
  be handled correctly.

0.10.0 / 2013-02-09
===================

* Switched from [optimist](https://github.com/substack/node-optimist) to
  [commander](https://github.com/visionmedia/commander.js) for CLI processing.
* Changed long options from `--removeempty` to `--remove-empty` and from `--keeplinebreaks` to `--keep-line-breaks`.
* Fixed performance issue with replacing multiple `@charset` declarations and issue
  with line break after `@charset` when using `keepLineBreaks` option. By [@rrjaime](https://github.com/rrjamie).
* Removed Makefile in favor to `npm run` commands (e.g. `make check` -> `npm run check`).
* Fixed issue [#47](https://github.com/GoalSmashers/clean-css/issues/47) - commandline issues on Windows.
* Fixed issue [#49](https://github.com/GoalSmashers/clean-css/issues/49) - remove empty selectors from media query.
* Fixed issue [#52](https://github.com/GoalSmashers/clean-css/issues/52) - strip fraction zeros if not needed.
* Fixed issue [#58](https://github.com/GoalSmashers/clean-css/issues/58) - remove colon where possible.
* Fixed issue [#59](https://github.com/GoalSmashers/clean-css/issues/59) - content property handling.

0.9.1 / 2012-12-19
==================

* Fixed issue [#37](https://github.com/GoalSmashers/clean-css/issues/37) - converting
  `white` and other colors in class names (reported by [@malgorithms](https://github.com/malgorithms)).

0.9.0 / 2012-12-15
==================

* Added stripping quotation from font names (if possible).
* Added stripping quotation from `@keyframes` declaration, `animation` and
  `animation-name` property.
* Added stripping quotations from attributes' value (e.g. `[data-target='x']`).
* Added better hex->name and name->hex color shortening.
* Added `font: normal` and `font: bold` shortening the same way as `font-weight` is.
* Refactored shorthand selectors and added `border-radius`, `border-style`
  and `border-color` shortening.
* Added `margin`, `padding` and `border-width` shortening.
* Added removing line break after commas.
* Fixed removing whitespace inside media query definition.
* Added removing line breaks after a comma, so all declarations are one-liners now.
* Speed optimizations (~10% despite many new features).
* Added [JSHint](https://github.com/jshint/jshint/) validation rules via `make check`.

0.8.3 / 2012-11-29
==================

* Fixed HSL/HSLA colors processing.

0.8.2 / 2012-10-31
==================

* Fixed shortening hex colors and their relation to hashes in URLs.
* Cleanup by [@XhmikosR](https://github.com/XhmikosR).

0.8.1 / 2012-10-28
==================

* Added better zeros processing for `rect(...)` syntax (clip property).

0.8.0 / 2012-10-21
==================

* Added removing URLs quotation if possible.
* Rewrote breaks processing.
* Added `keepBreaks`/`-b` option to keep line breaks in the minimized file.
* Reformatted [lib/clean.js](/lib/clean.js) so it's easier to follow the rules.
* Minimized test data is now minimized with line breaks so it's easier to
  compare the changes line by line.

0.7.0 / 2012-10-14
==================

* Added stripping special comments to CLI (`--s0` and `--s1` options).
* Added stripping special comments to programmatic interface
  (`keepSpecialComments` option).

0.6.0 / 2012-08-05
==================

* Full Windows support with tests (./test.bat).

0.5.0 / 2012-08-02
==================

* Made path to vows local.
* Explicit node.js 0.6 requirement.

0.4.2 / 2012-06-28
==================

* Updated binary `-v` option (version).
* Updated binary to output help when no options given (but not in piped mode).
* Added binary tests.

0.4.1 / 2012-06-10
==================

* Fixed stateless mode where calling `CleanCSS#process` directly was giving
  errors (reported by [@facelessuser](https://github.com/facelessuser)).

0.4.0 / 2012-06-04
==================

* Speed improvements up to 4x thanks to the rewrite of comments and CSS' content
  processing.
* Stripping empty CSS tags is now optional (see [bin/cleancss](/bin/cleancss) for details).
* Improved debugging mode (see [test/bench.js](/test/bench.js))
* Added `make bench` for a one-pass benchmark.

0.3.3 / 2012-05-27
==================

* Fixed tests, [package.json](/package.json) for development, and regex
  for removing empty declarations (thanks to [@vvo](https://github.com/vvo)).

0.3.2 / 2012-01-17
==================

* Fixed output method under node.js 0.6 which incorrectly tried to close
  `process.stdout`.

0.3.1 / 2011-12-16
==================

* Fixed cleaning up `0 0 0 0` expressions.

0.3.0 / 2011-11-29
==================

* Clean-css requires node.js 0.4.0+ to run.
* Removed node.js's 0.2.x 'sys' package dependency
  (thanks to [@jmalonzo](https://github.com/jmalonzo) for a patch).

0.2.6 / 2011-11-27
==================

* Fixed expanding `+` signs in `calc()` when mixed up with adjacent `+` selector.

0.2.5 / 2011-11-27
==================

* Fixed issue with cleaning up spaces inside `calc`/`-moz-calc` declarations
  (thanks to [@cvan](https://github.com/cvan) for reporting it).
* Fixed converting `#f00` to `red` in borders and gradients.

0.2.4 / 2011-05-25
==================

* Fixed problem with expanding `none` to `0` in partial/full background
  declarations.
* Fixed including clean-css library from binary (global to local).

0.2.3 / 2011-04-18
==================

* Fixed problem with optimizing IE filters.

0.2.2 / 2011-04-17
==================

* Fixed problem with space before color in `border` property.

0.2.1 / 2011-03-19
==================

* Added stripping space before `!important` keyword.
* Updated repository location and author information in [package.json](/package.json).

0.2.0 / 2011-03-02
==================

* Added options parsing via optimist.
* Changed code inclusion (thus the version bump).

0.1.0 / 2011-02-27
==================

* First version of clean-css library.
* Implemented all basic CSS transformations.
