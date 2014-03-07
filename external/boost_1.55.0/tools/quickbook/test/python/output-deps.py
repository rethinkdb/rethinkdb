#!/usr/bin/env python

# Copyright 2012-2013 Daniel James
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt)

import sys, os, subprocess, tempfile, re

def main(args, directory):
    if len(args) != 1:
        print "Usage: output-deps.py quickbook-command"
        exit(1)
    quickbook_command = args[0]

    failures = 0
    failures += run_quickbook(quickbook_command, 'svg_missing.qbk',
            deps_gold = 'svg_missing_deps.txt')
    failures += run_quickbook(quickbook_command, 'svg_missing.qbk',
            locations_gold = 'svg_missing_locs.txt')
    failures += run_quickbook(quickbook_command, 'missing_relative.qbk',
            deps_gold = 'missing_relative_deps.txt',
            locations_gold = 'missing_relative_locs.txt')
    failures += run_quickbook(quickbook_command, 'include_path.qbk',
            deps_gold = 'include_path_deps.txt',
            locations_gold = 'include_path_locs.txt',
            input_path = ['sub1', 'sub2'])

    if failures == 0:
        print "Success"
    else:
        print "Failures:",failures
        exit(failures)

def run_quickbook(quickbook_command, filename, output_gold = None,
        deps_gold = None, locations_gold = None, input_path = []):
    failures = 0

    command = [quickbook_command, '--debug', filename]

    output_filename = None
    if output_gold:
        output_filename = temp_filename('.qbk')
        command.extend(['--output-file', output_filename])

    deps_filename = None
    if deps_gold:
        deps_filename = temp_filename('.txt')
        command.extend(['--output-deps', deps_filename])

    locations_filename = None
    if locations_gold:
        locations_filename = temp_filename('.txt')
        command.extend(['--output-checked-locations', locations_filename])

    try:
        for path in input_path:
            command.extend(['-I', path])
        print 'Running: ' + ' '.join(command)
        print
        exit_code = subprocess.call(command)
        print
        success = not exit_code

        if output_filename:
            output = load_file(output_filename)
        else:
            output = None

        if deps_filename:
            deps = load_dependencies(deps_filename)
        else:
            deps = None

        if locations_filename:
            locations = load_locations(locations_filename)
        else:
            locations = None
    finally:
        if output_filename: os.unlink(output_filename)
        if deps_filename: os.unlink(deps_filename)

    if deps_gold:
        gold = load_dependencies(deps_gold, adjust_paths = True)
        if deps != gold:
            failures = failures + 1
            print "Dependencies don't match:"
            print "Gold:", gold
            print "Result:", deps
            print

    if locations_gold:
        gold = load_locations(locations_gold, adjust_paths = True)
        if locations != gold:
            failures = failures + 1
            print "Dependencies don't match:"
            print "Gold:", gold
            print "Result:", locations
            print

    if output_gold:
        gold = load_file(output_gold)
        if gold != output:
            failures = failures + 1
            print "Output doesn't match:"
            print
            print gold
            print
            print output
            print

    return failures

def load_dependencies(filename, adjust_paths = False):
    dependencies = set()
    f = open(filename, 'r')
    for path in f:
        if path[0] == '#': continue
        if adjust_paths:
            path = os.path.realpath(path)
        if path in dependencies:
            raise Exception("Duplicate path (%1s) in %2s" % (path, filename))
        dependencies.add(path)
    return dependencies

def load_locations(filename, adjust_paths = False):
    line_matcher = re.compile("^([+-]) (.*)$")
    dependencies = {}
    f = open(filename, 'r')
    for line in f:
        if line[0] == '#': continue
        m = line_matcher.match(line)
        if not m:
            raise Exception("Invalid dependency file: %1s" % filename)
        found = m.group(1) == '+'
        path = m.group(2)
        if adjust_paths:
            path = os.path.realpath(path)
        if path in dependencies:
            raise Exception("Duplicate path (%1s) in %2s" % (path, filename))
        dependencies[path] = found
    return dependencies

def temp_filename(extension):
    file = tempfile.mkstemp(suffix = extension)
    os.close(file[0])
    return file[1]

def load_file(filename):
    f = open(filename, 'r')
    try:
        return f.read()
    finally:
        f.close()

    return None

main(sys.argv[1:], os.path.dirname(sys.argv[0]))
