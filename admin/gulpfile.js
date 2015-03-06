'use strict';
var gulp = require('gulp-param')(require('gulp'), process.argv),
    lessc = require('gulp-less'),
    gutil = require('gulp-util'),
    browserify = require('browserify'),
    uberWatchify = require('uber-watchify'),
    File = require('vinyl'),
    vinylSource = require('vinyl-source-stream'),
    buffer = require('vinyl-buffer'),
    concat = require('gulp-concat'),
    replace = require('gulp-replace'),
    rename = require('gulp-rename'),
    newer = require('gulp-newer'),
    del = require('del'),
    fs = require('fs'),
    path = require('path'),
    less = require('gulp-less');

var ROOT_DIR = path.resolve(__dirname, '..'),
    BUILD_DIR = ROOT_DIR+'/build',
    WEBUI_DIR = ROOT_DIR+'/admin',
    WEB_ASSETS_DIR = BUILD_DIR+'/web_assets',
    WEB_OBJ_DIR = BUILD_DIR+'/web_obj',
    STATIC_DIR = WEBUI_DIR+'/static',
    COFFEE_DIR = STATIC_DIR+'/coffee',
    JS_BUNDLE_FILE = 'cluster-min.js',
    LESS_DIR = STATIC_DIR+'/less',
    FONTS_DIR = STATIC_DIR+'/fonts',
    IMAGES_DIR = STATIC_DIR+'/images',
    JS_DIR = STATIC_DIR+'/js',
    DRIVER_SRC_DIR = ROOT_DIR+'/drivers/javascript',
    VERSION_FILE = WEB_OBJ_DIR+'/version.coffee',
    BROWSERIFY_BUNDLE_ENTRY_POINT = COFFEE_DIR+'/body.coffee',
    BROWSERIFY_CACHE_FILE = WEB_OBJ_DIR+'/browserify-cache.json',
    INDEX_FILE = WEBUI_DIR+'/templates/cluster.html';

gulp.task('build', ['default']);

gulp.task('default', [
  'browserify',
  'external-js',
  'less',
  'favicon',
  'fonts',
  'images',
  'index',
]);

// Convenience task for cleaning out the web_assets build directory
gulp.task('clean', function(cb) {
  del([WEB_ASSETS_DIR+'/**', WEB_OBJ_DIR+'/**'], {force: true}, cb);
});

// Theoretically, we could add a css minifier in this task. In
// practice, the minification didn't make much of a difference. My
// guess is that this is because we have very deeply nested rules in
// our less files (overspecifying them) and this prevents coalescing
// rules that could otherwise be combined. Unnesting our rules is
// probably a large enough task that we're likely to rewrite the
// entire thing in sass or something before that happens.
gulp.task('less', function() {
  return gulp.src(LESS_DIR+'/styles.less')
    .pipe(less({
      paths: [LESS_DIR],
    }))
    .pipe(rename('cluster.css'))
    .pipe(gulp.dest(WEB_ASSETS_DIR))
    .on('error', gutil.log);
});

gulp.task('rethinkdb-version', function(version, cb) {
  fs.mkdir(WEB_OBJ_DIR, function(result, err) {
      fs.writeFile(
        VERSION_FILE,
        new Buffer('module.exports = "'+version+'";'),
        cb);
  });
});

gulp.task('rethinkdb-driver', function(js_build_dir) {
  gulp.src(js_build_dir+'/rethinkdb.js')
    .pipe(gulp.dest(WEB_ASSETS_DIR+'/js'));
});

gulp.task('index', function(version) {
  gulp.src([INDEX_FILE])
    .pipe(rename('index.html'))
    .pipe(replace(/{RETHINKDB_VERSION}/g, version))
    .pipe(gulp.dest(WEB_ASSETS_DIR))
    .on('error', gutil.log);
});

gulp.task('browserify', ['rethinkdb-version'], function(version) {
  browserifyShared(version, false);
});

gulp.task('browserify-watch', ['rethinkdb-version'], function(version){
  browserifyShared(version, true);
});

var browserifyShared = function(version, watch) {
  // Abstracts common functionality between watching for changes and not
  return uberWatchify(browserify({
    entries: [BROWSERIFY_BUNDLE_ENTRY_POINT],
    cache: uberWatchify.getCache(BROWSERIFY_CACHE_FILE),
    extensions: ['.coffee', '.hbs'],
    packageCache: {},
    fullPaths: true,
  }),{
    // special uber-watchify only options
    cacheFile: BROWSERIFY_CACHE_FILE,
    watch: false,
  })
    // Allows var r = require('rethinkdb') without including the
    // driver source in this bundle
    //.external('rethinkdb')
    .exclude('rethinkdb')
    // Need to exclude the version file so we don't accidentally pick
    // up something off the filesystem
    .exclude('rethinkdb-version')
    // convert coffee files first
    .transform('coffeeify')
    // convert handlebars files & insert handlebars runtime
    .transform('hbsfy')
    .require(VERSION_FILE, {expose: 'rethinkdb-version'})
    .require(BUILD_DIR+'/drivers/javascript/proto-def.js',
             {expose: './proto-def'})
    .require(
      DRIVER_SRC_DIR+'/rethinkdb.coffee',
      {
        expose: 'rethinkdb',
        //basedir: DRIVER_SRC_DIR,
      }
    )
    .bundle()
    .on('error', gutil.log)
    // We use a vinyl-source-stream to turn the browserify bundle into
    // a stream that gulp understands
    .pipe(vinylSource(JS_BUNDLE_FILE))
    .pipe(gulp.dest(WEB_ASSETS_DIR))
    .on('error', gutil.log);
};

// each entry will be turned into a gulp task copying the specified
// files to the web assets output directory
var copy_tasks = [
  ['favicon', './favicon.ico', ''],
  ['fonts', FONTS_DIR+'/**', '/fonts/'],
  ['images', IMAGES_DIR+'/**', '/images/'],
  ['external-js', JS_DIR+'/**', '/js/'],
];

copy_tasks.map(function(taskdef){
  var taskName = taskdef[0], files = taskdef[1], dest = taskdef[2];
  var destDir = WEB_ASSETS_DIR+dest;
  return gulp.task(taskName, function() {
    gulp.src(files)
      // only copy if the src is newer than dest
      .pipe(newer(destDir))
      .pipe(gulp.dest(destDir))
      .on('error', gutil.log);
  });
});


gulp.task('watch', ['browserify-watch'], function() {
  copy_tasks.map(function(taskdef) {
    var taskName = taskdef[0], files = taskdef[1];
    gulp.watch(files, [taskName]);
  });
  gulp.watch(INDEX_FILE, ['index']);
  gulp.watch(LESS_DIR+"/**", ['less']);
});
