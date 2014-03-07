/* /libs/serialization/xml_performance/macro.hpp *******************************

(C) Copyright 2010 Bryce Lelbach

Use, modification and distribution is subject to the Boost Software License,
Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at 
http://www.boost.org/LICENSE_1_0.txt)

*******************************************************************************/

#if !defined(BOOST_SERIALIZATION_XML_PERFORMANCE_MACRO_HPP)
#define BOOST_SERIALIZATION_XML_PERFORMANCE_MACRO_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
  #pragma once
#endif

#include <boost/preprocessor.hpp>

#if !defined(BSL_NODE_MAX)
  #define BSL_NODE_MAX 4
#endif

#if !defined(BSL_DEPTH)
  #define BSL_DEPTH 2
#endif

#if !defined(BSL_ROUNDS)
  #define BSL_ROUNDS 256
#endif

#if !defined(BSL_TYPE)
  #define BSL_TYPE int
#endif

#if !defined(BSL_SAVE_TMPFILE)
  #define BSL_SAVE_TMPFILE 0
#endif

#if !defined(BSL_RESULTS_FILE)
  #define BSL_RESULTS_FILE                                \
    BOOST_PP_STRINGIZE(BSL_TYPE)                          \
    BOOST_PP_STRINGIZE(BSL_EXP(BSL_NODE_MAX, BSL_DEPTH))  \
    "_results.xml"                                        \
  /**/
#endif

// utility print macro

#define BSL_PRINT(Z, N, T) T

// preprocessor power function, BSL_EXP

#define BSL_EXP_PRED(B, D) BOOST_PP_TUPLE_ELEM(3, 0, D)

#define BSL_EXP_OP(B, D)                                                       \
   (                                                                           \
      BOOST_PP_DEC(BOOST_PP_TUPLE_ELEM(3, 0, D)),                              \
      BOOST_PP_TUPLE_ELEM(3, 1, D),                                            \
      BOOST_PP_MUL_D(                                                          \
         B,                                                                    \
         BOOST_PP_TUPLE_ELEM(3, 2, D),                                         \
         BOOST_PP_TUPLE_ELEM(3, 1, D)                                          \
      )                                                                        \
   )                                                                           \
   /**/

#define BSL_EXP(X, N)                                                          \
  BOOST_PP_TUPLE_ELEM(                                                         \
    3, 2, BOOST_PP_WHILE(BSL_EXP_PRED, BSL_EXP_OP, (N, X, 1))                  \
  )                                                                            \
  /**/

// boost::archive::xml::node macros

#define BSL_NODE_DECL_MEMBER(Z, N, _)   T ## N  element ## N  ;
#define BSL_NODE_DECL_NONE(Z, N, _)     unused_type  element ## N  ;
#define BSL_NODE_xDECL_CTOR()           node (void) { }

#define BSL_NODE_DECL_CTOR(P)                                                  \
  BOOST_PP_IF(P,                                                               \
    BSL_NODE_xDECL_CTOR,                                                       \
    BOOST_PP_EMPTY                                                             \
  )()                                                                          \
  /**/

#define BSL_NODE_SERIALIZE(Z, N, _)                                            \
  & BOOST_SERIALIZATION_NVP(BOOST_PP_CAT(element, N))                          \
  /**/

#define BSL_NODE_INIT_LIST(Z, N, _)                                            \
  BOOST_PP_COMMA_IF(N)  BOOST_PP_CAT(element, N)                               \
  BOOST_PP_LPAREN() BOOST_PP_CAT(p, N) BOOST_PP_RPAREN()                       \
  /**/

#define BSL_NODE_DECL(Z, N, _)                                                 \
  template<BOOST_PP_ENUM_PARAMS_Z(Z, N, typename T)>                           \
  struct node<                                                                 \
    BOOST_PP_ENUM_PARAMS_Z(Z, N, T)                                            \
    BOOST_PP_COMMA_IF(N)                                                       \
    BOOST_PP_ENUM_ ## Z(BOOST_PP_SUB(BSL_NODE_MAX, N), BSL_PRINT, unused_type) \
  > {                                                                          \
    BOOST_PP_REPEAT_ ## Z(N, BSL_NODE_DECL_MEMBER, _)                          \
                                                                               \
    BOOST_PP_REPEAT_FROM_TO_ ## Z(N, BSL_NODE_MAX, BSL_NODE_DECL_NONE, _)      \
                                                                               \
    template<class ARC>                                                        \
    void serialize (ARC& ar, const unsigned int) {                             \
      ar BOOST_PP_REPEAT_ ## Z(N, BSL_NODE_SERIALIZE, _);                      \
    }                                                                          \
                                                                               \
    BSL_NODE_DECL_CTOR(N)                                                      \
                                                                               \
    node (BOOST_PP_ENUM_BINARY_PARAMS_Z(Z, N, T, p)):                          \
      BOOST_PP_REPEAT_ ## Z(N, BSL_NODE_INIT_LIST, _) { }                      \
  };                                                                           \
  /**/

// instantiation macros

