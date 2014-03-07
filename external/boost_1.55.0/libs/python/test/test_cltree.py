# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#!/usr/bin/env python

from cltree import basic,symbol,constant,variable

b = basic()
c = constant()
s = symbol()
v = variable()

assert isinstance(b,basic)
assert not isinstance(b,symbol)
assert not isinstance(b,constant)
assert not isinstance(b,variable)

assert isinstance(c,basic)
assert isinstance(c,constant)
assert not isinstance(c,symbol)
assert not isinstance(c,variable)

assert not isinstance(s,basic)
assert isinstance(s,symbol)
assert not isinstance(s,constant)
assert not isinstance(s,variable)

assert isinstance(v,basic)
assert not isinstance(v,symbol)
assert not isinstance(v,constant)
assert isinstance(v,variable)

print 'b=',b
assert repr(b)=='cltree.basic()'
print 's=',s
assert repr(s)!='cltree.wrapped_symbol()' # because not isinstance(s,basic)
print 'c=',c
assert repr(c)=='cltree.constant()'
print 'v=',v
assert repr(v)=='cltree.wrapped_variable()'


print 'ok'
