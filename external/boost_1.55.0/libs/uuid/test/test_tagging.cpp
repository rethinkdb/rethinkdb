//  (C) Copyright Andy Tompkins 2009. Permission to copy, use, modify, sell and
//  distribute this software is granted provided this copyright notice appears
//  in all copies. This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.

// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  libs/uuid/test/test_tagging.cpp  -------------------------------//

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/detail/lightweight_test.hpp>

class object
{
public:
    object()
        : tag(boost::uuids::random_generator()())
        , state(0)
    {}

    explicit object(int state)
        : tag(boost::uuids::random_generator()())
        , state(state)
    {}
    
    object(object const& rhs)
        : tag(rhs.tag)
        , state(rhs.state)
    {}
    
    bool operator==(object const& rhs) const {
        return tag == rhs.tag;
    }
    bool operator!=(object const& rhs) const {
      return !(operator==(rhs));
    }
    
    object& operator=(object const& rhs) {
        tag = rhs.tag;
        state = rhs.state;
        return *this;
    }
    
    int get_state() const { return state; }
    void set_state(int new_state) { state = new_state; }
    
private:
    boost::uuids::uuid tag;
    
    int state;
};

template <typename elem, typename traits>
std::basic_ostream<elem, traits>& operator<<(std::basic_ostream<elem, traits>& os, object const& o)
{
    os << o.get_state();
    return os;
}

int main(int, char*[])
{
    object o1(1);

    object o2 = o1;
    BOOST_TEST_EQ(o1, o2);

    o2.set_state(2);
    BOOST_TEST_EQ(o1, o2);
    
    object o3;
    o3.set_state(3);

    BOOST_TEST_NE(o1, o3);
    BOOST_TEST_NE(o2, o3);
    
    return boost::report_errors();
}
