#!/usr/bin/python

"""Basic statistics utility functions.

The implementation of Student's t distribution inverse CDF was ported to Python
from JSci. The parameters are set to only be accurate to approximately 5
decimal places.

The JSci port comes frist. "New" code is near the bottom.

JSci information:
http://jsci.sourceforge.net/
Original Author: Mark Hale
Original Licence: LGPL
"""

import math

# Relative machine precision.
EPS = 2.22e-16
# The smallest positive floating-point number such that 1/xminin is machine representable.
XMININ = 2.23e-308
# Square root of 2 * pi
SQRT2PI = 2.5066282746310005024157652848110452530069867406099
LOGSQRT2PI = math.log(SQRT2PI);
# Rough estimate of the fourth root of logGamma_xBig
lg_frtbig = 2.25e76
pnt68 = 0.6796875
# lower value = higher precision
PRECISION = 4.0*EPS
def betaFraction(x, p, q):
    """Evaluates of continued fraction part of incomplete beta function.
    
    Based on an idea from Numerical Recipes (W.H. Press et al, 1992)."""

    sum_pq  = p + q
    p_plus  = p + 1.0
    p_minus = p - 1.0
    h = 1.0-sum_pq*x/p_plus;
    if abs(h) < XMININ:
        h = XMININ
    h = 1.0/h
    frac = h
    m = 1
    delta = 0.0
    c = 1.0

    while m <= MAX_ITERATIONS and abs(delta-1.0) > PRECISION:
        m2 = 2*m

        # even index for d 
        d=m*(q-m)*x/((p_minus+m2)*(p+m2))
        h=1.0+d*h
        if abs(h) < XMININ: h=XMININ
        h=1.0/h;
        c=1.0+d/c;
        if abs(c) < XMININ: c=XMININ
        frac *= h*c;

        # odd index for d
        d = -(p+m)*(sum_pq+m)*x/((p+m2)*(p_plus+m2))
        h=1.0+d*h
        if abs(h) < XMININ: h=XMININ;
        h=1.0/h
        c=1.0+d/c
        if abs(c) < XMININ: c = XMININ
        delta=h*c
        frac *= delta
        m += 1

    return frac

# The largest argument for which <code>logGamma(x)</code> is representable in the machine.
LOG_GAMMA_X_MAX_VALUE = 2.55e305
# Log Gamma related constants
lg_d1 = -0.5772156649015328605195174;
lg_d2 = 0.4227843350984671393993777;
lg_d4 = 1.791759469228055000094023;
lg_p1 = [ 4.945235359296727046734888,
    201.8112620856775083915565, 2290.838373831346393026739,
    11319.67205903380828685045, 28557.24635671635335736389,
    38484.96228443793359990269, 26377.48787624195437963534,
    7225.813979700288197698961 ]
lg_q1 = [ 67.48212550303777196073036,
    1113.332393857199323513008, 7738.757056935398733233834,
    27639.87074403340708898585, 54993.10206226157329794414,
    61611.22180066002127833352, 36351.27591501940507276287,
    8785.536302431013170870835 ]
lg_p2 = [ 4.974607845568932035012064,
    542.4138599891070494101986, 15506.93864978364947665077,
    184793.2904445632425417223, 1088204.76946882876749847,
    3338152.967987029735917223, 5106661.678927352456275255,
    3074109.054850539556250927 ]
lg_q2 = [ 183.0328399370592604055942,
    7765.049321445005871323047, 133190.3827966074194402448,
    1136705.821321969608938755, 5267964.117437946917577538,
    13467014.54311101692290052, 17827365.30353274213975932,
    9533095.591844353613395747 ]
lg_p4 = [ 14745.02166059939948905062,
    2426813.369486704502836312, 121475557.4045093227939592,
    2663432449.630976949898078, 29403789566.34553899906876,
    170266573776.5398868392998, 492612579337.743088758812,
    560625185622.3951465078242 ]
lg_q4 = [ 2690.530175870899333379843,
    639388.5654300092398984238, 41355999.30241388052042842,
    1120872109.61614794137657, 14886137286.78813811542398,
    101680358627.2438228077304, 341747634550.7377132798597,
    446315818741.9713286462081 ]
