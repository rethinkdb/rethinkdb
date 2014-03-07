// Copyright David Abrahams 2004. Use, modification and distribution is
// subject to the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef NODE_ITERATOR1_DWA2004110_HPP
# define NODE_ITERATOR1_DWA2004110_HPP

# include "node.hpp"
# include <boost/iterator/iterator_facade.hpp>

class node_iterator
  : public boost::iterator_facade<
        node_iterator
      , node_base
      , boost::forward_traversal_tag
    >
{
 public:
    node_iterator()
      : m_node(0)
    {}

    explicit node_iterator(node_base* p)
      : m_node(p)
    {}

 private:
    friend class boost::iterator_core_access;

    void increment()
    { m_node = m_node->next(); }
    
    bool equal(node_iterator const& other) const
    { return this->m_node == other.m_node; }
    
    node_base& dereference() const
    { return *m_node; }

    node_base* m_node;
};


#endif // NODE_ITERATOR1_DWA2004110_HPP
