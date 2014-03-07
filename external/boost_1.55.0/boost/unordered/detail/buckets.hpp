
// Copyright (C) 2003-2004 Jeremy B. Maitin-Shepard.
// Copyright (C) 2005-2011 Daniel James
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_UNORDERED_DETAIL_MANAGER_HPP_INCLUDED
#define BOOST_UNORDERED_DETAIL_MANAGER_HPP_INCLUDED

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <boost/unordered/detail/util.hpp>
#include <boost/unordered/detail/allocate.hpp>
#include <boost/type_traits/aligned_storage.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/is_nothrow_move_constructible.hpp>
#include <boost/type_traits/is_nothrow_move_assignable.hpp>
#include <boost/swap.hpp>
#include <boost/assert.hpp>
#include <boost/limits.hpp>
#include <boost/iterator.hpp>

namespace boost { namespace unordered { namespace detail {

    template <typename Types> struct table;
    template <typename NodePointer> struct bucket;
    struct ptr_bucket;
    template <typename Types> struct table_impl;
    template <typename Types> struct grouped_table_impl;

}}}

// The 'iterator_detail' namespace was a misguided attempt at avoiding ADL
// in the detail namespace. It didn't work because the template parameters
// were in detail. I'm not changing it at the moment to be safe. I might
// do in the future if I change the iterator types.
namespace boost { namespace unordered { namespace iterator_detail {

    ////////////////////////////////////////////////////////////////////////////
    // Iterators
    //
    // all no throw

    template <typename Node> struct iterator;
    template <typename Node, typename ConstNodePointer> struct c_iterator;
    template <typename Node, typename Policy> struct l_iterator;
    template <typename Node, typename ConstNodePointer, typename Policy>
        struct cl_iterator;

    // Local Iterators
    //
    // all no throw

    template <typename Node, typename Policy>
    struct l_iterator
        : public boost::iterator<
            std::forward_iterator_tag,
            typename Node::value_type,
            std::ptrdiff_t,
            typename Node::node_pointer,
            typename Node::value_type&>
    {
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
        template <typename Node2, typename ConstNodePointer, typename Policy2>
        friend struct boost::unordered::iterator_detail::cl_iterator;
    private:
#endif
        typedef typename Node::node_pointer node_pointer;
        typedef boost::unordered::iterator_detail::iterator<Node> iterator;
        node_pointer ptr_;
        std::size_t bucket_;
        std::size_t bucket_count_;

    public:

        typedef typename Node::value_type value_type;

        l_iterator() BOOST_NOEXCEPT : ptr_() {}

        l_iterator(iterator x, std::size_t b, std::size_t c) BOOST_NOEXCEPT
            : ptr_(x.node_), bucket_(b), bucket_count_(c) {}

        value_type& operator*() const {
            return ptr_->value();
        }

        value_type* operator->() const {
            return ptr_->value_ptr();
        }

        l_iterator& operator++() {
            ptr_ = static_cast<node_pointer>(ptr_->next_);
            if (ptr_ && Policy::to_bucket(bucket_count_, ptr_->hash_)
                    != bucket_)
                ptr_ = node_pointer();
            return *this;
        }

        l_iterator operator++(int) {
            l_iterator tmp(*this);
            ++(*this);
            return tmp;
        }

        bool operator==(l_iterator x) const BOOST_NOEXCEPT {
            return ptr_ == x.ptr_;
        }

        bool operator!=(l_iterator x) const BOOST_NOEXCEPT {
            return ptr_ != x.ptr_;
        }
    };

