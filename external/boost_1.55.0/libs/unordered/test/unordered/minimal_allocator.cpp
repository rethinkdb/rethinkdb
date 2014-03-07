
// Copyright 2011 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/unordered/detail/allocate.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include "../objects/test.hpp"

template <class Tp> 
struct SimpleAllocator
{ 
    typedef Tp value_type;

    SimpleAllocator()
    {
    }

    template <class T> SimpleAllocator(const SimpleAllocator<T>& other)
    {
    }

    Tp *allocate(std::size_t n)
    {
        return static_cast<Tp*>(::operator new(n * sizeof(Tp)));
    }

    void deallocate(Tp* p, std::size_t)
    {
        ::operator delete((void*) p);
    }
};

template <typename T>
void test_simple_allocator()
{
    test::check_instances check_;

    typedef boost::unordered::detail::allocator_traits<
        SimpleAllocator<T> > traits;

    BOOST_STATIC_ASSERT((boost::is_same<typename traits::allocator_type, SimpleAllocator<T> >::value));

    BOOST_STATIC_ASSERT((boost::is_same<typename traits::value_type, T>::value));

    BOOST_STATIC_ASSERT((boost::is_same<typename traits::pointer, T* >::value));
    BOOST_STATIC_ASSERT((boost::is_same<typename traits::const_pointer, T const*>::value));
    //BOOST_STATIC_ASSERT((boost::is_same<typename traits::void_pointer, void* >::value));
    //BOOST_STATIC_ASSERT((boost::is_same<typename traits::const_void_pointer, void const*>::value));

    BOOST_STATIC_ASSERT((boost::is_same<typename traits::difference_type, std::ptrdiff_t>::value));

#if BOOST_UNORDERED_USE_ALLOCATOR_TRAITS == 1
    BOOST_STATIC_ASSERT((boost::is_same<typename traits::size_type,
        std::make_unsigned<std::ptrdiff_t>::type>::value));
#else
    BOOST_STATIC_ASSERT((boost::is_same<typename traits::size_type, std::size_t>::value));
#endif

    BOOST_TEST(!traits::propagate_on_container_copy_assignment::value);
    BOOST_TEST(!traits::propagate_on_container_move_assignment::value);
    BOOST_TEST(!traits::propagate_on_container_swap::value);

    // rebind_alloc
    // rebind_traits

    SimpleAllocator<T> a;

    T* ptr1 = traits::allocate(a, 1);
    //T* ptr2 = traits::allocate(a, 1, static_cast<void const*>(ptr1));
    
    traits::construct(a, ptr1, T(10));
    //traits::construct(a, ptr2, T(30), ptr1);

    BOOST_TEST(*ptr1 == T(10));
    //BOOST_TEST(*ptr2 == T(30));

    traits::destroy(a, ptr1);
    //traits::destroy(a, ptr2);

    //traits::deallocate(a, ptr2, 1);
    traits::deallocate(a, ptr1, 1);

    traits::max_size(a);
}

int main()
{
    test_simple_allocator<int>();
    test_simple_allocator<test::object>();

    return boost::report_errors();
}
