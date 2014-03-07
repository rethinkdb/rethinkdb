# Copyright David Abrahams 2006. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
class _printer(object):
    def __init__(self):
        self.results = [];
    def __call__(self, *stuff):
        for x in stuff:
            self.results.append(str(x))
    def check(self, x):
        if self.results[0] != str(x):
            print '  Expected:\n %s\n  but the C++ interface gave:\n %s' % (x, self.results[0])
        del self.results[0]