    template <typename Node, typename ConstNodePointer, typename Policy>
    struct cl_iterator
        : public boost::iterator<
            std::forward_iterator_tag,
            typename Node::value_type,
            std::ptrdiff_t,
            ConstNodePointer,
            typename Node::value_type const&>
    {
        friend struct boost::unordered::iterator_detail::l_iterator
            <Node, Policy>;
    private:

        typedef typename Node::node_pointer node_pointer;
        typedef boost::unordered::iterator_detail::iterator<Node> iterator;
        node_pointer ptr_;
        std::size_t bucket_;
        std::size_t bucket_count_;

    public:

        typedef typename Node::value_type value_type;

        cl_iterator() BOOST_NOEXCEPT : ptr_() {}

        cl_iterator(iterator x, std::size_t b, std::size_t c) BOOST_NOEXCEPT :
            ptr_(x.node_), bucket_(b), bucket_count_(c) {}

        cl_iterator(boost::unordered::iterator_detail::l_iterator<
                Node, Policy> const& x) BOOST_NOEXCEPT :
            ptr_(x.ptr_), bucket_(x.bucket_), bucket_count_(x.bucket_count_)
        {}

        value_type const& operator*() const {
            return ptr_->value();
        }

        value_type const* operator->() const {
            return ptr_->value_ptr();
        }

        cl_iterator& operator++() {
            ptr_ = static_cast<node_pointer>(ptr_->next_);
            if (ptr_ && Policy::to_bucket(bucket_count_, ptr_->hash_)
                    != bucket_)
                ptr_ = node_pointer();
            return *this;
        }

        cl_iterator operator++(int) {
            cl_iterator tmp(*this);
            ++(*this);
            return tmp;
        }

        friend bool operator==(cl_iterator const& x, cl_iterator const& y)
            BOOST_NOEXCEPT
        {
            return x.ptr_ == y.ptr_;
        }

        friend bool operator!=(cl_iterator const& x, cl_iterator const& y)
            BOOST_NOEXCEPT
        {
            return x.ptr_ != y.ptr_;
        }
    };

    template <typename Node>
    struct iterator
        : public boost::iterator<
            std::forward_iterator_tag,
            typename Node::value_type,
            std::ptrdiff_t,
            typename Node::node_pointer,
            typename Node::value_type&>
    {
#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
        template <typename, typename>
        friend struct boost::unordered::iterator_detail::c_iterator;
        template <typename, typename>
        friend struct boost::unordered::iterator_detail::l_iterator;
        template <typename, typename, typename>
        friend struct boost::unordered::iterator_detail::cl_iterator;
        template <typename>
        friend struct boost::unordered::detail::table;
        template <typename>
        friend struct boost::unordered::detail::table_impl;
        template <typename>
        friend struct boost::unordered::detail::grouped_table_impl;
    private:
#endif
        typedef typename Node::node_pointer node_pointer;
        node_pointer node_;

    public:

        typedef typename Node::value_type value_type;

        iterator() BOOST_NOEXCEPT : node_() {}

        explicit iterator(typename Node::link_pointer x) BOOST_NOEXCEPT :
            node_(static_cast<node_pointer>(x)) {}

        value_type& operator*() const {
            return node_->value();
        }

        value_type* operator->() const {
            return &node_->value();
        }

        iterator& operator++() {
            node_ = static_cast<node_pointer>(node_->next_);
            return *this;
        }

        iterator operator++(int) {
            iterator tmp(node_);
            node_ = static_cast<node_pointer>(node_->next_);
            return tmp;
        }

        bool operator==(iterator const& x) const BOOST_NOEXCEPT {
            return node_ == x.node_;
        }

        bool operator!=(iterator const& x) const BOOST_NOEXCEPT {
            return node_ != x.node_;
        }
    };

