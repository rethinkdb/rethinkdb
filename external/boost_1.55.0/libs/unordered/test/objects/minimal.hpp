
// Copyright 2006-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Define some minimal classes which provide the bare minimum concepts to
// test that the containers don't rely on something that they shouldn't.
// They are not intended to be good examples of how to implement the concepts.

#if !defined(BOOST_UNORDERED_OBJECTS_MINIMAL_HEADER)
#define BOOST_UNORDERED_OBJECTS_MINIMAL_HEADER

#include <cstddef>
#include <boost/move/move.hpp>
#include <utility>

#if defined(BOOST_MSVC)
#pragma warning(push)
#pragma warning(disable:4100) // unreferenced formal parameter
#endif

namespace test
{
namespace minimal
{
    class destructible;
    class copy_constructible;
    class copy_constructible_equality_comparable;
    class default_assignable;
    class assignable;

    struct ampersand_operator_used {};

    template <class T> class hash;
    template <class T> class equal_to;
    template <class T> class ptr;
    template <class T> class const_ptr;
    template <class T> class allocator;
    template <class T> class cxx11_allocator;

    struct constructor_param
    {
        operator int() const { return 0; }
    };

    class destructible
    {
    public:
        destructible(constructor_param const&) {}
        ~destructible() {}
    private:
        destructible(destructible const&);
        destructible& operator=(destructible const&);
    };

    class copy_constructible
    {
    public:
        copy_constructible(constructor_param const&) {}
        copy_constructible(copy_constructible const&) {}
        ~copy_constructible() {}
    private:
        copy_constructible& operator=(copy_constructible const&);
        copy_constructible() {}
    };

    class copy_constructible_equality_comparable
    {
    public:
        copy_constructible_equality_comparable(constructor_param const&) {}

        copy_constructible_equality_comparable(
            copy_constructible_equality_comparable const&)
        {
        }

        ~copy_constructible_equality_comparable()
        {
        }

    private:
        copy_constructible_equality_comparable& operator=(
            copy_constructible_equality_comparable const&);
        copy_constructible_equality_comparable() {}
        ampersand_operator_used operator&() const { return ampersand_operator_used(); }
    };

    bool operator==(
        copy_constructible_equality_comparable,
        copy_constructible_equality_comparable)
    {
        return true;
    }

    bool operator!=(
        copy_constructible_equality_comparable,
        copy_constructible_equality_comparable)
    {
        return false;
    }

    class default_assignable
    {
    public:
        default_assignable(constructor_param const&) {}

        default_assignable()
        {
        }

        default_assignable(default_assignable const&)
        {
        }

        default_assignable& operator=(default_assignable const&)
        {
            return *this;
        }

        ~default_assignable()
        {
        }

    private:
        ampersand_operator_used operator&() const {
            return ampersand_operator_used(); }
    };

    class assignable
    {
    public:
        assignable(constructor_param const&) {}
        assignable(assignable const&) {}
        assignable& operator=(assignable const&) { return *this; }
        ~assignable() {}

    private:
        assignable() {}
        // TODO: This messes up a concept check in the tests.
        //ampersand_operator_used operator&() const { return ampersand_operator_used(); }
    };

    struct movable_init {};

    class movable1
    {
        BOOST_MOVABLE_BUT_NOT_COPYABLE(movable1)

    public:
        movable1(constructor_param const&) {}
        movable1() {}
        explicit movable1(movable_init) {}
        movable1(BOOST_RV_REF(movable1)) {}
        movable1& operator=(BOOST_RV_REF(movable1)) { return *this; }
        ~movable1() {}
    };

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    class movable2
    {
    public:
        movable2(constructor_param const&) {}
        explicit movable2(movable_init) {}
        movable2(movable2&&) {}
        ~movable2() {}
        movable2& operator=(movable2&&) { return *this; }
    private:
        movable2() {}
        movable2(movable2 const&);
        movable2& operator=(movable2 const&);
    };
#else
    typedef movable1 movable2;
#endif

    template <class T>
    class hash
    {
    public:
        hash(constructor_param const&) {}
        hash() {}
        hash(hash const&) {}
        hash& operator=(hash const&) { return *this; }
        ~hash() {}

        std::size_t operator()(T const&) const { return 0; }
    private:
        ampersand_operator_used operator&() const { return ampersand_operator_used(); }
    };

    template <class T>
    class equal_to
    {
    public:
        equal_to(constructor_param const&) {}
        equal_to() {}
        equal_to(equal_to const&) {}
        equal_to& operator=(equal_to const&) { return *this; }
        ~equal_to() {}

