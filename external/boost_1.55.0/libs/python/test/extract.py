# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
    >>> from extract_ext import *

Just about anything has a truth value in Python

    >>> assert check_bool(None)
    >>> extract_bool(None)
    0

    >>> assert check_bool(2)
    >>> extract_bool(2)
    1

    >>> assert not check_bool('')

Check that object manager types work properly. These are a different
case because they wrap Python objects instead of being wrapped by them.

    >>> assert not check_list(2)
    >>> try: x = extract_list(2)
    ... except TypeError, x:
    ...     if str(x) != 'Expecting an object of type list; got an object of type int instead':
    ...         print x
    ... else:
    ...     print 'expected an exception, got', x, 'instead'

Can't extract a list from a tuple. Use list(x) to convert a sequence
to a list:
    
    >>> assert not check_list((1, 2, 3))
    >>> assert check_list([1, 2, 3])
    >>> extract_list([1, 2, 3])
    [1, 2, 3]

Can get a char const* from a Python string:

    >>> assert check_cstring('hello')
    >>> extract_cstring('hello')
    'hello'

Can't get a char const* from a Python int:
    
    >>> assert not check_cstring(1)
    >>> try: x = extract_cstring(1)
    ... except TypeError: pass
    ... else:
    ...     print 'expected an exception, got', x, 'instead'

Extract an std::string (class) rvalue from a native Python type

    >>> assert check_string('hello')
    >>> extract_string('hello')
    'hello'

Constant references are not treated as rvalues for the purposes of
extract:

    >>> assert not check_string_cref('hello')

We can extract lvalues where appropriate:

    >>> x = X(42)
    >>> check_X(x)
    1
    >>> extract_X(x)
    X(42)

    >>> check_X_ptr(x)
    1
    >>> extract_X_ptr(x)
    X(42)
    >>> extract_X_ref(x)
    X(42)

Demonstrate that double-extraction of an rvalue works, and all created
copies of the object are destroyed:

    >>> n = count_Xs()
    >>> double_X(333)
    666
    >>> count_Xs() - n
    0

General check for cleanliness:

    >>> del x
    >>> count_Xs()
    0
'''

def run(args = None):
    import sys
    import doctest

    if args is not None:
        sys.argv = args
    return doctest.testmod(sys.modules.get(__name__))
    
if __name__ == '__main__':
    print "running..."
    import sys
    status = run()[0]
    if (status == 0): print "Done."
    sys.exit(status)
