/*=============================================================================
    Copyright (c) 2007 Tobias Schwinger
  
    Use modification and distribution are subject to the Boost Software 
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt).
==============================================================================*/

#include <boost/functional/factory.hpp>
#include <boost/detail/lightweight_test.hpp>

#include <cstddef>
#include <memory>
#include <boost/shared_ptr.hpp>

using std::size_t;

class sum 
{
    int val_sum;
  public:
    sum(int a, int b) : val_sum(a + b) { }

    operator int() const { return this->val_sum; }
};

template< typename T >
class counting_allocator : public std::allocator<T>
{
  public:
    counting_allocator()
    { }

    template< typename OtherT >
    struct rebind { typedef counting_allocator<OtherT> other; };

    template< typename OtherT >
    counting_allocator(counting_allocator<OtherT> const& that)
    { }

    static size_t n_allocated;
    T* allocate(size_t n, void const* hint = 0l)
    {
        n_allocated += 1;
        return std::allocator<T>::allocate(n,hint);
    }

    static size_t n_deallocated;
    void deallocate(T* ptr, size_t n)
    {
        n_deallocated += 1;
        return std::allocator<T>::deallocate(ptr,n);
    }
};
template< typename T > size_t counting_allocator<T>::n_allocated = 0;
template< typename T > size_t counting_allocator<T>::n_deallocated = 0;

int main()
{
    int one = 1, two = 2;
    {
      boost::shared_ptr<sum> instance(
          boost::factory< boost::shared_ptr<sum>, counting_allocator<void>, 
              boost::factory_alloc_for_pointee_and_deleter >()(one,two) );
      BOOST_TEST(*instance == 3);
    }
    BOOST_TEST(counting_allocator<sum>::n_allocated == 1); 
    BOOST_TEST(counting_allocator<sum>::n_deallocated == 1);
    {
      boost::shared_ptr<sum> instance(
          boost::factory< boost::shared_ptr<sum>, counting_allocator<void>,
              boost::factory_passes_alloc_to_smart_pointer >()(one,two) );
      BOOST_TEST(*instance == 3);
    }
    BOOST_TEST(counting_allocator<sum>::n_allocated == 2); 
    BOOST_TEST(counting_allocator<sum>::n_deallocated == 2);
    return boost::report_errors();
}

