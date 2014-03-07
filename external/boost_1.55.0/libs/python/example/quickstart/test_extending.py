# Copyright Ralf W. Grosse-Kunstleve 2006. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

# Using the doctest module here to ensure that the results are as expected.
r'''>>> from extending import *
    >>> hi = hello('California')
    >>> hi.greet()
    'Hello from California'
    >>> invite(hi)
    'Hello from California! Please come soon!'
    >>> hi.invite()
    'Hello from California! Please come soon!'

    >>> class wordy(hello):
    ...     def greet(self):
    ...         return hello.greet(self) + ', where the weather is fine'
    ...
    >>> hi2 = wordy('Florida')
    >>> hi2.greet()
    'Hello from Florida, where the weather is fine'
    >>> invite(hi2)
    'Hello from Florida! Please come soon!'
'''

def run(args = None):
    if args is not None:
        import sys
        sys.argv = args
    import doctest, test_extending
    return doctest.testmod(test_extending, verbose=True)

if __name__ == '__main__':
    import sys
    sys.exit(run()[0])