lg_c = [ -0.001910444077728,8.4171387781295e-4,
    -5.952379913043012e-4, 7.93650793500350248e-4,
    -0.002777777777777681622553, 0.08333333333333333331554247,
    0.0057083835261 ]
def logGamma(x):
    """The natural logarithm of the gamma function.
Based on public domain NETLIB (Fortran) code by W. J. Cody and L. Stoltz<BR>
Applied Mathematics Division<BR>
Argonne National Laboratory<BR>
Argonne, IL 60439<BR>
<P>
References:
<OL>
<LI>W. J. Cody and K. E. Hillstrom, 'Chebyshev Approximations for the Natural Logarithm of the Gamma Function,' Math. Comp. 21, 1967, pp. 198-203.
<LI>K. E. Hillstrom, ANL/AMD Program ANLC366S, DGAMMA/DLGAMA, May, 1969.
<LI>Hart, Et. Al., Computer Approximations, Wiley and sons, New York, 1968.
</OL></P><P>
From the original documentation:
</P><P>
This routine calculates the LOG(GAMMA) function for a positive real argument X.
Computation is based on an algorithm outlined in references 1 and 2.
The program uses rational functions that theoretically approximate LOG(GAMMA)
to at least 18 significant decimal digits.  The approximation for X > 12 is from reference 3,
while approximations for X < 12.0 are similar to those in reference 1, but are unpublished.
The accuracy achieved depends on the arithmetic system, the compiler, the intrinsic functions,
and proper selection of the machine-dependent constants.
</P><P>
Error returns:<BR>
The program returns the value XINF for X .LE. 0.0 or when overflow would occur.
The computation is believed to be free of underflow and overflow."""

    y = x
    if y < 0.0 or y > LOG_GAMMA_X_MAX_VALUE:
        # Bad arguments
        return float("inf")

    if y <= EPS:
        return -math.log(y)

    if y <= 1.5:
        if (y < pnt68):
            corr = -math.log(y)
            xm1 = y
        else:
            corr = 0.0;
            xm1 = y - 1.0;

        if y <= 0.5 or y >= pnt68:
            xden = 1.0;
            xnum = 0.0;
            for i in xrange(8):
                xnum = xnum * xm1 + lg_p1[i];
                xden = xden * xm1 + lg_q1[i];
            return corr + xm1 * (lg_d1 + xm1 * (xnum / xden));
        else:
            xm2 = y - 1.0;
            xden = 1.0;
            xnum = 0.0;
            for i in xrange(8):
                xnum = xnum * xm2 + lg_p2[i];
                xden = xden * xm2 + lg_q2[i];
            return corr + xm2 * (lg_d2 + xm2 * (xnum / xden));

    if (y <= 4.0):
        xm2 = y - 2.0;
        xden = 1.0;
        xnum = 0.0;
        for i in xrange(8):
            xnum = xnum * xm2 + lg_p2[i];
            xden = xden * xm2 + lg_q2[i];
        return xm2 * (lg_d2 + xm2 * (xnum / xden));

    if y <= 12.0:
        xm4 = y - 4.0;
        xden = -1.0;
        xnum = 0.0;
        for i in xrange(8):
            xnum = xnum * xm4 + lg_p4[i];
            xden = xden * xm4 + lg_q4[i];
        return lg_d4 + xm4 * (xnum / xden);

    assert y <= lg_frtbig
    res = lg_c[6];
    ysq = y * y;
    for i in xrange(6):
        res = res / ysq + lg_c[i];
    res /= y;
    corr = math.log(y);
    res = res + LOGSQRT2PI - 0.5 * corr;
    res += y * (corr - 1.0);
    return res


def logBeta(p, q):
    """The natural logarithm of the beta function."""
    assert p > 0
    assert q > 0
    if p <= 0 or q <= 0 or p + q > LOG_GAMMA_X_MAX_VALUE:
        return 0

    return logGamma(p)+logGamma(q)-logGamma(p+q)


