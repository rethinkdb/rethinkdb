# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

false = 0;
true = 1;

import doctest, numeric_tests
def _count_failures(test_modules = (numeric_tests,)):
    failures = 0
    for m in test_modules:
        failures += doctest.testmod(m)[0]
    return failures

def _run(args = None):
    import sys, numarray_tests, numeric_tests

    if args is not None:
        sys.argv = args

    # See which of the numeric modules are installed
    has_numeric = 0
    try: import Numeric
    except ImportError: pass
    else:
        has_numeric = 1
        m = Numeric

    has_numarray = 0
    try: import numarray
    except ImportError: pass
    else:
        has_numarray = 1
        m = numarray
    
    # Bail if neither one is installed
    if not (has_numeric or has_numarray):
        return 0

    # test the info routine outside the doctest. See numpy.cpp for an
    # explanation
    import numpy_ext
    if (has_numarray):
        numpy_ext.info(m.array((1,2,3)))

    failures = 0

    #
    # Run tests 4 different ways if both modules are installed, just
    # to show that set_module_and_type() is working properly
    #
    
    # run all the tests with default module search
    print 'testing default extension module:', \
          numpy_ext.get_module_name() or '[numeric support not installed]'

    failures += _count_failures()
        
    # test against Numeric if installed
    if has_numeric:
        print 'testing Numeric module explicitly'
        numpy_ext.set_module_and_type('Numeric', 'ArrayType')
        
        failures += _count_failures()
            
    if has_numarray:
        print 'testing numarray module explicitly'
        numpy_ext.set_module_and_type('numarray', 'NDArray')
        # Add the _numarray_tests to the list of things to test in
        # this case.
        failures += _count_failures((numarray_tests, numeric_tests))

    # see that we can go back to the default
    numpy_ext.set_module_and_type('', '')
    print 'testing default module again:', \
          numpy_ext.get_module_name() or '[numeric support not installed]'
    
    failures += _count_failures()
    
    return failures
    
if __name__ == '__main__':
    print "running..."
    import sys
    status = _run()
    if (status == 0): print "Done."
    sys.exit(status)
