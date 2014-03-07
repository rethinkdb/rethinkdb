
// Copyright (C) 2009-2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/local_function

#include <boost/config.hpp>
#ifdef BOOST_NO_CXX11_VARIADIC_MACROS
#   error "variadic macros required"
#else

#include <boost/local_function.hpp>
#include <boost/typeof/typeof.hpp>
#include BOOST_TYPEOF_INCREMENT_REGISTRATION_GROUP()

struct s;
BOOST_TYPEOF_REGISTER_TYPE(s) // Register before binding `this_` below.

// Compile all local function declaration combinations.
struct s {
    void f(double p = 1.23, double q = -1.23) {
        { // Only params.
            void BOOST_LOCAL_FUNCTION(int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(l)
            l(1);
        }
        { // Only const binds.
            int a, b;

            const int& BOOST_LOCAL_FUNCTION(const bind a,
                    const bind& b, const bind& p, const bind q) {
                return b;
            } BOOST_LOCAL_FUNCTION_NAME(l)
            l();

            const s& BOOST_LOCAL_FUNCTION(const bind this_) {
                return *this_;
            } BOOST_LOCAL_FUNCTION_NAME(t)
            t();

            const int BOOST_LOCAL_FUNCTION(const bind a,
                    const bind& b, const bind& p, const bind q,
                    const bind this_) {
                return a;
            } BOOST_LOCAL_FUNCTION_NAME(lt)
            lt();
        }
        { // Only plain binds.
            int c, d;

            int& BOOST_LOCAL_FUNCTION(bind c, bind& d,
                    bind& p, bind& q) {
                return d;
            } BOOST_LOCAL_FUNCTION_NAME(l)
            l();

            s& BOOST_LOCAL_FUNCTION(bind this_) {
                return *this_;
            } BOOST_LOCAL_FUNCTION_NAME(t)
            t();

            int BOOST_LOCAL_FUNCTION(bind c, bind& d,
                    bind& p, bind& q, bind this_) {
                return c;
            } BOOST_LOCAL_FUNCTION_NAME(lt)
            lt();
        }

        { // Both params and const binds.
            int a, b;

            void BOOST_LOCAL_FUNCTION(const bind a, const bind& b,
                    const bind& p, const bind q,
                    int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(l)
            l(1);

            void BOOST_LOCAL_FUNCTION(const bind this_,
                    int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(t)
            t(1);

            void BOOST_LOCAL_FUNCTION(const bind a, const bind this_,
                    const bind& b, const bind& p, const bind q,
                    int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(lt)
            lt(1);
        }
        { // Both params and plain binds.
            int c, d;

            void BOOST_LOCAL_FUNCTION(bind c, bind& d, bind& p, bind q,
                    int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(l)
            l(1);

            void BOOST_LOCAL_FUNCTION(bind this_,
                    int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(t)
            t(1);
            
            void BOOST_LOCAL_FUNCTION(bind c, bind& d,
                    bind& p, bind this_, bind q,
                    int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(lt)
            lt(1);
        }
        { // Both const and plain binds.
            int a, b, c, d;

            void BOOST_LOCAL_FUNCTION(const bind a, const bind& b,
                    const bind p, bind c, bind& d, bind q) {
            } BOOST_LOCAL_FUNCTION_NAME(l)
            l();

            void BOOST_LOCAL_FUNCTION(const bind this_,
                    bind c, bind& d, bind q) {
            } BOOST_LOCAL_FUNCTION_NAME(ct)
            ct();
            void BOOST_LOCAL_FUNCTION(const bind this_,
                    const bind a, const bind& b, const bind p,
                    bind c, bind& d, bind q) {
            } BOOST_LOCAL_FUNCTION_NAME(lct)
            lct();

            void BOOST_LOCAL_FUNCTION(const bind a, const bind& b,
                    const bind p, bind this_) {
            } BOOST_LOCAL_FUNCTION_NAME(pt)
            pt();
            void BOOST_LOCAL_FUNCTION(const bind a, const bind& b,
                    const bind p, bind c, bind this_, bind& d, bind q) {
            } BOOST_LOCAL_FUNCTION_NAME(lpt)
            lpt();
        }

        { // All params, const binds, and plain binds.
            int a, b, c, d;
            
            void BOOST_LOCAL_FUNCTION(
                    const bind a, const bind& b, const bind& p,
                    bind c, bind& d, bind& q, int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(l)
            l(1);

            void BOOST_LOCAL_FUNCTION(const bind this_,
                    bind c, bind& d, bind& q,
                    int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(ct)
            ct(1);
            void BOOST_LOCAL_FUNCTION(
                    const bind a, const bind& b, const bind& p,
                    bind this_, int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(pt)
            pt(1);

            void BOOST_LOCAL_FUNCTION(const bind a, const bind this_,
                    const bind& b, const bind& p, bind c, bind& d,
                    bind& q, int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(lct)
            lct(1);
            void BOOST_LOCAL_FUNCTION(const bind a, const bind& b,
                    const bind& p, bind c, bind& d, bind this_, bind& q,
                    int x, int y, default 0) {
            } BOOST_LOCAL_FUNCTION_NAME(lpt)
            lpt(1);
        }
    }
};

int main(void) {
    s().f();
    return 0;
}

#endif // VARIADIC_MACROS

