set -ex

build_all () {
    mkdir -p dist
    build-js
    build-css
    build-static
}

build_js () {
    ${BROWSERIFY:-browserify} \
	-t ${WEBUI_NODE_MODULES:-}coffeeify \
	-t ${WEBUI_NODE_MODULES}browserify-handlebars \
	${WEBUI_BUNDLE:+ -x rethinkdb} \
	src/coffee/app.coffee \
	-o ${WEBUI_DIST_DIR:-dist}/cluster.js
}

build_css () {
    cd src/less
    ${LESSC:-lessc} styles-new.less ${WEBUI_DIST_DIR:-../../dist}/cluster.css
}

build_static () {
    cd assets
    cp -a * ${WEBUI_DIST_DIR:-../dist}/
}

build_${1:-all}

