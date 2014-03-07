/*
 * Copyright (c) 2004 Michael Stevens
 * Use, modification and distribution are subject to the
 * Boost Software License, Version 1.0. (See accompanying file
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

/*
 * Default construct test when possible
 */

template <class E>
struct default_construct
{
    static void test() {}
}; 
template <class VC>
struct default_construct<boost::numeric::ublas::vector_container<VC> >
{
    static void test ()
    {
        VC default_constuct;
        initialize_vector (default_constuct);
        std::cout << "default construct = " << default_constuct << std::endl;
    }
};
template <class MC>
struct default_construct<boost::numeric::ublas::matrix_container<MC> >
{
    static void test ()
    {
        MC default_constuct;
        initialize_vector (default_constuct);
        std::cout << "default construct = " << default_constuct << std::endl;
    }
};

/*
 * Initialise test values in vector/matrix
 */

template<class V>
void initialize_vector (V &v) {
    typename V::size_type size = v.size ();
    for (typename V::size_type i = 0; i < size; ++ i)
        v [i] = typename V::value_type ( i + 1.f );
}

template<class M>
void initialize_matrix_impl (M &m, ublas::packed_proxy_tag) {
    typename M::size_type size1 = m.size1 ();
#ifndef BOOST_UBLAS_NO_NESTED_CLASS_RELATION
    for (typename M::iterator1 i = m.begin1(); i != m.end1(); ++ i)
        for (typename M::iterator2 j = i.begin(); j != i.end(); ++ j)
            *j = typename M::value_type ( i.index1() * size1 + j.index2() + 1.f );
#else
    for (typename M::iterator1 i = m.begin1(); i != m.end1(); ++ i)
        for (typename M::iterator2 j = ublas::begin (i, ublas::iterator1_tag ()); j != ublas::end (i, ublas::iterator1_tag ()); ++ j)
            *j = typename M::value_type ( i.index1() * size1 + j.index2() + 1.f );
#endif
}

template<class M>
void initialize_matrix_impl (M &m, ublas::sparse_proxy_tag) {
    typename M::size_type size1 = m.size1 ();
    typename M::size_type size2 = m.size2 ();
    for (typename M::size_type i = 0; i < size1; ++ i)
        for (typename M::size_type j = 0; j < size2; ++ j)
            m (i, j) = typename M::value_type (i * size1 + j + 1.f);
}

template<class M>
void initialize_matrix (M &m) {
    initialize_matrix_impl (m, typename M::storage_category());
}

template<class M>
void initialize_matrix (M &m, ublas::lower_tag) {
    typename M::size_type size1 = m.size1 ();
    typename M::size_type size2 = m.size2 ();
    for (typename M::size_type i = 0; i < size1; ++ i) {
        typename M::size_type j = 0;
        for (; j <= i; ++ j)
            m (i, j) = i * size1 + j + 1.f;
        for (; j < size2; ++ j)
            m (i, j) = 0.f;
    }
}
template<class M>
void initialize_matrix (M &m, ublas::upper_tag) {
    typename M::size_type size1 = m.size1 ();
    typename M::size_type size2 = m.size2 ();
    for (typename M::size_type i = 0; i < size1; ++ i) {
        typename M::size_type j = 0;
        for (; j < i; ++ j)
            m (i, j) = 0.f;
        for (; j < size2; ++ j)
            m (i, j) = i * size1 + j + 1.f;
    }
}
