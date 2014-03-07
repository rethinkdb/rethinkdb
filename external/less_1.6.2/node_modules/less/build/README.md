# [Less.js v<%= pkg.version %>](http://lesscss.org)

> The **dynamic** stylesheet language. [http://lesscss.org](http://lesscss.org).

This is the JavaScript, and now official, stable version of LESS.


## Getting Started

Options for adding Less.js to your project:

* Install with [NPM](https://npmjs.org/): `npm install less`
* [Download the latest release][download]
* Clone the repo: `git clone git://github.com/less/less.js.git`



## Feature Highlights
LESS extends CSS with dynamic features such as:

* [nesting](#nesting)
* [variables](#variables)
* [operations](#operations)
* [mixins](#mixins)
* [extend](#extend) (selector inheritance)

To learn about the many other features Less.js has to offer please visit [http://lesscss.org](http://lesscss.org) and [the Less.js wiki][wiki]


### Examples
#### nesting
Take advantage of nesting to make code more readable and maintainable. This:

```less
.nav > li > a {
  border: 1px solid #f5f5f5;
  &:hover {
    border-color: #ddd;
  }
}
```

renders to:

```css
.nav > li > a {
  border: 1px solid #f5f5f5;
}
.nav > li > a:hover {
  border-color: #ddd;
}
```


#### variables
Updated commonly used values from a single location.

```less
// Variables ("inline" comments like this can be used)
@link-color:  #428bca; // appears as "sea blue"

/* Or "block comments" that span
   multiple lines, like this */
a {
  color: @link-color; // use the variable in styles
}
```

Variables can also be used in `@import` statements, URLs, selector names, and more.



#### operations
Continuing with the same example above, we can use our variables even easier to maintain with _operations_, which enables the use of addition, subraction, multiplication and division in your styles:

```less
// Variables
@link-color:        #428bca;
@link-color-hover:  darken(@link-color, 10%);

// Styles
a {
  color: @link-color;
}
a:hover {
  color: @link-color-hover;
}
```
renders to:

```css
a {
  color: #428bca;
}
a:hover {
  color: #3071a9;
}
```

#### mixins
##### "implicit" mixins
Mixins enable you to apply the styles of one selector inside another selector like this:

```less
// Variables
@link-color:        #428bca;

// Any "regular" class...
.link {
  color: @link-color;
}
a {
  font-weight: bold;
  .link; // ...can be used as an "implicit" mixin
}
```

renders to:

```css
.link {
  color: #428bca;
}
a {
  font-weight: bold;
  color: #428bca;
}
```

So any selector can be an "implicit mixin". We'll show you a DRYer way to do this below.



##### parametric mixins
Mixins can also accept parameters:

```less
// Transition mixin
.transition(@transition) {
  -webkit-transition: @transition;
     -moz-transition: @transition;
       -o-transition: @transition;
          transition: @transition;
}
```

used like this:

```less
// Variables
@link-color:        #428bca;
@link-color-hover:  darken(@link-color, 10%);

//Transition mixin would be anywhere here

a {
  font-weight: bold;
  color: @link-color;
  .transition(color .2s ease-in-out);
  // Hover state
  &:hover {
    color: @link-color-hover;
  }
}
```

renders to:

```css
a {
  font-weight: bold;
  color: #428bca;
  -webkit-transition: color 0.2s ease-in-out;
     -moz-transition: color 0.2s ease-in-out;
       -o-transition: color 0.2s ease-in-out;
          transition: color 0.2s ease-in-out;
}
a:hover {
  color: #3071a9;
}
```


#### extend
The `extend` feature can be thought of as the _inverse_ of mixins. It accomplishes the goal of "borrowing styles", but rather than copying all the rules of _Selector A_ over to _Selector B_, `extend` copies the name of the _inheriting selector_ (_Selector B_) over to the _extending selector_ (_Selector A_). So continuing with the example used for [mixins](#mixins) above, extend works like this:

```less
// Variables
@link-color:        #428bca;

.link {
  color: @link-color;
}
a:extend(.link) {
  font-weight: bold;
}
// Can also be written as
a {
  &:extend(.link);
  font-weight: bold;
}
```

renders to:

```css
.link, a {
  color: #428bca;
}
a {
  font-weight: bold;
}
```

## Usage

### Compiling and Parsing
Invoke the compiler from node:

```javascript
var less = require('less');

less.render('.class { width: (1 + 1) }', function (e, css) {
    console.log(css);
});
```

Outputs:

```css
.class {
  width: 2;
}
```

You may also manually invoke the parser and compiler:

```javascript
var parser = new(less.Parser);

parser.parse('.class { width: (1 + 1) }', function (err, tree) {
    if (err) { return console.error(err) }
    console.log(tree.toCSS());
});
```


### Configuration
You may also pass options to the compiler:

```javascript
var parser = new(less.Parser)({
    paths: ['.', './src/less'], // Specify search paths for @import directives
    filename: 'style.less'      // Specify a filename, for better error messages
});

parser.parse('.class { width: (1 + 1) }', function (e, tree) {
    tree.toCSS({ compress: true }); // Minify CSS output
});
```

## More information

For general information on the language, configuration options or usage visit [lesscss.org](http://lesscss.org) or [the less wiki][wiki].

Here are other resources for using Less.js:

* [stackoverflow.com][so] is a great place to get answers about Less.
* [node.js tools](https://github.com/less/less.js/wiki/Converting-LESS-to-CSS) for converting Less to CSS
* [GUI compilers for Less](https://github.com/less/less.js/wiki/GUI-compilers-that-use-LESS.js)
* [Less.js Issues][issues] for reporting bugs



## Contributing
Please read [CONTRIBUTING.md](./CONTRIBUTING.md). Add unit tests for any new or changed functionality. Lint and test your code using [Grunt](http://gruntjs.com/).

### Reporting Issues

Before opening any issue, please search for existing issues and read the [Issue Guidelines](https://github.com/necolas/issue-guidelines), written by [Nicolas Gallagher](https://github.com/necolas/). After that if you find a bug or would like to make feature request, [please open a new issue][issues].

Please report documentation issues in [the documentation project](https://github.com/less/less-docs).

### Development

#### Install Less.js

Start by either [downloading this project][download] manually, or in the command line:

```shell
git clone https://github.com/less/less.js.git "less"
```
and then `cd less`.


#### Install dependencies

To install all the dependencies for less development, run:

```shell
npm install
```

If you haven't run grunt before, install grunt-cli globally so you can just run `grunt`

```shell
npm install grunt-cli -g
```

You should now be able to build Less.js, run tests, benchmarking, and other tasks listed in the Gruntfile.

## Using Less.js Grunt

Tests, benchmarking and building is done using Grunt `<%= pkg.devDependencies.grunt %>`. If you haven't used [Grunt](http://gruntjs.com/) before, be sure to check out the [Getting Started](http://gruntjs.com/getting-started) guide, as it explains how to install and use Grunt plugins, which are necessary for development with Less.js.

The Less.js [Gruntfile](Gruntfile.js) is configured with the following "convenience tasks" :

#### test - `grunt`
Runs jshint, nodeunit and headless jasmine tests using [phantomjs](http://code.google.com/p/phantomjs/). You must have phantomjs installed for the jasmine tests to run.

#### test - `grunt benchmark`
Runs the benchmark suite.

#### build for testing browser - 'grunt browser'
This builds less.js and puts it in 'test/browser/less.js'

#### build - `grunt stable | grunt beta | grunt alpha`
Builds Less.js from from the `/lib/less` source files. This is done by the developer releasing a new release, do not do this if you are creating a pull request.

#### readme - `grunt readme`
Build the README file from [a template](build/README.md) to ensure that metadata is up-to-date and (more likely to be) correct.

Please review the [Gruntfile](Gruntfile.js) to become acquainted with the other available tasks.

**Please note** that if you have any issues installing dependencies or running any of the Gruntfile commands, please make sure to uninstall any previous versions, both in the local node_modules directory, and clear your global npm cache, and then try running `npm install` again. After that if you still have issues, please let us know about it so we can help.


## Release History
See the [changelog](CHANGELOG.md)

## [License](LICENSE)

Copyright (c) 2009-<%= grunt.template.today("yyyy") %> [Alexis Sellier](http://cloudhead.io/) & The Core Less Team
Licensed under the [Apache License](LICENSE).


[so]: http://stackoverflow.com/questions/tagged/twitter-bootstrap+less "StackOverflow.com"
[issues]: https://github.com/less/less.js/issues "GitHub Issues for Less.js"
[wiki]: https://github.com/less/less.js/wiki "The official wiki for Less.js"
[download]: https://github.com/less/less.js/zipball/master "Download Less.js"