// Copyright 2002 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software 
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Boost.MultiArray Library
//  Authors: Ronald Garcia
//           Jeremy Siek
//           Andrew Lumsdaine
//  See http://www.boost.org/libs/multi_array for documentation.

// 
// slice.cpp - testing out slicing on a matrices
//

#include "generative_tests.hpp"
#include "boost/array.hpp"
#include "boost/mpl/if.hpp"
#include "boost/type_traits/is_same.hpp"

template <typename Array>
struct view_traits_mutable {
public:
#if 0 // RG - MSVC can't handle templates nested in templates. Use traits
  typedef typename Array::template array_view<3>::type array_view3;
  typedef typename Array::template array_view<2>::type array_view2;
#endif
  typedef typename boost::array_view_gen<Array,3>::type array_view3;
  typedef typename boost::array_view_gen<Array,2>::type array_view2;
};

template <typename Array>
struct view_traits_const {
#if 0 // RG - MSVC can't handle templates nested in templates. Use traits
  typedef typename Array::template const_array_view<3>::type array_view3;
  typedef typename Array::template const_array_view<2>::type array_view2;
#endif
  typedef typename boost::const_array_view_gen<Array,3>::type array_view3;
  typedef typename boost::const_array_view_gen<Array,2>::type array_view2;
};


// Meta-program selects the proper view_traits implementation.
template <typename Array, typename ConstTag>
struct view_traits_generator :
  boost::mpl::if_< boost::is_same<ConstTag,const_array_tag>,
                   view_traits_const<Array>,
                   view_traits_mutable<Array> >
{};


template <typename Array, typename ViewTraits>
void test_views(Array& A, const ViewTraits&) {
  typedef typename Array::index index;
  typedef typename Array::index_range range;
  typename Array::index_gen indices;

  const index idx0 = A.index_bases()[0];
  const index idx1 = A.index_bases()[1];
  const index idx2 = A.index_bases()[2];

  // Standard View
  {
    typename ViewTraits::array_view3 B = A[
      indices[range(idx0+0,idx0+2)]
             [range(idx1+1,idx1+3)]
             [range(idx2+0,idx2+4,2)]
    ];
    
    for (index i = 0; i != 2; ++i)
      for (index j = 0; j != 2; ++j)
        for (index k = 0; k != 2; ++k) {
          BOOST_CHECK(B[i][j][k] == A[idx0+i][idx1+j+1][idx2+k*2]);
          boost::array<index,3> elmts;
          elmts[0]=i; elmts[1]=j; elmts[2]=k;
          BOOST_CHECK(B(elmts) == A[idx0+i][idx1+j+1][idx2+k*2]);
        }
  }
  // Degenerate dimensions
  {
    typename ViewTraits::array_view2 B =
      A[indices[range(idx0+0,idx0+2)][idx1+1][range(idx2+0,idx2+4,2)]];
    
    for (index i = 0; i != 2; ++i)
      for (index j = 0; j != 2; ++j) {
        BOOST_CHECK(B[i][j] == A[idx0+i][idx1+1][idx2+j*2]);
        boost::array<index,2> elmts;
        elmts[0]=i; elmts[1]=j;
        BOOST_CHECK(B(elmts) == A[idx0+i][idx1+1][idx2+j*2]);
      }
  }

  // Flip the third dimension
  {
    typename ViewTraits::array_view3 B = A[
      indices[range(idx0+0,idx0+2)]
             [range(idx1+0,idx1+2)]
             [range(idx2+2,idx2+0,-1)]
    ];

    //    typename ViewTraits::array_view3 B =
    //      A[indices[range(idx0+0,idx0+2)][idx1+1][range(idx2+0,idx2+4,2)]];
    
    for (index i = 0; i != 2; ++i)
      for (index j = 0; j != 2; ++j) 
        for (index k = 0; k != 2; ++k) {
        BOOST_CHECK(B[i][j][k] == A[idx0+i][idx1+j][idx2+2-k]);
        boost::array<index,3> elmts;
        elmts[0]=i; elmts[1]=j; elmts[2]=k;
        BOOST_CHECK(B(elmts) == A[idx0+i][idx1+j][idx2+2-k]);
      }
  }

  ++tests_run;
}


template <typename Array>
void access(Array& A, const mutable_array_tag&) {
  assign(A);

  typedef typename view_traits_generator<Array,mutable_array_tag>::type
    m_view_traits;

  typedef typename view_traits_generator<Array,const_array_tag>::type
    c_view_traits;

  test_views(A,m_view_traits());
  test_views(A,c_view_traits());

  const Array& CA = A;
  test_views(CA,c_view_traits());
}

template <typename Array>
void access(Array& A, const const_array_tag&) {
  typedef typename view_traits_generator<Array,const_array_tag>::type
    c_view_traits;
  test_views(A,c_view_traits());
}


int test_main(int,char*[]) {
  return run_generative_tests();
}
