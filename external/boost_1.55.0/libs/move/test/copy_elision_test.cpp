// Copyright David Abrahams 2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/move/detail/config_begin.hpp>
#include <iostream>

#ifdef NO_MOVE
# undef BOOST_COPY_ASSIGN_REF
# define BOOST_COPY_ASSIGN_REF(X) X const&
# undef BOOST_COPYABLE_AND_MOVABLE
# define BOOST_COPYABLE_AND_MOVABLE(X)
# define MOVE(x) (x)
#else
#include <boost/move/utility.hpp>
# define MOVE(x) boost::move(x)
#endif

struct X
{
    X() : id(instances++)
    {
        std::cout << "X" << id << ": construct\n";
    }
    
    X(X const& rhs) : id(instances++)
    {
        std::cout << "X" << id << ": <- " << "X" << rhs.id << ": **copy**\n";
        ++copies;
    }

    // This particular test doesn't exercise assignment, but for
    // completeness:
    X& operator=(BOOST_COPY_ASSIGN_REF(X) rhs)
    {
        std::cout << "X" << id << ": <- " << "X" << rhs.id << ": assign\n";
        return *this;
    }

#ifndef NO_MOVE
    X& operator=(BOOST_RV_REF(X) rhs)
    {
        std::cout << "X" << id << ": <- " << "X" << rhs.id << ": move assign\n";
        return *this;
    }
    
    X(BOOST_RV_REF(X) rhs) : id(instances++)
    {
        std::cout << "X" << id << ": <- " << "X" << rhs.id << ": ..move construct..\n";
        ++copies;
    }
#endif 

    ~X() { std::cout << "X" << id << ": destroy\n"; }

    unsigned id;
    
    static unsigned copies;
    static unsigned instances;

    BOOST_COPYABLE_AND_MOVABLE(X)
};

unsigned X::copies = 0;
unsigned X::instances = 0;

#define CHECK_COPIES( stmt, min, max, comment )                         \
{                                                                       \
    unsigned const old_copies = X::copies;                              \
                                                                        \
    std::cout << "\n" comment "\n" #stmt "\n===========\n";             \
    {                                                                   \
        stmt;                                                           \
    }                                                                   \
    unsigned const n = X::copies - old_copies;                          \
    if (n > max)                                                        \
        std::cout << "*** max is too low or compiler is buggy ***\n";   \
    if (n < min)                                                        \
        std::cout << "*** min is too high or compiler is buggy ***\n";  \
                                                                        \
    std::cout << "-----------\n"                                        \
              << n << "/" << max                                        \
              << " possible copies/moves made\n"                        \
              << max - n << "/" << max - min                            \
              << " possible elisions performed\n\n";                    \
                                                                        \
    if (n > min)                                                        \
        std::cout << "*** " << n - min                                  \
                  << " possible elisions missed! ***\n";                \
}

struct trace
{
    trace(char const* name)
        : m_name(name)
    {
        std::cout << "->: " << m_name << "\n";
    }
    
    ~trace()
    {
        std::cout << "<-: " << m_name << "\n";
    }
    
    char const* m_name;
};

void sink(X a)
{
  trace t("sink");
}

X nrvo_source()
{
    trace t("nrvo_source");
    X a;
    return a;
}

X urvo_source()
{
    trace t("urvo_source");
    return X();
}

X identity(X a)
{
    trace t("identity");
    return a;
}

X lvalue_;
X& lvalue()
{
    return lvalue_;
}
typedef X rvalue;

X ternary( bool y )
{
    X a, b;
    return MOVE(y?a:b);
}

int main(int argc, char* argv[])
{
   (void)argv;
    // Double parens prevent "most vexing parse"
    CHECK_COPIES( X a(( lvalue() )), 1, 1, "Direct initialization from lvalue");
    CHECK_COPIES( X a(( rvalue() )), 0, 1, "Direct initialization from rvalue");
    
    CHECK_COPIES( X a = lvalue(), 1, 1, "Copy initialization from lvalue" );
    CHECK_COPIES( X a = rvalue(), 0, 1, "Copy initialization from rvalue" );

    CHECK_COPIES( sink( lvalue() ), 1, 1, "Pass lvalue by value" );
    CHECK_COPIES( sink( rvalue() ), 0, 1, "Pass rvalue by value" );

    CHECK_COPIES( nrvo_source(), 0, 1, "Named return value optimization (NRVO)" );
    CHECK_COPIES( urvo_source(), 0, 1, "Unnamed return value optimization (URVO)" );

    // Just to prove these things compose properly
    CHECK_COPIES( X a(urvo_source()), 0, 2, "Return value used as ctor arg" );
    
    // Expect to miss one possible elision here
    CHECK_COPIES( identity( rvalue() ), 0, 2, "Return rvalue passed by value" );

    // Expect to miss an elision in at least one of the following lines
    CHECK_COPIES( X a = ternary( argc == 1000 ), 0, 2, "Return result of ternary operation" );
    CHECK_COPIES( X a = ternary( argc != 1000 ), 0, 2, "Return result of ternary operation again" );
    return 0;
}

#include <boost/move/detail/config_end.hpp>
