# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Checks C++ and Objective-C files for illegal includes."""

import codecs
import os
import re

import results
from rules import Rule, MessageRule


class CppChecker(object):

  EXTENSIONS = [
      '.h',
      '.cc',
      '.cpp',
      '.m',
      '.mm',
  ]

  # The maximum number of non-include lines we can see before giving up.
  _MAX_UNINTERESTING_LINES = 50

  # The maximum line length, this is to be efficient in the case of very long
  # lines (which can't be #includes).
  _MAX_LINE_LENGTH = 128

  # This regular expression will be used to extract filenames from include
  # statements.
  _EXTRACT_INCLUDE_PATH = re.compile(
      '[ \t]*#[ \t]*(?:include|import)[ \t]+"(.*)"')

  def __init__(self, verbose):
    self._verbose = verbose

  def CheckLine(self, rules, line, dependee_path, fail_on_temp_allow=False):
    """Checks the given line with the given rule set.

    Returns a tuple (is_include, dependency_violation) where
    is_include is True only if the line is an #include or #import
    statement, and dependency_violation is an instance of
    results.DependencyViolation if the line violates a rule, or None
    if it does not.
    """
    found_item = self._EXTRACT_INCLUDE_PATH.match(line)
    if not found_item:
      return False, None  # Not a match

    include_path = found_item.group(1)

    if '\\' in include_path:
      return True, results.DependencyViolation(
          include_path,
          MessageRule('Include paths may not include backslashes.'),
          rules)

    if '/' not in include_path:
      # Don't fail when no directory is specified. We may want to be more
      # strict about this in the future.
      if self._verbose:
        print ' WARNING: include specified with no directory: ' + include_path
      return True, None

    rule = rules.RuleApplyingTo(include_path, dependee_path)
    if (rule.allow == Rule.DISALLOW or
        (fail_on_temp_allow and rule.allow == Rule.TEMP_ALLOW)):
      return True, results.DependencyViolation(include_path, rule, rules)
    return True, None

  def CheckFile(self, rules, filepath):
    if self._verbose:
      print 'Checking: ' + filepath

    dependee_status = results.DependeeStatus(filepath)
    ret_val = ''  # We'll collect the error messages in here
    last_include = 0
    with codecs.open(filepath, encoding='utf-8') as f:
      in_if0 = 0
      for line_num, line in enumerate(f):
        if line_num - last_include > self._MAX_UNINTERESTING_LINES:
          break

        line = line.strip()

        # Check to see if we're at / inside an #if 0 block
        if line.startswith('#if 0'):
          in_if0 += 1
          continue
        if in_if0 > 0:
          if line.startswith('#if'):
            in_if0 += 1
          elif line.startswith('#endif'):
            in_if0 -= 1
          continue

        is_include, violation = self.CheckLine(rules, line, filepath)
        if is_include:
          last_include = line_num
        if violation:
          dependee_status.AddViolation(violation)

    return dependee_status

  @staticmethod
  def IsCppFile(file_path):
    """Returns True iff the given path ends in one of the extensions
    handled by this checker.
    """
    return os.path.splitext(file_path)[1] in CppChecker.EXTENSIONS
