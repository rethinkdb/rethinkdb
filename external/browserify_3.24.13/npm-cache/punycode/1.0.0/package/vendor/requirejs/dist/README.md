# RequireJS dist

This directory contains the tools that are used to build distributions of RequireJS and its web site.

When doing a release, do the following:

* Update files to the new version number:
    * require.js, both places
    * all plugins, both places
    * docs/download.md: check for nested paths too, add new release section
    * pre.html
    * post.html
* Update version in x.js in the r.js project if necessary.
* .updatesubs.sh
* Check in changes to r.js project.
* Check version of cs plugin, update download.html if necessary.
* Check version of jQuery in the jQuery sample project, update the download.html if necessary.
    * Upload change in jQuery project to website even before the current release.
* Commit/push changes
* Commit changes to:
    * require-cs: make a new tag if cs.js changed since last release.
    * require-jquery
    * jqueryui-amd: update the downloadable content if necessary.
* Update the requirejs-npm directory
  * Update version in package.json
  * Modify bin/r.js to add: #!/usr/bin/env node
  * npm uninstall -g requirejs
  * npm install . -g
  * r.js -v
  * node (then use repl to do require("requirejs"))
  * Try a local install.
  * npm publish (in the requirejs-npm/requirejs directory)

* Tag the requirejs and r.js trees:
    * git tag -am "Release 0.0.0" 0.0.0
    * git push --tags

Now pull down the tagged version to do a distribution, do this in git/ directory:

* rm -rf ./requirejs-dist ./requirejs-build
* git clone git://github.com/jrburke/requirejs.git requirejs-dist
* cd requirejs-dist
* git checkout 0.0.0
* cd dist

Run the distribution tasks.

To generate a build

* ./dist-build.sh 0.0.0

To generate the web site:

* node dist-site.js
* cd dist-site
* zip -r docs.zip ./*
* mv docs.zip ../../../requirejs-build/

Be sure the links for the CoffeeScript and jQuery Sample project work.

When done, reset versions to:

* 0.0.0+ in require.js
* X.X.X in pre.html
