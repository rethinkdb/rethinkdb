#!/usr/bin/env python
# Copyright 2014 the V8 project authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Performance runner for d8.

Call e.g. with tools/run-perf.py --arch ia32 some_suite.json

The suite json format is expected to be:
{
  "path": <relative path chunks to perf resources and main file>,
  "name": <optional suite name, file name is default>,
  "archs": [<architecture name for which this suite is run>, ...],
  "binary": <name of binary to run, default "d8">,
  "flags": [<flag to d8>, ...],
  "run_count": <how often will this suite run (optional)>,
  "run_count_XXX": <how often will this suite run for arch XXX (optional)>,
  "resources": [<js file to be loaded before main>, ...]
  "main": <main js perf runner file>,
  "results_regexp": <optional regexp>,
  "results_processor": <optional python results processor script>,
  "units": <the unit specification for the performance dashboard>,
  "tests": [
    {
      "name": <name of the trace>,
      "results_regexp": <optional more specific regexp>,
      "results_processor": <optional python results processor script>,
      "units": <the unit specification for the performance dashboard>,
    }, ...
  ]
}

The tests field can also nest other suites in arbitrary depth. A suite
with a "main" file is a leaf suite that can contain one more level of
tests.

A suite's results_regexp is expected to have one string place holder
"%s" for the trace name. A trace's results_regexp overwrites suite
defaults.

