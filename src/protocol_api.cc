#include "protocol_api.hpp"
RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(cannot_perform_query_exc_t, message, query_state);

namespace_interface_access_t::namespace_interface_access_t() :
    nif(NULL), ref_tracker(NULL), thread(INVALID_THREAD)
{ }

namespace_interface_access_t::namespace_interface_access_t(
    const namespace_interface_access_t& access) :
    nif(access.nif), ref_tracker(access.ref_tracker), thread(access.thread)
{
    if (ref_tracker != NULL) {
        rassert(get_thread_id() == thread);
        ref_tracker->add_ref();
    }
}

namespace_interface_access_t::namespace_interface_access_t(namespace_interface_t *_nif,
    ref_tracker_t *_ref_tracker, threadnum_t _thread) :
    nif(_nif), ref_tracker(_ref_tracker), thread(_thread)
{
    if (ref_tracker != NULL) {
        rassert(get_thread_id() == thread);
        ref_tracker->add_ref();
    }
}

namespace_interface_access_t &namespace_interface_access_t::operator=(
    const namespace_interface_access_t &access) {
    if (this != &access) {
        if (ref_tracker != NULL) {
            rassert(get_thread_id() == thread);
            ref_tracker->release();
        }
        nif = access.nif;
        ref_tracker = access.ref_tracker;
        thread = access.thread;
        if (ref_tracker != NULL) {
            rassert(get_thread_id() == thread);
            ref_tracker->add_ref();
        }
    }
    return *this;
}

namespace_interface_access_t::~namespace_interface_access_t() {
    if (ref_tracker != NULL) {
        rassert(get_thread_id() == thread);
        ref_tracker->release();
    }
}

namespace_interface_t *namespace_interface_access_t::get() {
    rassert(thread == get_thread_id());
    return nif;
}
