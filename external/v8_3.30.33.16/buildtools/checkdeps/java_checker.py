# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Checks Java files for illegal imports."""

import codecs
import os
import re

import results
from rules import Rule


class JavaChecker(object):
  """Import checker for Java files.

  The CheckFile method uses real filesystem paths, but Java imports work in
  terms of package names. To deal with this, we have an extra "prescan" pass
  that reads all the .java files and builds a mapping of class name -> filepath.
  In CheckFile, we convert each import statement into a real filepath, and check
  that against the rules in the DEPS files.

  Note that in Java you can always use classes in the same directory without an
  explicit import statement, so these imports can't be blocked with DEPS files.
  But that shouldn't be a problem, because same-package imports are pretty much
  always correct by definition. (If we find a case where this is *not* correct,
  it probably means the package is too big and needs to be split up.)

  Properties:
    _classmap: dict of fully-qualified Java class name -> filepath
  """

  EXTENSIONS = ['.java']

  def __init__(self, base_directory, verbose):
    self._base_directory = base_directory
    self._verbose = verbose
    self._classmap = {}
    self._PrescanFiles()

  def _PrescanFiles(self):
    for root, dirs, files in os.walk(self._base_directory):
      # Skip unwanted subdirectories. TODO(husky): it would be better to do
      # this via the skip_child_includes flag in DEPS files. Maybe hoist this
      # prescan logic into checkdeps.py itself?
      for d in dirs:
        # Skip hidden directories.
        if d.startswith('.'):
          dirs.remove(d)
        # Skip the "out" directory, as dealing with generated files is awkward.
        # We don't want paths like "out/Release/lib.java" in our DEPS files.
        # TODO(husky): We need some way of determining the "real" path to
        # a generated file -- i.e., where it would be in source control if
        # it weren't generated.
        if d == 'out':
          dirs.remove(d)
        # Skip third-party directories.
        if d in ('third_party', 'ThirdParty'):
          dirs.remove(d)
      for f in files:
        if f.endswith('.java'):
          self._PrescanFile(os.path.join(root, f))

  def _PrescanFile(self, filepath):
    if self._verbose:
      print 'Prescanning: ' + filepath
    with codecs.open(filepath, encoding='utf-8') as f:
      short_class_name, _ = os.path.splitext(os.path.basename(filepath))
      for line in f:
        for package in re.findall('^package\s+([\w\.]+);', line):
          full_class_name = package + '.' + short_class_name
          if full_class_name in self._classmap:
            print 'WARNING: multiple definitions of %s:' % full_class_name
            print '    ' + filepath
            print '    ' + self._classmap[full_class_name]
            print
          else:
            self._classmap[full_class_name] = filepath
          return
      print 'WARNING: no package definition found in %s' % filepath

  def CheckFile(self, rules, filepath):
    if self._verbose:
      print 'Checking: ' + filepath

    dependee_status = results.DependeeStatus(filepath)
    with codecs.open(filepath, encoding='utf-8') as f:
      for line in f:
        for clazz in re.findall('^import\s+(?:static\s+)?([\w\.]+)\s*;', line):
          if clazz not in self._classmap:
            # Importing a class from outside the Chromium tree. That's fine --
            # it's probably a Java or Android system class.
            continue
          include_path = os.path.relpath(
              self._classmap[clazz], self._base_directory)
          # Convert Windows paths to Unix style, as used in DEPS files.
          include_path = include_path.replace(os.path.sep, '/')
          rule = rules.RuleApplyingTo(include_path, filepath)
          if rule.allow == Rule.DISALLOW:
            dependee_status.AddViolation(
                results.DependencyViolation(include_path, rule, rules))
        if '{' in line:
          # This is code, so we're finished reading imports for this file.
          break

    return dependee_status
