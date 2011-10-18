#ifndef CONTAINERS_REFC_PTR_IMPL_HPP_
#define CONTAINERS_REFC_PTR_IMPL_HPP_

// A typical refcount adjustment function for types that have refcounts.
template <class T>
void refc_ptr_prototypical_adjust_ref(T *p, int adjustment) {
    struct ref_adder_t : public thread_message_t {
        void on_thread_switch() {
            ptr->ref_count += adj;
            rassert(ptr->ref_count >= 0);
            if (ptr->ref_count == 0) {
                ptr->destroy();
            }
            delete this;
        }
        T *ptr;
        int adj;
        ref_adder_t(T *p, int _adj) : ptr(p), adj(_adj) { }
    };

    int p_thread = p->home_thread();
    if (get_thread_id() == p_thread) {
        p->ref_count += adjustment;
        rassert(p->ref_count >= 0);
        if (p->ref_count == 0) {
            p->destroy();
        }
    } else {
        bool res = continue_on_thread(p_thread, new ref_adder_t(p, adjustment));
        rassert(!res);
    }
}



#endif  // CONTAINERS_REFC_PTR_IMPL_HPP_

