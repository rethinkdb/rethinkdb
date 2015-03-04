# Copyright (c) 2012 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""Results object and results formatters for checkdeps tool."""


import json


class DependencyViolation(object):
  """A single dependency violation."""

  def __init__(self, include_path, violated_rule, rules):
    # The include or import path that is in violation of a rule.
    self.include_path = include_path

    # The violated rule.
    self.violated_rule = violated_rule

    # The set of rules containing self.violated_rule.
    self.rules = rules


class DependeeStatus(object):
  """Results object for a dependee file."""

  def __init__(self, dependee_path):
    # Path of the file whose nonconforming dependencies are listed in
    # self.violations.
    self.dependee_path = dependee_path

    # List of DependencyViolation objects that apply to the dependee
    # file.  May be empty.
    self.violations = []

  def AddViolation(self, violation):
    """Adds a violation."""
    self.violations.append(violation)

  def HasViolations(self):
    """Returns True if this dependee is violating one or more rules."""
    return not not self.violations


class ResultsFormatter(object):
  """Base class for results formatters."""

  def AddError(self, dependee_status):
    """Add a formatted result to |self.results| for |dependee_status|,
    which is guaranteed to return True for
    |dependee_status.HasViolations|.
    """
    raise NotImplementedError()

  def GetResults(self):
    """Returns the results.  May be overridden e.g. to process the
    results that have been accumulated.
    """
    raise NotImplementedError()

  def PrintResults(self):
    """Prints the results to stdout."""
    raise NotImplementedError()


class NormalResultsFormatter(ResultsFormatter):
  """A results formatting object that produces the classical,
  detailed, human-readable output of the checkdeps tool.
  """

  def __init__(self, verbose):
    self.results = []
    self.verbose = verbose

  def AddError(self, dependee_status):
    lines = []
    lines.append('\nERROR in %s' % dependee_status.dependee_path)
    for violation in dependee_status.violations:
      lines.append(self.FormatViolation(violation, self.verbose))
    self.results.append('\n'.join(lines))

  @staticmethod
  def FormatViolation(violation, verbose=False):
    lines = []
    if verbose:
      lines.append('  For %s' % violation.rules)
    lines.append(
        '  Illegal include: "%s"\n    Because of %s' %
        (violation.include_path, str(violation.violated_rule)))
    return '\n'.join(lines)

  def GetResults(self):
    return self.results

  def PrintResults(self):
    for result in self.results:
      print result
    if self.results:
      print '\nFAILED\n'


class JSONResultsFormatter(ResultsFormatter):
  """A results formatter that outputs results to a file as JSON."""

  def __init__(self, output_path, wrapped_formatter=None):
    self.output_path = output_path
    self.wrapped_formatter = wrapped_formatter

    self.results = []

  def AddError(self, dependee_status):
    self.results.append({
        'dependee_path': dependee_status.dependee_path,
        'violations': [{
            'include_path': violation.include_path,
            'violated_rule': violation.violated_rule.AsDependencyTuple(),
        } for violation in dependee_status.violations]
    })

    if self.wrapped_formatter:
      self.wrapped_formatter.AddError(dependee_status)

  def GetResults(self):
    with open(self.output_path, 'w') as f:
      f.write(json.dumps(self.results))

    return self.results

  def PrintResults(self):
    if self.wrapped_formatter:
      self.wrapped_formatter.PrintResults()
      return

    print self.results


class TemporaryRulesFormatter(ResultsFormatter):
  """A results formatter that produces a single line per nonconforming
  include. The combined output is suitable for directly pasting into a
  DEPS file as a list of temporary-allow rules.
  """

  def __init__(self):
    self.violations = set()

  def AddError(self, dependee_status):
    for violation in dependee_status.violations:
      self.violations.add(violation.include_path)

  def GetResults(self):
    return ['  "!%s",' % path for path in sorted(self.violations)]

  def PrintResults(self):
    for result in self.GetResults():
      print result


class CountViolationsFormatter(ResultsFormatter):
  """A results formatter that produces a number, the count of #include
  statements that are in violation of the dependency rules.

  Note that you normally want to instantiate DepsChecker with
  ignore_temp_rules=True when you use this formatter.
  """

  def __init__(self):
    self.count = 0

  def AddError(self, dependee_status):
    self.count += len(dependee_status.violations)

  def GetResults(self):
    return '%d' % self.count

  def PrintResults(self):
    print self.count
