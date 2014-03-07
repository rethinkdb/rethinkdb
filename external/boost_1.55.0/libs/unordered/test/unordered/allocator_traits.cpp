
// Copyright 2011 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include <boost/unordered/detail/allocate.hpp>
#include <boost/detail/lightweight_test.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/static_assert.hpp>
#include <boost/limits.hpp>

// Boilerplate

#define ALLOCATOR_METHODS(name)                                             \
    template <typename U> struct rebind {                                   \
        typedef name<U> other;                                              \
    };                                                                      \
                                                                            \
    name() {}                                                               \
    template <typename Y> name(name<Y> const&) {}                           \
    T* address(T& r) { return &r;}                                          \
    T const* address(T const& r) { return &r; }                             \
    T* allocate(std::size_t n)                                              \
    { return static_cast<T*>(::operator new(n * sizeof(T))); }              \
    T* allocate(std::size_t n, void const* u)                               \
    { return static_cast<T*>(::operator new(n * sizeof(T))); }              \
    void deallocate(T* p, std::size_t n) { ::operator delete((void*) p); }  \
    void construct(T* p, T const& t) { new(p) T(t); }                       \
    void destroy(T* p) { p->~T(); }                                         \
    std::size_t max_size() const                                            \
    { return (std::numeric_limits<std::size_t>::max)(); }                   \
    bool operator==(name<T> const&) { return true; }                        \
    bool operator!=(name<T> const&) { return false; }                       \
/**/

#define ALLOCATOR_METHODS_TYPEDEFS(name)                                    \
    template <typename U> struct rebind {                                   \
        typedef name<U> other;                                              \
    };                                                                      \
                                                                            \
    name() {}                                                               \
    template <typename Y> name(name<Y> const&) {}                           \
    pointer address(T& r) { return &r;}                                     \
    const_pointer address(T const& r) { return &r; }                        \
    pointer allocate(std::size_t n)                                         \
    { return pointer(::operator new(n * sizeof(T))); }                      \
    pointer allocate(std::size_t n, void const* u)                          \
    { return pointer(::operator new(n * sizeof(T))); }                      \
    void deallocate(pointer p, std::size_t n)                               \
    { ::operator delete((void*) p); }                                       \
    void construct(T* p, T const& t) { new(p) T(t); }                       \
    void destroy(T* p) { p->~T(); }                                         \
    size_type max_size() const                                              \
    { return (std::numeric_limits<size_type>::max)(); }                     \
    bool operator==(name<T> const&) { return true; }                        \
    bool operator!=(name<T> const&) { return false; }                       \
/**/

struct yes_type { enum { value = true }; };
struct no_type { enum { value = false }; };

// For tracking calls...

static int selected;
void reset() {
    selected = 0;
}

template <typename Allocator>
int call_select()
{
    typedef boost::unordered::detail::allocator_traits<Allocator> traits;
    Allocator a;

    reset();
    BOOST_TEST(traits::select_on_container_copy_construction(a) == a);
    return selected;
}

// Empty allocator test

template <typename T>
struct empty_allocator
{
    typedef T value_type;
    ALLOCATOR_METHODS(empty_allocator)
};

void test_empty_allocator()
{
    typedef empty_allocator<int> allocator;
    typedef boost::unordered::detail::allocator_traits<allocator> traits;
#if BOOST_UNORDERED_USE_ALLOCATOR_TRAITS == 1
    BOOST_STATIC_ASSERT((boost::is_same<traits::size_type,
        std::make_unsigned<std::ptrdiff_t>::type>::value));
#else
    BOOST_STATIC_ASSERT((boost::is_same<traits::size_type, std::size_t>::value));
#endif
    BOOST_STATIC_ASSERT((boost::is_same<traits::difference_type, std::ptrdiff_t>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::pointer, int*>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::const_pointer, int const*>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::value_type, int>::value));
    BOOST_TEST(!traits::propagate_on_container_copy_assignment::value);
    BOOST_TEST(!traits::propagate_on_container_move_assignment::value);
    BOOST_TEST(!traits::propagate_on_container_swap::value);
    BOOST_TEST(call_select<allocator>() == 0);
}

// allocator 1

template <typename T>
struct allocator1
{
    typedef T value_type;
    ALLOCATOR_METHODS(allocator1)
    
