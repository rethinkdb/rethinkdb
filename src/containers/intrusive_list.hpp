// Copyright 2010-2015 RethinkDB, all rights reserved.

/// @file intrusive_list.hpp
/// @brief Intrusive doubly-linked list container
///
/// Provides an intrusive linked list implementation where the list node is embedded
/// within the list element itself. This eliminates extra allocations and improves
/// cache locality compared to external node storage.
///
/// @defgroup IntrinsicContainers Intrusive Container Structures
/// Containers where node pointers are embedded in elements
/// @{

#ifndef CONTAINERS_INTRUSIVE_LIST_HPP_
#define CONTAINERS_INTRUSIVE_LIST_HPP_

#include "errors.hpp"

template <class T> class intrusive_list_t;

/// @brief Base class for elements in an intrusive_list_t
///
/// Derived classes inherit from intrusive_list_node_t<T> to embed list node
/// pointers directly in the element structure. This provides efficient O(1) removal
/// and prevents extra allocations.
///
/// @tparam T The derived class type that will inherit from this node
/// @example
/// @code
/// class Task : public intrusive_list_node_t<Task> {
/// public:
///     std::string name;
///     void execute() { /* ... */ }
/// };
///
/// intrusive_list_t<Task> todo_list;
/// Task t;
/// todo_list.push_back(&t);
/// @endcode
///
/// @note Elements must remain valid for the entire duration they're in a list
/// @warning Removing from one list and adding to another is safe, but removing
///          without care can cause list corruption
template <class T>
class intrusive_list_node_t {
public:
    /// @brief Checks if this node is currently in a list
    /// @return true if node is linked in a list, false if standalone
    bool in_a_list() const {
        guarantee((next_ == nullptr) == (prev_ == nullptr));
        return prev_ != nullptr;
    }

protected:
    /// @brief Default constructor creates an unlinked node
    /// Node starts in a detached state, not part of any list
    intrusive_list_node_t() : prev_(nullptr), next_(nullptr) { }

    /// @brief Destructor ensures node was properly detached
    /// Asserts that the node is not still linked in a list
    ~intrusive_list_node_t() {
        guarantee(prev_ == nullptr,
                  "non-detached intrusive list node destroyed");
        guarantee(next_ == nullptr,
                  "inconsistent intrusive list node!");
    }

    /// @brief Move constructor transfers list linkage
    /// If the node is linked, it updates the adjacent nodes' pointers
    /// to point to the new location. The source is left unlinked.
    /// @param movee The node to move from
    intrusive_list_node_t(intrusive_list_node_t &&movee) {
        guarantee((movee.prev_ == nullptr) == (movee.next_ == nullptr));
        guarantee(movee.prev_ != &movee,
                "Only intrusive_list_t can be a self-pointing node.");
        prev_ = movee.prev_;
        next_ = movee.next_;
        if (prev_ != nullptr) {
            prev_->next_ = this;
            next_->prev_ = this;
        }
        movee.prev_ = nullptr;
        movee.next_ = nullptr;
    }

    /// @brief Move assignment is explicitly deleted
    /// Use move constructor in derived class assignment operators instead
    intrusive_list_node_t &operator=(intrusive_list_node_t &&movee) = delete;

private:
    friend class intrusive_list_t<T>;

    intrusive_list_node_t *prev_;  ///< Pointer to previous node in list
    intrusive_list_node_t *next_;  ///< Pointer to next node in list

    DISABLE_COPYING(intrusive_list_node_t);
};

/// @brief Intrusive doubly-linked list container
///
/// `intrusive_list_t` manages a doubly-linked list where the list nodes are
/// embedded in the elements themselves (via inheritance from intrusive_list_node_t).
/// This provides O(1) element removal given an element pointer, improved cache
/// locality, and eliminates separate node allocations.
///
/// Elements must inherit from intrusive_list_node_t<T> to be used in an
/// intrusive_list_t<T>.
///
/// @tparam T The element type (must inherit from intrusive_list_node_t<T>)
///
/// @example
/// @code
/// struct Task : intrusive_list_node_t<Task> {
///     std::string name;
/// };
///
/// intrusive_list_t<Task> tasks;
/// Task t1, t2, t3;
///
/// tasks.push_back(&t1);
/// tasks.push_back(&t2);
/// tasks.push_back(&t3);
///
/// // Iterate forward
/// for (auto it = tasks.begin(); it != tasks.end(); ++it) {
///     std::cout << it->name << "\n";
/// }
///
/// // Remove an element - O(1) operation
/// tasks.remove(&t2);
/// @endcode
///
/// @note This list uses a sentinel node (the list object itself)
/// @note All operations are O(1) except iteration
template <class T>
class intrusive_list_t : private intrusive_list_node_t<T> {
public:
    /// @brief Default constructor creates an empty list
    /// The list starts with just a sentinel node
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
        this->prev_ = nullptr;
        this->next_ = nullptr;
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
        guarantee(before != nullptr);
        guarantee(before->in_a_list());
        guarantee(after != nullptr);
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
        node->prev_ = nullptr;
        node->next_ = nullptr;
    }

    T *null_if_self(intrusive_list_node_t<T> *node) const {
        return node == this ? nullptr : static_cast<T *>(node);
    }

    size_t size_;

    DISABLE_COPYING(intrusive_list_t);
};


#endif // CONTAINERS_INTRUSIVE_LIST_HPP_
