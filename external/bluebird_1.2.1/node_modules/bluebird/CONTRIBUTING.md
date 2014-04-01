# Contributing to bluebird

1. [Directory structure](#directory-structure)
2. [Style guide](#style-guide)
3. [Scripts and macros](#scripts-and-macros)
4. [JSHint](#jshint)
5. [Testing](#testing)

## Directory structure

- `/benchmark` contains benchmark scripts and stats of benchmarks

- `/browser` contains scripts and output for browser testing environment

- `/js` contains automatically generated build output. **NOTE** never commit any changes to these files to git.

    - `/js/browser` contains a file suitable for use in browsers
    - `/js/main` contains the main build to be used with node. The npm package points to /js/main/bluebird.js
    - `/js/debug` contains the debug build to be used with node. Used when running tests
    - `/js/zalgo` contains the zalgo build not to be used by any mortals.

- `/node_modules` contains development dependencies such as grunt

- `/src` contains the source code

- `/test/mocha` contains tests using the mocha testing framework

## Scripts and macros

Scripts and macros are necessary for the code the code to remain readable and performant. For example, there is no way to turn the `arguments` object into an array without using a build step macro unless you want to compromise readability or performance.

`/ast_passes.js` contains functions called ast passes that will parse input source code into an AST, modify it in some way and spit out new source code with the changes reflected.

`/src/constants.js` contains declarations for constants that will be inlined in the resulting code during in the build step. JavaScript lacks a way to express constants, particularly if you are expecting the performance implications.

`/Gruntfile.js` contains task definitions to be used with the Grunt build framework. It for example sets up source code transformations.

`/bench` a bash script to run benchmarks.

`/mocharun.js` a hack script to make mocha work when running multiple tests in parallel processes

## JSHint

Due to JSHint globals being dynamic, the JSHint rules are declared in `/Gruntfile.js`.

## Style guide

Use the same style as is used in the surrounding code.

###Whitespace

- No more than 80 columns per line
- 4 space indentation
- No trailing whitespace
- LF at end of files
- Curly braces can be left out of single statement `if/else/else if`s when it is obvious there will never be multiple statements such as null check at the top of a function for an early return.
- Add an additional new line between logical sections of code.

###Variables

- Use multiple `var` statements instead of a single one with comma separator. Do not declare variables until you need them.

###Equality and type checks

- Always use `===` except when checking for null or undefined. To check for null or undefined, use `x == null`.
- For checks that can be done with `typeof`: do not make helper functions, save results of `typeof` to a variable or make the type string a non-constant. Always write the check in the form `typeof expression === "constant string"` even if it feels like repeating yourself.

## Testing
