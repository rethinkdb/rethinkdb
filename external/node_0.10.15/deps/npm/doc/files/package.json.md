package.json(5) -- Specifics of npm's package.json handling
===========================================================

## DESCRIPTION

This document is all you need to know about what's required in your package.json
file.  It must be actual JSON, not just a JavaScript object literal.

A lot of the behavior described in this document is affected by the config
settings described in `npm-config(7)`.

## DEFAULT VALUES

npm will default some values based on package contents.

* `"scripts": {"start": "node server.js"}`

  If there is a `server.js` file in the root of your package, then npm
  will default the `start` command to `node server.js`.

* `"scripts":{"preinstall": "node-waf clean || true; node-waf configure build"}`

  If there is a `wscript` file in the root of your package, npm will
  default the `preinstall` command to compile using node-waf.

* `"scripts":{"preinstall": "node-gyp rebuild"}`

  If there is a `binding.gyp` file in the root of your package, npm will
  default the `preinstall` command to compile using node-gyp.

* `"contributors": [...]`

  If there is an `AUTHORS` file in the root of your package, npm will
  treat each line as a `Name <email> (url)` format, where email and url
  are optional.  Lines which start with a `#` or are blank, will be
  ignored.

## name

The *most* important things in your package.json are the name and version fields.
Those are actually required, and your package won't install without
them.  The name and version together form an identifier that is assumed
to be completely unique.  Changes to the package should come along with
changes to the version.

The name is what your thing is called.  Some tips:

* Don't put "js" or "node" in the name.  It's assumed that it's js, since you're
  writing a package.json file, and you can specify the engine using the "engines"
  field.  (See below.)
* The name ends up being part of a URL, an argument on the command line, and a
  folder name. Any name with non-url-safe characters will be rejected.
  Also, it can't start with a dot or an underscore.
* The name will probably be passed as an argument to require(), so it should
  be something short, but also reasonably descriptive.
* You may want to check the npm registry to see if there's something by that name
  already, before you get too attached to it.  http://registry.npmjs.org/

## version

The *most* important things in your package.json are the name and version fields.
Those are actually required, and your package won't install without
them.  The name and version together form an identifier that is assumed
to be completely unique.  Changes to the package should come along with
changes to the version.

