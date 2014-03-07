
// Copyright 2006-2011 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_UNORDERED_TEST_CXX11_ALLOCATOR_HEADER)
#define BOOST_UNORDERED_TEST_CXX11_ALLOCATOR_HEADER

#include <boost/config.hpp>
#include <boost/limits.hpp>
#include <cstddef>

#include "../helpers/fwd.hpp"
#include "../helpers/memory.hpp"

namespace test
{
    struct allocator_false
    {
        enum {
            is_select_on_copy = 0,
            is_propagate_on_swap = 0,
            is_propagate_on_assign = 0,
            is_propagate_on_move = 0,
            cxx11_construct = 0
        };
    };

    struct allocator_flags_all
    {
        enum {
            is_select_on_copy = 1,
            is_propagate_on_swap = 1,
            is_propagate_on_assign = 1,
            is_propagate_on_move = 1,
            cxx11_construct = 1
        };
    };
    
    struct select_copy : allocator_false
    { enum { is_select_on_copy = 1 }; };
    struct propagate_swap : allocator_false
    { enum { is_propagate_on_swap = 1 }; };
    struct propagate_assign : allocator_false
    { enum { is_propagate_on_assign = 1 }; };
    struct propagate_move : allocator_false
    { enum { is_propagate_on_move = 1 }; };

    struct no_select_copy : allocator_flags_all
    { enum { is_select_on_copy = 0 }; };
    struct no_propagate_swap : allocator_flags_all
    { enum { is_propagate_on_swap = 0 }; };
    struct no_propagate_assign : allocator_flags_all
    { enum { is_propagate_on_assign = 0 }; };
    struct no_propagate_move : allocator_flags_all
    { enum { is_propagate_on_move = 0 }; };
    
    template <typename Flag>
    struct swap_allocator_base
    {
        struct propagate_on_container_swap {
            enum { value = Flag::is_propagate_on_swap }; };
    };

    template <typename Flag>
    struct assign_allocator_base
    {
        struct propagate_on_container_copy_assignment {
            enum { value = Flag::is_propagate_on_assign }; };
    };

    template <typename Flag>
    struct move_allocator_base
    {
        struct propagate_on_container_move_assignment {
            enum { value = Flag::is_propagate_on_move }; };
    };

    namespace
    {
        // boostinspect:nounnamed
        bool force_equal_allocator_value = false;
    }

    struct force_equal_allocator
    {
        bool old_value_;
    
        explicit force_equal_allocator(bool value)
            : old_value_(force_equal_allocator_value)
        { force_equal_allocator_value = value; }
        
        ~force_equal_allocator()
        { force_equal_allocator_value = old_value_; }
    };

    template <typename T>
    struct cxx11_allocator_base
    {
        int tag_;
        int selected_;

        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef T* pointer;
        typedef T const* const_pointer;
        typedef T& reference;
        typedef T const& const_reference;
        typedef T value_type;

        explicit cxx11_allocator_base(int t)
            : tag_(t), selected_(0)
        {
            detail::tracker.allocator_ref();
        }
        
        template <typename Y> cxx11_allocator_base(
                cxx11_allocator_base<Y> const& x)
            : tag_(x.tag_), selected_(x.selected_)
        {
            detail::tracker.allocator_ref();
        }

        cxx11_allocator_base(cxx11_allocator_base const& x)
            : tag_(x.tag_), selected_(x.selected_)
        {
            detail::tracker.allocator_ref();
        }

        ~cxx11_allocator_base()
        {
            detail::tracker.allocator_unref();
        }

        pointer address(reference r)
        {
            return pointer(&r);
        }

        const_pointer address(const_reference r)
        {
            return const_pointer(&r);
        }

        pointer allocate(size_type n) {
            pointer ptr(static_cast<T*>(::operator new(n * sizeof(T))));
            detail::tracker.track_allocate((void*) ptr, n, sizeof(T), tag_);
            return ptr;
        }

        pointer allocate(size_type n, void const* u)
        {
            pointer ptr(static_cast<T*>(::operator new(n * sizeof(T))));
            detail::tracker.track_allocate((void*) ptr, n, sizeof(T), tag_);
            return ptr;
        }

