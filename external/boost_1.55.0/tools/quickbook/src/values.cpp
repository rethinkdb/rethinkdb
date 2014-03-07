/*=============================================================================
    Copyright (c) 2010-2011 Daniel James

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/

#include "values.hpp"
#include "files.hpp"
#include <boost/current_function.hpp>
#include <boost/lexical_cast.hpp>

#define UNDEFINED_ERROR() \
    throw value_undefined_method( \
        std::string(BOOST_CURRENT_FUNCTION) +\
        " not defined for " + \
        this->type_name() + \
        " values." \
        );

namespace quickbook
{
    ////////////////////////////////////////////////////////////////////////////
    // Value Error
    
    struct value_undefined_method : value_error
    {
        value_undefined_method(std::string const&);
    };
    
    value_error::value_error(std::string const& x)
        : std::logic_error(x) {}

    value_undefined_method::value_undefined_method(std::string const& x)
        : value_error(x) {}

    ////////////////////////////////////////////////////////////////////////////
    // Node

    namespace detail
    {
        value_node::value_node(tag_type t)
            : ref_count_(0), tag_(t), next_() {
        }        
    
        value_node::~value_node() {
        }
        
        file_ptr value_node::get_file() const { UNDEFINED_ERROR(); }
        string_iterator value_node::get_position() const { UNDEFINED_ERROR(); }
        int value_node::get_int() const { UNDEFINED_ERROR(); }
        boost::string_ref value_node::get_quickbook() const { UNDEFINED_ERROR(); }
        std::string value_node::get_encoded() const { UNDEFINED_ERROR(); }
        value_node* value_node::get_list() const { UNDEFINED_ERROR(); }

        bool value_node::empty() const { return false; }
        bool value_node::check() const { return true; }
        bool value_node::is_list() const { return false; }
        bool value_node::is_encoded() const { return false; }
        bool value_node::equals(value_node*) const { UNDEFINED_ERROR(); }
    }

    ////////////////////////////////////////////////////////////////////////////
    // List end value
    //
    // A special value for marking the end of lists.

    namespace detail
    {
        struct value_list_end_impl : public value_node
        {
            static value_list_end_impl instance;
        private:
            value_list_end_impl()
                : value_node(value::default_tag)
            {
                intrusive_ptr_add_ref(&instance);
                next_ = this;
            }

            virtual char const* type_name() const { return "list end"; }
            virtual value_node* clone() const { UNDEFINED_ERROR(); }

            virtual bool equals(value_node* other) const
            { return this == other; }

            bool empty() const { UNDEFINED_ERROR(); }
            bool check() const { UNDEFINED_ERROR(); }
            bool is_list() const { UNDEFINED_ERROR(); }
            bool is_encoded() const { UNDEFINED_ERROR(); }
        };

        value_list_end_impl value_list_end_impl::instance;
    }
        
    ////////////////////////////////////////////////////////////////////////////
    // Empty/nil values
    //
    // (nil is just a special case of empty, don't be mislead by the name
    //  the type is not important).

    namespace detail
    {
        struct empty_value_impl : public value_node
        {
            static value_node* new_(value::tag_type t);

        protected:
            explicit empty_value_impl(value::tag_type t)
                : value_node(t) {}

        private:
            char const* type_name() const { return "empty"; }
        
            virtual value_node* clone() const
                { return new empty_value_impl(tag_); }

            virtual bool empty() const
                { return true; }

            virtual bool check() const
                { return false; }

            virtual bool equals(value_node* other) const
                { return !other->check(); }
            
            friend value quickbook::empty_value(value::tag_type);
        };
    
        struct value_nil_impl : public empty_value_impl
        {
            static value_nil_impl instance;
        private:
            value_nil_impl()
                : empty_value_impl(value::default_tag)
            {
                intrusive_ptr_add_ref(&instance);
                next_ = &value_list_end_impl::instance;
            }
        };

        value_nil_impl value_nil_impl::instance;

        value_node* empty_value_impl::new_(value::tag_type t) {
            // The return value from this function is always placed in an
            // intrusive_ptr which will manage the memory correctly.
            // Note that value_nil_impl increments its reference count
            // in its constructor, so that it will never be deleted by the
            // intrusive pointer.

            if (t == value::default_tag)
                return &value_nil_impl::instance;
            else
                return new empty_value_impl(t);
        }
    }

    value empty_value(value::tag_type t)
    {
        return value(detail::empty_value_impl::new_(t));
    }

    ////////////////////////////////////////////////////////////////////////////
    // value_counted

    namespace detail
    {
        value_counted::value_counted()
            : value_base(&value_nil_impl::instance)
        {
            // Even though empty is not on the heap, its reference
            // counter needs to be incremented so that the destructor
            // doesn't try to delete it.

            intrusive_ptr_add_ref(value_);
        }
    
        value_counted::value_counted(value_counted const& x)
            : value_base(x)
        {
            intrusive_ptr_add_ref(value_);
        }
    
        value_counted::value_counted(value_base const& x)
            : value_base(x)
        {
            intrusive_ptr_add_ref(value_);
        }
    
        value_counted::value_counted(value_node* x)
            : value_base(x)
        {
            intrusive_ptr_add_ref(value_);
        }
    
        value_counted::~value_counted()
        {
            intrusive_ptr_release(value_);
        }
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // value

    value::value()
        : detail::value_counted()
    {
    }

    value::value(value const& x)
        : detail::value_counted(x)
    {
    }

    value::value(detail::value_base const& x)
        : detail::value_counted(x)
    {
    }

    value::value(detail::value_node* x)
        : detail::value_counted(x)
    {
    }

    value& value::operator=(value x)
    {
        swap(x);
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////
    // Integers

    namespace detail
    {
        struct int_value_impl : public value_node
        {
        public:
            explicit int_value_impl(int, value::tag_type);
        private:
            char const* type_name() const { return "integer"; }
            virtual value_node* clone() const;
            virtual int get_int() const;
            virtual std::string get_encoded() const;
            virtual bool empty() const;
            virtual bool is_encoded() const;
            virtual bool equals(value_node*) const;

            int value_;
        };

        int_value_impl::int_value_impl(int v, value::tag_type t)
            : value_node(t)
            , value_(v)
        {}

        value_node* int_value_impl::clone() const
        {
            return new int_value_impl(value_, tag_);
        }

        int int_value_impl::get_int() const
        {
            return value_;
        }

        std::string int_value_impl::get_encoded() const
        {
            return boost::lexical_cast<std::string>(value_);
        }

        bool int_value_impl::empty() const
        {
            return false;
        }

        bool int_value_impl::is_encoded() const
        {
            return true;
        }

        bool int_value_impl::equals(value_node* other) const {
            try {
                return value_ == other->get_int();
            }
            catch(value_undefined_method&) {
                return false;
            }
        }
    }

    value int_value(int v, value::tag_type t)
    {
        return value(new detail::int_value_impl(v, t));
    }

    ////////////////////////////////////////////////////////////////////////////
    // Strings

    namespace detail
    {
        struct encoded_value_impl : public value_node
        {
        public:
            explicit encoded_value_impl(std::string const&, value::tag_type);
        private:
            char const* type_name() const { return "encoded text"; }

            virtual ~encoded_value_impl();
            virtual value_node* clone() const;
            virtual std::string get_encoded() const;
            virtual bool empty() const;
            virtual bool is_encoded() const;
            virtual bool equals(value_node*) const;

            std::string value_;
        };
    
        struct qbk_value_impl : public value_node
        {
        public:
            explicit qbk_value_impl(
                    file_ptr const&,
                    string_iterator begin,
                    string_iterator end,
                    value::tag_type);
        private:
            char const* type_name() const { return "quickbook"; }

            virtual ~qbk_value_impl();
            virtual value_node* clone() const;
            virtual file_ptr get_file() const;
            virtual string_iterator get_position() const;
            virtual boost::string_ref get_quickbook() const;
            virtual bool empty() const;
            virtual bool equals(value_node*) const;

            file_ptr file_;
            string_iterator begin_;
            string_iterator end_;
        };
    
        struct encoded_qbk_value_impl : public value_node
        {
        private:
            char const* type_name() const { return "encoded text with quickbook reference"; }

            encoded_qbk_value_impl(file_ptr const&,
                    string_iterator, string_iterator,
                    std::string const&, value::tag_type);
    
            virtual ~encoded_qbk_value_impl();
            virtual value_node* clone() const;
            virtual file_ptr get_file() const;
            virtual string_iterator get_position() const;
            virtual boost::string_ref get_quickbook() const;
            virtual std::string get_encoded() const;
            virtual bool empty() const;
            virtual bool is_encoded() const;
            virtual bool equals(value_node*) const;

            file_ptr file_;
            string_iterator begin_;
            string_iterator end_;
            std::string encoded_value_;
            
            friend quickbook::value quickbook::encoded_qbk_value(
                    file_ptr const&, string_iterator, string_iterator,
                    std::string const&, quickbook::value::tag_type);
        };

        // encoded_value_impl
    
        encoded_value_impl::encoded_value_impl(
                std::string const& val,
                value::tag_type tag
            )
            : value_node(tag), value_(val)
        {
        }
    
        encoded_value_impl::~encoded_value_impl()
        {
        }

        value_node* encoded_value_impl::clone() const
        {
            return new encoded_value_impl(value_, tag_);
        }
    
        std::string encoded_value_impl::get_encoded() const
            { return value_; }

        bool encoded_value_impl::empty() const
            { return value_.empty(); }

        bool encoded_value_impl::is_encoded() const
            { return true; }

        bool encoded_value_impl::equals(value_node* other) const {
            try {
                return value_ == other->get_encoded();
            }
            catch(value_undefined_method&) {
                return false;
            }
        }

        // qbk_value_impl
    
        qbk_value_impl::qbk_value_impl(
                file_ptr const& f,
                string_iterator begin,
                string_iterator end,
                value::tag_type tag
            ) : value_node(tag), file_(f), begin_(begin), end_(end)
        {
        }
    
        qbk_value_impl::~qbk_value_impl()
        {
        }
    
        value_node* qbk_value_impl::clone() const
        {
            return new qbk_value_impl(file_, begin_, end_, tag_);
        }

        file_ptr qbk_value_impl::get_file() const
            { return file_; }

        string_iterator qbk_value_impl::get_position() const
            { return begin_; }

        boost::string_ref qbk_value_impl::get_quickbook() const
            { return boost::string_ref(begin_, end_ - begin_); }

        bool qbk_value_impl::empty() const
            { return begin_ == end_; }
    
        bool qbk_value_impl::equals(value_node* other) const {
            try {
                return this->get_quickbook() == other->get_quickbook();
            }
            catch(value_undefined_method&) {
                return false;
            }
        }

        // encoded_qbk_value_impl
    
        encoded_qbk_value_impl::encoded_qbk_value_impl(
                file_ptr const& f,
                string_iterator begin,
                string_iterator end,
                std::string const& encoded,
                value::tag_type tag)
            : value_node(tag)
            , file_(f)
            , begin_(begin)
            , end_(end)
            , encoded_value_(encoded)
            
        {
        }
    
        encoded_qbk_value_impl::~encoded_qbk_value_impl()
        {
        }

        value_node* encoded_qbk_value_impl::clone() const
        {
            return new encoded_qbk_value_impl(
                    file_, begin_, end_, encoded_value_, tag_);
        }

        file_ptr encoded_qbk_value_impl::get_file() const
            { return file_; }

        string_iterator encoded_qbk_value_impl::get_position() const
            { return begin_; }

        boost::string_ref encoded_qbk_value_impl::get_quickbook() const
            { return boost::string_ref(begin_, end_ - begin_); }

        std::string encoded_qbk_value_impl::get_encoded() const
            { return encoded_value_; }

        // Should this test the quickbook, the boostbook or both?
        bool encoded_qbk_value_impl::empty() const
            { return encoded_value_.empty(); }

        bool encoded_qbk_value_impl::is_encoded() const
            { return true; }

        bool encoded_qbk_value_impl::equals(value_node* other) const {
            try {
                return this->get_quickbook() == other->get_quickbook();
            }
            catch(value_undefined_method&) {}

            try {
                return this->get_encoded() == other->get_encoded();
            }
            catch(value_undefined_method&) {}

            return false;
        }
    }

    value qbk_value(file_ptr const& f, string_iterator x, string_iterator y, value::tag_type t)
    {
        return value(new detail::qbk_value_impl(f, x, y, t));
    }

    value encoded_value(std::string const& x, value::tag_type t)
    {
        return value(new detail::encoded_value_impl(x, t));
    }

    value encoded_qbk_value(
            file_ptr const& f, string_iterator x, string_iterator y,
            std::string const& z, value::tag_type t)
    {
        return value(new detail::encoded_qbk_value_impl(f,x,y,z,t));
    }

    //////////////////////////////////////////////////////////////////////////
    // List methods
    
    namespace detail
    {
    namespace {
        value_node** list_ref_back(value_node**);
        void list_ref(value_node*);
        void list_unref(value_node*);
        value_node** merge_sort(value_node**);
        value_node** merge_sort(value_node**, int);
        value_node** merge(value_node**, value_node**, value_node**);
        void rotate(value_node**, value_node**, value_node**);

        value_node** list_ref_back(value_node** back)
        {
            while(*back != &value_list_end_impl::instance) {
                intrusive_ptr_add_ref(*back);
                back = &(*back)->next_;
            }
            
            return back;
        }

        void list_ref(value_node* ptr)
        {
            while(ptr != &value_list_end_impl::instance) {
                intrusive_ptr_add_ref(ptr);
                ptr = ptr->next_;
            }
        }

        void list_unref(value_node* ptr)
        {
            while(ptr != &value_list_end_impl::instance) {
                value_node* next = ptr->next_;
                intrusive_ptr_release(ptr);
                ptr = next;
            }
        }

        value_node** merge_sort(value_node** l)
        {
            if(*l == &value_list_end_impl::instance)
                return l;
            else
                return merge_sort(l, 9999);
        }

        value_node** merge_sort(value_node** l, int recurse_limit)
        {
            value_node** p = &(*l)->next_;
            for(int count = 0;
                count < recurse_limit && *p != &value_list_end_impl::instance;
                ++count)
            {
                p = merge(l, p, merge_sort(p, count));
            }
            return p;
        }
        
        value_node** merge(
                value_node** first, value_node** second, value_node** third)
        {
            for(;;) {
                for(;;) {
                    if(first == second) return third;
                    if((*second)->tag_ < (*first)->tag_) break;
                    first = &(*first)->next_;
                }
    
                rotate(first, second, third);
                first = &(*first)->next_;
                
                // Since the two ranges were just swapped, the order is now:
                // first...third...second
                //
                // Also, that since the second section of the list was
                // originally before the first, if the heads are equal
                // we need to swap to maintain the order.
                
                for(;;) {
                    if(first == third) return second;
                    if((*third)->tag_ <= (*first)->tag_) break;
                    first = &(*first)->next_;
                }
    
                rotate(first, third, second);
                first = &(*first)->next_;
            }
        }

        void rotate(value_node** first, value_node** second, value_node** third)
        {
            value_node* tmp = *first;
            *first = *second;
            *second = *third;
            *third = tmp;
            //if(*second != &value_list_end_impl::instance) back = second;
        }
    }
    }

    //////////////////////////////////////////////////////////////////////////
    // Lists
    
    namespace detail
    {
        struct value_list_impl : public value_node
        {
            value_list_impl(value::tag_type);
            value_list_impl(value_list_builder&, value::tag_type);
        private:
            value_list_impl(value_list_impl const&);

            char const* type_name() const { return "list"; }

            virtual ~value_list_impl();
            virtual value_node* clone() const;
            virtual bool empty() const;
            virtual bool equals(value_node*) const;

            virtual bool is_list() const;
            virtual value_node* get_list() const;
    
            value_node* head_;
        };
        
        value_list_impl::value_list_impl(value::tag_type tag)
            : value_node(tag), head_(&value_list_end_impl::instance)
        {}
    
        value_list_impl::value_list_impl(value_list_builder& builder,
                value::tag_type tag)
            : value_node(tag), head_(builder.release())
        {
        }

        value_list_impl::value_list_impl(value_list_impl const& x)
            : value_node(x.tag_), head_(x.head_)
        {
            list_ref(head_);
        }
    
        value_list_impl::~value_list_impl()
        {
            list_unref(head_);
        }

        value_node* value_list_impl::clone() const
        {
            return new value_list_impl(*this);
        }

        bool value_list_impl::empty() const
        {
            return head_ == &value_list_end_impl::instance;
        }
    
        bool value_list_impl::is_list() const
        {
            return true;
        }
    
        value_node* value_list_impl::get_list() const
        {
            return head_;
        }

        bool value_list_impl::equals(value_node* other) const {
            value_node* x1;

            try {
                 x1 = other->get_list();
            }
            catch(value_undefined_method&) {
                return false;
            }

            for(value_node *x2 = head_; x1 != x2; x1 = x1->next_, x2 = x2->next_)
            {
                if (x2 == &value_list_end_impl::instance ||
                    !x1->equals(x2)) return false;
            }
            
            return true;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // List builder
    
    namespace detail
    {
        // value_list_builder
    
        value_list_builder::value_list_builder()
            : head_(&value_list_end_impl::instance)
            , back_(&head_)
        {}

        value_list_builder::value_list_builder(value_node* ptr)
            : head_(ptr)
            , back_(list_ref_back(&head_))
        {}

        value_list_builder::~value_list_builder()
        {
            list_unref(head_);
        }

        void value_list_builder::swap(value_list_builder& other) {
            std::swap(head_, other.head_);
            std::swap(back_, other.back_);
            if(back_ == &other.head_) back_ = &head_;
            if(other.back_ == &head_) other.back_ = &other.head_;
        }

        value_node* value_list_builder::release() {
            value_node* r = head_;
            head_ = &value_list_end_impl::instance;
            back_ = &head_;
            return r;
        }
    
        void value_list_builder::append(value_node* item)
        {
            if(item->next_) item = item->clone();
            intrusive_ptr_add_ref(item);
            item->next_ = *back_;
            *back_ = item;
            back_ = &item->next_;
        }
        
        void value_list_builder::sort()
        {
            back_ = merge_sort(&head_);
            assert(*back_ == &value_list_end_impl::instance);
        }

        bool value_list_builder::empty() const
        {
            return head_ == &value_list_end_impl::instance;
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Value builder
    
    value_builder::value_builder()
        : current()
        , list_tag(value::default_tag)
        , saved()
    {
    }
    
    void value_builder::swap(value_builder& other) {
        current.swap(other.current);
        std::swap(list_tag, other.list_tag);
        saved.swap(other.saved);
    }
    
    void value_builder::save() {
        boost::scoped_ptr<value_builder> store(new value_builder);
        swap(*store);
        saved.swap(store);
    }

    void value_builder::restore() {
        boost::scoped_ptr<value_builder> store;
        store.swap(saved);
        swap(*store);
    }

    value value_builder::release() {
        return value(new detail::value_list_impl(current, list_tag));
    }

    void value_builder::insert(value const& item) {
        current.append(item.value_);
    }

    void value_builder::extend(value const& list) {
        for (value::iterator begin = list.begin(), end = list.end();
            begin != end; ++begin)
        {
            insert(*begin);
        }
    }

    void value_builder::start_list(value::tag_type tag) {
        save();
        list_tag = tag;
    }

    void value_builder::finish_list() {
        value list = release();
        restore();
        insert(list);
    }

    void value_builder::clear_list() {
        restore();
    }

    void value_builder::sort_list()
    {
        current.sort();
    }

    bool value_builder::empty() const
    {
        return current.empty();
    }

    ////////////////////////////////////////////////////////////////////////////
    // Iterator

    namespace detail
    {
        value::iterator::iterator()
            : ptr_(&value_list_end_impl::instance) {}
    }
}