Version must be parseable by
[node-semver](https://github.com/isaacs/node-semver), which is bundled
with npm as a dependency.  (`npm install semver` to use it yourself.)

Here's how npm's semver implementation deviates from what's on semver.org:

* Versions can start with "v"
* A numeric item separated from the main three-number version by a hyphen
  will be interpreted as a "build" number, and will *increase* the version.
  But, if the tag is not a number separated by a hyphen, then it's treated
  as a pre-release tag, and is *less than* the version without a tag.
  So, `0.1.2-7 > 0.1.2-7-beta > 0.1.2-6 > 0.1.2 > 0.1.2beta`

This is a little bit confusing to explain, but matches what you see in practice
when people create tags in git like "v1.2.3" and then do "git describe" to generate
a patch version.

## description

Put a description in it.  It's a string.  This helps people discover your
package, as it's listed in `npm search`.

## keywords

Put keywords in it.  It's an array of strings.  This helps people
discover your package as it's listed in `npm search`.

## homepage

The url to the project homepage.

**NOTE**: This is *not* the same as "url".  If you put a "url" field,
then the registry will think it's a redirection to your package that has
been published somewhere else, and spit at you.

Literally.  Spit.  I'm so not kidding.

## bugs

The url to your project's issue tracker and / or the email address to which
issues should be reported. These are helpful for people who encounter issues
with your package.

It should look like this:

    { "url" : "http://github.com/owner/project/issues"
    , "email" : "project@hostname.com"
    }

You can specify either one or both values. If you want to provide only a url,
you can specify the value for "bugs" as a simple string instead of an object.

If a url is provided, it will be used by the `npm bugs` command.

## license

You should specify a license for your package so that people know how they are
permitted to use it, and any restrictions you're placing on it.

The simplest way, assuming you're using a common license such as BSD or MIT, is
to just specify the name of the license you're using, like this:

    { "license" : "BSD" }

If you have more complex licensing terms, or you want to provide more detail
in your package.json file, you can use the more verbose plural form, like this:

    "licenses" : [
      { "type" : "MyLicense"
      , "url" : "http://github.com/owner/project/path/to/license"
      }
    ]

It's also a good idea to include a license file at the top level in your package.

## people fields: author, contributors

The "author" is one person.  "contributors" is an array of people.  A "person"
is an object with a "name" field and optionally "url" and "email", like this:

    { "name" : "Barney Rubble"
    , "email" : "b@rubble.com"
    , "url" : "http://barnyrubble.tumblr.com/"
    }

Or you can shorten that all into a single string, and npm will parse it for you:

    "Barney Rubble <b@rubble.com> (http://barnyrubble.tumblr.com/)

Both email and url are optional either way.

npm also sets a top-level "maintainers" field with your npm user info.

## files

The "files" field is an array of files to include in your project.  If
you name a folder in the array, then it will also include the files
inside that folder. (Unless they would be ignored by another rule.)

You can also provide a ".npmignore" file in the root of your package,
which will keep files from being included, even if they would be picked
up by the files array.  The ".npmignore" file works just like a
".gitignore".

## main

The main field is a module ID that is the primary entry point to your program.
That is, if your package is named `foo`, and a user installs it, and then does
`require("foo")`, then your main module's exports object will be returned.

This should be a module ID relative to the root of your package folder.

For most modules, it makes the most sense to have a main script and often not
much else.

## bin

A lot of packages have one or more executable files that they'd like to
install into the PATH. npm makes this pretty easy (in fact, it uses this
feature to install the "npm" executable.)

To use this, supply a `bin` field in your package.json which is a map of
command name to local file name. On install, npm will symlink that file into
`prefix/bin` for global installs, or `./node_modules/.bin/` for local
installs.


For example, npm has this:

    { "bin" : { "npm" : "./cli.js" } }

So, when you install npm, it'll create a symlink from the `cli.js` script to
`/usr/local/bin/npm`.

If you have a single executable, and its name should be the name
of the package, then you can just supply it as a string.  For example:

    { "name": "my-program"
    , "version": "1.2.5"
    , "bin": "./path/to/program" }

would be the same as this:

    { "name": "my-program"
    , "version": "1.2.5"
    , "bin" : { "my-program" : "./path/to/program" } }

## man

Specify either a single file or an array of filenames to put in place for the
`man` program to find.

If only a single file is provided, then it's installed such that it is the
result from `man <pkgname>`, regardless of its actual filename.  For example:

    { "name" : "foo"
    , "version" : "1.2.3"
    , "description" : "A packaged foo fooer for fooing foos"
    , "main" : "foo.js"
    , "man" : "./man/doc.1"
    }

would link the `./man/doc.1` file in such that it is the target for `man foo`

If the filename doesn't start with the package name, then it's prefixed.
So, this:

    { "name" : "foo"
    , "version" : "1.2.3"
    , "description" : "A packaged foo fooer for fooing foos"
    , "main" : "foo.js"
    , "man" : [ "./man/foo.1", "./man/bar.1" ]
    }

will create files to do `man foo` and `man foo-bar`.

Man files must end with a number, and optionally a `.gz` suffix if they are
compressed.  The number dictates which man section the file is installed into.

    { "name" : "foo"
    , "version" : "1.2.3"
    , "description" : "A packaged foo fooer for fooing foos"
    , "main" : "foo.js"
    , "man" : [ "./man/foo.1", "./man/foo.2" ]
    }

will create entries for `man foo` and `man 2 foo`

## directories

The CommonJS [Packages](http://wiki.commonjs.org/wiki/Packages/1.0) spec details a
few ways that you can indicate the structure of your package using a `directories`
hash. If you look at [npm's package.json](http://registry.npmjs.org/npm/latest),
you'll see that it has directories for doc, lib, and man.

In the future, this information may be used in other creative ways.

### directories.lib

Tell people where the bulk of your library is.  Nothing special is done
with the lib folder in any way, but it's useful meta info.

### directories.bin

If you specify a "bin" directory, then all the files in that folder will
be used as the "bin" hash.

If you have a "bin" hash already, then this has no effect.

### directories.man

A folder that is full of man pages.  Sugar to generate a "man" array by
walking the folder.

### directories.doc

Put markdown files in here.  Eventually, these will be displayed nicely,
maybe, someday.

### directories.example

Put example scripts in here.  Someday, it might be exposed in some clever way.

## repository

Specify the place where your code lives. This is helpful for people who
want to contribute.  If the git repo is on github, then the `npm docs`
command will be able to find you.

Do it like this:

    "repository" :
      { "type" : "git"
      , "url" : "http://github.com/isaacs/npm.git"
      }

    "repository" :
      { "type" : "svn"
      , "url" : "http://v8.googlecode.com/svn/trunk/"
      }

The URL should be a publicly available (perhaps read-only) url that can be handed
directly to a VCS program without any modification.  It should not be a url to an
html project page that you put in your browser.  It's for computers.

## scripts

The "scripts" member is an object hash of script commands that are run
at various times in the lifecycle of your package.  The key is the lifecycle
event, and the value is the command to run at that point.

See `npm-scripts(7)` to find out more about writing package scripts.

## config

A "config" hash can be used to set configuration
parameters used in package scripts that persist across upgrades.  For
instance, if a package had the following:

    { "name" : "foo"
    , "config" : { "port" : "8080" } }

and then had a "start" command that then referenced the
`npm_package_config_port` environment variable, then the user could
override that by doing `npm config set foo:port 8001`.

See `npm-config(7)` and `npm-scripts(7)` for more on package
configs.

## dependencies

Dependencies are specified with a simple hash of package name to version
range. The version range is EITHER a string which has one or more
space-separated descriptors, OR a range like "fromVersion - toVersion"

**Please do not put test harnesses in your `dependencies` hash.**  See
`devDependencies`, below.

Version range descriptors may be any of the following styles, where "version"
is a semver compatible version identifier.

* `version` Must match `version` exactly
* `=version` Same as just `version`
* `>version` Must be greater than `version`
* `>=version` etc
* `<version`
* `<=version`
* `~version` See 'Tilde Version Ranges' below
* `1.2.x` See 'X Version Ranges' below
* `http://...` See 'URLs as Dependencies' below
* `*` Matches any version
* `""` (just an empty string) Same as `*`
* `version1 - version2` Same as `>=version1 <=version2`.
* `range1 || range2` Passes if either range1 or range2 are satisfied.
* `git...` See 'Git URLs as Dependencies' below

For example, these are all valid:

    { "dependencies" :
      { "foo" : "1.0.0 - 2.9999.9999"
      , "bar" : ">=1.0.2 <2.1.2"
      , "baz" : ">1.0.2 <=2.3.4"
      , "boo" : "2.0.1"
      , "qux" : "<1.0.0 || >=2.3.1 <2.4.5 || >=2.5.2 <3.0.0"
      , "asd" : "http://asdf.com/asdf.tar.gz"
      , "til" : "~1.2"
      , "elf" : "~1.2.3"
      , "two" : "2.x"
      , "thr" : "3.3.x"
      }
    }

### Tilde Version Ranges

A range specifier starting with a tilde `~` character is matched against
a version in the following fashion.

* The version must be at least as high as the range.
* The version must be less than the next major revision above the range.

For example, the following are equivalent:

* `"~1.2.3" = ">=1.2.3 <1.3.0"`
* `"~1.2" = ">=1.2.0 <1.3.0"`
* `"~1" = ">=1.0.0 <1.1.0"`

### X Version Ranges

An "x" in a version range specifies that the version number must start
with the supplied digits, but any digit may be used in place of the x.

The following are equivalent:

* `"1.2.x" = ">=1.2.0 <1.3.0"`
* `"1.x.x" = ">=1.0.0 <2.0.0"`
* `"1.2" = "1.2.x"`
* `"1.x" = "1.x.x"`
* `"1" = "1.x.x"`

You may not supply a comparator with a version containing an x.  Any
digits after the first "x" are ignored.

### URLs as Dependencies

Starting with npm version 0.2.14, you may specify a tarball URL in place
of a version range.

This tarball will be downloaded and installed locally to your package at
install time.

### Git URLs as Dependencies

Git urls can be of the form:

    git://github.com/user/project.git#commit-ish
    git+ssh://user@hostname:project.git#commit-ish
    git+ssh://user@hostname/project.git#commit-ish
    git+http://user@hostname/project/blah.git#commit-ish
    git+https://user@hostname/project/blah.git#commit-ish

The `commit-ish` can be any tag, sha, or branch which can be supplied as
an argument to `git checkout`.  The default is `master`.

## devDependencies

If someone is planning on downloading and using your module in their
program, then they probably don't want or need to download and build
the external test or documentation framework that you use.

In this case, it's best to list these additional items in a
`devDependencies` hash.

These things will be installed whenever the `--dev` configuration flag
is set.  This flag is set automatically when doing `npm link` or when doing
`npm install` from the root of a package, and can be managed like any other npm
configuration param.  See `npm-config(7)` for more on the topic.

## bundledDependencies

Array of package names that will be bundled when publishing the package.

If this is spelled `"bundleDependencies"`, then that is also honorable.

## optionalDependencies

If a dependency can be used, but you would like npm to proceed if it
cannot be found or fails to install, then you may put it in the
`optionalDependencies` hash.  This is a map of package name to version
or url, just like the `dependencies` hash.  The difference is that
failure is tolerated.

It is still your program's responsibility to handle the lack of the
dependency.  For example, something like this:

    try {
      var foo = require('foo')
      var fooVersion = require('foo/package.json').version
    } catch (er) {
      foo = null
    }
    if ( notGoodFooVersion(fooVersion) ) {
      foo = null
    }

    // .. then later in your program ..

    if (foo) {
      foo.doFooThings()
    }

Entries in `optionalDependencies` will override entries of the same name in
`dependencies`, so it's usually best to only put in one place.

## engines

You can specify the version of node that your stuff works on:

    { "engines" : { "node" : ">=0.1.27 <0.1.30" } }

And, like with dependencies, if you don't specify the version (or if you
specify "\*" as the version), then any version of node will do.

If you specify an "engines" field, then npm will require that "node" be
somewhere on that list. If "engines" is omitted, then npm will just assume
that it works on node.

You can also use the "engines" field to specify which versions of npm
are capable of properly installing your program.  For example:

    { "engines" : { "npm" : "~1.0.20" } }

Note that, unless the user has set the `engine-strict` config flag, this
field is advisory only.

## engineStrict

If you are sure that your module will *definitely not* run properly on
versions of Node/npm other than those specified in the `engines` hash,
then you can set `"engineStrict": true` in your package.json file.
This will override the user's `engine-strict` config setting.

Please do not do this unless you are really very very sure.  If your
engines hash is something overly restrictive, you can quite easily and
inadvertently lock yourself into obscurity and prevent your users from
updating to new versions of Node.  Consider this choice carefully.  If
people abuse it, it will be removed in a future version of npm.

## os

You can specify which operating systems your
module will run on:

    "os" : [ "darwin", "linux" ]

You can also blacklist instead of whitelist operating systems,
just prepend the blacklisted os with a '!':

    "os" : [ "!win32" ]

The host operating system is determined by `process.platform`

It is allowed to both blacklist, and whitelist, although there isn't any
good reason to do this.

## cpu

If your code only runs on certain cpu architectures,
you can specify which ones.

    "cpu" : [ "x64", "ia32" ]

Like the `os` option, you can also blacklist architectures:

    "cpu" : [ "!arm", "!mips" ]

The host architecture is determined by `process.arch`

## preferGlobal

If your package is primarily a command-line application that should be
installed globally, then set this value to `true` to provide a warning
if it is installed locally.

It doesn't actually prevent users from installing it locally, but it
does help prevent some confusion if it doesn't work as expected.

## private

If you set `"private": true` in your package.json, then npm will refuse
to publish it.

This is a way to prevent accidental publication of private repositories.
If you would like to ensure that a given package is only ever published
to a specific registry (for example, an internal registry),
then use the `publishConfig` hash described below
to override the `registry` config param at publish-time.

## publishConfig

This is a set of config values that will be used at publish-time.  It's
especially handy if you want to set the tag or registry, so that you can
ensure that a given package is not tagged with "latest" or published to
the global public registry by default.

Any config values can be overridden, but of course only "tag" and
"registry" probably matter for the purposes of publishing.

See `npm-config(7)` to see the list of config options that can be
overridden.

## SEE ALSO

* npm-semver(7)
* npm-init(1)
* npm-version(1)
* npm-config(1)
* npm-config(7)
* npm-help(1)
* npm-faq(7)
* npm-install(1)
* npm-publish(1)
* npm-rm(1)
