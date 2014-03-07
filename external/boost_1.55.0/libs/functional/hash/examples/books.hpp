
// Copyright 2005-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This example illustrates how to use boost::hash with a custom hash function.
// The implementation is contained in books.cpp

#include <cstddef>
#include <string>

namespace library
{
    struct book
    {
        int id;
        std::string author;
        std::string title;

        book(int i, std::string const& a, std::string const& t)
            : id(i), author(a), title(t) {}
    };

    bool operator==(book const&, book const&);
    std::size_t hash_value(book const&);
}
