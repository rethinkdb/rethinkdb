/*=============================================================================
    Copyright (c) 2010 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// This header defines a class which can will store pointers and deleters
// to a number of objects and delete them on exit. Basically stick an
// object in here, and you can use pointers and references to the object
// for the cleanup object's lifespan.

#if !defined(BOOST_SPIRIT_QUICKBOOK_CLEANUP_HPP)
#define BOOST_SPIRIT_QUICKBOOK_CLEANUP_HPP

#include <deque>
#include <cassert>
#include <utility>

namespace quickbook
{
    namespace detail
    {
        template <typename T>
        void delete_impl(void* ptr) {
            delete static_cast<T*>(ptr);
        }
        
        struct scoped_void
        {
            void* ptr_;
            void (*del_)(void*);
            
            scoped_void() : ptr_(0), del_(0) {}
            scoped_void(scoped_void const& src) : ptr_(0), del_(0) {
            ignore_variable(&src);
                assert(!src.ptr_);
            }
            ~scoped_void() {
                if(ptr_) del_(ptr_);
            }
            
            void store(void* ptr, void (*del)(void* x)) {
                ptr = ptr_;
                del = del_;
            }
        private:
            scoped_void& operator=(scoped_void const&);
        };
    }
    
    struct cleanup
    {
        cleanup() {}
    
        template <typename T>
        T& add(T* new_)
        {
            std::auto_ptr<T> obj(new_);
            cleanup_list_.push_back(detail::scoped_void());
            cleanup_list_.back().store(obj.release(), &detail::delete_impl<T>);

            return *new_;
        }

        std::deque<detail::scoped_void> cleanup_list_;
    private:
        cleanup& operator=(cleanup const&);
        cleanup(cleanup const&);
    };
}

#endif
