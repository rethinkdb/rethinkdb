/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   atomic_queue.hpp
 * \author Andrey Semashev
 * \date   14.07.2009
 *
 * \brief  This header is the Boost.Log library implementation, see the library documentation
 *         at http://www.boost.org/doc/libs/release/libs/log/doc/html/index.html.
 */

#ifndef BOOST_LOG_ATOMIC_QUEUE_HPP_INCLUDED_
#define BOOST_LOG_ATOMIC_QUEUE_HPP_INCLUDED_

#include <new>
#include <memory>
#include <utility>
#include <boost/log/detail/config.hpp>
#include <boost/log/detail/header.hpp>

// Detect if atomic CAS is supported
#if defined(__GNUC__)
#if (__SIZEOF_POINTER__ == 1 && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1)) ||\
    (__SIZEOF_POINTER__ == 2 && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2)) ||\
    (__SIZEOF_POINTER__ == 4 && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)) ||\
    (__SIZEOF_POINTER__ == 8 && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8)) ||\
    (__SIZEOF_POINTER__ == 16 && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_16))

#define BOOST_LOG_CAS_PTR(expected, new_value, pointer)\
    __sync_bool_compare_and_swap(pointer, expected, new_value)

#endif // __SIZEOF_POINTER__ == N && defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_N)
#endif // defined(__GNUC__)

// For sake of older GCC versions on Windows, this section should go after GCC
#if !defined(BOOST_LOG_CAS_PTR) && defined(BOOST_WINDOWS)

#include <boost/detail/interlocked.hpp>

#define BOOST_LOG_CAS_PTR(expected, new_value, pointer)\
    (BOOST_INTERLOCKED_COMPARE_EXCHANGE_POINTER((void**)(pointer), (void*)(new_value), (void*)(expected)) == (expected))

#endif // !defined(BOOST_LOG_CAS_PTR) && defined(BOOST_WINDOWS)

#if !defined(BOOST_LOG_CAS_PTR)
#include <boost/thread/locks.hpp>
#include <boost/log/detail/spin_mutex.hpp>
#endif // !defined(BOOST_LOG_CAS_PTR)

#if defined(__GNUC__)
#define BOOST_LOG_ALIGN(x) __attribute__((aligned(x)))
#elif defined(_MSC_VER)
#define BOOST_LOG_ALIGN(x) __declspec(align(x))
#else
#define BOOST_LOG_ALIGN(x)
#endif

#define BOOST_LOG_CPU_CACHE_LINE_SIZE 64

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

namespace aux {

/*!
 * A simple atomic queue class. Allows to put and eject elements from different threads concurrently.
 */
template< typename T, typename AllocatorT = std::allocator< T > >
class BOOST_LOG_ALIGN(BOOST_LOG_CPU_CACHE_LINE_SIZE) atomic_queue
{
public:
    typedef typename AllocatorT::BOOST_NESTED_TEMPLATE rebind< T >::other allocator;
    typedef typename allocator::value_type value_type;
    typedef typename allocator::reference reference;
    typedef typename allocator::pointer pointer;
    typedef typename allocator::const_reference const_reference;
    typedef typename allocator::const_pointer const_pointer;
    typedef typename allocator::difference_type difference_type;

    struct node;
    friend struct node;
    struct node
    {
        node* m_pPrev;
        node* m_pNext;
        value_type m_Value;

        node() : m_pPrev(NULL), m_pNext(NULL)
        {
        }
        explicit node(value_type const& val) : m_pPrev(NULL), m_pNext(NULL), m_Value(val)
        {
        }

        static void* operator new(std::size_t size)
        {
            BOOST_ASSERT(size == sizeof(node));
            return atomic_queue::g_Allocator.allocate(1);
        }
        static void operator delete(void* p)
        {
            atomic_queue::g_Allocator.deallocate(reinterpret_cast< node* >(p), 1);
        }
    };

private:
    typedef typename AllocatorT::BOOST_NESTED_TEMPLATE rebind< node >::other internal_allocator;