    template <typename Node, typename ConstNodePointer>
    struct c_iterator
        : public boost::iterator<
            std::forward_iterator_tag,
            typename Node::value_type,
            std::ptrdiff_t,
            ConstNodePointer,
            typename Node::value_type const&>
    {
        friend struct boost::unordered::iterator_detail::iterator<Node>;

#if !defined(BOOST_NO_MEMBER_TEMPLATE_FRIENDS)
        template <typename>
        friend struct boost::unordered::detail::table;
        template <typename>
        friend struct boost::unordered::detail::table_impl;
        template <typename>
        friend struct boost::unordered::detail::grouped_table_impl;

    private:
#endif
        typedef typename Node::node_pointer node_pointer;
        typedef boost::unordered::iterator_detail::iterator<Node> iterator;
        node_pointer node_;

    public:

        typedef typename Node::value_type value_type;

        c_iterator() BOOST_NOEXCEPT : node_() {}

        explicit c_iterator(typename Node::link_pointer x) BOOST_NOEXCEPT :
            node_(static_cast<node_pointer>(x)) {}

        c_iterator(iterator const& x) BOOST_NOEXCEPT : node_(x.node_) {}

        value_type const& operator*() const {
            return node_->value();
        }

        value_type const* operator->() const {
            return &node_->value();
        }

        c_iterator& operator++() {
            node_ = static_cast<node_pointer>(node_->next_);
            return *this;
        }

        c_iterator operator++(int) {
            c_iterator tmp(node_);
            node_ = static_cast<node_pointer>(node_->next_);
            return tmp;
        }

        friend bool operator==(c_iterator const& x, c_iterator const& y)
            BOOST_NOEXCEPT
        {
            return x.node_ == y.node_;
        }

        friend bool operator!=(c_iterator const& x, c_iterator const& y)
            BOOST_NOEXCEPT
        {
            return x.node_ != y.node_;
        }
    };
}}}

namespace boost { namespace unordered { namespace detail {

    ///////////////////////////////////////////////////////////////////
    //
    // Node construction

    template <typename NodeAlloc>
    struct node_constructor
    {
    private:

        typedef NodeAlloc node_allocator;
        typedef boost::unordered::detail::allocator_traits<NodeAlloc>
            node_allocator_traits;
        typedef typename node_allocator_traits::value_type node;
        typedef typename node_allocator_traits::pointer node_pointer;
        typedef typename node::value_type value_type;

    protected:

        node_allocator& alloc_;
        node_pointer node_;
        bool node_constructed_;
        bool value_constructed_;

    public:

        node_constructor(node_allocator& n) :
            alloc_(n),
            node_(),
            node_constructed_(false),
            value_constructed_(false)
        {
        }

        ~node_constructor();

        void construct();

        template <BOOST_UNORDERED_EMPLACE_TEMPLATE>
        void construct_with_value(BOOST_UNORDERED_EMPLACE_ARGS)
        {
            construct();
            boost::unordered::detail::func::construct_value_impl(
                alloc_, node_->value_ptr(), BOOST_UNORDERED_EMPLACE_FORWARD);
            value_constructed_ = true;
        }

        template <typename A0>
        void construct_with_value2(BOOST_FWD_REF(A0) a0)
        {
            construct();
            boost::unordered::detail::func::construct_value_impl(
                alloc_, node_->value_ptr(),
                BOOST_UNORDERED_EMPLACE_ARGS1(boost::forward<A0>(a0)));
            value_constructed_ = true;
        }

        value_type const& value() const {
            BOOST_ASSERT(node_ && node_constructed_ && value_constructed_);
            return node_->value();
        }

        // no throw
        node_pointer release()
        {
            BOOST_ASSERT(node_ && node_constructed_);
            node_pointer p = node_;
            node_ = node_pointer();
            return p;
        }

    private:
        node_constructor(node_constructor const&);
        node_constructor& operator=(node_constructor const&);
    };

    template <typename Alloc>
    node_constructor<Alloc>::~node_constructor()
    {
        if (node_) {
            if (value_constructed_) {
                boost::unordered::detail::func::destroy_value_impl(alloc_,
                    node_->value_ptr());
            }

            if (node_constructed_) {
                node_allocator_traits::destroy(alloc_,
                    boost::addressof(*node_));
            }

            node_allocator_traits::deallocate(alloc_, node_, 1);
        }
    }

    template <typename Alloc>
    void node_constructor<Alloc>::construct()
    {
        if(!node_) {
            node_constructed_ = false;
            value_constructed_ = false;

            node_ = node_allocator_traits::allocate(alloc_, 1);

            node_allocator_traits::construct(alloc_,
                boost::addressof(*node_), node());
            node_->init(node_);
            node_constructed_ = true;
        }
        else {
            BOOST_ASSERT(node_constructed_);

            if (value_constructed_)
            {
                boost::unordered::detail::func::destroy_value_impl(alloc_,
                    node_->value_ptr());
                value_constructed_ = false;
            }
        }
    }

