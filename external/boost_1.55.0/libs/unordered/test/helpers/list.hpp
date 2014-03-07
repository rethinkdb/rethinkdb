
// Copyright 2008-2009 Daniel James.
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// Gratuitous single linked list.
//
// Sadly some STL implementations aren't up to scratch and I need a simple
// cross-platform container. So here it is.

#if !defined(UNORDERED_TEST_LIST_HEADER)
#define UNORDERED_TEST_LIST_HEADER

#include <boost/iterator.hpp>
#include <boost/limits.hpp>
#include <functional>

namespace test
{
    template <typename It1, typename It2>
    bool equal(It1 begin, It1 end, It2 compare)
    {
        for(;begin != end; ++begin, ++compare)
            if(*begin != *compare) return false;
        return true;
    }

    template <typename It1, typename It2, typename Pred>
    bool equal(It1 begin, It1 end, It2 compare, Pred predicate)
    {
        for(;begin != end; ++begin, ++compare)
            if(!predicate(*begin, *compare)) return false;
        return true;
    }


    template <typename T> class list;

    namespace test_detail
    {
        template <typename T> class list_node;
        template <typename T> class list_data;
        template <typename T> class list_iterator;
        template <typename T> class list_const_iterator;

        template <typename T>
        class list_node
        {
            list_node(list_node const&);
            list_node& operator=(list_node const&);
        public:
            T value_;
            list_node* next_;
                    
            list_node(T const& v) : value_(v), next_(0) {}
            list_node(T const& v, list_node* n) : value_(v), next_(n) {}
        };

        template <typename T>
        class list_data
        {
        public:
            typedef list_node<T> node;
            typedef unsigned int size_type;

            node* first_;
            node** last_ptr_;
            size_type size_;
            
            list_data() : first_(0), last_ptr_(&first_), size_(0) {}

            ~list_data() {
                while(first_) {
                    node* tmp = first_;
                    first_ = first_->next_;
                    delete tmp;
                }
            }
        private:
            list_data(list_data const&);
            list_data& operator=(list_data const&);
        };

        template <typename T>
        class list_iterator
            : public boost::iterator<
                std::forward_iterator_tag, T,
                  int, T*, T&>
        {
            friend class list_const_iterator<T>;
            friend class test::list<T>;
            typedef list_node<T> node;
            typedef list_const_iterator<T> const_iterator;

            node* ptr_;
        public:
            list_iterator() : ptr_(0) {}
            explicit list_iterator(node* x) : ptr_(x) {}

            T& operator*() const { return ptr_->value_; }
            T* operator->() const { return &ptr_->value_; }
            list_iterator& operator++() {
                ptr_ = ptr_->next_; return *this; }
            list_iterator operator++(int) {
                list_iterator tmp = *this; ptr_ = ptr_->next_; return tmp; }
            bool operator==(const_iterator y) const { return ptr_ == y.ptr_; }
            bool operator!=(const_iterator y) const { return ptr_ != y.ptr_; }
        };

        template <typename T>
        class list_const_iterator
            : public boost::iterator<
                std::forward_iterator_tag, T,
                  int, T const*, T const&>
        {
            friend class list_iterator<T>;
            friend class test::list<T>;
            typedef list_node<T> node;
            typedef list_iterator<T> iterator;
            typedef list_const_iterator<T> const_iterator;

            node* ptr_;
        public:
            list_const_iterator() : ptr_(0) {}
            list_const_iterator(list_iterator<T> const& x) : ptr_(x.ptr_) {}

            T const& operator*() const { return ptr_->value_; }
            T const* operator->() const { return &ptr_->value_; }

            list_const_iterator& operator++()
            {
                ptr_ = ptr_->next_;
                return *this;
            }

            list_const_iterator operator++(int)
            {
                list_const_iterator tmp = *this;
                ptr_ = ptr_->next_;
                return tmp;
            }

            bool operator==(const_iterator y) const
            {
                return ptr_ == y.ptr_;
            }

            bool operator!=(const_iterator y) const
            {
                return ptr_ != y.ptr_;
            }
        };
    }

