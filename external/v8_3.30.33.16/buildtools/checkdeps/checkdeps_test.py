#!/usr/bin/env python
# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Tests for checkdeps.
"""

import os
import unittest


import checkdeps
import results


class CheckDepsTest(unittest.TestCase):

  def setUp(self):
    self.deps_checker = checkdeps.DepsChecker(
        being_tested=True,
        base_directory=os.path.join(os.path.dirname(__file__), os.path.pardir))

  def ImplTestRegularCheckDepsRun(self, ignore_temp_rules, skip_tests):
    self.deps_checker._ignore_temp_rules = ignore_temp_rules
    self.deps_checker._skip_tests = skip_tests
    self.deps_checker.CheckDirectory(
        os.path.join(self.deps_checker.base_directory,
                     'checkdeps/testdata'))

    problems = self.deps_checker.results_formatter.GetResults()
    if skip_tests:
      self.failUnlessEqual(3, len(problems))
    else:
      self.failUnlessEqual(4, len(problems))

    def VerifySubstringsInProblems(key_path, substrings_in_sequence):
      """Finds the problem in |problems| that contains |key_path|,
      then verifies that each of |substrings_in_sequence| occurs in
      that problem, in the order they appear in
      |substrings_in_sequence|.
      """
      found = False
      key_path = os.path.normpath(key_path)
      for problem in problems:
        index = problem.find(key_path)
        if index != -1:
          for substring in substrings_in_sequence:
            index = problem.find(substring, index + 1)
            self.failUnless(index != -1, '%s in %s' % (substring, problem))
          found = True
          break
      if not found:
        self.fail('Found no problem for file %s' % key_path)

    if ignore_temp_rules:
      VerifySubstringsInProblems('testdata/allowed/test.h',
                                 ['-checkdeps/testdata/disallowed',
                                  'temporarily_allowed.h',
                                  '-third_party/explicitly_disallowed',
                                  'Because of no rule applying'])
    else:
      VerifySubstringsInProblems('testdata/allowed/test.h',
                                 ['-checkdeps/testdata/disallowed',
                                  '-third_party/explicitly_disallowed',
                                  'Because of no rule applying'])

    VerifySubstringsInProblems('testdata/disallowed/test.h',
                               ['-third_party/explicitly_disallowed',
                                'Because of no rule applying',
                                'Because of no rule applying'])
    VerifySubstringsInProblems('disallowed/allowed/test.h',
                               ['-third_party/explicitly_disallowed',
                                'Because of no rule applying',
                                'Because of no rule applying'])

    if not skip_tests:
      VerifySubstringsInProblems('allowed/not_a_test.cc',
                                 ['-checkdeps/testdata/disallowed'])

  def testRegularCheckDepsRun(self):
    self.ImplTestRegularCheckDepsRun(False, False)

  def testRegularCheckDepsRunIgnoringTempRules(self):
    self.ImplTestRegularCheckDepsRun(True, False)

  def testRegularCheckDepsRunSkipTests(self):
    self.ImplTestRegularCheckDepsRun(False, True)

  def testRegularCheckDepsRunIgnoringTempRulesSkipTests(self):
    self.ImplTestRegularCheckDepsRun(True, True)

  def CountViolations(self, ignore_temp_rules):
    self.deps_checker._ignore_temp_rules = ignore_temp_rules
    self.deps_checker.results_formatter = results.CountViolationsFormatter()
    self.deps_checker.CheckDirectory(
        os.path.join(self.deps_checker.base_directory,
                     'checkdeps/testdata'))
    return self.deps_checker.results_formatter.GetResults()

  def testCountViolations(self):
    self.failUnlessEqual('10', self.CountViolations(False))

  def testCountViolationsIgnoringTempRules(self):
    self.failUnlessEqual('11', self.CountViolations(True))

  def testTempRulesGenerator(self):
    self.deps_checker.results_formatter = results.TemporaryRulesFormatter()
    self.deps_checker.CheckDirectory(
        os.path.join(self.deps_checker.base_directory,
                     'checkdeps/testdata/allowed'))
    temp_rules = self.deps_checker.results_formatter.GetResults()
    expected = [u'  "!checkdeps/testdata/disallowed/bad.h",',
                u'  "!checkdeps/testdata/disallowed/teststuff/bad.h",',
                u'  "!third_party/explicitly_disallowed/bad.h",',
                u'  "!third_party/no_rule/bad.h",']
    self.failUnlessEqual(expected, temp_rules)

  def testCheckAddedIncludesAllGood(self):
    problems = self.deps_checker.CheckAddedCppIncludes(
      [['checkdeps/testdata/allowed/test.cc',
        ['#include "checkdeps/testdata/allowed/good.h"',
         '#include "checkdeps/testdata/disallowed/allowed/good.h"']
      ]])
    self.failIf(problems)

  def testCheckAddedIncludesManyGarbageLines(self):
    garbage_lines = ["My name is Sam%d\n" % num for num in range(50)]
    problems = self.deps_checker.CheckAddedCppIncludes(
      [['checkdeps/testdata/allowed/test.cc', garbage_lines]])
    self.failIf(problems)

  def testCheckAddedIncludesNoRule(self):
    problems = self.deps_checker.CheckAddedCppIncludes(
      [['checkdeps/testdata/allowed/test.cc',
        ['#include "no_rule_for_this/nogood.h"']
      ]])
    self.failUnless(problems)

  def testCheckAddedIncludesSkippedDirectory(self):
    problems = self.deps_checker.CheckAddedCppIncludes(
      [['checkdeps/testdata/disallowed/allowed/skipped/test.cc',
        ['#include "whatever/whocares.h"']
      ]])
    self.failIf(problems)

  def testCheckAddedIncludesTempAllowed(self):
    problems = self.deps_checker.CheckAddedCppIncludes(
      [['checkdeps/testdata/allowed/test.cc',
        ['#include "checkdeps/testdata/disallowed/temporarily_allowed.h"']
      ]])
    self.failUnless(problems)

  def testCopyIsDeep(self):
    # Regression test for a bug where we were making shallow copies of
    # Rules objects and therefore all Rules objects shared the same
    # dictionary for specific rules.
    #
    # The first pair should bring in a rule from testdata/allowed/DEPS
    # into that global dictionary that allows the
    # temp_allowed_for_tests.h file to be included in files ending
    # with _unittest.cc, and the second pair should completely fail
    # once the bug is fixed, but succeed (with a temporary allowance)
    # if the bug is in place.
    problems = self.deps_checker.CheckAddedCppIncludes(
      [['checkdeps/testdata/allowed/test.cc',
        ['#include "/checkdeps/testdata/disallowed/temporarily_allowed.h"']
       ],
       ['checkdeps/testdata/disallowed/foo_unittest.cc',
        ['#include "checkdeps/testdata/bongo/temp_allowed_for_tests.h"']
       ]])
    # With the bug in place, there would be two problems reported, and
    # the second would be for foo_unittest.cc.
    self.failUnless(len(problems) == 1)
    self.failUnless(problems[0][0].endswith('/test.cc'))


if __name__ == '__main__':
  unittest.main()