    ///////////////////////////////////////////////////////////////////
    //
    // Node Holder
    //
    // Temporary store for nodes. Deletes any that aren't used.

    template <typename NodeAlloc>
    struct node_holder : private node_constructor<NodeAlloc>
    {
    private:
        typedef node_constructor<NodeAlloc> base;

        typedef NodeAlloc node_allocator;
        typedef boost::unordered::detail::allocator_traits<NodeAlloc>
            node_allocator_traits;
        typedef typename node_allocator_traits::value_type node;
        typedef typename node_allocator_traits::pointer node_pointer;
        typedef typename node::value_type value_type;
        typedef typename node::link_pointer link_pointer;
        typedef boost::unordered::iterator_detail::iterator<node> iterator;

        node_pointer nodes_;

    public:

        template <typename Table>
        explicit node_holder(Table& b) :
            base(b.node_alloc()),
            nodes_()
        {
            if (b.size_) {
                typename Table::link_pointer prev = b.get_previous_start();
                nodes_ = static_cast<node_pointer>(prev->next_);
                prev->next_ = link_pointer();
                b.size_ = 0;
            }
        }

        ~node_holder();

        void node_for_assignment()
        {
            if (!this->node_ && nodes_) {
                this->node_ = nodes_;
                nodes_ = static_cast<node_pointer>(nodes_->next_);
                this->node_->init(this->node_);
                this->node_->next_ = link_pointer();

                this->node_constructed_ = true;
                this->value_constructed_ = true;
            }
        }

        template <typename T>
        inline void assign_impl(T const& v) {
            if (this->node_ && this->value_constructed_) {
                this->node_->value() = v;
            }
            else {
                this->construct_with_value2(v);
            }
        }

        template <typename T1, typename T2>
        inline void assign_impl(std::pair<T1 const, T2> const& v) {
            this->construct_with_value2(v);
        }

        template <typename T>
        inline void move_assign_impl(T& v) {
            if (this->node_ && this->value_constructed_) {
                this->node_->value() = boost::move(v);
            }
            else {
                this->construct_with_value2(boost::move(v));
            }
        }

        template <typename T1, typename T2>
        inline void move_assign_impl(std::pair<T1 const, T2>& v) {
            this->construct_with_value2(boost::move(v));
        }

        node_pointer copy_of(value_type const& v)
        {
            node_for_assignment();
            assign_impl(v);
            return base::release();
        }

        node_pointer move_copy_of(value_type& v)
        {
            node_for_assignment();
            move_assign_impl(v);
            return base::release();
        }

        iterator begin() const
        {
            return iterator(nodes_);
        }
    };

    template <typename Alloc>
    node_holder<Alloc>::~node_holder()
    {
        while (nodes_) {
            node_pointer p = nodes_;
            nodes_ = static_cast<node_pointer>(p->next_);

            boost::unordered::detail::func::destroy_value_impl(this->alloc_,
                p->value_ptr());
            node_allocator_traits::destroy(this->alloc_, boost::addressof(*p));
            node_allocator_traits::deallocate(this->alloc_, p, 1);
        }
    }

    ///////////////////////////////////////////////////////////////////
    //
    // Bucket

    template <typename NodePointer>
    struct bucket
    {
        typedef NodePointer link_pointer;
        link_pointer next_;

        bucket() : next_() {}

        link_pointer first_from_start()
        {
            return next_;
        }

        enum { extra_node = true };
    };

    struct ptr_bucket
    {
        typedef ptr_bucket* link_pointer;
        link_pointer next_;

        ptr_bucket() : next_(0) {}

        link_pointer first_from_start()
        {
            return this;
        }

        enum { extra_node = false };
    };

