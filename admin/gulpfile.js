'use strict';
var gulp = require('gulp'),
    lessc = require('gulp-less'),
    gutil = require('gulp-util'),
    replace = require('gulp-replace'),
    rename = require('gulp-rename'),
    newer = require('gulp-newer'),
    less = require('gulp-less'),
    browserify = require('browserify'),
    uberWatchify = require('uber-watchify'),
    File = require('vinyl'),
    vinylSource = require('vinyl-source-stream'),
    buffer = require('vinyl-buffer'),
    del = require('del'),
    fs = require('fs'),
    path = require('path'),
    argv = require('yargs').argv,
    coffeeify = require('coffeeify'),
    uglify = require('uglifyify'),
    hbsfy = require('hbsfy');

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
    DRIVER_BUILD_DIR = BUILD_DIR+'/packages/js',
    VERSION_FILE = WEB_OBJ_DIR+'/version.coffee',
    BROWSERIFY_BUNDLE_ENTRY_POINT = COFFEE_DIR+'/body.coffee',
    BROWSERIFY_CACHE_FILE = WEB_OBJ_DIR+'/browserify-cache.json',
    INDEX_FILE = WEBUI_DIR+'/templates/cluster.html';

var watch = false,  // Whether to watch the browserify output
    bundler = null; // holds the bundler once it's created

function error() {
    gutil.log.apply(null, arguments);
    console.log('node path: ', process.env.NODE_PATH);
    process.exit(1);
}

var info = argv.silent ? function(){} : gutil.log;

gulp.task('default', ['build', 'watch']);

gulp.task('build', [
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
  return gulp.src(LESS_DIR+'/styles*.less')
    .pipe(less({
      paths: [LESS_DIR],
    }))
    .pipe(rename('cluster.css'))
    .pipe(gulp.dest(WEB_ASSETS_DIR))
    .on('error', error);
});

gulp.task('rethinkdb-version', function(cb) {
  fs.mkdir(WEB_OBJ_DIR, function(result, err) {
      fs.writeFile(
        VERSION_FILE,
        new Buffer('module.exports = "'+argv.version+'";'),
        cb);
  });
});

gulp.task('index', function() {
  gulp.src([INDEX_FILE])
    .pipe(rename('index.html'))
    .pipe(replace(/{RETHINKDB_VERSION}/g, argv.version))
    .pipe(gulp.dest(WEB_ASSETS_DIR))
    .on('error', error);
});

gulp.task('browserify', ['rethinkdb-version'], function() {
  bundler = createBundler(false);
  return rebundle();
});

gulp.task('browserify-watch', ['rethinkdb-version'], function(){
  bundler = createBundler(true);
  return rebundle();
});


function createBundler(watch) {
  // This is the browserify bundler, used by both the build and watch
  // tasks. Only one should be created, and re-used so that it can
    // cache build results when rebuilding on file changes
    console.log("BAR: " + BROWSERIFY_BUNDLE_ENTRY_POINT);
    console.log("FOO: " + BROWSERIFY_BUNDLE_ENTRY_POINT.replace(/\//g, "\\"));
  var retval = uberWatchify(browserify({
    entries: [BROWSERIFY_BUNDLE_ENTRY_POINT.replace(/\//g, "\\")],
    cache: uberWatchify.getCache(BROWSERIFY_CACHE_FILE),
    extensions: ['.coffee', '.hbs'],
    packageCache: {},
    fullPaths: true,
  }),{
    // special uber-watchify only options
    cacheFile: BROWSERIFY_CACHE_FILE,
    watch: watch,
  })
    // Allows var r = require('rethinkdb') without including the
    // driver source in this bundle
    .exclude('rethinkdb')
    // Need to exclude the version file so we don't accidentally pick
    // up something off the filesystem
    .exclude('rethinkdb-version')
    // convert coffee files first
    .transform(coffeeify)
    // convert handlebars files & insert handlebars runtime
    .transform(hbsfy);

  if(argv.uglify) {
    retval.transform({global: true}, uglify);
  }

  return retval
    .add(VERSION_FILE, {expose: 'rethinkdb-version'})
    .add(DRIVER_BUILD_DIR+'/rethinkdb.js',
         {expose: 'rethinkdb'})
    .on('update', rebundle)
    .on('error', function(e){error("Browserify error:", e);})
    .on('log', function(msg){info("Browserify: "+msg);});
}

function rebundle(files) {
  if (files){
    info("Files changed: ", files);
  }
  return bundler.bundle()
  // We use a vinyl-source-stream to turn the browserify bundle into
  // a stream that gulp understands
    .pipe(vinylSource(JS_BUNDLE_FILE))
    .pipe(gulp.dest(WEB_ASSETS_DIR));
}


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
      .on('error', error);
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
