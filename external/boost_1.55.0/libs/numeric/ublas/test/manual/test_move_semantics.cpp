/** test move semantics - run with and without BOOST_UBLAS_MOVE_SEMANTICS defined */

//          Copyright Nasos Iliopoulos, Gunter Winkler 2009.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE_1_0.txt or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#define BOOST_UBLAS_MOVE_SEMANTICS
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/io.hpp>

namespace ublas= boost::numeric::ublas;
std::vector<double> a;

ublas::vector<double> doubleit(ublas::vector<double> m)
{
    ublas::vector<double> r;
    r=2.0*m;
    std::cout << "Temporary pointer: " << &r[0] << std::endl;
    return r;
}
template <class T,size_t N>
ublas::bounded_vector<T,N > doubleit(ublas::bounded_vector<T, N> m)
{
    ublas::bounded_vector<T,N> r;
    r=2.0*m;
    std::cout << "Temporary pointer: " << &r[0] << std::endl;
    return r;
}

template <class T,size_t N>
ublas::c_vector<T,N > doubleit(ublas::c_vector<T, N> m)
{
    ublas::c_vector<T,N> r;
    r=2.0*m;
    std::cout << "Temporary pointer: " << &r[0] << std::endl;
    return r;
}

ublas::matrix<double> doubleit(ublas::matrix<double> m)
{
    ublas::matrix<double> r;
    r=2.0*m;
    std::cout << "Temporary pointer r: " << &r(0,0) << std::endl;
    return r;
}
template <class T,size_t N, size_t M>
ublas::bounded_matrix<T,N, M > doubleit(ublas::bounded_matrix<T, N, M> m)
{
    ublas::bounded_matrix<T,N, M> r;
    r=2.0*m;
    std::cout << "Temporary pointer: " <<  &(r(0,0)) << std::endl;
    return r;
}

template <class T,size_t N, size_t M>
ublas::c_matrix<T,N, M > doubleit(ublas::c_matrix<T, N, M> m)
{
    ublas::c_matrix<T,N, M> r;
    r=2.0*m;
    std::cout << "Temporary pointer: " << &(r(0,0)) << std::endl;
    return r;
}


void test1()
{
    std::cout << "vector<double> --------------------------------------------------------------------" << std::endl;
    ublas::vector<double> a(ublas::scalar_vector<double>(2,2.0));
    a = doubleit(a);
    std::cout << "Pointer (must be equal to temp. pointer if move semantics are enabled) : " << &a[0] << std::endl;

    std::cout << a << std::endl;

    std::cout << "bounded_vector<double,2> --------------------------------------------------------------------" << std::endl;
    ublas::bounded_vector<double,2> b(ublas::scalar_vector<double>(2,2.0));
    ublas::bounded_vector<double,2> c;
    noalias(c)=doubleit(b);
    std::cout << "Pointer (bounded_vector swaps by copy this should be different than temp. pointer) : " << &c[0] << std::endl;
    c(1)=0.0;
    std::cout << b << std::endl;
    std::cout << c << std::endl;

    std::cout << "c_vector<double,2> --------------------------------------------------------------------" << std::endl;
    ublas::c_vector<double,2> e=ublas::scalar_vector<double>(2,2.0);
    ublas::c_vector<double,2> f;
    f=doubleit(e);
    std::cout << "Pointer (c_vector swaps by copy this should be different than temp. pointer) : " << &f[0] << std::endl;
    f(1)=0;
    std::cout << e << std::endl;
    std::cout << f << std::endl;

}

void test2() {
    std::cout << "matrix<double> --------------------------------------------------------------------" << std::endl;
    ublas::matrix<double> a(ublas::scalar_matrix<double>(2, 3, 2.0));
    a = doubleit(a);
    std::cout << "Pointer (must be equal to temp. pointer if move semantics are enabled) : " << &(a(0,0)) << std::endl;
    std::cout << a << std::endl;

    std::cout << "bounded_matrix<double,2, 3> --------------------------------------------------------------------" << std::endl;
    ublas::bounded_matrix<double,2, 3> b(ublas::scalar_matrix<double>(2,3, 2.0));
    ublas::bounded_matrix<double,2, 3> c;
    noalias(c)=doubleit(b);
    std::cout << "Pointer (bounded_matrix swaps by copy this should be different than temp. pointer) : " << &(c(0,0)) << std::endl;
    c(1,1)=0.0;
    std::cout << b << std::endl;
    std::cout << c << std::endl;

    std::cout << "c_matrix<double,2 ,3> --------------------------------------------------------------------" << std::endl;
    ublas::c_matrix<double,2, 3> e=ublas::scalar_matrix<double>(2,3, 2.0);
    ublas::c_matrix<double,2, 3> f;
    f=doubleit(e);
    std::cout << "Pointer (c_matrix swaps by copy this should be different than temp. pointer) : " << &(f(0,0)) << std::endl;
    f(1,1)=0;
    std::cout << e << std::endl;
    std::cout << f << std::endl;
}

int main(){
    test1();
    test2();
    return 0;
}

