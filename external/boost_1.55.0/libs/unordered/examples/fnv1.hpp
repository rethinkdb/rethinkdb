
// Copyright 2008-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// This code is also released into the public domain.

// Algorithm from: http://www.isthe.com/chongo/tech/comp/fnv/

#include <string>

namespace hash
{
    template <std::size_t FnvPrime, std::size_t OffsetBasis>
    struct basic_fnv_1
    {
        std::size_t operator()(std::string const& text) const
        {
            std::size_t hash = OffsetBasis;
            for(std::string::const_iterator it = text.begin(), end = text.end();
                    it != end; ++it)
            {
                hash *= FnvPrime;
                hash ^= *it;
            }

            return hash;
        }
    };

    template <std::size_t FnvPrime, std::size_t OffsetBasis>
    struct basic_fnv_1a
    {
        std::size_t operator()(std::string const& text) const
        {
            std::size_t hash = OffsetBasis;
            for(std::string::const_iterator it = text.begin(), end = text.end();
                    it != end; ++it)
            {
                hash ^= *it;
                hash *= FnvPrime;
            }

            return hash;
        }
    };

    // For 32 bit machines:
    const std::size_t fnv_prime = 16777619u;
    const std::size_t fnv_offset_basis = 2166136261u;

    // For 64 bit machines:
    // const std::size_t fnv_prime = 1099511628211u;
    // const std::size_t fnv_offset_basis = 14695981039346656037u;

    // For 128 bit machines:
    // const std::size_t fnv_prime = 309485009821345068724781401u;
    // const std::size_t fnv_offset_basis =
    //     275519064689413815358837431229664493455u;

    // For 256 bit machines:
    // const std::size_t fnv_prime =
    //     374144419156711147060143317175368453031918731002211u;
    // const std::size_t fnv_offset_basis =
    //     100029257958052580907070968620625704837092796014241193945225284501741471925557u;

    typedef basic_fnv_1<fnv_prime, fnv_offset_basis> fnv_1;
    typedef basic_fnv_1a<fnv_prime, fnv_offset_basis> fnv_1a;
}
