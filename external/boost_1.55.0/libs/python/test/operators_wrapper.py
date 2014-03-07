from operators_wrapper_ext import *

class D2(vector): pass
d2 = D2()

for lhs in (v,d,d2):
    -lhs
    for rhs in (v,d,d2):
        lhs + rhs
        lhs += rhs

