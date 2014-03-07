// Copyright (c) 2009 Erik Bryan
// Copyright (c) 2007-2010 Hartmut Kaiser
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file licence_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <string>
#include <vector>

#include <boost/array.hpp>

#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

///////////////////////////////////////////////////////////////////////////////
// create a wrapper holding the boost::array and a current insertion point
namespace client 
{ 
    namespace detail
    {
        template <typename T>
        struct adapt_array;

        template <typename T, std::size_t N>
        struct adapt_array<boost::array<T, N> >
        {
            typedef boost::array<T, N> array_type;

            adapt_array(array_type& arr)
              : arr_(arr), current_(0) {}

            // expose a push_back function compatible with std containers
            bool push_back(typename array_type::value_type const& val)
            {
                // if the array is full, we need to bail out
                // returning false will fail the parse
                if (current_ >= N) 
                    return false;

                arr_[current_++] = val;
                return true;
            }

            array_type& arr_;
            std::size_t current_;
        };
    }

    namespace result_of
    {
        template <typename T>
        struct adapt_array;

        template <typename T, std::size_t N>
        struct adapt_array<boost::array<T, N> >
        {
            typedef detail::adapt_array<boost::array<T, N> > type;
        };
    }

    template <typename T, std::size_t N>
    inline detail::adapt_array<boost::array<T, N> > 
    adapt_array(boost::array<T, N>& arr)
    {
        return detail::adapt_array<boost::array<T, N> >(arr);
    }
}

///////////////////////////////////////////////////////////////////////////////
// specialize Spirit's container specific customization points for our adaptor
namespace boost { namespace spirit { namespace traits
{
    template <typename T, std::size_t N>
     struct is_container<client::detail::adapt_array<boost::array<T, N> > > 
       : boost::mpl::true_
     {};

    template <typename T, std::size_t N>
    struct container_value<client::detail::adapt_array<boost::array<T, N> > >
    {
        typedef T type;     // value type of container
    };

    template <typename T, std::size_t N>
    struct push_back_container<
      client::detail::adapt_array<boost::array<T, N> >, T>
    {
        static bool call(client::detail::adapt_array<boost::array<T, N> >& c
          , T const& val)
        {
            return c.push_back(val);
        }
    };
}}}

int main()
{
    typedef std::string::const_iterator iterator_type;
    typedef boost::array<int, 2> array_type; 
    typedef client::result_of::adapt_array<array_type>::type adapted_type; 

    array_type arr;

    std::string str = "1 2";
    iterator_type iter = str.begin();
    iterator_type end = str.end();

    qi::rule<iterator_type, adapted_type(), ascii::space_type> r = *qi::int_;

    adapted_type attr = client::adapt_array(arr);
    bool result = qi::phrase_parse(iter, end, r, ascii::space, attr);

    if (result) 
        std::cout << "Parsed: " << arr[0] << ", " << arr[1] << std::endl;

    return 0;
}