    ///////////////////////////////////////////////////////////////////
    //
    // Hash Policy

    template <typename SizeT>
    struct prime_policy
    {
        template <typename Hash, typename T>
        static inline SizeT apply_hash(Hash const& hf, T const& x) {
            return hf(x);
        }

        static inline SizeT to_bucket(SizeT bucket_count, SizeT hash) {
            return hash % bucket_count;
        }

        static inline SizeT new_bucket_count(SizeT min) {
            return boost::unordered::detail::next_prime(min);
        }

        static inline SizeT prev_bucket_count(SizeT max) {
            return boost::unordered::detail::prev_prime(max);
        }
    };

    template <typename SizeT>
    struct mix64_policy
    {
        template <typename Hash, typename T>
        static inline SizeT apply_hash(Hash const& hf, T const& x) {
            SizeT key = hf(x);
            key = (~key) + (key << 21); // key = (key << 21) - key - 1;
            key = key ^ (key >> 24);
            key = (key + (key << 3)) + (key << 8); // key * 265
            key = key ^ (key >> 14);
            key = (key + (key << 2)) + (key << 4); // key * 21
            key = key ^ (key >> 28);
            key = key + (key << 31);
            return key;
        }

        static inline SizeT to_bucket(SizeT bucket_count, SizeT hash) {
            return hash & (bucket_count - 1);
        }

        static inline SizeT new_bucket_count(SizeT min) {
            if (min <= 4) return 4;
            --min;
            min |= min >> 1;
            min |= min >> 2;
            min |= min >> 4;
            min |= min >> 8;
            min |= min >> 16;
            min |= min >> 32;
            return min + 1;
        }

        static inline SizeT prev_bucket_count(SizeT max) {
            max |= max >> 1;
            max |= max >> 2;
            max |= max >> 4;
            max |= max >> 8;
            max |= max >> 16;
            max |= max >> 32;
            return (max >> 1) + 1;
        }
    };

    template <int digits, int radix>
    struct pick_policy_impl {
        typedef prime_policy<std::size_t> type;
    };

    template <>
    struct pick_policy_impl<64, 2> {
        typedef mix64_policy<std::size_t> type;
    };

    struct pick_policy :
        pick_policy_impl<
            std::numeric_limits<std::size_t>::digits,
            std::numeric_limits<std::size_t>::radix> {};

    ////////////////////////////////////////////////////////////////////////////
    // Functions

    // Assigning and swapping the equality and hash function objects
    // needs strong exception safety. To implement that normally we'd
    // require one of them to be known to not throw and the other to
    // guarantee strong exception safety. Unfortunately they both only
    // have basic exception safety. So to acheive strong exception
    // safety we have storage space for two copies, and assign the new
    // copies to the unused space. Then switch to using that to use
    // them. This is implemented in 'set_hash_functions' which
    // atomically assigns the new function objects in a strongly
    // exception safe manner.

    template <class H, class P, bool NoThrowMoveAssign>
    class set_hash_functions;

    template <class H, class P>
    class functions
    {
    public:
        static const bool nothrow_move_assignable =
                boost::is_nothrow_move_assignable<H>::value &&
                boost::is_nothrow_move_assignable<P>::value;
        static const bool nothrow_move_constructible =
                boost::is_nothrow_move_constructible<H>::value &&
                boost::is_nothrow_move_constructible<P>::value;

    private:
        friend class boost::unordered::detail::set_hash_functions<H, P,
               nothrow_move_assignable>;
        functions& operator=(functions const&);

        typedef compressed<H, P> function_pair;

        typedef typename boost::aligned_storage<
            sizeof(function_pair),
            boost::alignment_of<function_pair>::value>::type aligned_function;

        bool current_; // The currently active functions.
        aligned_function funcs_[2];

        function_pair const& current() const {
            return *static_cast<function_pair const*>(
                static_cast<void const*>(&funcs_[current_]));
        }

        function_pair& current() {
            return *static_cast<function_pair*>(
                static_cast<void*>(&funcs_[current_]));
        }