        void deallocate(pointer p, size_type n)
        {
            // Only checking tags when propagating swap.
            // Note that tags will be tested
            // properly in the normal allocator.
            detail::tracker.track_deallocate((void*) p, n, sizeof(T), tag_,
                 !force_equal_allocator_value);
            ::operator delete((void*) p);
        }

        void construct(T* p, T const& t) {
            detail::tracker.track_construct((void*) p, sizeof(T), tag_);
            new(p) T(t);
        }

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
        template<typename... Args> void construct(T* p, BOOST_FWD_REF(Args)... args) {
            detail::tracker.track_construct((void*) p, sizeof(T), tag_);
            new(p) T(boost::forward<Args>(args)...);
        }
#endif

        void destroy(T* p) {
            detail::tracker.track_destroy((void*) p, sizeof(T), tag_);
            p->~T();
        }

        size_type max_size() const {
            return (std::numeric_limits<size_type>::max)();
        }
    };

    template <typename T, typename Flags = propagate_swap,
        typename Enable = void>
    struct cxx11_allocator;

    template <typename T, typename Flags>
    struct cxx11_allocator<
        T, Flags,
        typename boost::disable_if_c<Flags::is_select_on_copy>::type
    > : public cxx11_allocator_base<T>,
        public swap_allocator_base<Flags>,
        public assign_allocator_base<Flags>,
        public move_allocator_base<Flags>,
        Flags
    {
        template <typename U> struct rebind {
            typedef cxx11_allocator<U, Flags> other;
        };

        explicit cxx11_allocator(int t = 0)
            : cxx11_allocator_base<T>(t)
        {
        }
        
        template <typename Y> cxx11_allocator(
                cxx11_allocator<Y, Flags> const& x)
            : cxx11_allocator_base<T>(x)
        {
        }

        cxx11_allocator(cxx11_allocator const& x)
            : cxx11_allocator_base<T>(x)
        {
        }

        // When not propagating swap, allocators are always equal
        // to avoid undefined behaviour.
        bool operator==(cxx11_allocator const& x) const
        {
            return force_equal_allocator_value || (this->tag_ == x.tag_);
        }

        bool operator!=(cxx11_allocator const& x) const
        {
            return !(*this == x);
        }
    };

    template <typename T, typename Flags>
    struct cxx11_allocator<
        T, Flags,
        typename boost::enable_if_c<Flags::is_select_on_copy>::type
    > : public cxx11_allocator_base<T>,
        public swap_allocator_base<Flags>,
        public assign_allocator_base<Flags>,
        public move_allocator_base<Flags>,
        Flags
    {
        cxx11_allocator select_on_container_copy_construction() const
        {
            cxx11_allocator tmp(*this);
            ++tmp.selected_;
            return tmp;
        }

        template <typename U> struct rebind {
            typedef cxx11_allocator<U, Flags> other;
        };

        explicit cxx11_allocator(int t = 0)
            : cxx11_allocator_base<T>(t)
        {
        }
        
        template <typename Y> cxx11_allocator(
                cxx11_allocator<Y, Flags> const& x)
            : cxx11_allocator_base<T>(x)
        {
        }

        cxx11_allocator(cxx11_allocator const& x)
            : cxx11_allocator_base<T>(x)
        {
        }

        // When not propagating swap, allocators are always equal
        // to avoid undefined behaviour.
        bool operator==(cxx11_allocator const& x) const
        {
            return force_equal_allocator_value || (this->tag_ == x.tag_);
        }

        bool operator!=(cxx11_allocator const& x) const
        {
            return !(*this == x);
        }
    };

    template <typename T, typename Flags>
    bool equivalent_impl(
            cxx11_allocator<T, Flags> const& x,
            cxx11_allocator<T, Flags> const& y,
            test::derived_type)
    {
        return x.tag_ == y.tag_;
    }

    // Function to check how many times an allocator has been selected,
    // return 0 for other allocators.

    struct convert_from_anything
    {
        template <typename T>
        convert_from_anything(T const&) {}
    };

    inline int selected_count(convert_from_anything)
    {
        return 0;
    }

    template <typename T, typename Flags>
    int selected_count(cxx11_allocator<T, Flags> const& x)
    {
        return x.selected_;
    }
}

#endif
