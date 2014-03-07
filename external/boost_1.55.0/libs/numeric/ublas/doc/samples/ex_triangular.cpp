//          Copyright Gunter Winkler 2004 - 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)


#include <iostream>

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/triangular.hpp>

#include <boost/numeric/ublas/io.hpp>

using std::cout;
using std::endl;



namespace ublas = boost::numeric::ublas;


int main(int argc, char * argv[] ) {

  ublas::matrix<double> M (3, 3);
  for (std::size_t i=0; i < M.data().size(); ++i) { M.data()[i] = 1+i ; }

  std::cout << "full         M  = " << M << "\n" ;

  ublas::triangular_matrix<double, ublas::lower> L;
  ublas::triangular_matrix<double, ublas::unit_lower> UL;
  ublas::triangular_matrix<double, ublas::strict_lower> SL;
  
  L  = ublas::triangular_adaptor<ublas::matrix<double>, ublas::lower> (M);
  SL = ublas::triangular_adaptor<ublas::matrix<double>, ublas::strict_lower> (M); 
  UL = ublas::triangular_adaptor<ublas::matrix<double>, ublas::unit_lower> (M);  
  
  std::cout << "lower        L  = " << L << "\n" 
            << "strict lower SL = " << SL << "\n" 
            << "unit lower   UL = " << UL << "\n" ;

  ublas::triangular_matrix<double, ublas::upper> U;
  ublas::triangular_matrix<double, ublas::unit_upper> UU;
  ublas::triangular_matrix<double, ublas::strict_upper> SU;
  
  U =  ublas::triangular_adaptor<ublas::matrix<double>, ublas::upper> (M);
  SU = ublas::triangular_adaptor<ublas::matrix<double>, ublas::strict_upper> (M); 
  UU = ublas::triangular_adaptor<ublas::matrix<double>, ublas::unit_upper> (M);  

  std::cout << "upper        U  = " << U << "\n" 
            << "strict upper SU = " << SU << "\n" 
            << "unit upper   UU = " << UU << "\n" ;

  std::cout << "M = L + SU ? " << ((norm_inf( M - (L + SU) ) == 0.0)?"ok":"failed") << "\n";
  std::cout << "M = U + SL ? " << ((norm_inf( M - (U + SL) ) == 0.0)?"ok":"failed") << "\n";

}