        void construct(bool which, H const& hf, P const& eq)
        {
            new((void*) &funcs_[which]) function_pair(hf, eq);
        }

        void construct(bool which, function_pair const& f,
                boost::unordered::detail::false_type =
                    boost::unordered::detail::false_type())
        {
            new((void*) &funcs_[which]) function_pair(f);
        }
        
        void construct(bool which, function_pair& f,
                boost::unordered::detail::true_type)
        {
            new((void*) &funcs_[which]) function_pair(f,
                boost::unordered::detail::move_tag());
        }

        void destroy(bool which)
        {
            boost::unordered::detail::func::destroy((function_pair*)(&funcs_[which]));
        }
        
    public:

        typedef boost::unordered::detail::set_hash_functions<H, P,
                nothrow_move_assignable> set_hash_functions;

        functions(H const& hf, P const& eq)
            : current_(false)
        {
            construct(current_, hf, eq);
        }

        functions(functions const& bf)
            : current_(false)
        {
            construct(current_, bf.current());
        }

        functions(functions& bf, boost::unordered::detail::move_tag)
            : current_(false)
        {
            construct(current_, bf.current(),
                boost::unordered::detail::integral_constant<bool,
                    nothrow_move_constructible>());
        }

        ~functions() {
            this->destroy(current_);
        }

        H const& hash_function() const {
            return current().first();
        }

        P const& key_eq() const {
            return current().second();
        }
    };

    template <class H, class P>
    class set_hash_functions<H, P, false>
    {
        set_hash_functions(set_hash_functions const&);
        set_hash_functions& operator=(set_hash_functions const&);

        typedef functions<H, P> functions_type;
    
        functions_type& functions_;
        bool tmp_functions_;

    public:

        set_hash_functions(functions_type& f, H const& h, P const& p)
          : functions_(f),
            tmp_functions_(!f.current_)
        {
            f.construct(tmp_functions_, h, p);
        }

        set_hash_functions(functions_type& f, functions_type const& other)
          : functions_(f),
            tmp_functions_(!f.current_)
        {
            f.construct(tmp_functions_, other.current());
        }

        ~set_hash_functions()
        {
            functions_.destroy(tmp_functions_);
        }

        void commit()
        {
            functions_.current_ = tmp_functions_;
            tmp_functions_ = !tmp_functions_;
        }
    };

    template <class H, class P>
    class set_hash_functions<H, P, true>
    {
        set_hash_functions(set_hash_functions const&);
        set_hash_functions& operator=(set_hash_functions const&);

        typedef functions<H, P> functions_type;

        functions_type& functions_;
        H hash_;
        P pred_;
    
    public:

        set_hash_functions(functions_type& f, H const& h, P const& p) :
            functions_(f),
            hash_(h),
            pred_(p) {}

        set_hash_functions(functions_type& f, functions_type const& other) :
            functions_(f),
            hash_(other.hash_function()),
            pred_(other.key_eq()) {}

        void commit()
        {
            functions_.current().first() = boost::move(hash_);
            functions_.current().second() = boost::move(pred_);
        }
    };
    
    ////////////////////////////////////////////////////////////////////////////
    // rvalue parameters when type can't be a BOOST_RV_REF(T) parameter
    // e.g. for int

#if !defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
#   define BOOST_UNORDERED_RV_REF(T) BOOST_RV_REF(T)
#else
    struct please_ignore_this_overload {
        typedef please_ignore_this_overload type;
    };

    template <typename T>
    struct rv_ref_impl {
        typedef BOOST_RV_REF(T) type;
    };

    template <typename T>
    struct rv_ref :
        boost::detail::if_true<
            boost::is_class<T>::value
        >::BOOST_NESTED_TEMPLATE then <
            boost::unordered::detail::rv_ref_impl<T>,
            please_ignore_this_overload
        >::type
    {};

#   define BOOST_UNORDERED_RV_REF(T) \
        typename boost::unordered::detail::rv_ref<T>::type
#endif
}}}

#endif