#define BSL_INST_BASE(Z, N, L)                                                 \
  T0 T0 ## _ ## N(BOOST_PP_ENUM_ ## Z(                                         \
    BSL_NODE_MAX, BSL_PRINT,                                                   \
    boost::archive::xml::random<BSL_TYPE> BOOST_PP_LPAREN() BOOST_PP_RPAREN()  \
  ));                                                                          \
  /**/

#define BSL_INST_yNODES(Z, N, L)                                               \
  BOOST_PP_COMMA_IF(N)                                                         \
  BOOST_PP_CAT(T,                                                              \
    BOOST_PP_CAT(BOOST_PP_LIST_AT(L, 1),                                       \
      BOOST_PP_CAT(_,                                                          \
        BOOST_PP_ADD(N,                                                        \
          BOOST_PP_LIST_AT(L, 0)                                               \
        )                                                                      \
      )                                                                        \
    )                                                                          \
  )                                                                            \
  /**/

#define BSL_INST_xNODES(Z, N, L)                                               \
  T ## L T ## L ## _ ## N(                                                     \
    BOOST_PP_REPEAT_ ## Z(                                                     \
      BSL_NODE_MAX, BSL_INST_yNODES,                                           \
      (BOOST_PP_MUL(N, BSL_NODE_MAX), (BOOST_PP_SUB(L, 1), BOOST_PP_NIL))      \
    )                                                                          \
  );                                                                           \
  /**/

#define BSL_INST_NODES(Z, N, L)                                                \
  BOOST_PP_REPEAT_ ## Z(                                                       \
    BSL_EXP(BSL_NODE_MAX, BOOST_PP_SUB(BSL_DEPTH, N)),                         \
    BSL_INST_xNODES, N                                                         \
  )                                                                            \
  /**/

#define BSL_TYPEDEF_NODES(Z, N, L)                                             \
  typedef boost::archive::xml::node<                                           \
    BOOST_PP_ENUM_ ## Z(                                                       \
      BSL_NODE_MAX, BSL_PRINT, BOOST_PP_CAT(T, BOOST_PP_SUB(N, 1))             \
    )                                                                          \
  > T ## N;                                                                    \
  /**/

// main macro

#define BSL_MAIN                                                               \
  int main (void) {                                                            \
    using namespace boost::archive;                                            \
    using namespace boost::archive::xml;                                       \
                                                                               \
    typedef node<BOOST_PP_ENUM(BSL_NODE_MAX, BSL_PRINT, BSL_TYPE)> T0;         \
                                                                               \
    BOOST_PP_REPEAT_FROM_TO(1, BSL_DEPTH, BSL_TYPEDEF_NODES, _)                \
                                                                               \
    typedef node<BOOST_PP_ENUM(                                                \
      BSL_NODE_MAX, BSL_PRINT,                                                 \
      BOOST_PP_CAT(T, BOOST_PP_SUB(BSL_DEPTH, 1))                              \
    )> HEAD;                                                                   \
                                                                               \
    result_set results;                                                        \
    std::size_t rounds = BSL_ROUNDS;                                           \
                                                                               \
    while (rounds --> 0) {                                                     \
      BOOST_PP_REPEAT(BSL_EXP(BSL_NODE_MAX, BSL_DEPTH), BSL_INST_BASE, _)      \
                                                                               \
      BOOST_PP_REPEAT_FROM_TO(1, BSL_DEPTH, BSL_INST_NODES, _)                 \
                                                                               \
      HEAD h(BOOST_PP_ENUM_PARAMS(                                             \
        BSL_NODE_MAX,                                                          \
        BOOST_PP_CAT(T, BOOST_PP_CAT(BOOST_PP_SUB(BSL_DEPTH, 1), _))           \
      ));                                                                      \
                                                                               \
      std::string fn = save_archive(h);                                        \
                                                                               \
      std::pair<double, HEAD> r = restore_archive<HEAD>(fn);                   \
                                                                               \
      std::cout << "round "                                                    \
                << ((BSL_ROUNDS - 1) - rounds)                                 \
                << " -> " << fn << "\n";                                       \
                                                                               \
      BOOST_PP_IF(BSL_SAVE_TMPFILE,                                            \
        BOOST_PP_EMPTY(),                                                      \
        std::remove(fn.c_str());                                               \
      )                                                                        \
                                                                               \
      results.entries.push_back(entry(                                         \
         BOOST_PP_STRINGIZE(BSL_TYPE),                                         \
         BSL_EXP(BSL_NODE_MAX, BSL_DEPTH), r.first                             \
      ));                                                                      \
    }                                                                          \
                                                                               \
    std::fstream fs(BSL_RESULTS_FILE, std::fstream::in);                       \
                                                                               \
    if (fs.good()) {                                                           \
      xml_iarchive ia(fs);                                                     \
      ia >> BOOST_SERIALIZATION_NVP(results);                                  \
      fs.close();                                                              \
    }                                                                          \
                                                                               \
    fs.open(BSL_RESULTS_FILE, std::fstream::out | std::fstream::trunc);        \
    xml_oarchive oa(fs);                                                       \
    oa << BOOST_SERIALIZATION_NVP(results);                                    \
                                                                               \
    fs.close();                                                                \
  }                                                                            \
  /**/

#endif // BOOST_SERIALIZATION_XML_PERFORMANCE_MACRO_HPP
