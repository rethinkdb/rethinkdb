#ifndef CONTAINERS_HALF_INTRUSIVE_LIST_HPP_
#define CONTAINERS_HALF_INTRUSIVE_LIST_HPP_

#include "errors.hpp"

template <class T>
class half_intrusive_list_node_t;

template <class T>
class half_intrusive_list_t;

template <class T>
class half_intrusive_list_half_node_t {
public:
    half_intrusive_list_half_node_t() : next_(nullptr) { }
    ~half_intrusive_list_half_node_t() {
        guarantee(next_ == nullptr, "non-detached intrusive list half-node destroyed");
    }

protected:
    friend class half_intrusive_list_t<T>;
    half_intrusive_list_node_t<T> *next_;

    DISABLE_COPYING(half_intrusive_list_half_node_t);
};

template <class T>
class half_intrusive_list_node_t : private half_intrusive_list_half_node_t<T> {
public:
    half_intrusive_list_node_t() : prev_(nullptr) { }
    ~half_intrusive_list_node_t() {
        guarantee(prev_ == nullptr,
                  "non-detached half-intrusive-list node destroyed");
    }

    bool in_a_list() const {
        return prev_ != nullptr;
    }

private:
    friend class half_intrusive_list_t<T>;
    half_intrusive_list_half_node_t<T> *prev_;

    DISABLE_COPYING(half_intrusive_list_node_t);
};


template <class T>
class half_intrusive_list_t : private half_intrusive_list_half_node_t<T> {
public:
    half_intrusive_list_t() { }
    ~half_intrusive_list_t() { }

    bool empty() const {
        return this->next_ == nullptr;
    }

    T *head() const {
        rassert(this->next_ == nullptr || this->next_->prev_ == this);
        return static_cast<T *>(this->next_);
    }

    T *next(T *elem) const {
        guarantee(elem->in_a_list());
        rassert(elem->next_ == nullptr || elem->next_->prev_ == elem);
        rassert(elem->prev_->next_ == elem);
        half_intrusive_list_node_t<T> *p = elem;
        return static_cast<T *>(p->next_);
    }

    void push_front(T *elem) {
        guarantee(!elem->in_a_list());
        elem->prev_ = this;
        elem->next_ = this->next_;
        if (this->next_ != nullptr) {
            rassert(this->next_->prev_ == this);
            this->next_->prev_ = elem;
        }
        this->next_ = elem;
    }

    void remove(T *elem) {
        guarantee(elem->in_a_list());
        if (elem->next_ != nullptr) {
            elem->next_->prev_ = elem->prev_;
        }
        elem->prev_->next_ = elem->next_;
        elem->next_ = nullptr;
        elem->prev_ = nullptr;
    }
};


#endif  // CONTAINERS_HALF_INTRUSIVE_LIST_HPP_

