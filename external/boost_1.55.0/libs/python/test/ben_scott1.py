# Copyright David Abrahams 2004. Distributed under the Boost
# Software License, Version 1.0. (See accompanying
# file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
# This regression test checks that call_method<T>(...) where T is a
# non-reference, non-pointer type that happens to be held inside the
# result object (and thus is found as an lvalue) works.
from ben_scott1_ext import *

class CreatorImpl(Creator):
   def create(self):
      return Product()

factory = Factory()
c = CreatorImpl()
factory.reg(c)

a = factory.create()
