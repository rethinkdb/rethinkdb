# Copyright 2012 the V8 project authors. All rights reserved.
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of Google Inc. nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


import os
import shutil
import time

from pool import Pool
from . import commands
from . import perfdata
from . import utils


class Job(object):
  def __init__(self, command, dep_command, test_id, timeout, verbose):
    self.command = command
    self.dep_command = dep_command
    self.id = test_id
    self.timeout = timeout
    self.verbose = verbose


def RunTest(job):
  start_time = time.time()
  if job.dep_command is not None:
    dep_output = commands.Execute(job.dep_command, job.verbose, job.timeout)
    # TODO(jkummerow): We approximate the test suite specific function
    # IsFailureOutput() by just checking the exit code here. Currently
    # only cctests define dependencies, for which this simplification is
    # correct.
    if dep_output.exit_code != 0:
      return (job.id, dep_output, time.time() - start_time)
  output = commands.Execute(job.command, job.verbose, job.timeout)
  return (job.id, output, time.time() - start_time)

class Runner(object):

  def __init__(self, suites, progress_indicator, context):
    self.datapath = os.path.join("out", "testrunner_data")
    self.perf_data_manager = perfdata.PerfDataManager(self.datapath)
    self.perfdata = self.perf_data_manager.GetStore(context.arch, context.mode)
    self.perf_failures = False
    self.printed_allocations = False
    self.tests = [ t for s in suites for t in s.tests ]
    if not context.no_sorting:
      for t in self.tests:
        t.duration = self.perfdata.FetchPerfData(t) or 1.0
      self.tests.sort(key=lambda t: t.duration, reverse=True)
    self._CommonInit(len(self.tests), progress_indicator, context)

  def _CommonInit(self, num_tests, progress_indicator, context):
    self.indicator = progress_indicator
    progress_indicator.runner = self
    self.context = context
    self.succeeded = 0
    self.total = num_tests
    self.remaining = num_tests
    self.failed = []
    self.crashed = 0
    self.reran_tests = 0

  def _RunPerfSafe(self, fun):
    try:
      fun()
    except Exception, e:
      print("PerfData exception: %s" % e)
      self.perf_failures = True

  def _GetJob(self, test):
    command = self.GetCommand(test)
    timeout = self.context.timeout
    if ("--stress-opt" in test.flags or
        "--stress-opt" in self.context.mode_flags or
        "--stress-opt" in self.context.extra_flags):
      timeout *= 4
    if test.dependency is not None:
      dep_command = [ c.replace(test.path, test.dependency) for c in command ]
    else:
      dep_command = None
    return Job(command, dep_command, test.id, timeout, self.context.verbose)

  def _MaybeRerun(self, pool, test):
    if test.run <= self.context.rerun_failures_count:
      # Possibly rerun this test if its run count is below the maximum per
      # test. <= as the flag controls reruns not including the first run.
      if test.run == 1:
        # Count the overall number of reran tests on the first rerun.
        if self.reran_tests < self.context.rerun_failures_max:
          self.reran_tests += 1
        else:
          # Don't rerun this if the overall number of rerun tests has been
          # reached.
          return
      if test.run >= 2 and test.duration > self.context.timeout / 20.0:
        # Rerun slow tests at most once.
        return

      # Rerun this test.
      test.duration = None
      test.output = None
      test.run += 1
      pool.add([self._GetJob(test)])
      self.remaining += 1

  def _ProcessTestNormal(self, test, result, pool):
    self.indicator.AboutToRun(test)
    test.output = result[1]
    test.duration = result[2]
    has_unexpected_output = test.suite.HasUnexpectedOutput(test)
    if has_unexpected_output:
      self.failed.append(test)
      if test.output.HasCrashed():
        self.crashed += 1
    else:
      self.succeeded += 1
    self.remaining -= 1
    # For the indicator, everything that happens after the first run is treated
    # as unexpected even if it flakily passes in order to include it in the
    # output.
    self.indicator.HasRun(test, has_unexpected_output or test.run > 1)
    if has_unexpected_output:
      # Rerun test failures after the indicator has processed the results.
      self._MaybeRerun(pool, test)
    # Update the perf database if the test succeeded.
    return not has_unexpected_output

  def _ProcessTestPredictable(self, test, result, pool):
    def HasDifferentAllocations(output1, output2):
      def AllocationStr(stdout):
        for line in reversed((stdout or "").splitlines()):
          if line.startswith("### Allocations = "):
            self.printed_allocations = True
            return line
        return ""
      return (AllocationStr(output1.stdout) != AllocationStr(output2.stdout))

    # Always pass the test duration for the database update.
    test.duration = result[2]
    if test.run == 1 and result[1].HasTimedOut():
      # If we get a timeout in the first run, we are already in an
      # unpredictable state. Just report it as a failure and don't rerun.
      self.indicator.AboutToRun(test)
      test.output = result[1]
      self.remaining -= 1
      self.failed.append(test)
      self.indicator.HasRun(test, True)
    if test.run > 1 and HasDifferentAllocations(test.output, result[1]):
      # From the second run on, check for different allocations. If a
      # difference is found, call the indicator twice to report both tests.
      # All runs of each test are counted as one for the statistic.
      self.indicator.AboutToRun(test)
      self.remaining -= 1
      self.failed.append(test)
      self.indicator.HasRun(test, True)
      self.indicator.AboutToRun(test)
      test.output = result[1]
      self.indicator.HasRun(test, True)
    elif test.run >= 3:
      # No difference on the third run -> report a success.
      self.indicator.AboutToRun(test)
      self.remaining -= 1
      self.succeeded += 1
      test.output = result[1]
      self.indicator.HasRun(test, False)
    else:
      # No difference yet and less than three runs -> add another run and
      # remember the output for comparison.
      test.run += 1
      test.output = result[1]
      pool.add([self._GetJob(test)])
    # Always update the perf database.
    return True

  def Run(self, jobs):
    self.indicator.Starting()
    self._RunInternal(jobs)
    self.indicator.Done()
    if self.failed or self.remaining:
      return 1
    return 0

  def _RunInternal(self, jobs):
    pool = Pool(jobs)
    test_map = {}
    # TODO(machenbach): Instead of filling the queue completely before
    # pool.imap_unordered, make this a generator that already starts testing
    # while the queue is filled.
    queue = []
    queued_exception = None
    for test in self.tests:
      assert test.id >= 0
      test_map[test.id] = test
      try:
        queue.append([self._GetJob(test)])
      except Exception, e:
        # If this failed, save the exception and re-raise it later (after
        # all other tests have had a chance to run).
        queued_exception = e
        continue
    try:
      it = pool.imap_unordered(RunTest, queue)
      for result in it:
        test = test_map[result[0]]
        if self.context.predictable:
          update_perf = self._ProcessTestPredictable(test, result, pool)
        else:
          update_perf = self._ProcessTestNormal(test, result, pool)
        if update_perf:
          self._RunPerfSafe(lambda: self.perfdata.UpdatePerfData(test))
    finally:
      pool.terminate()
      self._RunPerfSafe(lambda: self.perf_data_manager.close())
      if self.perf_failures:
        # Nuke perf data in case of failures. This might not work on windows as
        # some files might still be open.
        print "Deleting perf test data due to db corruption."
        shutil.rmtree(self.datapath)
    if queued_exception:
      raise queued_exception

    # Make sure that any allocations were printed in predictable mode.
    assert not self.context.predictable or self.printed_allocations

  def GetCommand(self, test):
    d8testflag = []
    shell = test.suite.shell()
    if shell == "d8":
      d8testflag = ["--test"]
    if utils.IsWindows():
      shell += ".exe"
    cmd = (self.context.command_prefix +
           [os.path.abspath(os.path.join(self.context.shell_dir, shell))] +
           d8testflag +
           ["--random-seed=%s" % self.context.random_seed] +
           test.suite.GetFlagsForTestCase(test, self.context) +
           self.context.extra_flags)
    return cmd


class BreakNowException(Exception):
  def __init__(self, value):
    self.value = value
  def __str__(self):
    return repr(self.value)