    typedef yes_type propagate_on_container_copy_assignment;
    typedef yes_type propagate_on_container_move_assignment;
    typedef yes_type propagate_on_container_swap;
    
    allocator1<T> select_on_container_copy_construction() const {
        ++selected;
        return allocator1<T>();
    }
};

void test_allocator1()
{
    typedef allocator1<int> allocator;
    typedef boost::unordered::detail::allocator_traits<allocator> traits;
#if BOOST_UNORDERED_USE_ALLOCATOR_TRAITS == 1
    BOOST_STATIC_ASSERT((boost::is_same<traits::size_type,
        std::make_unsigned<std::ptrdiff_t>::type>::value));
#else
    BOOST_STATIC_ASSERT((boost::is_same<traits::size_type, std::size_t>::value));
#endif
    BOOST_STATIC_ASSERT((boost::is_same<traits::difference_type, std::ptrdiff_t>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::pointer, int*>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::const_pointer, int const*>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::value_type, int>::value));
    BOOST_TEST(traits::propagate_on_container_copy_assignment::value);
    BOOST_TEST(traits::propagate_on_container_move_assignment::value);
    BOOST_TEST(traits::propagate_on_container_swap::value);
    BOOST_TEST(call_select<allocator>() == 1);
}

// allocator 2

template <typename Alloc>
struct allocator2_base
{
    Alloc select_on_container_copy_construction() const {
        ++selected;
        return Alloc();
    }
};

template <typename T>
struct allocator2 : allocator2_base<allocator2<T> >
{
    typedef T value_type;
    typedef T* pointer;
    typedef T const* const_pointer;
    typedef std::size_t size_type;
    
    ALLOCATOR_METHODS(allocator2)
    
    typedef no_type propagate_on_container_copy_assignment;
    typedef no_type propagate_on_container_move_assignment;
    typedef no_type propagate_on_container_swap;
};

void test_allocator2()
{
    typedef allocator2<int> allocator;
    typedef boost::unordered::detail::allocator_traits<allocator> traits;
    BOOST_STATIC_ASSERT((boost::is_same<traits::size_type, std::size_t>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::difference_type, std::ptrdiff_t>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::pointer, int*>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::const_pointer, int const*>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::value_type, int>::value));
    BOOST_TEST(!traits::propagate_on_container_copy_assignment::value);
    BOOST_TEST(!traits::propagate_on_container_move_assignment::value);
    BOOST_TEST(!traits::propagate_on_container_swap::value);
    BOOST_TEST(call_select<allocator>() == 1);
}

// allocator 3

template <typename T>
struct ptr
{
    T* value_;
    
    ptr(void* v) : value_((T*) v) {}
    T& operator*() const { return *value_; }
};

template <>
struct ptr<void>
{
    void* value_;
    ptr(void* v) : value_(v) {}
};

template <>
struct ptr<const void>
{
    void const* value_;
    ptr(void const* v) : value_(v) {}
};

template <typename T>
struct allocator3
{
    typedef T value_type;
    typedef ptr<T> pointer;
    typedef ptr<T const> const_pointer;
    typedef unsigned short size_type;

    ALLOCATOR_METHODS_TYPEDEFS(allocator3)
    
    typedef yes_type propagate_on_container_copy_assignment;
    typedef no_type propagate_on_container_move_assignment;

    allocator3<T> select_on_container_copy_construction() const {
        ++selected;
        return allocator3<T>();
    }
};

void test_allocator3()
{
    typedef allocator3<int> allocator;
    typedef boost::unordered::detail::allocator_traits<allocator> traits;
    BOOST_STATIC_ASSERT((boost::is_same<traits::size_type, unsigned short>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::difference_type, std::ptrdiff_t>::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::pointer, ptr<int> >::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::const_pointer, ptr<int const> >::value));
    BOOST_STATIC_ASSERT((boost::is_same<traits::value_type, int>::value));
    BOOST_TEST(traits::propagate_on_container_copy_assignment::value);
    BOOST_TEST(!traits::propagate_on_container_move_assignment::value);
    BOOST_TEST(!traits::propagate_on_container_swap::value);
    BOOST_TEST(call_select<allocator>() == 1);
}

int main()
{
    test_empty_allocator();
    test_allocator1();
    test_allocator2();
    test_allocator3();
    return boost::report_errors();
}
