//  Boost string_algo library iterator_test.cpp file  ---------------------------//

//  Copyright Pavol Droba 2002-2003. Use, modification and
//  distribution is subject to the Boost Software License, Version
//  1.0. (See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/classification.hpp>
// equals predicate is used for result comparison
#include <boost/algorithm/string/predicate.hpp>

// Include unit test framework
#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

#include <string>
#include <vector>
#include <iostream>

#include <boost/test/test_tools.hpp>


using namespace std;
using namespace boost;

bool is_not_empty(const std::string& str)
{
    return !str.empty();
}

void join_test()
{
    // Prepare inputs
    vector<string> tokens1;
    tokens1.push_back("xx");
    tokens1.push_back("abc");
    tokens1.push_back("xx");

    vector<string> tokens2;
    tokens2.push_back("");
    tokens2.push_back("xx");
    tokens2.push_back("abc");
    tokens2.push_back("");
    tokens2.push_back("abc");
    tokens2.push_back("xx");
    tokens2.push_back("");

    vector<string> tokens3;
    tokens3.push_back("");
    tokens3.push_back("");
    tokens3.push_back("");

    vector<string> empty_tokens;

    vector<vector<int> > vtokens;
    for(unsigned int n=0; n<tokens2.size(); ++n)
    {
        vtokens.push_back(vector<int>(tokens2[n].begin(), tokens2[n].end()));
    }

    BOOST_CHECK( equals(join(tokens1, "-"), "xx-abc-xx") );
    BOOST_CHECK( equals(join(tokens2, "-"), "-xx-abc--abc-xx-") );
    BOOST_CHECK( equals(join(vtokens, "-"), "-xx-abc--abc-xx-") );
    BOOST_CHECK( equals(join(empty_tokens, "-"), "") );

    BOOST_CHECK( equals(join_if(tokens2, "-", is_not_empty), "xx-abc-abc-xx") );
    BOOST_CHECK( equals(join_if(empty_tokens, "-", is_not_empty), "") );
    BOOST_CHECK( equals(join_if(tokens3, "-", is_not_empty), "") );
}


BOOST_AUTO_TEST_CASE( test_main )
{
    join_test();
}
