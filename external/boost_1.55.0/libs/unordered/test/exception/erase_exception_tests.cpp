
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "./containers.hpp"
#include "../helpers/random_values.hpp"
#include "../helpers/invariants.hpp"
#include "../helpers/helpers.hpp"

test::seed_t initialize_seed(835193);

template <class T>
struct erase_test_base : public test::exception_base
{
    test::random_values<T> values;
    erase_test_base(unsigned int count = 5) : values(count) {}

    typedef T data_type;

    data_type init() const {
        return T(values.begin(), values.end());
    }

    void check BOOST_PREVENT_MACRO_SUBSTITUTION(T const& x) const {
        std::string scope(test::scope);

        BOOST_TEST(scope.find("hash::") != std::string::npos ||
                scope.find("equal_to::") != std::string::npos ||
                scope == "operator==(object, object)");

        test::check_equivalent_keys(x);
    }
};

template <class T>
struct erase_by_key_test1 : public erase_test_base<T>
{
    void run(T& x) const
    {
        typedef BOOST_DEDUCED_TYPENAME
            test::random_values<T>::const_iterator iterator;

        for(iterator it = this->values.begin(), end = this->values.end();
                it != end; ++it)
        {
            x.erase(test::get_key<T>(*it));
        }
    }
};

EXCEPTION_TESTS(
    (erase_by_key_test1),
    CONTAINER_SEQ)
RUN_TESTS()
