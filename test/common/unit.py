#!/user/bin/env python
# Copyright 2014-2016 RethinkDB, all rights reserved.

import collections, os, subprocess, sys

import test_framework, utils

class AllUnitTests(test_framework.Test):
    def __init__(self, filters=[]):
        super(AllUnitTests, self).__init__()
        self.filters = filters
        self.configured = False
        self.tests = None

    def filter(self, filter):
        return AllUnitTests(self.filters + [filter])

    def configure(self, conf):
        unit_executable = os.path.join(conf['BUILD_DIR'], "rethinkdb-unittest")
        if not os.access(unit_executable, os.X_OK):
            sys.stderr.write('Warning: no useable rethinkdb-unittest executable at: %s\n' % unit_executable)
            return test_framework.TestTree()
        gtestProcess = subprocess.Popen([unit_executable, "--gtest_list_tests"], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        output, _ = gtestProcess.communicate()
        if gtestProcess.returncode != 0:
            sys.stderr.write('Error: failed to run %s --gtest_list_tests\n\t%s' % (unit_executable, output))
            raise subprocess.CalledProcessError(gtestProcess.returncode, [unit_executable, "--gtest_list_tests"])
        key = None
        dict = collections.defaultdict(list)
        for line in output.split():
            if not line:
                continue
            elif line[-1] == '.':
                key = line[:-1]
            else:
                dict[key].append(line.strip())
        tests = test_framework.TestTree(
            (group, UnitTest(unit_executable, group, tests))
            for group, tests in list(dict.items()))
        for filter in self.filters:
            tests = tests.filter(filter)
        return tests

class UnitTest(test_framework.Test):
    def __init__(self, unit_executable, test, child_tests=[]):
        super(UnitTest, self).__init__()
        self.unit_executable = unit_executable
        self.test = test or "*"
        self.child_tests = child_tests

    def run(self):
        filter = self.test
        if self.child_tests:
            filter = filter + ".*"
        command = self.unit_executable + " --gtest_filter=" + filter
        exit_code = os.system(command)
        if exit_code:
            raise Exception("command failed (" + str(exit_code) + "): " + command)

    def filter(self, filter):
        if filter.all_same() or not self.child_tests:
            return self if filter.match() else None
        tests = test_framework.TestTree((
            (child, UnitTest(self.unit_executable, self.test + "." + child))
            for child in self.child_tests))
        return tests.filter(filter)