    struct implementation
    {
        node* m_pLast;
#if !defined(BOOST_LOG_CAS_PTR)
        spin_mutex m_Mutex;
#endif // !defined(BOOST_LOG_CAS_PTR)
    };

private:
    implementation m_Impl;

    // Padding to avoid false aliasing
    unsigned char padding
    [
        ((sizeof(implementation) + BOOST_LOG_CPU_CACHE_LINE_SIZE - 1) / BOOST_LOG_CPU_CACHE_LINE_SIZE)
            * BOOST_LOG_CPU_CACHE_LINE_SIZE - sizeof(implementation)
    ];

    static internal_allocator g_Allocator;

public:
    //! Constructor. Creates an empty queue.
    atomic_queue()
    {
        m_Impl.m_pLast = NULL;
    }
    //! Destructor. Destroys all contained elements, if any.
    ~atomic_queue()
    {
        register node* p = m_Impl.m_pLast;
        while (p != NULL)
        {
            register node* prev = p->m_pPrev;
            delete p;
            p = prev;
        }
    }

    //! Puts an element to the queue.
    void push(value_type const& val)
    {
#if !defined(BOOST_LOG_CAS_PTR)

        register node* p = new node(val);
        lock_guard< spin_mutex > _(m_Impl.m_Mutex);
        p->m_pPrev = m_Impl.m_pLast;
        m_Impl.m_pLast = p;

#else // !defined(BOOST_LOG_CAS_PTR)

        register node* p = new node(val);
        register node* last;
        do
        {
            last = static_cast< node* volatile& >(m_Impl.m_pLast);
            p->m_pPrev = last;
        }
        while (!BOOST_LOG_CAS_PTR(last, p, &m_Impl.m_pLast));

#endif // !defined(BOOST_LOG_CAS_PTR)
    }

    //! Attempts to put an element to the queue.
    bool try_push(value_type const& val)
    {
#if !defined(BOOST_LOG_CAS_PTR)

        unique_lock< spin_mutex > lock(m_Impl.m_Mutex, try_to_lock);
        if (lock)
        {
            register node* p = new node(val);
            p->m_pPrev = m_Impl.m_pLast;
            m_Impl.m_pLast = p;
            return true;
        }
        else
            return false;

#else // !defined(BOOST_LOG_CAS_PTR)

/*
        register node* p = new node(val);
        register node* last;
        last = static_cast< node* volatile& >(m_Impl.m_pLast);
        p->m_pPrev = last;
        register bool done = BOOST_LOG_CAS_PTR(last, p, &m_Impl.m_pLast);
        if (!done)
            delete p;
        return done;
*/
        // No point for the backoff scheme since we'll most likely lose more
        // on memory (de)allocation than on one or several loops of trial to CAS.
        push(val);
        return true;

#endif // !defined(BOOST_LOG_CAS_PTR)
    }

    //! Removes all elements from the queue and returns a reference to the first element.
    std::pair< node*, node* > eject_nodes()
    {
        register node* p;

#if !defined(BOOST_LOG_CAS_PTR)

        {
            lock_guard< spin_mutex > _(m_Impl.m_Mutex);
            p = m_Impl.m_pLast;
            m_Impl.m_pLast = NULL;
        }

#else // !defined(BOOST_LOG_CAS_PTR)

        do
        {
            p = static_cast< node* volatile& >(m_Impl.m_pLast);
        }
        while (!BOOST_LOG_CAS_PTR(p, static_cast< node* >(NULL), &m_Impl.m_pLast));

#endif // !defined(BOOST_LOG_CAS_PTR)

        // Reconstruct forward references
        std::pair< node*, node* > res(static_cast< node* >(NULL), p);
        if (p)
        {
            for (register node* prev = p->m_pPrev; prev != NULL; prev = p->m_pPrev)
            {
                prev->m_pNext = p;
                p = prev;
            }
            res.first = p;
        }

        return res;
    }
};

template< typename T, typename AllocatorT >
typename atomic_queue< T, AllocatorT >::internal_allocator atomic_queue< T, AllocatorT >::g_Allocator;

} // namespace aux

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_ATOMIC_QUEUE_HPP_INCLUDED_
