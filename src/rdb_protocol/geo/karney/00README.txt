This is a C implementation of the geodesic algorithms described in

  C. F. F. Karney,
  Algorithms for geodesics,
  J. Geodesy 87, 43-55 (2013);
  http://dx.doi.org/10.1007/s00190-012-0578-z
  Addenda: http://geographiclib.sf.net/geod-addenda.html

For documentation, see

  http://geographiclib.sourceforge.net/html/C/

The code in this directory is entirely self-contained.  In particular,
it does not depend on the C++ classes.  You can compile and link the
example programs directly with something like:

  cc -o inverse inverse.c geodesic.c -lm
  echo 30 0 29.5 179.5 | ./inverse

Alternatively, you can build the examples using cmake.  For example, on
Linux systems you might do:

  mkdir BUILD
  cd BUILD
  cmake ..
  make
  echo 30 0 29.5 179.5 | ./inverse
