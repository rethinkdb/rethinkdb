"""
 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Stochastic euler stepper example and Ornstein-Uhlenbeck process

 Distributed under the Boost Software License, Version 1.0.
(See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
"""


from pylab import *
from scipy import special

data1 = loadtxt("elliptic1.dat")
data2 = loadtxt("elliptic2.dat")
data3 = loadtxt("elliptic3.dat")

sn1,cn1,dn1,phi1 = special.ellipj( data1[:,0] , 0.51 )
sn2,cn2,dn2,phi2 = special.ellipj( data2[:,0] , 0.51 )
sn3,cn3,dn3,phi3 = special.ellipj( data3[:,0] , 0.51 )

semilogy( data1[:,0] , abs(data1[:,1]-sn1) )
semilogy( data2[:,0] , abs(data2[:,1]-sn2) , 'ro' )
semilogy( data3[:,0] , abs(data3[:,1]-sn3) , '--' )

show()



