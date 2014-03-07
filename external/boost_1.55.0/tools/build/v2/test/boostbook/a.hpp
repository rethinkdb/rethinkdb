/* Copyright 2004, 2006 Vladimir Prus */
/* Distributed under the Boost Software License, Version 1.0. */
/* (See accompanying file LICENSE_1_0.txt or http://www.boost.org/LICENSE_1_0.txt) */


// Seems like Boostbook does like classes outside of namespaces,
// and won't generate anything for them.
namespace boost {

/// A class
class A {
public:
    /// A constructor
    A();    
};
}