def incompleteBeta(x, p, q):
    """Incomplete beta function.

    The computation is based on formulas from Numerical Recipes, Chapter 6.4 (W.H. Press et al, 1992).
    Ported from Java: http://jsci.sourceforge.net/"""

    assert 0 <= x <= 1
    assert p > 0
    assert q > 0
 
    # Range checks to avoid numerical stability issues?
    if x <= 0.0:
        return 0.0
    if x >= 1.0:
        return 1.0
    if p <= 0.0 or q <= 0.0 or (p+q) > LOG_GAMMA_X_MAX_VALUE:
        return 0.0

    beta_gam = math.exp(-logBeta(p,q) + p*math.log(x) + q*math.log(1.0-x))
    if x < (p+1.0)/(p+q+2.0):
        return beta_gam*betaFraction(x, p, q)/p
    else:
        return 1.0-(beta_gam*betaFraction(1.0-x,q,p)/q)


ACCURACY = 10**-7
MAX_ITERATIONS = 10000
def findRoot(value, x_low, x_high, function):
    """Use the bisection method to find root such that function(root) == value."""

    guess = (x_high + x_low) / 2.0
    v = function(guess)
    difference = v - value
    i = 0
    while abs(difference) > ACCURACY and i < MAX_ITERATIONS:
        i += 1
        if difference > 0:
            x_high = guess
        else:
            x_low = guess
        
        guess = (x_high + x_low) / 2.0
        v = function(guess)
        difference = v - value

    return guess


def StudentTCDF(degree_of_freedom, X):
    """Student's T distribution CDF. Returns probability that a value x < X.
    
    Ported from Java: http://jsci.sourceforge.net/"""

    A = 0.5 * incompleteBeta(degree_of_freedom/(degree_of_freedom+X*X), 0.5*degree_of_freedom, 0.5)
    if X > 0:
        return 1 - A
    return A


def InverseStudentT(degree_of_freedom, probability):
    """Inverse of Student's T distribution CDF. Returns the value x such that CDF(x) = probability.

    Ported from Java: http://jsci.sourceforge.net/

    This is not the best algorithm in the world. SciPy has a Fortran version
    (see special.stdtrit):
    http://svn.scipy.org/svn/scipy/trunk/scipy/stats/distributions.py
    http://svn.scipy.org/svn/scipy/trunk/scipy/special/cdflib/cdft.f

    Very detailed information:
    http://www.maths.ox.ac.uk/~shaww/finpapers/tdist.pdf
    """
    
    assert 0 <= probability <= 1
    
    if probability == 1:
        return float("inf")
    if probability == 0:
        return float("-inf")
    if probability == 0.5:
        return 0.0

    def f(x):
        return StudentTCDF(degree_of_freedom, x)

    return findRoot(probability, -10**4, 10**4, f)


def tinv(p, degree_of_freedom):
    """Similar to the TINV function in Excel
    
    p: 1-confidence (eg. 0.05 = 95% confidence)"""
    
    assert 0 <= p <= 1
    confidence = 1 - p
    return InverseStudentT(degree_of_freedom, (1+confidence)/2.0)


def memoize(function):
    cache = {}
    def closure(*args):
        if args not in cache:
            cache[args] = function(*args)
        return cache[args]
    return closure

# Cache tinv results, since we typically call it with the same args over and over
cached_tinv = memoize(tinv)


def stats(r, confidence_interval=0.05):
    """Returns statistics about a sequence of numbers.

    By default it computes the 95% confidence interval.

    Returns (average, median, standard deviation, min, max, confidence interval)"""

    total = sum(r)
    average = total/float(len(r))
    sum_deviation_squared = sum([(i-average)**2 for i in r])
    standard_deviation = math.sqrt(sum_deviation_squared/(len(r)-1 or 1))
    s = list(r)
    s.sort()
    median = s[len(s)/2]
    minimum = s[0]
    maximum = s[-1]
    # See: http://davidmlane.com/hyperstat/
    # confidence_95 = 1.959963984540051 * standard_deviation / math.sqrt(len(r))
    # We must estimate both using the t distribution:
    # http://davidmlane.com/hyperstat/B7483.html
    # s_m = s / sqrt(N)
    s_m = standard_deviation / math.sqrt(len(r))
    # Degrees of freedom = n-1
    # t = tinv(0.05, degrees_of_freedom)
    # confidence = +/- t * s_m
    confidence = cached_tinv(confidence_interval, len(r)-1) * s_m
    return average, median, standard_deviation, minimum, maximum, confidence
