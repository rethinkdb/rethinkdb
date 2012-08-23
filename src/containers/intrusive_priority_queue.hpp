#ifndef CONTAINERS_INTRUSIVE_PRIORITY_QUEUE_HPP_
#define CONTAINERS_INTRUSIVE_PRIORITY_QUEUE_HPP_

#include <algorithm>
#include <vector>

#include "errors.hpp"

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

    virtual bool is_higher_priority_than(node_t *competitor) = 0;

protected:
    virtual ~intrusive_priority_queue_node_t() {
        rassert(queue == NULL);
    }

private:
    friend class intrusive_priority_queue_t<node_t>;
#ifndef NDEBUG
    intrusive_priority_queue_t<node_t> *queue;
#endif
    int index;
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
        rassert(x);
        rassert(get_mixin(x)->queue == NULL);
        DEBUG_ONLY_CODE(get_mixin(x)->queue = this);

        nodes.push_back(x);
        get_mixin(x)->index = nodes.size() - 1;
        bubble_towards_root(x);
    }

    void remove(node_t *x) {
        rassert(x);
        rassert(get_mixin(x)->queue == this);
        DEBUG_ONLY_CODE(get_mixin(x)->queue = NULL);
        // TODO: static_cast?  really?
        if (get_mixin(x)->index == static_cast<int>(nodes.size()) - 1) {
            nodes.pop_back();
        } else {
            node_t *replacement = nodes[get_mixin(x)->index] = nodes.back();
            get_mixin(replacement)->index = get_mixin(x)->index;
            nodes.pop_back();
            bubble_towards_root(replacement);
            bubble_towards_leaves(replacement);
        }
    }

    node_t *peek() {
        if (nodes.empty()) {
            return NULL;
        } else {
            return nodes.front();
        }
    }

    node_t *pop() {
        if (nodes.empty()) {
            return NULL;
        } else {
            node_t *x = nodes.front();
            rassert(get_mixin(x)->queue == this);
            DEBUG_ONLY_CODE(x->queue = NULL);
            if (nodes.size() == 1) {
                nodes.pop_back();
            } else {
                node_t *replacement = nodes[0] = nodes.back();
                get_mixin(replacement)->index = 0;
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
        rassert(get_mixin(to_remove)->queue == this);
        rassert(get_mixin(to_insert)->queue == NULL);
        rassert(!get_mixin(to_remove)->is_higher_priority_than(to_insert));
        rassert(!get_mixin(to_insert)->is_higher_priority_than(to_remove));
        DEBUG_ONLY_CODE(get_mixin(to_insert)->queue = this);
        get_mixin(to_insert)->index = get_mixin(to_remove)->index;
        nodes[get_mixin(to_remove)->index] = to_insert;
        DEBUG_ONLY_CODE(get_mixin(to_remove)->queue = NULL);
    }

private:
    static intrusive_priority_queue_node_t<node_t> *get_mixin(node_t *node) {
        return node;
    }

    static int compute_parent_index(int index) {
        rassert(index != 0);
        return (index + (index % 2) - 2) / 2;
    }

    static int compute_left_child_index(int index) {
        return index * 2 + 1;
    }

    static int compute_right_child_index(int index) {
        return index * 2 + 2;
    }

    void swap_nodes(int i, int j) {
        get_mixin(nodes[i])->index = j;
        get_mixin(nodes[j])->index = i;
        std::swap(nodes[i], nodes[j]);
    }

    void bubble_towards_root(node_t *node) {
        while (get_mixin(node)->index != 0 &&
                get_mixin(node)->is_higher_priority_than(nodes[compute_parent_index(get_mixin(node)->index)])) {
            swap_nodes(get_mixin(node)->index, compute_parent_index(get_mixin(node)->index));
        }
    }

    void bubble_towards_leaves(node_t *node) {
        while (true) {
            int left_index = compute_left_child_index(get_mixin(node)->index);
            int right_index = compute_right_child_index(get_mixin(node)->index);
            int winner = get_mixin(node)->index;
            if (left_index < static_cast<int>(nodes.size()) &&
                    get_mixin(nodes[left_index])->is_higher_priority_than(nodes[winner])) {
                winner = left_index;
            }
            if (right_index < static_cast<int>(nodes.size()) &&
                    get_mixin(nodes[right_index])->is_higher_priority_than(nodes[winner])) {
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
