// Copyright 2010-2015 RethinkDB, all rights reserved.
#ifndef CONTAINERS_INTRUSIVE_LIST_HPP_
#define CONTAINERS_INTRUSIVE_LIST_HPP_

#include "errors.hpp"

template <class T> class intrusive_list_t;

template <class T>
class intrusive_list_node_t {
public:
    bool in_a_list() const {
        guarantee((next_ == NULL) == (prev_ == NULL));
        return prev_ != NULL;
    }

protected:
    intrusive_list_node_t() : prev_(NULL), next_(NULL) { }
    ~intrusive_list_node_t() {
        guarantee(prev_ == NULL,
                  "non-detached intrusive list node destroyed");
        guarantee(next_ == NULL,
                  "inconsistent intrusive list node!");
    }

    intrusive_list_node_t(intrusive_list_node_t &&movee) {
        guarantee((movee.prev_ == NULL) == (movee.next_ == NULL));
        guarantee(movee.prev_ != &movee,
                "Only intrusive_list_t can be a self-pointing node.");
        prev_ = movee.prev_;
        next_ = movee.next_;
        if (prev_ != NULL) {
            prev_->next_ = this;
            next_->prev_ = this;
        }
        movee.prev_ = NULL;
        movee.next_ = NULL;
    }

    // Don't implement this.  _Maybe_ you won't fuck it up.  Just use the
    // move-constructor in subclasses' assignment operator implementations.
    intrusive_list_node_t &operator=(intrusive_list_node_t &&movee) = delete;

private:
    friend class intrusive_list_t<T>;

    intrusive_list_node_t *prev_;
    intrusive_list_node_t *next_;

    DISABLE_COPYING(intrusive_list_node_t);
};

template <class T>
class intrusive_list_t : private intrusive_list_node_t<T> {
public:
    intrusive_list_t() : size_(0) {
        this->prev_ = this;
        this->next_ = this;
    }

    intrusive_list_t(intrusive_list_t &&movee) : size_(0) {
        // We just initialize ourselves to empty and then use append_and_clear.
        this->prev_ = this;
        this->next_ = this;

        append_and_clear(&movee);
    }

    // We don't support generic assignment because non-empty intrusive lists may not
    // be destroyed.  You have to manually remove all elements of the intrusive list
    // before destroying it.
    void operator=(intrusive_list_t &&movee) = delete;

    ~intrusive_list_t() {
        guarantee(this->prev_ == this, "non-empty intrusive list destroyed");
        guarantee(this->next_ == this, "inconsistent intrusive list (end node)!");
        guarantee(size_ == 0, "empty intrusive list destroyed with non-zero size");

        // Set these to NULL to appease base class destructor's assertions.
        this->prev_ = NULL;
        this->next_ = NULL;
    }

    bool empty() const {
        return this->prev_ == this;
    }

    T *head() const {
        return null_if_self(this->next_);
    }

    T *tail() const {
        return null_if_self(this->prev_);
    }

    T *next(T *elem) const {
        return null_if_self(elem->next_);
    }

    T *prev(T *elem) const {
        return null_if_self(elem->prev_);
    }

    void push_front(T *node) {
        insert_between(node, this, this->next_);
        ++size_;
    }

    void push_back(T *node) {
        insert_between(node, this->prev_, this);
        ++size_;
    }

    void remove(T *value) {
        intrusive_list_node_t<T> *node = value;
        guarantee(node->in_a_list());
        remove_node(value);
        --size_;
    }

    void pop_front() {
        guarantee(!empty());
        remove_node(static_cast<T *>(this->next_));
        --size_;
    }

    void pop_back() {
        guarantee(!empty());
        remove_node(static_cast<T *>(this->prev_));
        --size_;
    }

    size_t size() const {
        return size_;
    }

    void append_and_clear(intrusive_list_t<T> *appendee) {
        if (!appendee->empty()) {
            intrusive_list_node_t<T> *t = this->prev_;
            t->next_ = appendee->next_;
            appendee->next_->prev_ = t;
            this->prev_ = appendee->prev_;
            appendee->prev_->next_ = this;

            appendee->next_ = appendee;
            appendee->prev_ = appendee;

            size_ += appendee->size_;
            appendee->size_ = 0;
        }
    }

#ifndef NDEBUG
    void validate() const {
        const intrusive_list_node_t<T> *node = this;
        size_t count = 0;
        do {
            ++count;
            guarantee(node->prev_ != NULL, "count = %zu", count);
            guarantee(node->prev_->next_ == node);
            guarantee(node->next_ != NULL);
            guarantee(node->next_->prev_ == node);
            node = node->next_;
        } while (node != this);
        guarantee(count == size_ + 1);
    }
#endif // NDEBUG

private:
    static void insert_between(T *item,
                               intrusive_list_node_t<T> *before,
                               intrusive_list_node_t<T> *after) {
        intrusive_list_node_t<T> *node = item;
        guarantee(!node->in_a_list());
        guarantee(before != NULL);
        guarantee(before->in_a_list());
        guarantee(after != NULL);
        guarantee(after->in_a_list());
        before->next_ = node;
        after->prev_ = node;
        node->prev_ = before;
        node->next_ = after;
    }

    static void remove_node(T *item) {
        intrusive_list_node_t<T> *node = item;
        guarantee(node->in_a_list());
        node->prev_->next_ = node->next_;
        node->next_->prev_ = node->prev_;
        node->prev_ = NULL;
        node->next_ = NULL;
    }

    T *null_if_self(intrusive_list_node_t<T> *node) const {
        return node == this ? NULL : static_cast<T *>(node);
    }

    size_t size_;

    DISABLE_COPYING(intrusive_list_t);
};


#endif // CONTAINERS_INTRUSIVE_LIST_HPP_
