// Copyright David Abrahams 2004. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "node_iterator2.hpp"
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>
#include <boost/mem_fn.hpp>
#include <cassert>

int main()
{
    std::auto_ptr<node<int> > nodes(new node<int>(42));
    nodes->append(new node<std::string>(" is greater than "));
    nodes->append(new node<int>(13));

    // Check interoperability
    assert(node_iterator(nodes.get()) == node_const_iterator(nodes.get()));
    assert(node_const_iterator(nodes.get()) == node_iterator(nodes.get()));
    
    assert(node_iterator(nodes.get()) != node_const_iterator());
    assert(node_const_iterator(nodes.get()) != node_iterator());
    
    std::copy(
        node_iterator(nodes.get()), node_iterator()
      , std::ostream_iterator<node_base>(std::cout, " ")
    );
    std::cout << std::endl;
    
    std::for_each(
        node_iterator(nodes.get()), node_iterator()
      , boost::mem_fn(&node_base::double_me)
    );

    std::copy(
        node_const_iterator(nodes.get()), node_const_iterator()
      , std::ostream_iterator<node_base>(std::cout, "/")
    );
    std::cout << std::endl;
    return 0;
}