    template <typename T>
    class list
    {
        typedef test::test_detail::list_data<T> data;
        typedef test::test_detail::list_node<T> node;
        data data_;
    public:
        typedef T value_type;
        typedef value_type& reference;
        typedef value_type const& const_reference;
        typedef unsigned int size_type;

        typedef test::test_detail::list_iterator<T> iterator;
        typedef test::test_detail::list_const_iterator<T> const_iterator;

        list() : data_() {}

        list(list const& other) : data_() {
            insert(other.begin(), other.end());
        }

        template <class InputIterator>
        list(InputIterator i, InputIterator j) : data_() {
            insert(i, j);
        }

        list& operator=(list const& other) {
            clear();
            insert(other.begin(), other.end());
            return *this;
        }

        iterator begin() { return iterator(data_.first_); }
        iterator end() { return iterator(); }
        const_iterator begin() const { return iterator(data_.first_); }
        const_iterator end() const { return iterator(); }
        const_iterator cbegin() const { return iterator(data_.first_); }
        const_iterator cend() const { return iterator(); }

        template <class InputIterator>
        void insert(InputIterator i, InputIterator j) {
            for(; i != j; ++i)
                push_back(*i);
        }

        void push_front(value_type const& v) {
            data_.first_ = new node(v, data_.first_);
            if(!data_.size_) data_.last_ptr_ = &(*data_.last_ptr_)->next_;
            ++data_.size_;
        }
    
        void push_back(value_type const& v) {
            *data_.last_ptr_ = new node(v);
            data_.last_ptr_ = &(*data_.last_ptr_)->next_;
            ++data_.size_;
        }
        
        void clear() {
            while(data_.first_) {
                node* tmp = data_.first_;
                data_.first_ = data_.first_->next_;
                --data_.size_;
                delete tmp;
            }
            data_.last_ptr_ = &data_.first_;
        }

        void erase(const_iterator i, const_iterator j) {
            node** ptr = &data_.first_;

            while(*ptr != i.ptr_) {
                ptr = &(*ptr)->next_;
            }

            while(*ptr != j.ptr_) {
                node* to_delete = *ptr;
                *ptr = (*ptr)->next_;
                --data_.size_;
                delete to_delete;
            }

            if(!*ptr) data_.last_ptr_ = ptr;
        }

        bool empty() const {
            return !data_.size_;
        }

        size_type size() const {
            return data_.size_;
        }

        void sort() {
            sort(std::less<T>());
        }

        template <typename Less>
        void sort(Less less = Less()) {
            if(!empty()) merge_sort(&data_.first_,
                    (std::numeric_limits<size_type>::max)(), less);
        }

        bool operator==(list const& y) const {
            return size() == y.size() &&
                test::equal(begin(), end(), y.begin());
        }

        bool operator!=(list const& y) const {
            return !(*this == y);
        }

    private:
        template <typename Less>
        node** merge_sort(node** l, size_type recurse_limit, Less less)
        {
            node** ptr = &(*l)->next_;
            for(size_type count = 0; count < recurse_limit && *ptr; ++count)
            {
                ptr = merge_adjacent_ranges(l, ptr,
                        merge_sort(ptr, count, less), less);
            }
            return ptr;
        }
        
        template <typename Less>
        node** merge_adjacent_ranges(node** first, node** second,
                node** third, Less less)
        {
            for(;;) {
                for(;;) {
                    if(first == second) return third;
                    if(less((*second)->value_, (*first)->value_)) break;
                    first = &(*first)->next_;
                }

                swap_adjacent_ranges(first, second, third);
                first = &(*first)->next_;
                
                // Since the two ranges we just swapped, the order is now:
                // first...third...second
                
                for(;;) {
                    if(first == third) return second;
                    if(!less((*first)->value_, (*third)->value_)) break;
                    first = &(*first)->next_;
                }

                swap_adjacent_ranges(first, third, second);
                first = &(*first)->next_;
            }
        }
        
        void swap_adjacent_ranges(node** first, node** second, node** third)
        {
            node* tmp = *first;
            *first = *second;
            *second = *third;
            *third = tmp;
            if(!*second) data_.last_ptr_ = second;
        }
    };
}

#endif
