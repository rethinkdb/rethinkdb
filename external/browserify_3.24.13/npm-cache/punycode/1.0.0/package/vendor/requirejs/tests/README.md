# RequireJS Testing

This is a list of things to test before doing a RequireJS release.

**Update all subprojects via ./updatesubs.sh FIRST**

## Items in tests directory

* All browsers: tests/doh/runner.html?testUrl=../all in all the browsers
* go to tests/ run alln.sh and allj.sh
* Optional, only do on major refactors: tests/commonjs, run each file in browser.

## Node testing

Go to r.js project and run the following:

* cd tests
* ./alln.sh
* node allNode.js
* ./allj.sh
* node ../../r.js index.js
* node canvasTest.js (use nave and do the npm installs listed in the JS file)
* cd ../build/tests
* ./alln.sh
* ./allj.sh

Try manual testing:

* node ../../r.js -o simpleNamespace.build.js - load builds/simpleNamespace/NAMESPACE.html to test namespacing.
* node ../../r.js -o indexBuilder.build.js - confirm plugin write calls are done.
* node ../../r.js -o text.build.js - confirm plugin write calls are done.
* node ../../r.js -o textExclude.build.js - confirm that 'text!subwidget2.html' is excluded.
* node ../../r.js -o i18n.build.js - confirm plugin required the nls/en-us-surfer/colors module.
* node ../../r.js -o order.build.js - confirm one, two, three are in order.
* node ../../r.js -o hasTestModule.build.js - confirm has replacements have been done.

* plugins with dependencies, test in r.js/build/tests/lib/plugins:
node ../../../../r.js main.js (tests if code runs unoptimized)
node ../../../../r.js -o build.js
node ../../../../r.js main-built.js (compare output with unoptimized case)

Notably, the plug!shouldbeuppercasetext resource should be in the output, and
the build should complete as normal, no errors because of the plugin's
dependencies are not defined properly.

Try r.js a larger node project if available.

Do these tests for each version of Node to support.

# Sample jQuery project

Test the sample jQuery project, complete with running the optimizer, testing the output.

# CoffeeScript plugin

Check it.

# jQueryUI AMD project

Check it.