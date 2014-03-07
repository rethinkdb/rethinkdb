"""
 Copyright 2009-2012 Karsten Ahnert
 Copyright 2009-2012 Mario Mulansky

 Distributed under the Boost Software License, Version 1.0.
 (See accompanying file LICENSE_1_0.txt or
 copy at http://www.boost.org/LICENSE_1_0.txt)
"""


from os import popen
from os import system
from os.path import isfile
from numpy import *
#from pylab import *

#toolset = "gcc-4.5"
#toolset = "intel-11.1"
toolset = "msvc"
#toolset = "msvc-10.0"

#bin_path = "bin/gcc-4.5/release/"
#bin_path = "bin/intel-linux-11.1/release/"
bin_path = "bin\\msvc-10.0\\release\\threading-multi\\"
extension = ".exe"
#extension = ""

bins = [ "odeint_rk4_lorenz_array" , "odeint_rk4_lorenz_range" , "generic_odeint_rk4_lorenz" , "nr_rk4_lorenz" ,  "rt_generic_rk4_lorenz" , "gsl_rk4_lorenz" ]

results = []

print "Performance tests for " , bin_path
print

for bin in bins:
        #system( "bjam toolset=" + toolset + " -a " + bin );
        if isfile( bin_path + bin + extension):
                print "Running" , bin
                res = popen( bin_path+bin+extension ).read()
                print bin , res
                results.append( res )
        else:
                print "no executable found:" , bin_path + bin + extension
                results.append( 0 )

print "Results from" , bin_path
print

for i in range(len(bins)):
        print bins[i] , results[i]

res = array( results , dtype='float' )
savetxt( bin_path + "rk4_lorenz.dat" , res )

res = 100*res[0]/res

bar_width = 0.6

"""
figure(1)
title("Runge-Kutta 4 with " + toolset , fontsize=20)
bar( arange(6) , res , bar_width , color='blue' , linewidth=4 , edgecolor='blue' , ecolor='red') #, elinewidth=2, ecolor='red' )
xlim( -0.5 , 5.5+bar_width )
xticks( arange(6)+bar_width/2 , ('array' , 'range' , 'generic' , 'NR' , 'rt gen' , 'gsl' ) )
ylabel('Performance in %' , fontsize=20)

savefig( bin_path + "rk4_lorenz.png" )

show()
"""
