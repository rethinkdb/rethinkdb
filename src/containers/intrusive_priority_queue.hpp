// Copyright 2010-2013 RethinkDB, all rights reserved.
#ifndef CONTAINERS_INTRUSIVE_PRIORITY_QUEUE_HPP_
#define CONTAINERS_INTRUSIVE_PRIORITY_QUEUE_HPP_

#include <utility>
#include <vector>

#include "errors.hpp"

// TODO: Make this type generally not be awful: Make intrusive_priority_queue_node_t not have
// virtual functions, make it not use an O(n) vector.

template <class node_t>
class intrusive_priority_queue_t;

template <class node_t>
class intrusive_priority_queue_node_t {
public:
    intrusive_priority_queue_node_t()
#ifndef NDEBUG
        : queue(NULL)
#endif
        { }

protected:
    ~intrusive_priority_queue_node_t() {
        rassert(queue == NULL);
    }

private:
    friend class intrusive_priority_queue_t<node_t>;
#ifndef NDEBUG
    intrusive_priority_queue_t<node_t> *queue;
#endif
    size_t index;

    DISABLE_COPYING(intrusive_priority_queue_node_t);
};

template <class node_t>
class intrusive_priority_queue_t {
public:
    intrusive_priority_queue_t() { }

    ~intrusive_priority_queue_t() {
        rassert(empty());
    }

    bool empty() {
        return nodes.empty();
    }

    size_t size() {
        return nodes.size();
    }

    void push(node_t *x) {
        rassert(x != NULL);
        rassert(node_queue(x) == NULL);
        DEBUG_ONLY_CODE(node_queue(x) = this);

        nodes.push_back(x);
        node_index(x) = nodes.size() - 1;
        bubble_towards_root(x);
    }

    void remove(node_t *x) {
        rassert(x);
        rassert(node_queue(x) == this);
        DEBUG_ONLY_CODE(node_queue(x) = NULL);
        if (node_index(x) == nodes.size() - 1) {
            nodes.pop_back();
        } else {
            node_t *replacement = nodes[node_index(x)] = nodes.back();
            node_index(replacement) = node_index(x);
            nodes.pop_back();
            bubble_towards_root(replacement);
            bubble_towards_leaves(replacement);
        }
    }

    node_t *peek() {
        if (nodes.empty()) {
            return nullptr;
        } else {
            return nodes.front();
        }
    }

    node_t *pop() {
        if (nodes.empty()) {
            return nullptr;
        } else {
            node_t *x = nodes.front();
            rassert(node_queue(x) == this);
            DEBUG_ONLY_CODE(x->queue = NULL);
            if (nodes.size() == 1) {
                nodes.pop_back();
            } else {
                node_t *replacement = nodes[0] = nodes.back();
                node_index(replacement) = 0;
                nodes.pop_back();
                bubble_towards_leaves(replacement);
            }
            return x;
        }
    }

    void update(node_t *node) {
        rassert(node);
        bubble_towards_root(node);
        bubble_towards_leaves(node);
    }

    void swap_in_place(node_t *to_remove, node_t *to_insert) {
        rassert(to_remove);
        rassert(to_insert);
        rassert(node_queue(to_remove) == this);
        rassert(node_queue(to_insert) == NULL);
        // Pass const pointers to one call of left_is_higher_priority to enforce that it be const.
        rassert(!left_is_higher_priority(const_cast<const node_t *>(to_remove), const_cast<const node_t *>(to_insert)));
        rassert(!left_is_higher_priority(to_insert, to_remove));
        DEBUG_ONLY_CODE(node_queue(to_insert) = this);
        node_index(to_insert) = node_index(to_remove);
        nodes[node_index(to_remove)] = to_insert;
        DEBUG_ONLY_CODE(node_queue(to_remove) = NULL);
    }

private:
    static size_t& node_index(node_t *x) {
        intrusive_priority_queue_node_t<node_t> *node = x;
        return node->index;
    }

    static intrusive_priority_queue_t<node_t> *& node_queue(node_t *x) {
        intrusive_priority_queue_node_t<node_t> *node = x;
        return node->queue;
    }

    static size_t parent_index(size_t index) {
        rassert(index != 0);
        return (index + (index % 2) - 2) / 2;
    }

    static size_t left_child_index(size_t index) {
        return index * 2 + 1;
    }

    static size_t right_child_index(size_t index) {
        return index * 2 + 2;
    }

    void swap_nodes(size_t i, size_t j) {
        node_index(nodes[i]) = j;
        node_index(nodes[j]) = i;
        std::swap(nodes[i], nodes[j]);
    }

    void bubble_towards_root(node_t *node) {
        while (node_index(node) != 0 &&
               left_is_higher_priority(node, nodes[parent_index(node_index(node))])) {
            swap_nodes(node_index(node), parent_index(node_index(node)));
        }
    }

    void bubble_towards_leaves(node_t *node) {
        while (true) {
            const size_t left_index = left_child_index(node_index(node));
            const size_t right_index = right_child_index(node_index(node));
            size_t winner = node_index(node);
            if (left_index < nodes.size() &&
                left_is_higher_priority(nodes[left_index], nodes[winner])) {
                winner = left_index;
            }
            if (right_index < nodes.size() &&
                left_is_higher_priority(nodes[right_index], nodes[winner])) {
                winner = right_index;
            }
            if (winner == node->index) {
                break;
            } else {
                swap_nodes(winner, node->index);
            }
        }
    }

    // TODO: Vectors are O(n).
    std::vector<node_t *> nodes;

    DISABLE_COPYING(intrusive_priority_queue_t);
};

#endif  // CONTAINERS_INTRUSIVE_PRIORITY_QUEUE_HPP_