A suite's results_processor may point to an optional python script. If
specified, it is called after running the tests like this (with a path
relatve to the suite level's path):
<results_processor file> <same flags as for d8> <suite level name> <output>

The <output> is a temporary file containing d8 output. The results_regexp will
be applied to the output of this script.

A suite without "tests" is considered a performance test itself.

Full example (suite with one runner):
{
  "path": ["."],
  "flags": ["--expose-gc"],
  "archs": ["ia32", "x64"],
  "run_count": 5,
  "run_count_ia32": 3,
  "main": "run.js",
  "results_regexp": "^%s: (.+)$",
  "units": "score",
  "tests": [
    {"name": "Richards"},
    {"name": "DeltaBlue"},
    {"name": "NavierStokes",
     "results_regexp": "^NavierStokes: (.+)$"}
  ]
}

Full example (suite with several runners):
{
  "path": ["."],
  "flags": ["--expose-gc"],
  "archs": ["ia32", "x64"],
  "run_count": 5,
  "units": "score",
  "tests": [
    {"name": "Richards",
     "path": ["richards"],
     "main": "run.js",
     "run_count": 3,
     "results_regexp": "^Richards: (.+)$"},
    {"name": "NavierStokes",
     "path": ["navier_stokes"],
     "main": "run.js",
     "results_regexp": "^NavierStokes: (.+)$"}
  ]
}

Path pieces are concatenated. D8 is always run with the suite's path as cwd.
"""

from collections import OrderedDict
import json
import math
import optparse
import os
import re
import sys

from testrunner.local import commands
from testrunner.local import utils

ARCH_GUESS = utils.DefaultArch()
SUPPORTED_ARCHS = ["android_arm",
                   "android_arm64",
                   "android_ia32",
                   "arm",
                   "ia32",
                   "mips",
                   "mipsel",
                   "nacl_ia32",
                   "nacl_x64",
                   "x64",
                   "arm64"]

GENERIC_RESULTS_RE = re.compile(r"^RESULT ([^:]+): ([^=]+)= ([^ ]+) ([^ ]*)$")
RESULT_STDDEV_RE = re.compile(r"^\{([^\}]+)\}$")
RESULT_LIST_RE = re.compile(r"^\[([^\]]+)\]$")



def GeometricMean(values):
  """Returns the geometric mean of a list of values.

  The mean is calculated using log to avoid overflow.
  """
  values = map(float, values)
  return str(math.exp(sum(map(math.log, values)) / len(values)))


class Results(object):
  """Place holder for result traces."""
  def __init__(self, traces=None, errors=None):
    self.traces = traces or []
    self.errors = errors or []

  def ToDict(self):
    return {"traces": self.traces, "errors": self.errors}

  def WriteToFile(self, file_name):
    with open(file_name, "w") as f:
      f.write(json.dumps(self.ToDict()))

  def __add__(self, other):
    self.traces += other.traces
    self.errors += other.errors
    return self

  def __str__(self):  # pragma: no cover
    return str(self.ToDict())


class Node(object):
  """Represents a node in the suite tree structure."""
  def __init__(self, *args):
    self._children = []

  def AppendChild(self, child):
    self._children.append(child)


class DefaultSentinel(Node):
  """Fake parent node with all default values."""
  def __init__(self):
    super(DefaultSentinel, self).__init__()
    self.binary = "d8"
    self.run_count = 10
    self.timeout = 60
    self.path = []
    self.graphs = []
    self.flags = []
    self.resources = []
    self.results_regexp = None
    self.stddev_regexp = None
    self.units = "score"
    self.total = False


class Graph(Node):
  """Represents a suite definition.

  Can either be a leaf or an inner node that provides default values.
  """
  def __init__(self, suite, parent, arch):
    super(Graph, self).__init__()
    self._suite = suite

    assert isinstance(suite.get("path", []), list)
    assert isinstance(suite["name"], basestring)
    assert isinstance(suite.get("flags", []), list)
    assert isinstance(suite.get("resources", []), list)

    # Accumulated values.
    self.path = parent.path[:] + suite.get("path", [])
    self.graphs = parent.graphs[:] + [suite["name"]]
    self.flags = parent.flags[:] + suite.get("flags", [])
    self.resources = parent.resources[:] + suite.get("resources", [])

    # Descrete values (with parent defaults).
    self.binary = suite.get("binary", parent.binary)
    self.run_count = suite.get("run_count", parent.run_count)
    self.run_count = suite.get("run_count_%s" % arch, self.run_count)
    self.timeout = suite.get("timeout", parent.timeout)
    self.units = suite.get("units", parent.units)
    self.total = suite.get("total", parent.total)

    # A regular expression for results. If the parent graph provides a
    # regexp and the current suite has none, a string place holder for the
    # suite name is expected.
    # TODO(machenbach): Currently that makes only sense for the leaf level.
    # Multiple place holders for multiple levels are not supported.
    if parent.results_regexp:
      regexp_default = parent.results_regexp % re.escape(suite["name"])
    else:
      regexp_default = None
    self.results_regexp = suite.get("results_regexp", regexp_default)

    # A similar regular expression for the standard deviation (optional).
    if parent.stddev_regexp:
      stddev_default = parent.stddev_regexp % re.escape(suite["name"])
    else:
      stddev_default = None
    self.stddev_regexp = suite.get("stddev_regexp", stddev_default)


class Trace(Graph):
  """Represents a leaf in the suite tree structure.

  Handles collection of measurements.
  """
  def __init__(self, suite, parent, arch):
    super(Trace, self).__init__(suite, parent, arch)
    assert self.results_regexp
    self.results = []
    self.errors = []
    self.stddev = ""

  def ConsumeOutput(self, stdout):
    try:
      self.results.append(
          re.search(self.results_regexp, stdout, re.M).group(1))
    except:
      self.errors.append("Regexp \"%s\" didn't match for test %s."
                         % (self.results_regexp, self.graphs[-1]))

    try:
      if self.stddev_regexp and self.stddev:
        self.errors.append("Test %s should only run once since a stddev "
                           "is provided by the test." % self.graphs[-1])
      if self.stddev_regexp:
        self.stddev = re.search(self.stddev_regexp, stdout, re.M).group(1)
    except:
      self.errors.append("Regexp \"%s\" didn't match for test %s."
                         % (self.stddev_regexp, self.graphs[-1]))

  def GetResults(self):
    return Results([{
      "graphs": self.graphs,
      "units": self.units,
      "results": self.results,
      "stddev": self.stddev,
    }], self.errors)


class Runnable(Graph):
  """Represents a runnable suite definition (i.e. has a main file).
  """
  @property
  def main(self):
    return self._suite.get("main", "")

  def ChangeCWD(self, suite_path):
    """Changes the cwd to to path defined in the current graph.

    The tests are supposed to be relative to the suite configuration.
    """
    suite_dir = os.path.abspath(os.path.dirname(suite_path))
    bench_dir = os.path.normpath(os.path.join(*self.path))
    os.chdir(os.path.join(suite_dir, bench_dir))

  def GetCommand(self, shell_dir):
    # TODO(machenbach): This requires +.exe if run on windows.
    return (
      [os.path.join(shell_dir, self.binary)] +
      self.flags +
      self.resources +
      [self.main]
    )

  def Run(self, runner):
    """Iterates over several runs and handles the output for all traces."""
    for stdout in runner():
      for trace in self._children:
        trace.ConsumeOutput(stdout)
    res = reduce(lambda r, t: r + t.GetResults(), self._children, Results())

    if not res.traces or not self.total:
      return res

    # Assume all traces have the same structure.
    if len(set(map(lambda t: len(t["results"]), res.traces))) != 1:
      res.errors.append("Not all traces have the same number of results.")
      return res

    # Calculate the geometric means for all traces. Above we made sure that
    # there is at least one trace and that the number of results is the same
    # for each trace.
    n_results = len(res.traces[0]["results"])
    total_results = [GeometricMean(t["results"][i] for t in res.traces)
                     for i in range(0, n_results)]
    res.traces.append({
      "graphs": self.graphs + ["Total"],
      "units": res.traces[0]["units"],
      "results": total_results,
      "stddev": "",
    })
    return res

class RunnableTrace(Trace, Runnable):
  """Represents a runnable suite definition that is a leaf."""
  def __init__(self, suite, parent, arch):
    super(RunnableTrace, self).__init__(suite, parent, arch)

  def Run(self, runner):
    """Iterates over several runs and handles the output."""
    for stdout in runner():
      self.ConsumeOutput(stdout)
    return self.GetResults()


class RunnableGeneric(Runnable):
  """Represents a runnable suite definition with generic traces."""
  def __init__(self, suite, parent, arch):
    super(RunnableGeneric, self).__init__(suite, parent, arch)

  def Run(self, runner):
    """Iterates over several runs and handles the output."""
    traces = OrderedDict()
    for stdout in runner():
      for line in stdout.strip().splitlines():
        match = GENERIC_RESULTS_RE.match(line)
        if match:
          stddev = ""
          graph = match.group(1)
          trace = match.group(2)
          body = match.group(3)
          units = match.group(4)
          match_stddev = RESULT_STDDEV_RE.match(body)
          match_list = RESULT_LIST_RE.match(body)
          if match_stddev:
            result, stddev = map(str.strip, match_stddev.group(1).split(","))
            results = [result]
          elif match_list:
            results = map(str.strip, match_list.group(1).split(","))
          else:
            results = [body.strip()]

          trace_result = traces.setdefault(trace, Results([{
            "graphs": self.graphs + [graph, trace],
            "units": (units or self.units).strip(),
            "results": [],
            "stddev": "",
          }], []))
          trace_result.traces[0]["results"].extend(results)
          trace_result.traces[0]["stddev"] = stddev

    return reduce(lambda r, t: r + t, traces.itervalues(), Results())


def MakeGraph(suite, arch, parent):
  """Factory method for making graph objects."""
  if isinstance(parent, Runnable):
    # Below a runnable can only be traces.
    return Trace(suite, parent, arch)
  elif suite.get("main"):
    # A main file makes this graph runnable.
    if suite.get("tests"):
      # This graph has subgraphs (traces).
      return Runnable(suite, parent, arch)
    else:
      # This graph has no subgraphs, it's a leaf.
      return RunnableTrace(suite, parent, arch)
  elif suite.get("generic"):
    # This is a generic suite definition. It is either a runnable executable
    # or has a main js file.
    return RunnableGeneric(suite, parent, arch)
  elif suite.get("tests"):
    # This is neither a leaf nor a runnable.
    return Graph(suite, parent, arch)
  else:  # pragma: no cover
    raise Exception("Invalid suite configuration.")


def BuildGraphs(suite, arch, parent=None):
  """Builds a tree structure of graph objects that corresponds to the suite
  configuration.
  """
  parent = parent or DefaultSentinel()

  # TODO(machenbach): Implement notion of cpu type?
  if arch not in suite.get("archs", ["ia32", "x64"]):
    return None

  graph = MakeGraph(suite, arch, parent)
  for subsuite in suite.get("tests", []):
    BuildGraphs(subsuite, arch, graph)
  parent.AppendChild(graph)
  return graph


def FlattenRunnables(node):
  """Generator that traverses the tree structure and iterates over all
  runnables.
  """
  if isinstance(node, Runnable):
    yield node
  elif isinstance(node, Node):
    for child in node._children:
      for result in FlattenRunnables(child):
        yield result
  else:  # pragma: no cover
    raise Exception("Invalid suite configuration.")


# TODO: Implement results_processor.
def Main(args):
  parser = optparse.OptionParser()
  parser.add_option("--arch",
                    help=("The architecture to run tests for, "
                          "'auto' or 'native' for auto-detect"),
                    default="x64")
  parser.add_option("--buildbot",
                    help="Adapt to path structure used on buildbots",
                    default=False, action="store_true")
  parser.add_option("--json-test-results",
                    help="Path to a file for storing json results.")
  parser.add_option("--outdir", help="Base directory with compile output",
                    default="out")
  (options, args) = parser.parse_args(args)

  if len(args) == 0:  # pragma: no cover
    parser.print_help()
    return 1

  if options.arch in ["auto", "native"]:  # pragma: no cover
    options.arch = ARCH_GUESS

  if not options.arch in SUPPORTED_ARCHS:  # pragma: no cover
    print "Unknown architecture %s" % options.arch
    return 1

  workspace = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))

  if options.buildbot:
    shell_dir = os.path.join(workspace, options.outdir, "Release")
  else:
    shell_dir = os.path.join(workspace, options.outdir,
                             "%s.release" % options.arch)

  results = Results()
  for path in args:
    path = os.path.abspath(path)

    if not os.path.exists(path):  # pragma: no cover
      results.errors.append("Configuration file %s does not exist." % path)
      continue

    with open(path) as f:
      suite = json.loads(f.read())

    # If no name is given, default to the file name without .json.
    suite.setdefault("name", os.path.splitext(os.path.basename(path))[0])

    for runnable in FlattenRunnables(BuildGraphs(suite, options.arch)):
      print ">>> Running suite: %s" % "/".join(runnable.graphs)
      runnable.ChangeCWD(path)

      def Runner():
        """Output generator that reruns several times."""
        for i in xrange(0, max(1, runnable.run_count)):
          # TODO(machenbach): Allow timeout per arch like with run_count per
          # arch.
          output = commands.Execute(runnable.GetCommand(shell_dir),
                                    timeout=runnable.timeout)
          print ">>> Stdout (#%d):" % (i + 1)
          print output.stdout
          if output.stderr:  # pragma: no cover
            # Print stderr for debugging.
            print ">>> Stderr (#%d):" % (i + 1)
            print output.stderr
          if output.timed_out:
            print ">>> Test timed out after %ss." % runnable.timeout
          yield output.stdout

      # Let runnable iterate over all runs and handle output.
      results += runnable.Run(Runner)

  if options.json_test_results:
    results.WriteToFile(options.json_test_results)
  else:  # pragma: no cover
    print results

  return min(1, len(results.errors))

if __name__ == "__main__":  # pragma: no cover
  sys.exit(Main(sys.argv[1:]))
