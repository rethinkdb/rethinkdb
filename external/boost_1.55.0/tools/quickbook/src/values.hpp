/*=============================================================================
    Copyright (c) 2010-2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

// An easy way to store data parsed for quickbook.

#if !defined(BOOST_SPIRIT_QUICKBOOK_VALUES_HPP)
#define BOOST_SPIRIT_QUICKBOOK_VALUES_HPP

#include <utility>
#include <string>
#include <cassert>
#include <boost/scoped_ptr.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/utility/string_ref.hpp>
#include <stdexcept>
#include "fwd.hpp"
#include "files.hpp"

namespace quickbook
{
    struct value;
    struct value_builder;
    struct value_error;

    namespace detail
    {
        ////////////////////////////////////////////////////////////////////////
        // Node
    
        struct value_node
        {
        private:
            value_node(value_node const&);
            value_node& operator=(value_node const&);

        public:
            typedef int tag_type;

        protected:
            explicit value_node(tag_type);
            virtual ~value_node();

        public:
            virtual char const* type_name() const = 0;
            virtual value_node* clone() const = 0;

            virtual file_ptr get_file() const;
            virtual string_iterator get_position() const;
            virtual boost::string_ref get_quickbook() const;
            virtual std::string get_encoded() const;
            virtual int get_int() const;

            virtual bool check() const;
            virtual bool empty() const;
            virtual bool is_encoded() const;
            virtual bool is_list() const;
            virtual bool equals(value_node*) const;

            virtual value_node* get_list() const;
            
            int ref_count_;
            const tag_type tag_;
            value_node* next_;

            friend void intrusive_ptr_add_ref(value_node* ptr)
                { ++ptr->ref_count_; }
            friend void intrusive_ptr_release(value_node* ptr)
                { if(--ptr->ref_count_ == 0) delete ptr; }
        };

        ////////////////////////////////////////////////////////////////////////
        // Value base
        //
        // This defines most of the public methods for value.
        // 'begin' and 'end' are defined with the iterators later.
    
        struct value_base
        {
        public:
            struct iterator;

            typedef iterator const_iterator;
            typedef value_node::tag_type tag_type;
            enum { default_tag = 0 };

        protected:
            explicit value_base(value_node* base)
                : value_(base)
            {
                assert(value_);
            }

            ~value_base() {}

            void swap(value_base& x) { std::swap(value_, x.value_); }
        public:
            bool check() const { return value_->check(); }
            bool empty() const { return value_->empty(); }
            bool is_encoded() const { return value_->is_encoded(); }
            bool is_list() const { return value_->is_list(); }

            iterator begin() const;
            iterator end() const;

            // Item accessors
            int get_tag() const { return value_->tag_; }
            file_ptr get_file() const
            { return value_->get_file(); }
            string_iterator get_position() const
            { return value_->get_position(); }
            boost::string_ref get_quickbook() const
            { return value_->get_quickbook(); }
            std::string get_encoded() const
            { return value_->get_encoded(); }
            int get_int() const
            { return value_->get_int(); }

            // Equality is pretty inefficient. Not really designed for anything
            // more than testing purposes.
            friend bool operator==(value_base const& x, value_base const& y)
            { return x.value_->equals(y.value_); }

        protected:
            value_node* value_;

            // value_builder needs to access 'value_' to get the node
            // from a value.
            friend struct quickbook::value_builder;
        };
        
        ////////////////////////////////////////////////////////////////////////
        // Reference and proxy values for use in iterators

        struct value_ref : public value_base
        {
        public:
            explicit value_ref(value_node* base) : value_base(base) {}
        };
        
        struct value_proxy : public value_base
        {
        public:
            explicit value_proxy(value_node* base) : value_base(base) {}
            value_proxy* operator->() { return this; }
            value_ref operator*() const { return value_ref(value_); }
        };
    
        ////////////////////////////////////////////////////////////////////////
        // Iterators

        struct value_base::iterator
            : public boost::forward_iterator_helper<
                iterator, value, int, value_proxy, value_ref>
        {
        public:
            iterator();
            explicit iterator(value_node* p) : ptr_(p) {}
            friend bool operator==(iterator x, iterator y)
                { return x.ptr_ == y.ptr_; }
            iterator& operator++() { ptr_ = ptr_->next_; return *this; }
            value_ref operator*() const { return value_ref(ptr_); }
            value_proxy operator->() const { return value_proxy(ptr_); }
        private:
            value_node* ptr_;
        };

        inline value_base::iterator value_base::begin() const
        {
            return iterator(value_->get_list());
        }

        inline value_base::iterator value_base::end() const
        {
            return iterator();
        }

        ////////////////////////////////////////////////////////////////////////
        // Reference counting for values

        struct value_counted : public value_base
        {
            value_counted& operator=(value_counted const&);
        protected:
            value_counted();
            value_counted(value_counted const&);
            value_counted(value_base const&);
            value_counted(value_node*);
            ~value_counted();
        };

        ////////////////////////////////////////////////////////////////////////
        // List builder
        //
        // Values are immutable, so this class is used to build a list of
        // value nodes before constructing the value.

        struct value_list_builder {
            value_list_builder(value_list_builder const&);
            value_list_builder& operator=(value_list_builder const&);
        public:
            value_list_builder();
            value_list_builder(value_node*);
            ~value_list_builder();
            void swap(value_list_builder& b);
            value_node* release();

            void append(value_node*);
            void sort();

            bool empty() const;
        private:
            value_node* head_;
            value_node** back_;
        };
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Value
    //
    // Most of the methods are in value_base.

    struct value : public detail::value_counted
    {
    public:
        value();
        value(value const&);
        value(detail::value_base const&);
        explicit value(detail::value_node*);
        value& operator=(value);
        void swap(value& x) { detail::value_counted::swap(x); }
    };
    
    // Empty
    value empty_value(value::tag_type = value::default_tag);

    // Integers
    value int_value(int, value::tag_type = value::default_tag);

    // String types

    // Quickbook strings contain a reference to the original quickbook source.
    value qbk_value(file_ptr const&, string_iterator, string_iterator,
            value::tag_type = value::default_tag);

    // Encoded strings are either plain text or boostbook.
    value encoded_value(std::string const&,
            value::tag_type = value::default_tag);

    // An encoded quickbook string is an encoded string that contains a
    // reference to the quickbook source it was generated from.
    value encoded_qbk_value(file_ptr const&, string_iterator, string_iterator,
            std::string const&, value::tag_type = value::default_tag);

    ////////////////////////////////////////////////////////////////////////////
    // Value Builder
    //
    // Used to incrementally build a valueeter tree.

    struct value_builder {
    public:
        value_builder();
        void swap(value_builder& b);
        
        void save();
        void restore();

        value release();

        void insert(value const&);
        void extend(value const&);

        void start_list(value::tag_type = value::default_tag);
        void finish_list();
        void clear_list();
        void sort_list();

        bool empty() const;

    private:
        detail::value_list_builder current;
        value::tag_type list_tag;
        boost::scoped_ptr<value_builder> saved;
    };

    ////////////////////////////////////////////////////////////////////////////
    // Value Error
    //
    
    struct value_error : public std::logic_error
    {
    public:
        explicit value_error(std::string const&);
    };

    ////////////////////////////////////////////////////////////////////////////
    // Value Consumer
    //
    // Convenience class for unpacking value values.

    struct value_consumer {
    public:
        struct iterator
            : public boost::input_iterator_helper<iterator,
                boost::iterator_value<value::iterator>::type,
                boost::iterator_difference<value::iterator>::type,
                boost::iterator_pointer<value::iterator>::type,
                boost::iterator_reference<value::iterator>::type>
        {
        public:
            iterator();
            explicit iterator(value::iterator* p) : ptr_(p) {}
            friend bool operator==(iterator x, iterator y)
                { return *x.ptr_ == *y.ptr_; }
            iterator& operator++() { ++*ptr_; return *this; }
            reference operator*() const { return **ptr_; }
            pointer operator->() const { return ptr_->operator->(); }
        private:
            value::iterator* ptr_;
        };

        typedef iterator const_iterator;
        typedef iterator::reference reference;
    
        value_consumer(value const& x)
            : list_(x)
            , pos_(x.begin())
            , end_(x.end())
        {}

        value_consumer(reference x)
            : list_(x)
            , pos_(x.begin())
            , end_(x.end())
        {}

        reference consume()
        {
            assert_check();
            return *pos_++;
        }

        reference consume(value::tag_type t)
        {
            assert_check(t);
            return *pos_++;
        }

        value optional_consume()
        {
            if(check()) {
                return *pos_++;
            }
            else {
                return value();
            }
        }

        value optional_consume(value::tag_type t)
        {
            if(check(t)) {
                return *pos_++;
            }
            else {
                return value();
            }
        }

        bool check() const
        {
            return pos_ != end_;
        }

        bool check(value::tag_type t) const
        {
            return pos_ != end_ && t == pos_->get_tag();
        }
        
        void finish() const
        {
            if (pos_ != end_)
            throw value_error("Not all values handled.");
        }

        iterator begin() { return iterator(&pos_); }
        iterator end() { return iterator(&end_); }
    private:

    void assert_check() const
    {
        if (pos_ == end_)
        throw value_error("Attempt to read past end of value list.");
    }

    void assert_check(value::tag_type t) const
    {
        assert_check();
        if (t != pos_->get_tag())
        throw value_error("Incorrect value tag.");
    }

        value list_;
        value::iterator pos_, end_;
    };
}

#endif
