'use strict';
var gulp = require('gulp-param')(require('gulp'), process.argv),
    lessc = require('gulp-less'),
    browserify = require('browserify'),
    File = require('vinyl'),
    vinylSource = require('vinyl-source-stream'),
    buffer = require('vinyl-buffer'),
    concat = require('gulp-concat'),
    replace = require('gulp-replace'),
    rename = require('gulp-rename'),
    del = require('del'),
    fs = require('fs'),
    less = require('gulp-less');

var BUILD_DIR = '../build',
    WEB_ASSETS_DIR = BUILD_DIR+'/web_assets',
    STATIC_DIR = './static',
    COFFEE_DIR = STATIC_DIR+'/coffee',
    BROWSERIFY_BUNDLE_ENTRY_POINT = COFFEE_DIR+'/body.coffee',
    JS_BUNDLE_FILE = 'cluster-min.js',
    LESS_DIR = STATIC_DIR+'/less',
    FONTS_DIR = STATIC_DIR+'/fonts',
    IMAGES_DIR = STATIC_DIR+'/images',
    VERSION_FILE = COFFEE_DIR+'version.coffee',
    INDEX_FILE = './templates/cluster.html';


gulp.task('default', [
  'browserify-js-driver',
  'browserify',
  'less',
  'favicon',
  'fonts',
  'images',
  'index',
]);

// Convenience task for cleaning out the web_assets build directory
gulp.task('clean', function(cb) {
  del([WEB_ASSETS_DIR+'/**'], cb);
});

gulp.task('version-file', function(version){
  //create the version file
  fs.writeFileSync(
    VERSION_FILE,
    "module.exports = "+version);
});

// Theoretically, we could add a css minifier in this task. In
// practice, I found the minification didn't make much of a
// difference. My guess is that this is because we have very deeply
// nested rules in our less files (overspecifying them) and this
// prevents coalescing rules that could otherwise be
// combined. Unnesting our rules is probably a large enough task that
// we're likely to rewrite the entire thing in sass or something
// before that happens.
gulp.task('less', function() {
  return gulp.src(LESS_DIR+'/styles.less')
    .pipe(less({
      paths: [LESS_DIR],
    }))
    .pipe(rename('cluster.css'))
    .pipe(gulp.dest(WEB_ASSETS_DIR));
});

gulp.task('browserify-js-driver', function() {
  // copy the driver over
});

gulp.task('index', function(version) {
  gulp.src([INDEX_FILE])
    .pipe(rename('index.html'))
    .pipe(replace(/{RETHINKDB_VERSION}/g, version))
    .pipe(gulp.dest(WEB_ASSETS_DIR));
});

gulp.task('browserify', function(version) {
  return browserify({
    entries: [BROWSERIFY_BUNDLE_ENTRY_POINT],
    extensions: ['.coffee', '.hbs'],
    detectGlobals: false, // this speeds up building, and we don't use
                          // Node.js globals in the webui code
  })
    // Allows var r = require('rethinkdb') without including the
    // driver source in this bundle
    .external('rethinkdb')
    // Need to exclude the version file so we don't accidentally pick
    // up something off the filesystem
    .exclude('rethinkdb-version')
    // Don't want to accidentally pick up the wrong rethinkdb module
    .exclude('rethinkdb')
    // convert coffee files first
    .transform('coffeeify')
    // convert handlebars files & insert handlebars runtime
    .transform('hbsfy')
    // Add a fake version module that never lives on the
    // filesystem. Will be available with require('rethinkdb-version')
    .require(
      new File({contents: new Buffer('module.exports = "'+version+'";')}),
      {
        expose: 'rethinkdb-version',
        basedir: COFFEE_DIR+'/',
      }
    )
    .bundle()
    // We use a vinyl-source-stream to turn the browserify bundle into
    // a stream that gulp understands
    .pipe(vinylSource(JS_BUNDLE_FILE))
    .pipe(gulp.dest(WEB_ASSETS_DIR));
});

// each entry will be turned into a gulp task copying the specified
// files to the web assets output directory
var copy_tasks = [
  ['favicon', './favicon.ico', ''],
  ['fonts', FONTS_DIR+'/*.{woff,ttf,svg,eot,css}', '/fonts/'],
  ['images', IMAGES_DIR+'/*.{gif,png,jpg,jpeg', '/images/'],
  ['coffee', COFFEE_DIR+'/*.{coffee}'],
];

copy_tasks.map(function(taskdef){
  var taskName = taskdef[0], files = taskdef[1], dest = taskdef[2];
  return gulp.task(taskName, function() {
    gulp.src(files).pipe(gulp.dest(WEB_ASSETS_DIR+dest));
  });
});
