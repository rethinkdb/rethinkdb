# Copyright David Abrahams 2006. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
import printer

# So we can coerce portably across Python versions
bool = type(1 == 1)

'''
>>> from numpy_ext import *
>>> x = new_array()
>>> y = x.copy()
>>> p = _printer()
>>> check = p.check
>>> exercise_numarray(x, p)

>>> check(str(y))

>>> check(y.argmax());
>>> check(y.argmax(0));

>>> check(y.argmin());
>>> check(y.argmin(0));

>>> check(y.argsort());
>>> check(y.argsort(1));

>>> y.byteswap();
>>> check(y);

>>> check(y.diagonal());
>>> check(y.diagonal(1));
>>> check(y.diagonal(0, 0));
>>> check(y.diagonal(0, 1, 0));

>>> check(y.is_c_array());

# coerce because numarray still returns an int and the C++ interface forces
# the return type to bool
>>> check( bool(y.isbyteswapped()) ); 

>>> check(y.trace());
>>> check(y.trace(1));
>>> check(y.trace(0, 0));
>>> check(y.trace(0, 1, 0));

>>> check(y.new('D').getshape());
>>> check(y.new('D').type());
>>> y.sort();
>>> check(y);
>>> check(y.type());

>>> check(y.factory((1.2, 3.4)));
>>> check(y.factory((1.2, 3.4), "f8"))
>>> check(y.factory((1.2, 3.4), "f8", true))
>>> check(y.factory((1.2, 3.4), "f8", true, false))
>>> check(y.factory((1.2, 3.4), "f8", true, false, None))
>>> check(y.factory((1.2, 3.4), "f8", true, false, None, (1,2,1)))

>>> p.results
[]
>>> del p
'''
