
// Copyright (C) 2006-2009, 2012 Alexander Nasonov
// Copyright (C) 2012 Lorenzo Caminiti
// Distributed under the Boost Software License, Version 1.0
// (see accompanying file LICENSE_1_0.txt or a copy at
// http://www.boost.org/LICENSE_1_0.txt)
// Home at http://www.boost.org/libs/scope_exit

#include <boost/scope_exit.hpp>
#include <boost/typeof/std/string.hpp>
#include <boost/typeof/std/map.hpp>
#include <map>
#include <string>
#include <utility>

int main(void) {
    bool commit = false;
    std::string currency("EUR");
    double rate = 1.3326;
    std::map<std::string, double> rates;
    bool currency_rate_inserted =
            rates.insert(std::make_pair(currency, rate)).second;

    BOOST_SCOPE_EXIT( (currency_rate_inserted) (&commit) (&rates)
            (&currency) ) {
        if(currency_rate_inserted && !commit) rates.erase(currency);
    } BOOST_SCOPE_EXIT_END

    // ...

    commit = true;

    return 0;
}

