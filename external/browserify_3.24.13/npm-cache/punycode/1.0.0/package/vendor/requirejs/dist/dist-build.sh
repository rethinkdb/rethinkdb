#!/bin/bash

#@license RequireJS Copyright (c) 2010-2011, The Dojo Foundation All Rights Reserved.
#Available via the MIT or new BSD license.
#see: http://github.com/jrburke/requirejs for details

#version should be something like 0.9.0beta or 0.9.0
version=$1
if [ -z $version ]; then
    echo "Please pass in a version number"
    exit 1
fi

myDir=`cd \`dirname "$0"\`; pwd`

# First update the sub-projects with the latest.
cd ..
./updatesubs.sh
cd dist

# Setup a build directory
rm -rf ../../requirejs-build
mkdir ../../requirejs-build

# Create the version output dir
cd ../../requirejs-build
mkdir $version
mkdir $version/minified
mkdir $version/comments

# Copy over the r.js file, and set up that project for a dist checkin.
cp ../r.js/r.js $version/r.js
cp ../r.js/r.js ../r.js/dist/r-$version.js

# Copy over basic script deliverables
cp $myDir/../require.js $version/comments/require.js
cp $myDir/../text.js $version/comments/text.js
cp $myDir/../domReady.js $version/comments/domReady.js
cp $myDir/../order.js $version/comments/order.js
cp $myDir/../i18n.js $version/comments/i18n.js

# Minify any of the browser-based JS files
cd $version/comments
java -jar ../../../r.js/lib/closure/compiler.jar --js require.js --js_output_file ../minified/require.js
java -jar ../../../r.js/lib/closure/compiler.jar --js text.js --js_output_file ../minified/text.js
java -jar ../../../r.js/lib/closure/compiler.jar --js domReady.js --js_output_file ../minified/domReady.js
java -jar ../../../r.js/lib/closure/compiler.jar --js order.js --js_output_file ../minified/order.js
java -jar ../../../r.js/lib/closure/compiler.jar --js i18n.js --js_output_file ../minified/i18n.js

cd ../../../
