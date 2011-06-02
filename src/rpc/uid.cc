#include "rpc/uid.hpp"
#include "rpc/core/cluster.hpp"

__thread int uid_counter = 0;

cluster_uid_t cluster_uid_t::create() {
    cluster_uid_t x;
    x.peer = get_cluster().us;   // TODO: Error if the cluster isn't up yet
    x.thread = get_thread_id();
    x.counter = uid_counter++;
    rassert(x.is_valid());
    return x;
}

cluster_uid_t::cluster_uid_t() : peer(-1), thread(-1), counter(-1) {
}

bool cluster_uid_t::is_valid() const {
    if (peer == -1) {
        rassert(thread == -1 && counter == -1);
        return false;
    } else {
        rassert(thread != -1 && counter != -1);
        return true;
    }
}

bool cluster_uid_t::operator==(const cluster_uid_t &x) const {
    return (peer == x.peer && thread == x.thread && counter == x.counter);
}

bool cluster_uid_t::operator!=(const cluster_uid_t &x) const {
    return !(*this == x);
}

bool cluster_uid_t::operator<(const cluster_uid_t &x) const {
    return peer < x.peer ||
        (peer == x.peer && thread < x.thread) ||
        (peer == x.peer && thread == x.thread && counter < x.counter);
}

bool cluster_uid_t::operator>(const cluster_uid_t &x) const {
    return (x < *this);
}

bool cluster_uid_t::operator<=(const cluster_uid_t &x) const {
    return (*this < x) || (*this == x);
}

bool cluster_uid_t::operator>=(const cluster_uid_t &x) const {
    return (*this > x) || (*this == x);
}
