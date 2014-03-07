/*==============================================================================
    Copyright (c) 2005-2010 Joel de Guzman
    Copyright (c) 2010 Thomas Heller

    Distributed under the Boost Software License, Version 1.0. (See accompanying
    file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
==============================================================================*/

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/stl/container.hpp>
#include <boost/phoenix/stl/algorithm.hpp>

#include <vector>

#include <iostream>

namespace phoenix = boost::phoenix;

using phoenix::actor;
using phoenix::function;
using phoenix::arg_names::arg1;

struct size_impl
{
    // result_of protocol:
    template <typename Sig>
    struct result;
    
    template <typename This, typename Container>
    struct result<This(Container)>
    {
        // Note, remove reference here, because Container can be anything
        typedef typename boost::remove_reference<Container>::type container_type;
        
        // The result will be size_type
        typedef typename container_type::size_type type;
    };
    
    template <typename Container>
    typename result<size_impl(Container const&)>::type
    operator()(Container const& container) const
    {
        return container.size();
    }
};

template <typename Expr>
struct container_actor
    : actor<Expr>
{
    typedef actor<Expr> base_type;
    typedef container_actor<Expr> that_type;

    container_actor( base_type const& base = base_type() )
        : base_type( base ) {}

    typename phoenix::expression::function<phoenix::stl::begin, that_type>::type const
    begin() const
    {
        return phoenix::begin(*this);
    }
    
    typename phoenix::expression::function<phoenix::stl::end, that_type>::type const
    end() const
    {
        return phoenix::begin(*this);
    }

    typename phoenix::expression::function<size_impl, that_type>::type const
    size() const
    {
        function<size_impl> const f = size_impl();
        return f(*this);
    }
    
    typename phoenix::expression::function<phoenix::stl::max_size, that_type>::type const
    max_size() const
    {
        return phoenix::max_size(*this);
    }

    typename phoenix::expression::function<phoenix::stl::empty, that_type>::type const
    empty() const
    {
        return phoenix::empty(*this);
    }

    template <typename Container>
    typename phoenix::expression::function<phoenix::impl::swap, that_type, Container>::type const
    swap(actor<Container> const& expr) const
    {
        return phoenix::swap(*this, expr);
    }
};

template <typename Expr>
container_actor<Expr> const
container( actor<Expr> const& expr )
{
    return expr;
}

int main()
{
    container_actor<phoenix::expression::argument<1>::type> const con1;
    std::vector<int> v;
    v.push_back(0);
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    
    std::cout << (container(arg1).size())(v) << " == " << v.size() << "\n";


    std::cout << (con1.size())(v) << " == " << v.size() << "\n";
}