        bool operator()(T const&, T const&) const { return true; }
    private:
        ampersand_operator_used operator&() const { return ampersand_operator_used(); }
    };

    template <class T> class ptr;
    template <class T> class const_ptr;

    struct void_ptr
    {
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
        template <typename T>
        friend class ptr;
    private:
#endif

        void* ptr_;

    public:
        void_ptr() : ptr_(0) {}

        template <typename T>
        explicit void_ptr(ptr<T> const& x) : ptr_(x.ptr_) {}

        // I'm not using the safe bool idiom because the containers should be
        // able to cope with bool conversions.
        operator bool() const { return !!ptr_; }

        bool operator==(void_ptr const& x) const { return ptr_ == x.ptr_; }
        bool operator!=(void_ptr const& x) const { return ptr_ != x.ptr_; }
    };

    class void_const_ptr
    {
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
        template <typename T>
        friend class const_ptr;
    private:
#endif

        void* ptr_;

    public:
        void_const_ptr() : ptr_(0) {}

        template <typename T>
        explicit void_const_ptr(const_ptr<T> const& x) : ptr_(x.ptr_) {}

        // I'm not using the safe bool idiom because the containers should be
        // able to cope with bool conversions.
        operator bool() const { return !!ptr_; }

        bool operator==(void_const_ptr const& x) const { return ptr_ == x.ptr_; }
        bool operator!=(void_const_ptr const& x) const { return ptr_ != x.ptr_; }
    };

    template <class T>
    class ptr
    {
        friend class allocator<T>;
        friend class const_ptr<T>;
        friend struct void_ptr;

        T* ptr_;

        ptr(T* x) : ptr_(x) {}
    public:
        ptr() : ptr_(0) {}
        explicit ptr(void_ptr const& x) : ptr_((T*) x.ptr_) {}

        T& operator*() const { return *ptr_; }
        T* operator->() const { return ptr_; }
        ptr& operator++() { ++ptr_; return *this; }
        ptr operator++(int) { ptr tmp(*this); ++ptr_; return tmp; }
        ptr operator+(std::ptrdiff_t s) const { return ptr<T>(ptr_ + s); }
        friend ptr operator+(std::ptrdiff_t s, ptr p)
            { return ptr<T>(s + p.ptr_); }
        T& operator[](std::ptrdiff_t s) const { return ptr_[s]; }
        bool operator!() const { return !ptr_; }
        
        // I'm not using the safe bool idiom because the containers should be
        // able to cope with bool conversions.
        operator bool() const { return !!ptr_; }

        bool operator==(ptr const& x) const { return ptr_ == x.ptr_; }
        bool operator!=(ptr const& x) const { return ptr_ != x.ptr_; }
        bool operator<(ptr const& x) const { return ptr_ < x.ptr_; }
        bool operator>(ptr const& x) const { return ptr_ > x.ptr_; }
        bool operator<=(ptr const& x) const { return ptr_ <= x.ptr_; }
        bool operator>=(ptr const& x) const { return ptr_ >= x.ptr_; }
    private:
        // TODO:
        //ampersand_operator_used operator&() const { return ampersand_operator_used(); }
    };

    template <class T>
    class const_ptr
    {
        friend class allocator<T>;
        friend struct const_void_ptr;

        T const* ptr_;

        const_ptr(T const* ptr) : ptr_(ptr) {}
    public:
        const_ptr() : ptr_(0) {}
        const_ptr(ptr<T> const& x) : ptr_(x.ptr_) {}
        explicit const_ptr(void_const_ptr const& x) : ptr_((T const*) x.ptr_) {}

        T const& operator*() const { return *ptr_; }
        T const* operator->() const { return ptr_; }
        const_ptr& operator++() { ++ptr_; return *this; }
        const_ptr operator++(int) { const_ptr tmp(*this); ++ptr_; return tmp; }
        const_ptr operator+(std::ptrdiff_t s) const
            { return const_ptr(ptr_ + s); }
        friend const_ptr operator+(std::ptrdiff_t s, const_ptr p)
            { return ptr<T>(s + p.ptr_); }
        T const& operator[](int s) const { return ptr_[s]; }
        bool operator!() const { return !ptr_; }
        operator bool() const { return !!ptr_; }

