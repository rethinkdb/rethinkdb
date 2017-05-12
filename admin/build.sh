#!/usr/bin/env bash

set -e

watch=false

build_all () {
    mkdir -p dist
    build_js
    build_css
    build_static
}

build_watch () {
    mkdir -p dist
    watch=true
    build_js &
    build_css &
    build_static &
    wait
}

build_js () (
    local cmd
    if $watch; then
	cmd=watchify
    elif [[ "$WEBUI_BUNDLE" = 1 ]]; then
	cmd=${WEBUI_NODE_MODULES}.bin/browserify
    else
	cmd=browserify
    fi
    "$cmd" -v \
	-t ${WEBUI_NODE_MODULES:-}coffeeify \
	-t ${WEBUI_NODE_MODULES}browserify-handlebars \
	${WEBUI_BUNDLE:+ -x rethinkdb} \
	src/coffee/app.coffee \
	-o ${WEBUI_DIST_DIR:-dist}/cluster.js
)

build_css () (
    cd src/less
    set --
    if $watch; then
	set -- onchange --initial --verbose '**/*' -- lessc
    elif [[ "$WEBUI_BUNDLE" = 1 ]]; then
	set -- ${WEBUI_NODE_MODULES}.bin/lessc
    else
	set -- lessc
    fi
    "$@" --verbose styles-new.less ${WEBUI_DIST_DIR:-../../dist}/cluster.css
)

build_static () (
    cd assets
    if $watch; then
	cpx --verbose --watch '**/*' ${WEBUI_DIST_DIR:-../dist}/
    else
	cp -a * ${WEBUI_DIST_DIR:-../dist}/
    fi
)

build_${1:-all}

