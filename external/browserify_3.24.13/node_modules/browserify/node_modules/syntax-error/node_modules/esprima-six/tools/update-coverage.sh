#!/bin/bash

if [ `command -v cover` ]; then

    REVISION=`git log -1 --pretty=%h`
    DATE=`git log -1 --pretty=%cD | cut -c 6-16`

    echo "Running coverage analysis..."
    rm -rf .coverage_data
    rm -rf cover_index

    cover run test/runner.js
    cover report html

    cat test/coverage.header.html > test/coverage.html
    echo "Tested revision: <a href='https://github.com/ariya/esprima/commit/${REVISION}'>${REVISION}</a>" >> test/coverage.html
    echo " (dated ${DATE}).</p>" >> test/coverage.html

    echo '<pre>' >> test/coverage.html
    grep -h '^<span class' cover_html/*esprima\.js\.html >> test/coverage.html
    echo '</pre>' >> test/coverage.html
    cat test/coverage.footer.html >> test/coverage.html

    rm -rf cover_html
    rm -rf .coverage_data
else
    echo "Please install node-cover first!"
fi
