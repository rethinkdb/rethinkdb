//   Copyright (c) 2009 Matthias Vallentin
// 
//   Distributed under the Boost Software License, Version 1.0. (See accompanying
//   file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/config/warning_disable.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/iterator/indirect_iterator.hpp>
#include <boost/make_shared.hpp>
#include <boost/noncopyable.hpp>
#include <boost/spirit/include/karma.hpp>
#include <boost/spirit/include/karma_format.hpp>

#include <sstream>
#include <string>
#include <vector>

struct foo : boost::noncopyable
{
    foo() 
      : str("foo") 
    {
    }

    std::string str;
};

template <typename Stream>
Stream& operator<<(Stream& out, const foo& f)
{
    out << f.str;
    return out;
}

int main()
{
    using namespace boost::spirit;

    typedef boost::shared_ptr<foo> foo_ptr;
    std::vector<foo_ptr> v;

    std::size_t i = 10;
    while (i--)
        v.push_back(boost::make_shared<foo>());

    typedef boost::indirect_iterator<std::vector<foo_ptr>::const_iterator>
        iterator_type;

    std::stringstream strm;
    strm 
        << karma::format(stream % ',',
            boost::iterator_range<iterator_type>(
                iterator_type(v.begin()), iterator_type(v.end())
                )
            );
    BOOST_TEST(strm.str() == "foo,foo,foo,foo,foo,foo,foo,foo,foo,foo");

    return boost::report_errors();
}