        bool operator==(const_ptr const& x) const { return ptr_ == x.ptr_; }
        bool operator!=(const_ptr const& x) const { return ptr_ != x.ptr_; }
        bool operator<(const_ptr const& x) const { return ptr_ < x.ptr_; }
        bool operator>(const_ptr const& x) const { return ptr_ > x.ptr_; }
        bool operator<=(const_ptr const& x) const { return ptr_ <= x.ptr_; }
        bool operator>=(const_ptr const& x) const { return ptr_ >= x.ptr_; }
    private:
        // TODO:
        //ampersand_operator_used operator&() const { return ampersand_operator_used(); }
    };

    template <class T>
    class allocator
    {
    public:
        typedef std::size_t size_type;
        typedef std::ptrdiff_t difference_type;
        typedef void_ptr void_pointer;
        typedef void_const_ptr const_void_pointer;
        typedef ptr<T> pointer;
        typedef const_ptr<T> const_pointer;
        typedef T& reference;
        typedef T const& const_reference;
        typedef T value_type;

        template <class U> struct rebind { typedef allocator<U> other; };

        allocator() {}
        template <class Y> allocator(allocator<Y> const&) {}
        allocator(allocator const&) {}
        ~allocator() {}

        pointer address(reference r) { return pointer(&r); }
        const_pointer address(const_reference r) { return const_pointer(&r); }

        pointer allocate(size_type n) {
            return pointer(static_cast<T*>(::operator new(n * sizeof(T))));
        }

        template <class Y>
        pointer allocate(size_type n, const_ptr<Y> u)
        {
            return pointer(static_cast<T*>(::operator new(n * sizeof(T))));
        }

        void deallocate(pointer p, size_type)
        {
            ::operator delete((void*) p.ptr_);
        }

        void construct(T* p, T const& t) { new((void*)p) T(t); }

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
        template<class... Args> void construct(T* p, BOOST_FWD_REF(Args)... args) {
            new((void*)p) T(boost::forward<Args>(args)...);
        }
#endif

        void destroy(T* p) { p->~T(); }

        size_type max_size() const { return 1000; }

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP) || \
        BOOST_WORKAROUND(BOOST_MSVC, <= 1300)
    public: allocator& operator=(allocator const&) { return *this;}
#else
    private: allocator& operator=(allocator const&);
#endif
    private:
        ampersand_operator_used operator&() const { return ampersand_operator_used(); }
    };

    template <class T>
    inline bool operator==(allocator<T> const&, allocator<T> const&)
    {
        return true;
    }

    template <class T>
    inline bool operator!=(allocator<T> const&, allocator<T> const&)
    {
        return false;
    }

    template <class T>
    void swap(allocator<T>&, allocator<T>&)
    {
    }

    // C++11 allocator
    //
    // Not a fully minimal C++11 allocator, just what I support. Hopefully will
    // cut down further in the future.

    template <class T>
    class cxx11_allocator
    {
    public:
        typedef T value_type;
        template <class U> struct rebind { typedef cxx11_allocator<U> other; };

        cxx11_allocator() {}
        template <class Y> cxx11_allocator(cxx11_allocator<Y> const&) {}
        cxx11_allocator(cxx11_allocator const&) {}
        ~cxx11_allocator() {}

        T* address(T& r) { return &r; }
        T const* address(T const& r) { return &r; }

        T* allocate(std::size_t n) {
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }

        template <class Y>
        T* allocate(std::size_t n, const_ptr<Y> u) {
            return static_cast<T*>(::operator new(n * sizeof(T)));
        }

        void deallocate(T* p, std::size_t) {
            ::operator delete((void*) p);
        }

        void construct(T* p, T const& t) { new((void*)p) T(t); }

#if !defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
        template<class... Args> void construct(T* p, BOOST_FWD_REF(Args)... args) {
            new((void*)p) T(boost::forward<Args>(args)...);
        }
#endif

        void destroy(T* p) { p->~T(); }

        std::size_t max_size() const { return 1000u; }
    };

    template <class T>
    inline bool operator==(cxx11_allocator<T> const&, cxx11_allocator<T> const&)
    {
        return true;
    }

    template <class T>
    inline bool operator!=(cxx11_allocator<T> const&, cxx11_allocator<T> const&)
    {
        return false;
    }

    template <class T>
    void swap(cxx11_allocator<T>&, cxx11_allocator<T>&)
    {
    }
}
}

#if defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
namespace boost {
#else
namespace test {
namespace minimal {
#endif
    std::size_t hash_value(
        test::minimal::copy_constructible_equality_comparable)
    {
        return 1;
    }
#if !defined(BOOST_NO_ARGUMENT_DEPENDENT_LOOKUP)
}}
#else
}
#endif


#if defined(BOOST_MSVC)
#pragma warning(pop)
#endif

#endif
