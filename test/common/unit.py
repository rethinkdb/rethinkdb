from os.path import abspath, join, dirname, pardir
from test_framework import Test, TestTree
from subprocess import check_output, check_call
from collections import defaultdict

class AllUnitTests(Test):
    def __init__(self, filters=[]):
        Test.__init__(self)
        self.filters = filters
        self.configured = False
        self.tests = None

    def filter(self, filter):
        return AllUnitTests(self.filters + [filter])

    def configure(self, conf):
        unit_executable = join(conf['BUILD_DIR'], "rethinkdb-unittest")
        output = check_output([unit_executable, "--gtest_list_tests"])
        key = None
        dict = defaultdict(list)
        for line in output.split("\n"):
            if not line:
                continue
            elif line[-1] == '.':
                key = line[:-1]
            else:
                dict[key].append(line.strip())
        tests = TestTree(
            (group, UnitTest(unit_executable, group, tests))
            for group, tests in dict.iteritems())
        for filter in self.filters:
            tests = tests.filter(filter)
        return tests

class UnitTest(Test):
    def __init__(self, unit_executable, test, child_tests=[]):
        Test.__init__(self)
        self.unit_executable = unit_executable
        self.test = test
        self.child_tests = child_tests

    def run(self):
        filter = self.test
        if self.child_tests:
            filter = filter + ".*"
        check_call([self.unit_executable, "--gtest_filter=" + filter])

    def filter(self, filter):
        if filter.all_same() or not self.child_tests:
            return self if filter.match() else None
        tests = TestTree((
            (child, UnitTest(self.unit_executable, self.test + "." + child))
            for child in self.child_tests))
        return tests.filter(filter)
