
// Copyright (C) 2006-2009, 2012 Alexander Nasonov
// Copyright (C) 2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/scope_exit

// No #include guard for this header.

#include <boost/scope_exit.hpp>
#include <boost/config.hpp>

int tu1(void);
int tu2(void);

inline int inline_f(void) {
    int i = 99;
    {
        BOOST_SCOPE_EXIT( (&i) ) {
            i = -1;
        } BOOST_SCOPE_EXIT_END
    }
    return i;
}

#if !defined(BOOST_INTEL) && defined(__GNUC__) && \
        (__GNUC__ * 100 + __GNUC_MINOR__) >= 304
template<class Int>
Int template_f(Int i) {
    {
        BOOST_SCOPE_EXIT_TPL( (&i) ) {
            ++i;
        } BOOST_SCOPE_EXIT_END
    }
    return i;
}
#else
inline int template_f(int i) {
    {
        BOOST_SCOPE_EXIT( (&i) ) {
            ++i;
        } BOOST_SCOPE_EXIT_END
    }
    return i;
}
#endif

