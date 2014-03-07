
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./books.hpp"
#include <boost/functional/hash.hpp>
#include <cassert>

// If std::unordered_set was available:
//#include <unordered_set>

// This example illustrates how to use boost::hash with a custom hash function.
// For full details, see the tutorial.

int main()
{
    library::book knife(3458, "Zane Grey", "The Hash Knife Outfit");
    library::book dandelion(1354, "Paul J. Shanley", "Hash & Dandelion Greens");

    boost::hash<library::book> book_hasher;
    std::size_t knife_hash_value = book_hasher(knife);
    (void)knife_hash_value; // suppress unused variable warning

    // If std::unordered_set was available:
    //
    //std::unordered_set<library::book, boost::hash<library::book> > books;
    //books.insert(knife);
    //books.insert(library::book(2443, "Lindgren, Torgny", "Hash"));
    //books.insert(library::book(1953, "Snyder, Bernadette M.",
    //    "Heavenly Hash: A Tasty Mix of a Mother's Meditations"));

    //assert(books.find(knife) != books.end());
    //assert(books.find(dandelion) == books.end());

    return 0;
}

namespace library
{
    bool operator==(book const& a, book const& b)
    {
        return a.id == b.id;
    }

    std::size_t hash_value(book const& b)
    {
        boost::hash<int> hasher;
        return hasher(b.id);
    }
}
