# Copyright Ralf W. Grosse-Kunstleve 2006. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
r'''>>> import getting_started1
    >>> print getting_started1.greet()
    hello, world
    >>> number = 11
    >>> print number, '*', number, '=', getting_started1.square(number)
    11 * 11 = 121
'''

def run(args = None):
    if args is not None:
        import sys
        sys.argv = args
    import doctest, test_getting_started1
    return doctest.testmod(test_getting_started1)

if __name__ == '__main__':
    import sys
    sys.exit(run()[0])
