# Copyright David Abrahams & Ralf W. Grosse-Kunsteve 2004-2006.
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
'''
>>> from docstring_ext import *

>>> def selected_doc(obj, *args):
...   doc = obj.__doc__.splitlines()
...   return "\\n".join(["|"+doc[i] for i in args])

>>> print selected_doc(X.__init__, 1, 2, 3, 4, 5)
|__init__( (object)self, (int)value) -> None :
|    this is the __init__ function
|    its documentation has two lines.
|
|    C++ signature :

>>> print selected_doc(X.value, 1, 2, 4, 7, 8, 10)
|value( (X)self) -> int :
|    gets the value of the object
|    C++ signature :
|value( (X)self) -> int :
|    also gets the value of the object
|    C++ signature :

>>> print selected_doc(create, 1, 2, 3, 4)
|create( (int)value) -> X :
|    creates a new X object
|
|    C++ signature :

>>> print selected_doc(fact, 1, 2, 3, 4)
|fact( (int)n) -> int :
|    compute the factorial
|
|    C++ signature :

>>> len(fact_usr_off_1.__doc__.splitlines())
5
>>> print selected_doc(fact_usr_off_1, 1, 3)
|fact_usr_off_1( (int)n) -> int :
|    C++ signature :
>>> len(fact_usr_on_1.__doc__.splitlines())
6
>>> print selected_doc(fact_usr_on_1, 1, 2, 4)
|fact_usr_on_1( (int)n) -> int :
|    usr on 1
|    C++ signature :
>>> len(fact_usr_off_2.__doc__.splitlines())
5
>>> print selected_doc(fact_usr_off_2, 1, 3)
|fact_usr_off_2( (int)n) -> int :
|    C++ signature :
>>> len(fact_usr_on_2.__doc__.splitlines())
6
>>> print selected_doc(fact_usr_on_2, 1, 2, 4)
|fact_usr_on_2( (int)n) -> int :
|    usr on 2
|    C++ signature :


>>> len(fact_sig_off_1.__doc__.splitlines())
2
>>> print selected_doc(fact_sig_off_1, 1)
|sig off 1
>>> len(fact_sig_on_1.__doc__.splitlines())
6
>>> print selected_doc(fact_sig_on_1, 1, 2, 4)
|fact_sig_on_1( (int)n) -> int :
|    sig on 1
|    C++ signature :

>>> len(fact_sig_off_2.__doc__.splitlines())
2
>>> print selected_doc(fact_sig_off_2, 1)
|sig off 2
>>> len(fact_sig_on_2.__doc__.splitlines())
6
>>> print selected_doc(fact_sig_on_2, 1, 2, 4)
|fact_sig_on_2( (int)n) -> int :
|    sig on 2
|    C++ signature :


>>> print fact_usr_off_sig_off_1.__doc__
None
>>> len(fact_usr_on_sig_on_1.__doc__.splitlines())
6
>>> print selected_doc(fact_usr_on_sig_on_1, 1, 2, 4)
|fact_usr_on_sig_on_1( (int)n) -> int :
|    usr on sig on 1
|    C++ signature :

>>> len(fact_usr_on_sig_off_1.__doc__.splitlines())
2
>>> print selected_doc(fact_usr_on_sig_off_1, 1)
|usr on sig off 1
>>> len(fact_usr_on_sig_on_2.__doc__.splitlines())
6
>>> print selected_doc(fact_usr_on_sig_on_2, 1, 2, 4)
|fact_usr_on_sig_on_2( (int)n) -> int :
|    usr on sig on 2
|    C++ signature :

>>> print selected_doc(fact_usr_on_psig_on_csig_off_1, 1, 2)
|fact_usr_on_psig_on_csig_off_1( (int)n) -> int :
|    usr on psig on csig off 1

>>> print selected_doc(fact_usr_on_psig_off_csig_on_1, 1, 3)
|usr on psig off csig on 1
|C++ signature :

>>> print fact_usr_off_psig_on_csig_off_1.__doc__.splitlines()[1]
fact_usr_off_psig_on_csig_off_1( (int)n) -> int

>>> print selected_doc(fact_usr_off_psig_off_csig_on_1,1)
|C++ signature :


'''

def run(args = None):
    import sys
    import doctest

    if args is not None:
        sys.argv = args

    import docstring_ext

    result = doctest.testmod(sys.modules.get(__name__))

    import pydoc
    import re
    docmodule = lambda m: re.sub(".\10", "", pydoc.text.docmodule(m))
    try:
        print 'printing module help:'
        print docmodule(docstring_ext)
    except object, x:
        print '********* failed **********'
        print x
        result = list(result)
        result[0] += 1
        return tuple(result)

    return result

if __name__ == '__main__':
    print "running..."
    import sys
    status = run()[0]
    if (status == 0): print "Done."
    sys.exit(status)
