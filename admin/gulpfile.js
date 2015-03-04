'use strict';
var gulp = require('gulp'),
    handlebars = require('gulp-handlebars'),
    lessc = require('gulp-less'),
    browserify = require('browserify'),
    source = require('vinyl-source-stream'),
    transform = require('vinyl-transform'),
    buffer = require('vinyl-buffer'),
    concat = require('gulp-concat'),
    gutil = require('gulp-util');

var BUILD_DIR = './build',
    STATIC_DIR = './static',
    COFFEE_DIR = STATIC_DIR+'/coffee',
    COFFEE_SRC = COFFEE_DIR+'/**/*.coffee',
    LESS_DIR = STATIC_DIR+'/less',
    TEMPLATE_DIR = STATIC_DIR+'/handlebars';


gulp.task('default', ['browserify'], function() {
});

gulp.task('browserify', function() {
  var browserified = transform(function(filename) {
    return browserify(filename)
      .transform('coffeeify')
      .transform('browserify-handlebars')
      .external('rethinkdb')
      .bundle();
  });

  return gulp.src(['./static/coffee/body.coffee'])
    .pipe(browserified)
    .pipe(gulp.dest('./browserified_app.js'));
});

gulp.task('copy-files', function() {
  gulp.src(parameters.app_path+'/static/*')
    .pipe(gulp.dest(parameters.web_path+'/js/'));
});
