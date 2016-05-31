// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "rdb_protocol/changefeed.hpp"

#include <queue>

#include "boost_utils.hpp"
#include "btree/reql_specific.hpp"
#include "clustering/administration/auth/user_context.hpp"
#include "clustering/table_manager/table_meta_client.hpp"
#include "concurrency/cross_thread_signal.hpp"
#include "concurrency/interruptor.hpp"
#include "containers/archive/boost_types.hpp"
#include "rdb_protocol/artificial_table/backend.hpp"
#include "rdb_protocol/btree.hpp"
#include "rdb_protocol/env.hpp"
#include "rdb_protocol/geo/exceptions.hpp"
#include "rdb_protocol/geo/intersection.hpp"
#include "rdb_protocol/protocol.hpp"
#include "rdb_protocol/response.hpp"
#include "rdb_protocol/val.hpp"
#include "rpc/mailbox/typed.hpp"

#include "debug.hpp"

namespace ql {

namespace changefeed {

struct indexed_datum_t {
    indexed_datum_t(
            datum_t _val,
            boost::optional<std::string> _btree_index_key)
        : val(std::move(_val)),
          btree_index_key(std::move(_btree_index_key)) {
        guarantee(val.has());
    }
    datum_t val;
    boost::optional<std::string> btree_index_key;
    // This should be true, but older versions of boost don't support `move`
    // well in optionals.
    // MOVABLE_BUT_NOT_COPYABLE(indexed_datum_t);
};

struct stamped_range_t {
    explicit stamped_range_t(uint64_t _next_expected_stamp)
        : next_expected_stamp(_next_expected_stamp),
          left_fencepost(store_key_t::min()) { }
    const store_key_t &get_right_fencepost() {
        return ranges.size() == 0 ? left_fencepost : ranges.back().first.right.key();
    }
    uint64_t next_expected_stamp;
    store_key_t left_fencepost;
    std::deque<std::pair<key_range_t, uint64_t> > ranges;
    // This should be true, but it breaks with GCC 4.6's STL.  (The cost of
    // copying is low because if you look below we only ever copy
    // `stamped_range_t` before populating `ranges`.)
    // MOVABLE_BUT_NOT_COPYABLE(stamped_range_t);
};

void debug_print(printf_buffer_t *buf, const stamped_range_t &rng) {
    buf->appendf("stamped_range_t{");
    debug_print(buf, rng.next_expected_stamp);
    buf->appendf(", ");
    debug_print(buf, rng.left_fencepost);
    buf->appendf(", ");
    debug_print(buf, rng.ranges);
    buf->appendf("}");
}

struct change_val_t {
    change_val_t(std::pair<uuid_u, uint64_t> _source_stamp,
                 store_key_t _pkey,
                 boost::optional<indexed_datum_t> _old_val,
                 boost::optional<indexed_datum_t> _new_val
                 DEBUG_ONLY(, boost::optional<std::string> _sindex))
        : source_stamp(std::move(_source_stamp)),
          pkey(std::move(_pkey)),
          old_val(std::move(_old_val)),
          new_val(std::move(_new_val))
          DEBUG_ONLY(, sindex(std::move(_sindex))) {
        guarantee(old_val || new_val);
        if (old_val && new_val) {
            guarantee(static_cast<bool>(old_val->btree_index_key)
                == static_cast<bool>(new_val->btree_index_key));
            rassert(old_val->val != new_val->val);
        }
    }
    std::pair<uuid_u, uint64_t> source_stamp;
    store_key_t pkey;
    boost::optional<indexed_datum_t> old_val, new_val;
    DEBUG_ONLY(boost::optional<std::string> sindex;);
    // This should be true, but older versions of boost don't support `move`
    // well in optionals.
    // MOVABLE_BUT_NOT_COPYABLE(change_val_t);
};

namespace debug {
std::string print(const uuid_u &u) {
    printf_buffer_t buf;
    debug_print(&buf, u);
    return strprintf("uuid(%s)", buf.c_str());
}
std::string print(const datum_t &d) {
    return "datum(" + d.print() + ")";
}
std::string print(const indexed_datum_t &d) {
    return strprintf("indexed_datum_t(val: %s, index: %s)",
                     print(d.val).c_str(),
                     d.btree_index_key
                     ? key_to_debug_str(store_key_t(*d.btree_index_key)).c_str()
                     : "boost::none");
}
std::string print(const std::string &s) {
    return "str(" + s + ")";
}
std::string print(uint64_t i) {
    return strprintf("%" PRIu64, i);
}
std::string print(const key_range_t &rng) {
    return rng.print();
}
template<class A, class B>
std::string print(const std::pair<A, B> &p) {
    return strprintf("pair(%s, %s)", print(p.first).c_str(), print(p.second).c_str());
}
template<class T>
std::string print(const boost::optional<T> &t) {
    return strprintf("opt(%s)\n", t ? print(*t).c_str() : "");
}
std::string print(const msg_t::limit_change_t &change) {
    return strprintf("limit_change_t(%s, %s, %s)",
                     print(change.sub).c_str(),
                     print(change.old_key).c_str(),
                     print(change.new_val).c_str());
}
template<class T>
std::string print(const T &d) {
    std::string s = "[";
    for (const auto &t : d) {
        if (s.size() != 1) s += ", ";
        s += print(t);
    }
    s += "]";
    return s;
}
std::string print(const store_key_t &key) {
    printf_buffer_t buf;
    debug_print(&buf, key);
    return strprintf("store_key_t(%s)", buf.c_str());
}
std::string print(const stamped_range_t &srng) {
    return strprintf("stamped_range_t(%" PRIu64 ", %s, %s)",
                     srng.next_expected_stamp,
                     key_to_debug_str(srng.left_fencepost).c_str(),
                     print(srng.ranges).c_str());
}
std::string print(const change_val_t &cv) {
    return strprintf("change_val_t(%s, %s, %s, %s)\n",
                     print(cv.source_stamp).c_str(),
                     print(cv.pkey).c_str(),
                     print(cv.old_val).c_str(),
                     print(cv.new_val).c_str());
}
} // namespace debug

template<class T>
void debug_print(printf_buffer_t *buf, const T &t) {
    buf->appendf("%s", debug::print(t).c_str());
}

enum class pop_type_t { RANGE, POINT };
class maybe_squashing_queue_t {
public:
    virtual ~maybe_squashing_queue_t() { }
    virtual void add(change_val_t change_val) = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;
    virtual change_val_t pop() = 0;
    virtual const change_val_t &peek() = 0;
    virtual void purge_below(std::map<uuid_u, uint64_t> stamps) = 0;
};

class nonsquashing_queue_t final : public maybe_squashing_queue_t {
    void add(change_val_t change_val) final {
        queue.push_back(std::move(change_val));
    }
    size_t size() const final {
        return queue.size();
    }
    void clear() final {
        queue.clear();
    }
    const change_val_t &peek() final {
        guarantee(size() != 0);
        return queue.front();
    }
    change_val_t pop() final {
        guarantee(size() != 0);
        auto ret = std::move(queue.front());
        queue.pop_front();
        return ret;
    }
    void purge_below(std::map<uuid_u, uint64_t> stamps) final {
        std::map<uuid_u, uint64_t> orig, kept;
        std::deque<change_val_t> old_queue;
        old_queue.swap(queue);
        guarantee(queue.empty());
        for (auto &&cv : old_queue) {
            auto it = stamps.find(cv.source_stamp.first);
            orig.insert(std::make_pair(cv.source_stamp.first, 0)).first->second += 1;
            r_sanity_check(it != stamps.end());
            // We want `>=` here because the semantics are that the start stamp
            // is the first stamp we expect.
            if (cv.source_stamp.second >= it->second) {
                kept.insert(
                    std::make_pair(cv.source_stamp.first, 0)).first->second += 1;
                add(std::move(cv));
            }
        }
    }
    std::deque<change_val_t> queue;
};

class squashing_queue_t final : public maybe_squashing_queue_t {
public:
    void add(change_val_t change_val) final {
        auto it = queue.find(change_val.pkey);
        if (it == queue.end()) {
            auto order_it = queue_order.insert(queue_order.end(), change_val.pkey);
            auto pkey = change_val.pkey;
            auto pair = std::make_pair(
                std::move(pkey), std::make_pair(std::move(change_val), order_it));
            auto res = queue.insert(std::move(pair));
            it = res.first;
            guarantee(res.second);
        } else {
            change_val_t *change = &it->second.first;
            change_val.old_val = std::move(change->old_val);
            *change = std::move(change_val);
            bool has_old_val = change->old_val
                && change->old_val->val.get_type() != datum_t::R_NULL;
            bool has_new_val = change->new_val
                && change->new_val->val.get_type() != datum_t::R_NULL;
            if ((!has_old_val && !has_new_val)
                || (change->old_val && change->new_val
                    && change->old_val->val == change->new_val->val)) {
                queue_order.erase(it->second.second);
                queue.erase(it);
            }
        }
    }
    size_t size() const final {
        guarantee(queue.size() == queue_order.size());
        return queue.size();
    }
    void clear() final {
        queue.clear();
        queue_order.clear();
    }
    const change_val_t &peek() final {
        guarantee(size() != 0);
        auto it = queue.find(*queue_order.begin());
        guarantee(it != queue.end());
        return it->second.first;
    }
    change_val_t pop() final {
        guarantee(size() != 0);
        auto it = queue.find(*queue_order.begin());
        guarantee(it != queue.end());
        auto ret = std::move(it->second.first);
        queue.erase(it);
        queue_order.pop_front();
        return ret;
    }
    void purge_below(std::map<uuid_u, uint64_t>) final {
        // You should never purge a squashing queue.
        r_sanity_fail();
    }
private:
    std::map<store_key_t,
             std::pair<change_val_t, std::list<store_key_t>::iterator> > queue;
    std::list<store_key_t> queue_order;
};

boost::optional<datum_t> apply_ops(
    const datum_t &val,
    const std::vector<scoped_ptr_t<op_t> > &ops,
    env_t *env,
    const datum_t &key) THROWS_NOTHING {
    try {
        groups_t groups;
        groups[datum_t()] = std::vector<datum_t>{val};
        for (const auto &op : ops) {
            (*op)(env, &groups, [&]() { return key; });
        }
        // TODO: when we support `.group.changes` this will need to change.
        guarantee(groups.size() <= 1);
        std::vector<datum_t> *vec = &groups[datum_t()];
        guarantee(groups.size() == 1);
        // TODO: when we support `.concatmap.changes` this will need to change.
        guarantee(vec->size() <= 1);
        if (vec->size() == 1) {
            return (*vec)[0];
        } else {
            return boost::none;
        }
    } catch (const base_exc_t &) {
        // Do nothing.  This is similar to index behavior where we drop a row if
        // we fail to execute the code required to produce the index.  (In this
        // case, if you change the value of a row so that one of the
        // transformations errors on it, we report the row as being deleted from
        // the selection you asked for changes on.)
        return boost::none;
    }
}

server_t::client_info_t::client_info_t()
    : limit_clients(&opt_lt<std::string>),
      limit_clients_lock(new rwlock_t()) { }

server_t::server_t(mailbox_manager_t *_manager, store_t *_parent)
    : uuid(generate_uuid()),
      manager(_manager),
      parent(_parent),
      stop_mailbox(manager,
                   std::bind(&server_t::stop_mailbox_cb, this, ph::_1, ph::_2)),
      limit_stop_mailbox(manager, std::bind(&server_t::limit_stop_mailbox_cb,
                                            this, ph::_1, ph::_2, ph::_3, ph::_4)) { }

server_t::~server_t() { }

void server_t::stop_mailbox_cb(signal_t *, client_t::addr_t addr) {
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();
    auto it = clients.find(addr);
    // The client might have already been removed from e.g. a peer disconnect or
    // drainer destruction.  (Also, if we have multiple shards per btree this
    // will be called twice, and the second time it should be a no-op.)
    if (it != clients.end()) {
        it->second.cond->pulse_if_not_already_pulsed();
    }
}

void server_t::limit_stop_mailbox_cb(signal_t *,
                                     client_t::addr_t addr,
                                     boost::optional<std::string> sindex,
                                     uuid_u limit_uuid) {
    std::vector<scoped_ptr_t<limit_manager_t> > destroyable_lms;
    auto_drainer_t::lock_t lock(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();
    auto it = clients.find(addr);
    // The client might have already been removed from e.g. a peer disconnect or
    // drainer destruction.  (Also, if we have multiple shards per btree this
    // will be called twice, and the second time it should be a no-op.)
    if (it != clients.end()) {
        // It's OK not to have a drainer here because we're still holding a read
        // lock on `clients_lock`.
        rwlock_in_line_t lspot(it->second.limit_clients_lock.get(), access_t::write);
        lspot.read_signal()->wait_lazily_unordered();
        auto vec_it = it->second.limit_clients.find(sindex);
        if (vec_it != it->second.limit_clients.end()) {
            for (size_t i = 0; i < vec_it->second.size(); ++i) {
                auto *lm = &vec_it->second[i];
                guarantee((*lm).has());
                if ((*lm)->is_aborted()) {
                    continue;
                } else if ((*lm)->uuid == limit_uuid) {
                    lspot.write_signal()->wait_lazily_unordered();
                    auto *vec = &vec_it->second;
                    while (i < vec->size()) {
                        if (vec->back()->is_aborted()) {
                            vec->pop_back();
                        } else {
                            std::swap((*lm), vec->back());
                            // No need to hold onto the locks while we're
                            // freeing the limit managers.
                            destroyable_lms.push_back(std::move(vec->back()));
                            vec->pop_back();
                            break;
                        }
                    }
                    if (vec_it->second.size() == 0) {
                        it->second.limit_clients.erase(vec_it);
                    }
                    return;
                }
            }
        }
    }
}

void server_t::add_client(
        const client_t::addr_t &addr,
        region_t region,
        const auto_drainer_t::lock_t &keepalive) {
    keepalive.assert_is_holding(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    client_info_t *info = &clients[addr];

    // We do this regardless of whether there's already an entry for this
    // address, because we might be subscribed to multiple regions if we're
    // oversharded.  This will have to become smarter once you can unsubscribe
    // at finer granularity (i.e. when we support changefeeds on selections).
    info->regions.push_back(std::move(region));

    // The entry might already exist if we have multiple shards per btree, but
    // that's fine.
    if (!info->cond.has()) {
        info->stamp = 0;
        cond_t *stopped = new cond_t();
        info->cond.init(stopped);
        // Passing the raw pointer `stopped` is safe because `add_client_cb` is
        // the only function which can remove an entry from the map.
        // TODO: This `spawn_now_dangerously` could probably be a `spawn_sometime`.
        //   It used to be a `spawn_now_dangerously` because we used to acquire
        //   the auto drainer lock inside of `add_client_cb`, rather than passing
        //   `keepalive` in. This is no longer the case.
        //   We're keeping the `spawn_now_dangerously` for now to make sure that
        //   we don't introduce any subtle new bugs in 2.1.2.
        coro_t::spawn_now_dangerously(
            std::bind(&server_t::add_client_cb, this, stopped, addr, keepalive));
    }
}

void server_t::add_limit_client(
        const client_t::addr_t &addr,
        const region_t &region,
        const std::string &table,
        rdb_context_t *ctx,
        global_optargs_t optargs,
        auth::user_context_t user_context,
        const uuid_u &client_uuid,
        const keyspec_t::limit_t &spec,
        limit_order_t lt,
        std::vector<item_t> &&item_vec,
        const auto_drainer_t::lock_t &keepalive) {
    keepalive.assert_is_holding(&drainer);
    rwlock_in_line_t spot(&clients_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();
    auto it = clients.find(addr);

    // It's entirely possible the peer disconnected by the time we got here.
    if (it != clients.end()) {
        rwlock_in_line_t lspot(it->second.limit_clients_lock.get(), access_t::write);
        lspot.read_signal()->wait_lazily_unordered();
        auto *vec = &it->second.limit_clients[spec.range.sindex];
        auto lm = make_scoped<limit_manager_t>(
            &spot,
            region,
            table,
            ctx,
            std::move(optargs),
            std::move(user_context),
            client_uuid,
            this,
            it->first,
            spec,
            std::move(lt),
            std::move(item_vec));
        lspot.write_signal()->wait_lazily_unordered();
        vec->push_back(std::move(lm));
    }
}

void server_t::add_client_cb(
        signal_t *stopped,
        client_t::addr_t addr,
        auto_drainer_t::lock_t keepalive) {
    keepalive.assert_is_holding(&drainer);
    {
        disconnect_watcher_t disconnect(manager, addr.get_peer());
        wait_any_t wait_any(
            &disconnect, stopped, keepalive.get_drain_signal());
        wait_any.wait_lazily_unordered();
    }
    rwlock_in_line_t coro_spot(&clients_lock, access_t::write);
    coro_spot.read_signal()->wait_lazily_unordered();
    auto it = clients.find(addr);
    // We can be removed more than once safely (e.g. in the case of oversharding).
    if (it != clients.end()) {
        send_one_with_lock(&*it, msg_t(msg_t::stop_t()), keepalive);
    }
    coro_spot.write_signal()->wait_lazily_unordered();
    size_t erased = clients.erase(addr);
    // This is true even if we have multiple shards per btree because
    // `add_client` only spawns one of us.
    guarantee(erased == 1);
}

struct stamped_msg_t {
    stamped_msg_t() { }
    stamped_msg_t(uuid_u _server_uuid, uint64_t _stamp, msg_t _submsg)
        : server_uuid(std::move(_server_uuid)),
          stamp(_stamp),
          submsg(std::move(_submsg)) { }
    uuid_u server_uuid;
    uint64_t stamp;
    msg_t submsg;
};

RDB_MAKE_SERIALIZABLE_3(stamped_msg_t, server_uuid, stamp, submsg);

// This function takes a `lock_t` to make sure you have one.  (We can't just
// always acquire a drainer lock before sending because we sometimes send a
// `stop_t` during destruction, and you can't acquire a drain lock on a draining
// `auto_drainer_t`.)
void server_t::send_one_with_lock(
        std::pair<const client_t::addr_t, client_info_t> *client,
        msg_t msg,
        const auto_drainer_t::lock_t &keepalive) {
    keepalive.assert_is_holding(&drainer);
    uint64_t stamp;
    {
        // We don't need a write lock as long as we make sure the coroutine
        // doesn't block between reading and updating the stamp.
        ASSERT_NO_CORO_WAITING;
        stamp = client->second.stamp++;
    }
    send(manager, client->first, stamped_msg_t(uuid, stamp, std::move(msg)));
}

void server_t::send_all(
        const msg_t &msg,
        const store_key_t &key,
        rwlock_in_line_t *stamp_spot,
        const auto_drainer_t::lock_t &keepalive) {
    keepalive.assert_is_holding(&drainer);
    stamp_spot->guarantee_is_for_lock(&parent->cfeed_stamp_lock);
    stamp_spot->write_signal()->wait_lazily_unordered();

    rwlock_acq_t acq(&clients_lock, access_t::read);
    std::map<client_t::addr_t, uint64_t> stamps;
    for (auto &&pair : clients) {
        // We don't need a write lock as long as we make sure the coroutine
        // doesn't block between reading and updating the stamp.
        ASSERT_NO_CORO_WAITING;
        if (std::any_of(pair.second.regions.begin(),
                        pair.second.regions.end(),
                        std::bind(&region_contains_key, ph::_1, std::cref(key)))) {
            stamps[pair.first] = pair.second.stamp++;
        }
    }
    acq.reset();
    stamp_spot->reset(); // Done stamping, no need to hold onto it while we send.
    for (const auto &pair : stamps) {
        send(manager, pair.first, stamped_msg_t(uuid, pair.second, msg));
    }
}

server_t::addr_t server_t::get_stop_addr() {
    return stop_mailbox.get_address();
}

server_t::limit_addr_t server_t::get_limit_stop_addr() {
    return limit_stop_mailbox.get_address();
}

boost::optional<uint64_t> server_t::get_stamp(
        const client_t::addr_t &addr,
        const auto_drainer_t::lock_t &keepalive) {
    keepalive.assert_is_holding(&drainer);
    rwlock_acq_t stamp_acq(&parent->cfeed_stamp_lock, access_t::read);
    rwlock_acq_t client_acq(&clients_lock, access_t::read);
    auto it = clients.find(addr);
    if (it == clients.end()) {
        return boost::none;
    } else {
        return it->second.stamp;
    }
}

uuid_u server_t::get_uuid() {
    return uuid;
}

bool server_t::has_limit(
        const boost::optional<std::string> &sindex,
        const auto_drainer_t::lock_t &keepalive) {
    keepalive.assert_is_holding(&drainer);
    auto spot = make_scoped<rwlock_in_line_t>(&clients_lock, access_t::read);
    spot->read_signal()->wait_lazily_unordered();
    for (auto &&client : clients) {
        // We don't need a drainer lock here because we're still holding a read
        // lock on `clients_lock`.
        rwlock_in_line_t lspot(client.second.limit_clients_lock.get(), access_t::read);
        lspot.read_signal()->wait_lazily_unordered();
        client_info_t *info = &client.second;
        auto it = info->limit_clients.find(sindex);
        if (it != info->limit_clients.end()) {
            return true;
        }
    }
    return false;
}

auto_drainer_t::lock_t server_t::get_keepalive() {
    return drainer.lock();
}

void server_t::foreach_limit(
        const boost::optional<std::string> &sindex,
        const store_key_t *pkey,
        std::function<void(rwlock_in_line_t *,
                           rwlock_in_line_t *,
                           rwlock_in_line_t *,
                           limit_manager_t *)> f,
        const auto_drainer_t::lock_t &keepalive) THROWS_NOTHING {
    keepalive.assert_is_holding(&drainer);
    auto spot = make_scoped<rwlock_in_line_t>(&clients_lock, access_t::read);
    spot->read_signal()->wait_lazily_unordered();
    for (auto &&client : clients) {
        // We don't need a drainer lock here because we're still holding a read
        // lock on `clients_lock`.
        rwlock_in_line_t lspot(client.second.limit_clients_lock.get(), access_t::read);
        lspot.read_signal()->wait_lazily_unordered();
        client_info_t *info = &client.second;
        auto it = info->limit_clients.find(sindex);
        if (it == info->limit_clients.end()) {
            continue;
        }
        for (size_t i = 0; i < it->second.size(); ++i) {
            auto *lc = &it->second[i];
            guarantee((*lc).has());
            if ((*lc)->is_aborted()
                || (pkey && !(*lc)->region.inner.contains_key(*pkey))) {
                continue;
            }
            auto_drainer_t::lock_t lc_lock(&(*lc)->drainer);
            rwlock_in_line_t lc_spot(&(*lc)->lock, access_t::write);
            lc_spot.write_signal()->wait_lazily_unordered();
            try {
                f(spot.get(), &lspot, &lc_spot, (*lc).get());
            } catch (const exc_t &e) {
                (*lc)->abort(e);
                auto_drainer_t::lock_t sub_keepalive(keepalive);
                auto sub_spot = make_scoped<rwlock_in_line_t>(
                    &clients_lock, access_t::read);
                guarantee(sub_spot->read_signal()->is_pulsed());
                // We spawn immediately so it can steal our locks.
                coro_t::spawn_now_dangerously(
                    std::bind(&server_t::prune_dead_limit,
                              this, &sub_keepalive, &sub_spot, info, sindex, i));
                guarantee(!sub_keepalive.has_lock());
                guarantee(!sub_spot.has());
            }
        }
    }
}

// We do this stealable stuff rather than an rvalue reference because I can't
// find a way to pass one through `bind`, and you can't pass one through a
// lambda until C++14.
void server_t::prune_dead_limit(
    auto_drainer_t::lock_t *stealable_lock,
    scoped_ptr_t<rwlock_in_line_t> *stealable_clients_read_lock,
    client_info_t *info,
    boost::optional<std::string> sindex,
    size_t offset) {
    std::vector<scoped_ptr_t<limit_manager_t> > destroyable_lms;

    auto_drainer_t::lock_t lock;
    stealable_lock->assert_is_holding(&drainer);
    std::swap(lock, *stealable_lock);
    scoped_ptr_t<rwlock_in_line_t> spot;
    guarantee(stealable_clients_read_lock->has());
    std::swap(spot, *stealable_clients_read_lock);
    guarantee(spot->read_signal()->is_pulsed());
    rwlock_in_line_t lspot(info->limit_clients_lock.get(), access_t::write);
    lspot.write_signal()->wait_lazily_unordered();

    auto it = info->limit_clients.find(sindex);
    if (it == info->limit_clients.end()) {
        return;
    }
    auto *vec = &it->second;
    while (offset < vec->size()) {
        if (vec->back()->is_aborted()) {
            vec->pop_back();
        } else if ((*vec)[offset]->is_aborted()) {
            std::swap((*vec)[offset], vec->back());
            // No need to hold onto the locks while we're freeing the limit
            // managers.
            destroyable_lms.push_back(std::move(vec->back()));
            vec->pop_back();
            break;
        }
    }
    if (vec->size() == 0) {
        info->limit_clients.erase(sindex);
    }
}

limit_order_t::limit_order_t(sorting_t _sorting)
    : sorting(std::move(_sorting)) {
    rcheck_toplevel(
        sorting != sorting_t::UNORDERED, base_exc_t::LOGIC,
        "Cannot get changes on the first elements of an unordered stream.");
}

// Produes a primary key + tag pair, mangled so that it sorts correctly and can
// be safely stored in a datum.
std::string key_to_mangled_primary(store_key_t store_key, is_primary_t is_primary) {
    std::string s, raw_str = key_to_unescaped_str(store_key);
    components_t components;
    if (is_primary == is_primary_t::YES) {
        components.primary = raw_str; // No tag.
    } else {
        components = datum_t::extract_all(raw_str);
    }
    for (auto c : components.primary) {
        // We escape 0 because there are places where we don't support NULL
        // bytes, 1 because we use it to delineate the start of the tag, and 2
        // because it's the escape character.
        if (c == 0 || c == 1 || c == 2) {
            s.push_back(2);
            s.push_back(c+2);
        } else {
            s.push_back(c);
        }
    }
    s.push_back(1); // Append regardless of whether there's a tag.
    if (components.tag_num) {
        uint64_t u = *components.tag_num;
        // Shamelessly stolen from datum.cc.
        s += strprintf("%.*" PRIx64, static_cast<int>(sizeof(uint64_t) * 2), u);
    }
    return s;
}

store_key_t mangled_primary_to_pkey(const std::string &s) {
    guarantee(s.size() > 0);
    guarantee(s[s.size() - 1] == 1);
    std::string pkey;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == 2) {
            guarantee(i != s.size() - 1);
            pkey.push_back(s[++i]-2);
        } else if (s[i] == 1) {
            // Because we're unmangling a pkey, we know there's no tag.
            guarantee(i == s.size() - 1);
        } else {
            guarantee(i != s.size() - 1);
            pkey.push_back(s[i]);
        }
    }
    return store_key_t(pkey);
}

sorting_t flip(sorting_t sorting) {
    switch (sorting) {
    case sorting_t::ASCENDING: return sorting_t::DESCENDING;
    case sorting_t::DESCENDING: return sorting_t::ASCENDING;
    case sorting_t::UNORDERED: // fallthru
    default: unreachable();
    }
    unreachable();
}

std::vector<item_t> mangle_sort_truncate_stream(
    raw_stream_t &&stream, is_primary_t is_primary, sorting_t sorting, size_t n) {
    std::vector<item_t> vec;
    vec.reserve(stream.size());
    for (auto &&item : stream) {
        guarantee(is_primary ==
                  (item.sindex_key.has() ? is_primary_t::NO : is_primary_t::YES));
        vec.push_back(
            std::make_pair(
                key_to_mangled_primary(item.key, is_primary),
                std::make_pair(
                    is_primary == is_primary_t::YES ? datum_t::null() : item.sindex_key,
                    item.data)));
    }
    // We only need to sort to resolve truncated sindexes.
    if (is_primary == is_primary_t::NO) {
        // Note that we flip the sorting.  This is intentional, because we want to
        // drop the "highest" elements even though we usually use the sorting to put
        // the lowest elements at the back.
        std::sort(vec.begin(), vec.end(), limit_order_t(flip(sorting)));
    }
    if (vec.size() > n) {
        vec.resize(n);
    }
    return vec;
}

bool limit_order_t::subop(
    const std::string &a_str, const std::pair<datum_t, datum_t> &a_pair,
    const std::string &b_str, const std::pair<datum_t, datum_t> &b_pair) const {
    int cmp = a_pair.first.cmp(b_pair.first);
    switch (sorting) {
    case sorting_t::ASCENDING:
        return cmp > 0 ? true : ((cmp < 0) ? false : (*this)(a_str, b_str));
    case sorting_t::DESCENDING:
        return cmp < 0 ? true : ((cmp > 0) ? false : (*this)(a_str, b_str));
    case sorting_t::UNORDERED: // fallthru
    default: unreachable();
    }
    unreachable();
}

bool limit_order_t::operator()(const item_t &a, const item_t &b) const {
    return subop(a.first, a.second, b.first, b.second);
}

bool limit_order_t::operator()(const const_item_t &a, const const_item_t &b) const {
    return subop(a.first, a.second, b.first, b.second);
}

bool limit_order_t::operator()(const datum_t &a, const datum_t &b) const {
    int cmp = a.cmp(b);
    switch (sorting) {
    case sorting_t::ASCENDING: return cmp > 0;
    case sorting_t::DESCENDING: return cmp < 0;
    case sorting_t::UNORDERED: // fallthru
    default: unreachable();
    }
    unreachable();
}

bool limit_order_t::operator()(const std::string &a, const std::string &b) const {
    switch (sorting) {
    case sorting_t::ASCENDING: return a > b;
    case sorting_t::DESCENDING: return a < b;
    case sorting_t::UNORDERED: // fallthru
    default: unreachable();
    }
    unreachable();
}

void limit_manager_t::send(msg_t &&msg) {
    if (!parent->drainer.is_draining()) {
        auto_drainer_t::lock_t drain_lock(&parent->drainer);
        auto it = parent->clients.find(parent_client);
        guarantee(it != parent->clients.end());
        parent->send_one_with_lock(&*it, std::move(msg), drain_lock);
    }
}

limit_manager_t::limit_manager_t(
    rwlock_in_line_t *clients_lock,
    region_t _region,
    std::string _table,
    rdb_context_t *ctx,
    global_optargs_t optargs,
    auth::user_context_t user_context,
    uuid_u _uuid,
    server_t *_parent,
    client_t::addr_t _parent_client,
    keyspec_t::limit_t _spec,
    limit_order_t _gt,
    std::vector<item_t> &&item_vec)
    : region(std::move(_region)),
      table(std::move(_table)),
      uuid(std::move(_uuid)),
      parent(_parent),
      parent_client(std::move(_parent_client)),
      spec(std::move(_spec)),
      gt(std::move(_gt)),
      item_queue(gt),
      aborted(false) {
    guarantee(clients_lock->read_signal()->is_pulsed());

    // The final `NULL` argument means we don't profile any work done with this `env`.
    env = make_scoped<env_t>(
        ctx,
        return_empty_normal_batches_t::NO,
        drainer.get_drain_signal(),
        std::move(optargs),
        std::move(user_context),
        nullptr);

    guarantee(ops.size() == 0);
    for (const auto &transform : spec.range.transforms) {
        ops.push_back(make_op(transform));
    }

    guarantee(item_queue.size() == 0);
    for (const auto &pair : item_vec) {
        bool inserted = item_queue.insert(pair).second;
        guarantee(inserted);
    }
    send(msg_t(msg_t::limit_start_t(uuid, std::move(item_vec))));
}

void limit_manager_t::add(
    rwlock_in_line_t *spot,
    store_key_t sk,
    is_primary_t is_primary,
    datum_t key,
    datum_t val) THROWS_NOTHING {
    guarantee(spot->write_signal()->is_pulsed());
    guarantee((is_primary == is_primary_t::NO) == static_cast<bool>(spec.range.sindex));
    if ((is_primary == is_primary_t::YES && region.inner.contains_key(sk))
        || (is_primary == is_primary_t::NO && spec.range.datumspec.copies(key) != 0)) {
        if (boost::optional<datum_t> d = apply_ops(val, ops, env.get(), key)) {
            auto pair = added.insert(
                std::make_pair(
                    key_to_mangled_primary(sk, is_primary),
                    std::make_pair(std::move(key), *d)));
            guarantee(pair.second);
        }
    }
}

void limit_manager_t::del(
    rwlock_in_line_t *spot,
    store_key_t sk,
    is_primary_t is_primary) THROWS_NOTHING {
    guarantee(spot->write_signal()->is_pulsed());
    std::string key = key_to_mangled_primary(sk, is_primary);
    size_t erased = added.erase(key);
    // Note that we don't actually have to check whether or not the thing we're
    // deleting matches any predicates that might be in the operations, because
    // we already have to handle the case where we're deleting something that
    // isn't in the top `n`, so trying to delete too much doesn't hurt anything.
    if (erased == 0) {
        auto pair = deleted.insert(std::move(key));
        guarantee(pair.second);
    }
}

class ref_visitor_t : public boost::static_visitor<std::vector<item_t>> {
public:
    ref_visitor_t(env_t *_env,
                  std::vector<scoped_ptr_t<op_t> > *_ops,
                  const key_range_t *_pk_range,
                  const keyspec_t::limit_t *_spec,
                  sorting_t _sorting,
                  boost::optional<item_t> _start,
                  const item_queue_t *_item_queue)
        : env(_env),
          ops(_ops),
          pk_range(_pk_range),
          spec(_spec),
          sorting(_sorting),
          start(std::move(_start)),
          item_queue(_item_queue) { }

    std::vector<item_t> operator()(const primary_ref_t &ref) {
        rget_read_response_t resp;
        key_range_t range = *pk_range;
        switch (sorting) {
        case sorting_t::ASCENDING: {
            if (start) {
                store_key_t start_key = mangled_primary_to_pkey(start->first);
                start_key.increment(); // open bound
                range.left = std::move(start_key);
            }
        } break;
        case sorting_t::DESCENDING: {
            if (start) {
                store_key_t start_key = mangled_primary_to_pkey(start->first);
                // right bound is open by default
                range.right = key_range_t::right_bound_t(std::move(start_key));
            }
        } break;
        case sorting_t::UNORDERED: // fallthru
        default: unreachable();
        }
        size_t n = spec->limit - item_queue->size();
        rdb_rget_slice(
            ref.btree,
            region_t(),
            range,
            boost::none,
            ref.superblock,
            env,
            batchspec_t::all(),
            std::vector<transform_variant_t>(),
            boost::optional<terminal_variant_t>(
                limit_read_t{
                    is_primary_t::YES,
                    n,
                    // This code uses the same generic code path as a normal
                    // read, and a normal read needs to keep track of the
                    // region and last seen key for unsharding, but we
                    // discard this information from the response later so
                    // it can be whatever we want.
                    region_t(),
                    store_key_t::min(),
                    sorting,
                    ops}),
            sorting,
            &resp,
            release_superblock_t::KEEP);
        auto *gs = boost::get<ql::grouped_t<ql::stream_t> >(&resp.result);
        if (gs == NULL) {
            auto *exc = boost::get<ql::exc_t>(&resp.result);
            guarantee(exc != NULL);
            throw *exc;
        }
        stream_t stream = groups_to_batch(gs->get_underlying_map());
        guarantee(stream.substreams.size() <= 1);
        std::vector<item_t> item_vec;
        if (stream.substreams.size() == 1) {
            raw_stream_t *raw_stream = &stream.substreams.begin()->second.stream;
            guarantee(raw_stream->size() <= n);
            item_vec = mangle_sort_truncate_stream(
                std::move(*raw_stream), is_primary_t::YES, sorting, n);
        } else {
            guarantee(item_vec.size() == 0);
        }
        return item_vec;
    }

    std::vector<item_t> operator()(const sindex_ref_t &ref) {
        rget_read_response_t resp;
        guarantee(spec->range.sindex);
        // `.limit().changes()` is currently only allowed on a single range
        r_sanity_check(
            spec->range.datumspec.visit<bool>(
                [](const datum_range_t &) { return true; },
                [](const std::map<datum_t, uint64_t> &) { return false; }));
        datum_range_t srange = spec->range.datumspec.covering_range();
        size_t n = spec->limit - item_queue->size();
        if (start) {
            datum_t dstart = start->second.first;
            switch (sorting) {
            case sorting_t::ASCENDING:
                srange = srange.with_left_bound(dstart, key_range_t::bound_t::closed);
                break;
            case sorting_t::DESCENDING:
                srange = srange.with_right_bound(dstart, key_range_t::bound_t::closed);
                break;
            case sorting_t::UNORDERED: // fallthru
            default: unreachable();
            }

            // Because we're using closed bounds, we have to make sure to read enough.
            for (const auto &pair : *item_queue) {
                if (pair->second.first != dstart) {
                    break;
                }
                n += 1;
            }
        }
        reql_version_t reql_version =
            ref.sindex_info->mapping_version_info.latest_compatible_reql_version;
        rdb_rget_secondary_slice(
            ref.btree,
            region_t(),
            ql::datumspec_t(srange),
            srange.to_sindex_keyrange(reql_version),
            ref.superblock,
            env,
            batchspec_t::all(), // Terminal takes care of early termination
            std::vector<transform_variant_t>(),
            boost::optional<terminal_variant_t>(limit_read_t{
                    is_primary_t::NO,
                    n,
                    // This code uses the same generic code path as a normal
                    // read, and a normal read needs to keep track of the
                    // region and last seen key for unsharding, but we
                    // discard this information from the response later so
                    // it can be whatever we want.
                    region_t(),
                    store_key_t::min(),
                    sorting,
                    ops}),
            *pk_range,
            sorting,
            require_sindexes_t::NO,
            *ref.sindex_info,
            &resp,
            release_superblock_t::KEEP);
        auto *gs = boost::get<ql::grouped_t<ql::stream_t> >(&resp.result);
        if (gs == NULL) {
            auto *exc = boost::get<ql::exc_t>(&resp.result);
            guarantee(exc != NULL);
            throw *exc;
        }
        stream_t stream = groups_to_batch(gs->get_underlying_map());
        guarantee(stream.substreams.size() <= 1);
        std::vector<item_t> item_vec;
        if (stream.substreams.size() == 1) {
            raw_stream_t *raw_stream = &stream.substreams.begin()->second.stream;
            item_vec = mangle_sort_truncate_stream(
                std::move(*raw_stream), is_primary_t::NO, sorting, n);
        } else {
            guarantee(item_vec.size() == 0);
        }
        return item_vec;
    }

private:
    env_t *env;
    std::vector<scoped_ptr_t<op_t> > *ops;
    const key_range_t *pk_range;
    const keyspec_t::limit_t *spec;
    sorting_t sorting;
    boost::optional<item_t> start;
    const item_queue_t *item_queue;
};

std::vector<item_t> limit_manager_t::read_more(
    const boost::variant<primary_ref_t, sindex_ref_t> &ref,
    const boost::optional<item_t> &start) {
    guarantee(item_queue.size() < spec.limit);
    ref_visitor_t visitor(
        env.get(), &ops, &region.inner, &spec, spec.range.sorting, start, &item_queue);
    return boost::apply_visitor(visitor, ref);
}

void limit_manager_t::commit(
    rwlock_in_line_t *spot,
    const boost::variant<primary_ref_t, sindex_ref_t> &sindex_ref) THROWS_NOTHING {
    guarantee(spot->write_signal()->is_pulsed());
    if (added.size() == 0 && deleted.size() == 0) {
        return;
    }

    // Before we delete anything, we get the boundary between the active set and
    // the data that didn't make it into the set.  Anything <= that according to
    // our ordering could never be kicked out of the set because of a read from
    // disk.
    boost::optional<item_t> active_boundary;
    auto item_queue_it = item_queue.begin();
    if (item_queue_it != item_queue.end()) {
        active_boundary = **item_queue_it;
    }

    item_queue_t real_added(gt);
    std::set<std::string> real_deleted;
    for (const auto &id : deleted) {
        bool data_deleted = item_queue.del_id(id);
        if (data_deleted) {
            bool inserted = real_deleted.insert(id).second;
            guarantee(inserted);
        }
    }
    deleted.clear();
    bool added_on_disk = false;
    for (const auto &pair : added) {
        // We only add to the set if we know we beat anything that might be read
        // off of disk below.  This is fine because if the resulting set is
        // still too small, and the things we didn't add happen to beat the
        // other things in the table, we'll read them first.
        if (!(active_boundary && gt(item_t(pair), *active_boundary))) {
            bool inserted = item_queue.insert(pair).second;
            // We can never get two additions for the same key without a deletion
            // in-between.
            guarantee(inserted);
            inserted = real_added.insert(pair).second;
            guarantee(inserted);
        } else {
            added_on_disk = true;
        }
    }
    added.clear();

    std::vector<std::string> truncated = item_queue.truncate_top(spec.limit);
    for (auto &&id : truncated) {
        auto it = real_added.find_id(id);
        if (it != real_added.end()) {
            real_added.erase(it);
        } else {
            bool inserted = real_deleted.insert(std::move(id)).second;
            guarantee(inserted);
        }
    }

    bool anything_on_disk = real_deleted.size() != 0 || added_on_disk;
    if (item_queue.size() < spec.limit && anything_on_disk) {
        std::vector<item_t> s;
        boost::optional<exc_t> exc;
        try {
            s = read_more(sindex_ref, active_boundary);
        } catch (const exc_t &e) {
            exc = e;
        }
        // We need to do it this way because we can't do anything
        // coroutine-related in an exception handler.
        if (exc) {
            abort(*exc);
            return;
        }
        for (auto &&pair : s) {
            bool inserted = item_queue.insert(pair).second;
            // Reading duplicates from disk is fine.
            if (inserted) {
                bool added_insert = real_added.insert(pair).second;
                guarantee(added_insert);
            }
        }
        // We need to truncate again because `read_more` may read too much in
        // the secondary index case.
        std::vector<std::string> read_trunc = item_queue.truncate_top(spec.limit);
        for (auto &&id : read_trunc) {
            auto it = real_added.find_id(id);
            if (it != real_added.end()) {
                real_added.erase(it);
            } else {
                bool inserted = real_deleted.insert(std::move(id)).second;
                guarantee(inserted);
            }
        }
    }
    std::set<std::string> remaining_deleted;
    for (auto &&id : real_deleted) {
        auto it = real_added.find_id(id);
        if (it != real_added.end()) {
            msg_t::limit_change_t msg;
            msg.sub = uuid;
            msg.old_key = id;
            msg.new_val = std::move(**it);
            real_added.erase(it);
            send(msg_t(std::move(msg)));
        } else {
            remaining_deleted.insert(std::move(id));
        }
    }
    real_deleted.clear();

    for (const auto &id : remaining_deleted) {
        msg_t::limit_change_t msg;
        msg.sub = uuid;
        msg.old_key = id;
        auto it = real_added.begin();
        if (it != real_added.end()) {
            msg.new_val = std::move(**it);
            real_added.erase(it);
        }
        send(msg_t(std::move(msg)));
    }

    for (auto &&it : real_added) {
        msg_t::limit_change_t msg;
        msg.sub = uuid;
        msg.new_val = std::move(*it);
        send(msg_t(std::move(msg)));
    }
    real_added.clear();
}

void limit_manager_t::abort(exc_t e) {
    aborted = true;
    send(msg_t(msg_t::limit_stop_t{uuid, std::move(e)}));
}

RDB_IMPL_SERIALIZABLE_1_SINCE_v1_13(msg_t, op);
RDB_IMPL_SERIALIZABLE_2(msg_t::limit_start_t, sub, start_data);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(msg_t::limit_start_t);
RDB_IMPL_SERIALIZABLE_3(msg_t::limit_change_t, sub, old_key, new_val);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(msg_t::limit_change_t);
RDB_IMPL_SERIALIZABLE_2(msg_t::limit_stop_t, sub, exc);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(msg_t::limit_stop_t);
RDB_IMPL_SERIALIZABLE_5(
    msg_t::change_t,
    old_indexes, new_indexes, pkey, old_val, new_val);
INSTANTIATE_SERIALIZABLE_FOR_CLUSTER(msg_t::change_t);
RDB_IMPL_SERIALIZABLE_0_SINCE_v1_13(msg_t::stop_t);

enum class detach_t { NO, YES };

enum class state_t {
    NONE = 0,
    INITIALIZING = 1,
    READY = 2
};

datum_t initializing_datum() {
    return datum_t(
        std::map<datum_string_t, datum_t>{
            { datum_string_t("state"), datum_t("initializing") }});
}

datum_t ready_datum() {
    return datum_t(
        std::map<datum_string_t, datum_t>{
            { datum_string_t("state"), datum_t("ready") }});
}

datum_t state_datum(state_t state) {
    switch (state) {
    case state_t::INITIALIZING: return initializing_datum();
    case state_t::READY: return ready_datum();
    case state_t::NONE: // fallthru
    default: unreachable();
    }
    unreachable();
}

template<class Sub>
class stream_t : public eager_datum_stream_t {
public:
    template<class... Args>
    stream_t(scoped_ptr_t<Sub> &&_sub, Args &&... args)
        : eager_datum_stream_t(std::forward<Args>(args)...),
          sub(std::move(_sub)) { }
    virtual bool is_array() const { return false; }
    virtual bool is_exhausted() const { return false; }
    void set_notes(response_t *res) const final { sub->set_notes(res); }
    feed_type_t cfeed_type() const final { return sub->cfeed_type(); }
    virtual bool is_infinite() const { return true; }
    virtual std::vector<datum_t>
    next_raw_batch(env_t *env, const batchspec_t &bs) {
        rcheck(bs.get_batch_type() == batch_type_t::NORMAL
               || bs.get_batch_type() == batch_type_t::NORMAL_FIRST,
               base_exc_t::LOGIC,
               "Cannot call a terminal (`reduce`, `count`, etc.) on an "
               "infinite stream (such as a changefeed).");
        return next_stream_batch(env, bs);
    }
protected:
    scoped_ptr_t<Sub> sub;
    virtual std::vector<datum_t> next_stream_batch(env_t *env, const batchspec_t &bs) {
        batcher_t batcher = bs.to_batcher();
        return sub->get_els(&batcher,
                            env->return_empty_normal_batches,
                            env->interruptor);
    }
};

enum class change_type_t {
    ADD = 0,
    REMOVE = 1,
    CHANGE = 2,
    INITIAL = 3,
    UNINITIAL = 4,
    STATE = 5
};

// Uses the home thread of the subscriber, not the client.
class feed_t;
class subscription_t : public home_thread_mixin_t {
public:
    virtual ~subscription_t();
    virtual feed_type_t cfeed_type() const = 0;
    void set_notes(response_t *res) const;
    std::vector<datum_t> get_els(
        batcher_t *batcher,
        return_empty_normal_batches_t return_empty_normal_batches,
        const signal_t *interruptor);
    void stop(std::exception_ptr exc, detach_t should_detach);
    virtual counted_t<datum_stream_t> to_stream(
        env_t *env,
        std::string table,
        namespace_interface_t *nif,
        const client_t::addr_t &addr,
        counted_t<datum_stream_t> maybe_src,
        scoped_ptr_t<subscription_t> &&self,
        backtrace_id_t bt) = 0;
    virtual counted_t<datum_stream_t> to_artificial_stream(
        const uuid_u &uuid,
        const std::string &primary_key_name,
        const std::vector<datum_t> &initial_vals,
        bool include_initial,
        scoped_ptr_t<subscription_t> &&self,
        backtrace_id_t bt) = 0;
    virtual auto_drainer_t *get_drainer() = 0;
    feed_t *parent_feed() {
        if (feed != nullptr) {
            return feed;
        } else {
            guarantee(exc);
            std::rethrow_exception(exc);
        }
    }
protected:
    subscription_t(rdb_context_t *rdb_context,
                   const auth::user_context_t &user_context,
                   feed_t *feed,
                   configured_limits_t limits,
                   const datum_t &squash,
                   bool include_states,
                   bool include_types);
    void maybe_signal_cond() THROWS_NOTHING;
    void maybe_signal_queue_nearly_full_cond() THROWS_NOTHING;
    void destructor_cleanup(std::function<void()> del_sub) THROWS_NOTHING;

    datum_t maybe_add_type(datum_t &&datum, change_type_t type);
    // If an error occurs, we're detached and `exc` is set to an exception to rethrow.
    std::exception_ptr exc;
    // If we exceed the array size limit, elements are evicted from `els` and
    // `skipped` is incremented appropriately.  If `skipped` is non-0, we send
    // an error object to the user with the number of skipped elements before
    // continuing.
    size_t skipped;
    feed_t *feed; // The feed we're subscribed to.
    const configured_limits_t limits;
    const bool squash; // Whether or not to squash changes.
    const bool include_states; // Whether or not to include notes about the state.
    const bool include_types; // Whether or not to include a type field in items.
    // Whether we're in the middle of one logical batch (only matters for squashing).
    bool mid_batch;
private:
    friend class splice_stream_t;
    const double min_interval;

    virtual bool has_el() = 0;
    virtual datum_t pop_el() = 0;
    virtual void apply_queued_changes() = 0;

    rdb_context_t *rdb_context;
    auth::user_context_t user_context;

    // Used to block on more changes.  NULL unless we're waiting.
    cond_t *cond;
    cond_t *queue_nearly_full_cond;
    DISABLE_COPYING(subscription_t);
};

datum_string_t type_to_string(change_type_t type) {
    datum_string_t type_string;
    switch (type) {
    case change_type_t::ADD:
        type_string = datum_string_t("add");
        break;
    case change_type_t::REMOVE:
        type_string = datum_string_t("remove");
        break;
    case change_type_t::CHANGE:
        type_string = datum_string_t("change");
        break;
    case change_type_t::INITIAL:
        type_string = datum_string_t("initial");
        break;
    case change_type_t::UNINITIAL:
        type_string = datum_string_t("uninitial");
        break;
    case change_type_t::STATE:
        type_string = datum_string_t("state");
        break;
    default:
        unreachable();
    }

    return type_string;
}
datum_t add_type(datum_t &&datum, change_type_t type) {
    datum_string_t type_string = type_to_string(type);
    return datum.merge(
        datum_t{
            std::map<datum_string_t, datum_t>{
                std::pair<datum_string_t, datum_t>{
                    datum_string_t("type"),
                        datum_t(type_string)}}});
}

datum_t subscription_t::maybe_add_type(datum_t &&datum, change_type_t type) {
    if (!include_types) {
        return std::move(datum);
    }
    return add_type(std::move(datum), type);
}


datum_t vals_to_change(
    datum_t old_val,
    datum_t new_val,
    bool discard_old_val = false,
    bool discard_new_val = false,
    bool include_type = false,
    bool include_offsets = false,
    boost::optional<size_t> old_offset = boost::none,
    boost::optional<size_t> new_offset = boost::none) {
    change_type_t change_type;

    if (discard_old_val && !discard_new_val) {
        change_type = change_type_t::INITIAL;
        old_val = datum_t::null();
    } else if (!discard_old_val && discard_new_val) {
        change_type = change_type_t::UNINITIAL;
        new_val = datum_t::null();
    } else if (!discard_old_val
               && old_val.get_type() == datum_t::R_NULL) {
        change_type = change_type_t::ADD;
    } else if (!discard_new_val
               && new_val.get_type() == datum_t::R_NULL) {
        change_type = change_type_t::REMOVE;
    } else {
        // Either it's a change, or we're about to return.
        change_type = change_type_t::CHANGE;
    }
    // Status type is handled where statuses are generated.

    if ((discard_old_val || old_val.get_type() == datum_t::R_NULL)
        && (discard_new_val || new_val.get_type() == datum_t::R_NULL)) {
        return datum_t();
    } else {
        std::map<datum_string_t, datum_t> ret;
        if (!discard_old_val) {
            ret[datum_string_t("old_val")] = std::move(old_val);
            if (include_offsets) {
                ret[datum_string_t("old_offset")] = old_offset
                    ? datum_t(static_cast<double>(*old_offset))
                    : datum_t::null();
            }
        }
        if (!discard_new_val) {
            ret[datum_string_t("new_val")] = std::move(new_val);
            if (include_offsets) {
                ret[datum_string_t("new_offset")] = new_offset
                    ? datum_t(static_cast<double>(*new_offset))
                    : datum_t::null();
            }
        }
        guarantee(ret.size() != 0);

        if (include_type) {
            ret[datum_string_t("type")] =
                datum_t(
                    type_to_string(change_type));
        }
        datum_t ret_datum = datum_t(std::move(ret));
        return ret_datum;
    }
}

datum_t change_val_to_change(
    const change_val_t &change,
    bool discard_old_val = false,
    bool discard_new_val = false,
    bool include_type = false) {
    datum_t res = vals_to_change(
        change.old_val ? change.old_val->val : datum_t::null(),
        change.new_val ? change.new_val->val : datum_t::null(),
        discard_old_val,
        discard_new_val,
        include_type);
    return res;
}

enum class init_squashing_queue_t { NO, YES };
class flat_sub_t : public subscription_t {
public:
    template<class... Args>
    explicit flat_sub_t(init_squashing_queue_t init_squashing_queue, Args &&... args)
        : subscription_t(std::forward<Args>(args)...),
          last_stamp(std::make_pair(nil_uuid(), std::numeric_limits<uint64_t>::max())) {
        if (init_squashing_queue == init_squashing_queue_t::YES && squash) {
            queue = make_scoped<squashing_queue_t>();
        } else {
            queue = make_scoped<nonsquashing_queue_t>();
        }
    }
    virtual void add_el(
        const uuid_u &shard_uuid,
        uint64_t stamp,
        const store_key_t &pkey,
        const boost::optional<std::string> &DEBUG_ONLY(sindex),
        boost::optional<indexed_datum_t> old_val,
        boost::optional<indexed_datum_t> new_val) {
        if (!active()) return;
        auto stamp_pair = std::make_pair(shard_uuid, stamp);
        if (stamp_pair == last_stamp || update_stamp(shard_uuid, stamp)) {
            // If we get the same stamp multiple times in a row, we skip the
            // update step and always pass it through.  (This supports cases
            // like `.get_all(1, 1)`).
            last_stamp = stamp_pair;
            queue->add(change_val_t(
                std::make_pair(shard_uuid, stamp),
                pkey,
                old_val,
                new_val
                DEBUG_ONLY(, sindex)));
            if (queue->size() > limits.changefeed_queue_size()) {
                skipped += queue->size();
                queue->clear();
            } else if (queue->size() > limits.changefeed_queue_size() / 2) {
                // We do this even if the queue is only half full because we
                // expect it to take some time to process and we want to be
                // super safe.  (This will only affect anything if your `squash`
                // timer is super long, in which case a more aggressive upper
                // limit would let us respect the `squash` timer more closely,
                // but since the timer is a hint it's OK to be safe in this edge
                // case.)
                maybe_signal_queue_nearly_full_cond();
            }
            maybe_signal_cond();
        }
    }
    bool has_change_val() { return queue->size() != 0; }
    change_val_t pop_change_val() { return queue->pop(); }
    const change_val_t &peek_change_val() { return queue->peek(); }
    bool active() { return !exc; }
protected:
    // The queue of changes we've accumulated since the last time we were read from.
    scoped_ptr_t<maybe_squashing_queue_t> queue;
private:
    std::pair<uuid_u, uint64_t> last_stamp;
    virtual void apply_queued_changes() { } // Changes are never queued.
    virtual bool update_stamp(const uuid_u &uuid, uint64_t new_stamp) = 0;
};

class range_sub_t;
class empty_sub_t;
class point_sub_t;
class limit_sub_t;

class feed_t : public home_thread_mixin_t, public slow_atomic_countable_t<feed_t> {
public:
    feed_t(namespace_id_t const &, table_meta_client_t *);
    virtual ~feed_t();

    void add_point_sub(point_sub_t *sub, const store_key_t &key) THROWS_NOTHING;
    void del_point_sub(point_sub_t *sub, const store_key_t &key) THROWS_NOTHING;

    void add_range_sub(range_sub_t *sub) THROWS_NOTHING;
    void del_range_sub(range_sub_t *sub) THROWS_NOTHING;

    void add_empty_sub(empty_sub_t *sub) THROWS_NOTHING;
    void del_empty_sub(empty_sub_t *sub) THROWS_NOTHING;

    void add_limit_sub(limit_sub_t *sub, const uuid_u &uuid) THROWS_NOTHING;
    void del_limit_sub(limit_sub_t *sub, const uuid_u &uuid) THROWS_NOTHING;

    void each_range_sub(const auto_drainer_t::lock_t &lock,
                        const std::function<void(range_sub_t *)> &f) THROWS_NOTHING;
    void update_stamps(uuid_u server_uuid, uint64_t stamp);
    std::map<uuid_u, uint64_t> get_stamps();
    void on_point_sub(
        store_key_t key,
        const auto_drainer_t::lock_t &lock,
        const std::function<void(point_sub_t *)> &f) THROWS_NOTHING;
    void on_limit_sub(
        const uuid_u &uuid,
        const auto_drainer_t::lock_t &lock,
        const std::function<void(limit_sub_t *)> &f) THROWS_NOTHING;

    bool can_be_removed();

    virtual void abort_feed() = 0;
    void stop_subs(const auto_drainer_t::lock_t &lock);
    void mark_detached() { detached = true; }

    const std::string pkey;
    virtual auto_drainer_t::lock_t get_drainer_lock() = 0;

    namespace_id_t const &get_table_id() const {
        return table_id;
    }

    table_meta_client_t *get_table_meta_client() const {
        return table_meta_client;
    }
protected:
    bool detached;
    int64_t num_subs;
private:
    virtual void maybe_remove_feed() = 0;
    virtual void stop_limit_sub(limit_sub_t *sub) = 0;

    void add_sub_with_lock(
        rwlock_t *rwlock, const std::function<void()> &f) THROWS_NOTHING;
    void del_sub_with_lock(
        rwlock_t *rwlock, const std::function<size_t()> &f) THROWS_NOTHING;

    template<class Sub>
    void each_sub_in_vec(
        const std::vector<std::set<Sub *> > &vec,
        rwlock_in_line_t *spot,
        const auto_drainer_t::lock_t &lock,
        const std::function<void(Sub *)> &f) THROWS_NOTHING;
    template<class Sub>
    void each_sub_in_vec_cb(const std::function<void(Sub *)> &f,
                            const std::vector<std::set<Sub *> > &vec,
                            const std::vector<int> &sub_threads,
                            int i);
    void each_point_sub_cb(const std::function<void(point_sub_t *)> &f, int i);
    void each_point_sub_with_lock(
        rwlock_in_line_t *spot,
        const std::function<void(point_sub_t *)> &f) THROWS_NOTHING;
    void each_limit_sub_cb(const std::function<void(limit_sub_t *)> &f, int i);
    void each_limit_sub_with_lock(
        rwlock_in_line_t *spot,
        const std::function<void(limit_sub_t *)> &f) THROWS_NOTHING;

    std::map<store_key_t, std::vector<std::set<point_sub_t *> > > point_subs;
    rwlock_t point_subs_lock;
    std::vector<std::set<empty_sub_t *> > empty_subs;
    rwlock_t empty_subs_lock;
    std::vector<std::set<range_sub_t *> > range_subs;
    rwlock_t range_subs_lock;
    std::map<uuid_u, std::vector<std::set<limit_sub_t *> > > limit_subs;
    rwlock_t limit_subs_lock;

    // This stores the latest stamps we've received.  It's OK for this to
    // be a tiny bit behind what we've sent the subs.
    struct stamps_t {
        rwlock_t lock;
        std::map<uuid_u, uint64_t> latest;
    };
    // We use a `one_per_thread_t` because when we have lots of subs (the
    // expensive case), it's easier to fill this in multiple times than to have
    // every sub do a thread switch to read the value.
    one_per_thread_t<stamps_t> stamps;

    namespace_id_t table_id;
    table_meta_client_t *table_meta_client;
};

void feed_t::update_stamps(uuid_u server_uuid, uint64_t stamp) {
    pmap(get_num_threads(),
         [&](int thread) {
             on_thread_t th((threadnum_t(thread)));
             stamps_t *rs = stamps.get();
             rwlock_acq_t acq(&rs->lock, access_t::write);
             rassert(stamp >= rs->latest[server_uuid]);
             rs->latest[server_uuid] = stamp;
         });
}

// We have to return by value here because we release the lock right away.
std::map<uuid_u, uint64_t> feed_t::get_stamps() {
    stamps_t *rs = stamps.get();
    rwlock_acq_t acq(&rs->lock, access_t::read);
    return rs->latest;
}

class real_feed_t : public feed_t {
public:
    real_feed_t(auto_drainer_t::lock_t client_lock,
                client_t *client,
                mailbox_manager_t *manager,
                namespace_interface_t *ns_if,
                namespace_id_t const &table_id,
                signal_t *interruptor,
                table_meta_client_t *table_meta_client);
    ~real_feed_t();

    client_t::addr_t get_addr() const;
    void abort_feed() final { aborted.pulse_if_not_already_pulsed(); }
    virtual auto_drainer_t::lock_t get_drainer_lock() { return drainer.lock(); }
private:
    virtual void maybe_remove_feed() { client->maybe_remove_feed(client_lock, table_id); }
    virtual void stop_limit_sub(limit_sub_t *sub);

    void mailbox_cb(signal_t *interruptor, stamped_msg_t msg);
    void constructor_cb();

    auto_drainer_t::lock_t client_lock;
    client_t *client;
    namespace_id_t table_id;
    mailbox_manager_t *manager;
    mailbox_t<void(stamped_msg_t)> mailbox;
    std::vector<server_t::addr_t> stop_addrs;
    std::vector<scoped_ptr_t<disconnect_watcher_t> > disconnect_watchers;

    struct queue_t {
        explicit queue_t(uint64_t _next) : next(_next) { }
        rwlock_t lock;
        uint64_t next;
        struct lt_t {
            bool operator()(const stamped_msg_t &left, const stamped_msg_t &right) {
                return left.stamp > right.stamp; // We want the min val to be on top.
            }
        };
        std::priority_queue<stamped_msg_t, std::vector<stamped_msg_t>, lt_t> map;
    };
    // Maps from a `server_t`'s uuid_u.  We don't need a lock for this because
    // the set of `uuid_u`s never changes after it's initialized.
    std::map<uuid_u, scoped_ptr_t<queue_t> > queues;
    cond_t queues_ready;

    // Used to abort the feed when we get a `msg_t::stop_t`.
    cond_t aborted;

    auto_drainer_t drainer;
};

// This mustn't hold onto the `namespace_interface_t` after it returns.
real_feed_t::real_feed_t(auto_drainer_t::lock_t _client_lock,
                         client_t *_client,
                         mailbox_manager_t *_manager,
                         namespace_interface_t *ns_if,
                         namespace_id_t const &_table_id,
                         signal_t *interruptor,
                         table_meta_client_t *_table_meta_client)
    : feed_t(_table_id, _table_meta_client),
      client_lock(std::move(_client_lock)),
      client(_client),
      table_id(_table_id),
      manager(_manager),
      mailbox(manager, std::bind(&real_feed_t::mailbox_cb, this, ph::_1, ph::_2)) {
    try {
        read_t read(changefeed_subscribe_t(mailbox.get_address()),
                    profile_bool_t::DONT_PROFILE, read_mode_t::SINGLE);
        read_response_t read_resp;
        ns_if->read(
            auth::user_context_t(auth::permissions_t(true, false, false, false)),
            read,
            &read_resp,
            order_token_t::ignore,
            interruptor);
        auto resp = boost::get<changefeed_subscribe_response_t>(&read_resp.response);

        guarantee(resp != NULL);
        stop_addrs.reserve(resp->addrs.size());
        for (auto it = resp->addrs.begin(); it != resp->addrs.end(); ++it) {
            stop_addrs.push_back(std::move(*it));
        }

        std::set<peer_id_t> peers;
        for (auto it = stop_addrs.begin(); it != stop_addrs.end(); ++it) {
            peers.insert(it->get_peer());
        }
        for (auto it = peers.begin(); it != peers.end(); ++it) {
            disconnect_watchers.push_back(
                make_scoped<disconnect_watcher_t>(manager, *it));
        }

        for (const auto &server_uuid : resp->server_uuids) {
            auto res = queues.insert(
                std::make_pair(server_uuid, make_scoped<queue_t>(0)));
            guarantee(res.second);

            // In debug mode we put some junk messages in the queues to make sure
            // the queue logic actually does something (since mailboxes are
            // generally ordered right now).
#ifndef NDEBUG
            for (size_t i = 0; i < queues.size()-1; ++i) {
                res.first->second->map.push(
                    stamped_msg_t(
                        server_uuid,
                        std::numeric_limits<uint64_t>::max() - i,
                        msg_t()));
            }
#endif
        }
        queues_ready.pulse();

        // We spawn now so that the auto drainer lock is acquired immediately.
        coro_t::spawn_now_dangerously(std::bind(&real_feed_t::constructor_cb, this));
    } catch (...) {
        detached = true;
        throw;
    }
}

real_feed_t::~real_feed_t() {
    guarantee(num_subs == 0);
    detached = true;
    for (auto it = stop_addrs.begin(); it != stop_addrs.end(); ++it) {
        send(manager, *it, mailbox.get_address());
    }
}

client_t::addr_t real_feed_t::get_addr() const {
    return mailbox.get_address();
}

void real_feed_t::constructor_cb() {
    auto lock = make_scoped<auto_drainer_t::lock_t>(&drainer);
    {
        wait_any_t any_disconnect;
        for (size_t i = 0; i < disconnect_watchers.size(); ++i) {
            any_disconnect.add(disconnect_watchers[i].get());
        }
        wait_any_t wait_any(
            &aborted, &any_disconnect, lock->get_drain_signal());
        wait_any.wait_lazily_unordered();
    }
    // Clear the disconnect watchers so we don't keep the watched connections open
    // longer than necessary.
    disconnect_watchers.clear();
    if (!detached) {
        scoped_ptr_t<feed_t> self = client->detach_feed(client_lock, this);
        guarantee(detached);
        if (self.has()) {
            guarantee(lock.has());
            stop_subs(*lock);
            guarantee(num_subs == 0);
        } else {
            // We only get here if we were removed before we were detached.
            guarantee(num_subs == 0);
        }
        // We have to release the drainer lock before `self` is destroyed,
        // otherwise we'll block forever.
        lock.reset();
    }
}

class empty_sub_t : public flat_sub_t {
public:
    empty_sub_t(rdb_context_t *_rdb_context,
                const auth::user_context_t &_user_context,
                feed_t *_feed,
                configured_limits_t _limits,
                const datum_t &_squash,
                bool _include_states,
                bool _include_types)
    // There will never be any changes, safe to start squashing right away.
    : flat_sub_t(init_squashing_queue_t::YES,
                 _rdb_context,
                 _user_context,
                 _feed,
                 std::move(_limits),
                 _squash,
                 _include_states,
                 _include_types),
      state(state_t::INITIALIZING),
      sent_state(state_t::NONE),
      include_initial(false) {
        _feed->add_empty_sub(this);
    }
    virtual ~empty_sub_t() {
        destructor_cleanup(std::bind(&feed_t::del_empty_sub, feed, this));
    }
    feed_type_t cfeed_type() const final { return feed_type_t::stream; }
    bool update_stamp(const uuid_u &, uint64_t) final {
        r_sanity_fail();
    }
    datum_t pop_el() final {
        if (state != sent_state && include_states) {
            sent_state = state;
            state = state_t::READY;
            return maybe_add_type(
                state_datum(sent_state),
                change_type_t::STATE);
        }
        r_sanity_fail();
    }
    bool has_el() final {
        return (include_states && state != sent_state);
    }
    counted_t<datum_stream_t> to_stream(
        env_t *,
        std::string,
        namespace_interface_t *,
        const client_t::addr_t &,
        counted_t<datum_stream_t> maybe_src,
        scoped_ptr_t<subscription_t> &&self,
        backtrace_id_t bt) final {
        assert_thread();
        r_sanity_check(self.get() == this);
        include_initial = maybe_src.has();
        if (!include_initial) {
            state = state_t::READY;
        }
        return make_counted<stream_t<subscription_t> >(std::move(self), bt);
    }
    virtual counted_t<datum_stream_t> to_artificial_stream(
        const uuid_u &,
        const std::string &,
        const std::vector<datum_t> &,
        bool _include_initial,
        scoped_ptr_t<subscription_t> &&self,
        backtrace_id_t bt) {
        assert_thread();
        r_sanity_check(self.get() == this);
        include_initial = _include_initial;
        if (!include_initial) {
            state = state_t::READY;
        }
        return make_counted<stream_t<subscription_t> >(std::move(self), bt);
    }

private:
    state_t state, sent_state;
    bool include_initial;
    auto_drainer_t *get_drainer() final { return &drainer; }
    auto_drainer_t drainer;
};

class point_sub_t : public flat_sub_t {
public:
    // Throws QL exceptions.
    point_sub_t(rdb_context_t *_rdb_context,
                const auth::user_context_t &_user_context,
                feed_t *_feed,
                configured_limits_t _limits,
                const datum_t &_squash,
                bool _include_states,
                bool _include_types,
                datum_t _pkey)
        // For point changefeeds we start squashing right away.
        : flat_sub_t(init_squashing_queue_t::YES,
                     _rdb_context,
                     _user_context,
                     _feed,
                     std::move(_limits),
                     _squash,
                     _include_states,
                     _include_types),
          pkey(std::move(_pkey)),
          stamp(0),
          started(false),
          state(state_t::INITIALIZING),
          sent_state(state_t::NONE),
          include_initial(false) {
        _feed->add_point_sub(this, store_key_t(pkey.print_primary()));
    }
    virtual ~point_sub_t() {
        destructor_cleanup(std::bind(&feed_t::del_point_sub, feed, this,
                                     store_key_t(pkey.print_primary())));
    }
    feed_type_t cfeed_type() const final { return feed_type_t::point; }

    bool update_stamp(const uuid_u &, uint64_t new_stamp) final {
        if (new_stamp >= stamp) {
            stamp = new_stamp + 1;
            return true;
        }
        return false;
    }

    datum_t pop_el() final {
        if (state != sent_state && include_states) {
            sent_state = state;
            return maybe_add_type(state_datum(state),
                                  change_type_t::STATE);
        }
        datum_t ret;
        if (state != state_t::READY && include_initial) {
            r_sanity_check(initial_val);
            ret = change_val_to_change(*initial_val, true);
            if (!ret.has()) {
                // This is the one place where it's legal to have a document
                // like `{new_val: null}`.
                ret = datum_t(
                    std::map<datum_string_t, datum_t>{{
                            datum_string_t("new_val"),
                            datum_t::null()}});
            }
            ret = maybe_add_type(std::move(ret), change_type_t::INITIAL);
        } else {
            ret = change_val_to_change(pop_change_val(),
                                       false,
                                       false,
                                       include_types);
        }
        initial_val = boost::none;
        state = state_t::READY;
        return ret;
    }
    bool has_el() final {
        return (include_states && state != sent_state)
            || (include_initial && state != state_t::READY)
            || has_change_val();
    }
    counted_t<datum_stream_t> to_stream(
        env_t *env,
        std::string,
        namespace_interface_t *nif,
        const client_t::addr_t &addr,
        counted_t<datum_stream_t> maybe_src,
        scoped_ptr_t<subscription_t> &&self,
        backtrace_id_t bt) final {
        assert_thread();
        r_sanity_check(self.get() == this);

        include_initial = maybe_src.has();
        if (!include_initial) {
            state = state_t::READY;
        }

        read_response_t read_resp;
        nif->read(
            env->get_user_context(),
            read_t(changefeed_point_stamp_t{addr, store_key_t(pkey.print_primary())},
                   profile_bool_t::DONT_PROFILE, read_mode_t::SINGLE),
            &read_resp,
            order_token_t::ignore,
            env->interruptor);
        auto *res = boost::get<changefeed_point_stamp_response_t>(&read_resp.response);
        guarantee(res != nullptr);
        rcheck_datum(res->resp, base_exc_t::RESUMABLE_OP_FAILED,
                     "Unable to retrieve start stamp.  (Did you just reshard?)");
        auto *resp = &*res->resp;
        uint64_t start_stamp = resp->stamp.second;
        initial_val = change_val_t(
               resp->stamp,
               store_key_t(pkey.print_primary()),
               boost::none,
               indexed_datum_t(resp->initial_val, boost::none)
               DEBUG_ONLY(, boost::none));
        if (start_stamp > stamp) {
            stamp = start_stamp;
            queue->clear();
        } else {
            while (queue->size() != 0) {
                const change_val_t *cv = &peek_change_val();
                guarantee(cv->source_stamp.first == initial_val->source_stamp.first);
                // We use strict comparison because a normal stamp that's equal to
                // the start stamp wins (the semantics are that the start stamp is
                // the first "legal" stamp).
                if (cv->source_stamp.second < initial_val->source_stamp.second) {
                    pop_change_val();
                } else {
                    break;
                }
            }
        }
        started = true;

        return make_counted<stream_t<subscription_t> >(std::move(self), bt);
    }
    virtual counted_t<datum_stream_t> to_artificial_stream(
        const uuid_u &,
        const std::string &primary_key_name,
        const std::vector<datum_t> &initial_values,
        bool _include_initial,
        scoped_ptr_t<subscription_t> &&self,
        backtrace_id_t bt) {
        assert_thread();
        r_sanity_check(self.get() == this);

        include_initial = _include_initial;
        if (!include_initial) {
            state = state_t::READY;
        }

        datum_t initial = datum_t::null();
        /* Linear search is slow, but in practice there are few enough values that
           it's OK. */
        for (const datum_t &d : initial_values) {
            if (d.get_field(datum_string_t(primary_key_name)) == pkey) {
                initial = d;
                break;
            }
        }
        initial_val = change_val_t(
            std::make_pair(nil_uuid(), 0),
            store_key_t(pkey.print_primary()),
            boost::none,
            indexed_datum_t(initial, boost::none)
            DEBUG_ONLY(, boost::none));
        started = true;

        return make_counted<stream_t<subscription_t> >(std::move(self), bt);
    }
private:
    datum_t pkey;
    boost::optional<change_val_t> initial_val;
    uint64_t stamp;
    bool started;
    state_t state, sent_state;
    bool include_initial;

    auto_drainer_t *get_drainer() final { return &drainer; }
    auto_drainer_t drainer;
};

// This gets around some class ordering issues; `range_sub_t` needs to know how
// to construct a `splice_stream_t` and `splice_stream_t` needs to know about
// `range_sub_t`.
class splice_stream_t;
template<class... Args>
counted_t<splice_stream_t> make_splice_stream(Args &&...args) {
    return make_counted<splice_stream_t>(std::forward<Args>(args)...);
}

class range_sub_t : public flat_sub_t {
public:
    // Throws QL exceptions.
    range_sub_t(rdb_context_t *_rdb_context,
                const auth::user_context_t &_user_context,
                feed_t *_feed,
                configured_limits_t _limits,
                const datum_t &_squash,
                bool _include_states,
                bool _include_types,
                env_t *outer_env,
                keyspec_t::range_t _spec)
        // We don't turn on squashing until later for range subs.  (We need to
        // wait until we've purged and all the initial values are reconciled.)
        : flat_sub_t(init_squashing_queue_t::NO,
                     _rdb_context,
                     _user_context,
                     _feed,
                     std::move(_limits),
                     _squash,
                     _include_states,
                     _include_types),
          spec(std::move(_spec)),
          state(state_t::READY),
          sent_state(state_t::NONE),
          artificial_include_initial(false) {
        env = make_env(outer_env);
        for (const auto &transform : spec.transforms) {
            ops.push_back(make_op(transform));
        }
        store_keys = spec.datumspec.primary_key_map();
        if (!store_keys) {
            store_key_range = spec.datumspec.covering_range().to_primary_keyrange();
        }
        _feed->add_range_sub(this);
    }
    feed_type_t cfeed_type() const final { return feed_type_t::stream; }
    virtual ~range_sub_t() {
        destructor_cleanup(std::bind(&feed_t::del_range_sub, feed, this));
    }
    boost::optional<std::string> sindex() const { return spec.sindex; }
    size_t copies(const datum_t &sindex_key) const {
        guarantee(spec.sindex);
        if (spec.intersect_geometry) {
            try {
                if (!geo_does_intersect(*spec.intersect_geometry, sindex_key)) {
                    return 0;
                }
            } catch (const geo_exception_t &) {
                return 0;
            } catch (const base_exc_t &) {
                return 0;
            }
        }
        return spec.datumspec.copies(sindex_key);
    }
    size_t copies(const store_key_t &pkey) const {
        guarantee(!spec.sindex);
        if (store_keys) {
            guarantee(store_keys);
            auto it = store_keys->find(pkey);
            return it != store_keys->end() ? it->second : 0;
        } else {
            guarantee(store_key_range);
            return store_key_range->contains_key(pkey) ? 1 : 0;
        }
    }

    bool has_ops() { return ops.size() != 0; }

    boost::optional<datum_t> apply_ops(datum_t val) {
        guarantee(active());
        guarantee(env.has());
        guarantee(has_ops());

        // We acquire a lock here, and the drain signal is our interruptor for
        // `env`.  I think this is technically unnecessary right now because we
        // ban non-deterministic terms in `ops` and no deterministic terms
        // block, but better safe than sorry.
        auto_drainer_t::lock_t lock(&drainer);
        // It's safe to use `datum_t()` here for the same reason it's safe in
        // `eager_datum_stream_t::next_grouped_batch`, but if we add e.g. an
        // `r.current_index` term we'll need to make this smarter.
        return changefeed::apply_ops(val, ops, env.get(), datum_t());
    }
    boost::optional<datum_t> maybe_apply_ops(datum_t val) {
        return has_ops() ? apply_ops(std::move(val)) : std::move(val);
    }

    bool update_stamp(const uuid_u &uuid, uint64_t new_stamp) final {
        guarantee(active());
        auto it = next_stamps.find(uuid);
        if (it == next_stamps.end() || new_stamp >= it->second) {
            next_stamps[uuid] = new_stamp + 1;
            return true;
        }
        return false;
    }

    datum_t pop_el() final {
        if (state != sent_state && include_states) {
            sent_state = state;
            if (artificial_include_initial && artificial_initial_vals.size() == 0) {
                state = state_t::READY;
            }
            return maybe_add_type(state_datum(sent_state),
                                  change_type_t::STATE);
        }
        if (artificial_initial_vals.size() != 0) {
            datum_t d = artificial_initial_vals.back();
            artificial_initial_vals.pop_back();
            if (artificial_initial_vals.size() == 0) {
                state = state_t::READY;
            }
            return maybe_add_type(
                vals_to_change(datum_t(), d, true),
                change_type_t::INITIAL);
        }
        return change_val_to_change(pop_change_val(),
                                    false,
                                    false,
                                    include_types);
    }
    bool has_el() final {
        return (include_states && state != sent_state)
            || artificial_initial_vals.size() != 0
            || has_change_val();
    }

    void maybe_enable_squashing() {
        if (squash) {
            scoped_ptr_t<maybe_squashing_queue_t> old_queue = std::move(queue);
            queue = make_scoped<squashing_queue_t>();
            while (old_queue->size() != 0) {
                queue->add(old_queue->pop());
            }
        }
    }

    counted_t<datum_stream_t> to_stream(
        env_t *outer_env,
        std::string,
        namespace_interface_t *nif,
        const client_t::addr_t &addr,
        counted_t<datum_stream_t> maybe_src,
        scoped_ptr_t<subscription_t> &&self,
        backtrace_id_t bt) final {
        assert_thread();
        r_sanity_check(self.get() == this);

        read_response_t read_resp;
        // Note that we use the `outer_env`'s interruptor for the read.
        nif->read(
            outer_env->get_user_context(),
            read_t(changefeed_stamp_t(addr),
                   profile_bool_t::DONT_PROFILE,
                   read_mode_t::SINGLE),
            &read_resp, order_token_t::ignore, outer_env->interruptor);
        auto *resp = boost::get<changefeed_stamp_response_t>(&read_resp.response);
        guarantee(resp != nullptr);
        rcheck_datum(resp->stamp_infos, base_exc_t::RESUMABLE_OP_FAILED,
                     "Unable to retrieve the start stamps.  Did you just reshard?");
        std::map<uuid_u, uint64_t> purge_stamps;
        for (const auto &pair : *resp->stamp_infos) {
            const auto id_stamp_pair = std::make_pair(pair.first, pair.second.stamp);
            auto orig_res = orig_stamps.insert(id_stamp_pair);
            guarantee(orig_res.second);
            auto res = next_stamps.insert(id_stamp_pair);
            // If we already have stamps.
            if (!res.second) {
                purge_stamps.insert(id_stamp_pair);
                // Even though we're purging the early changes, we also need to
                // make sure we don't get any such changes *after* the purge.
                next_stamps[id_stamp_pair.first] =
                    std::max(next_stamps[id_stamp_pair.first], id_stamp_pair.second);
            }
        }
        queue->purge_below(purge_stamps);
        rcheck_datum(orig_stamps.size() != 0, base_exc_t::RESUMABLE_OP_FAILED,
                     "Empty start stamps.  Did you just reshard?");

        if (maybe_src) {
            // Nothing can happen between constructing the new `scoped_ptr_t` and
            // releasing the old one.
            scoped_ptr_t<range_sub_t> sub_self(this);
            UNUSED subscription_t *super_self = self.release();
            bool stamped = maybe_src->add_stamp(changefeed_stamp_t(addr));
            rcheck_src(bt, stamped, base_exc_t::LOGIC,
                       "Cannot call `include_initial` on an unstampable stream.");
            return make_splice_stream(maybe_src, std::move(sub_self), bt);
        } else {
            maybe_enable_squashing();
            return make_counted<stream_t<subscription_t> >(std::move(self), bt);
        }
    }
    virtual counted_t<datum_stream_t> to_artificial_stream(
        const uuid_u &uuid,
        const std::string &pkey_name,
        const std::vector<datum_t> &initial_vals,
        bool include_initial,
        scoped_ptr_t<subscription_t> &&self,
        backtrace_id_t bt) {
        assert_thread();
        r_sanity_check(self.get() == this);

        artificial_include_initial = include_initial;

        orig_stamps[uuid] = 0;
        next_stamps[uuid] = 0;
        if (artificial_include_initial) {
            state = state_t::INITIALIZING;
            datum_string_t pk(pkey_name);
            for (auto it = initial_vals.rbegin(); it != initial_vals.rend(); ++it) {
                if (boost::optional<datum_t> d = maybe_apply_ops(*it)) {
                    for (size_t i = 0;
                         i < spec.datumspec.copies(it->get_field(pk));
                         ++i) {
                        artificial_initial_vals.push_back(*d);
                    }
                }
            }
        }
        return make_counted<stream_t<subscription_t> >(std::move(self), bt);
    }
    const std::map<uuid_u, uint64_t> &get_next_stamps() { return next_stamps; }
    const std::map<uuid_u, uint64_t> &get_orig_stamps() { return orig_stamps; }
private:
    scoped_ptr_t<env_t> make_env(env_t *outer_env) {
        // This is to support fake environments from the unit tests that don't
        // actually have a context.
        return outer_env->get_rdb_ctx() == nullptr
            ? make_scoped<env_t>(outer_env->interruptor,
                                 outer_env->return_empty_normal_batches,
                                 outer_env->reql_version())
            : make_scoped<env_t>(
                outer_env->get_rdb_ctx(),
                outer_env->return_empty_normal_batches,
                drainer.get_drain_signal(),
                outer_env->get_all_optargs(),
                outer_env->get_user_context(),
                nullptr/*don't profile*/);
    }

    scoped_ptr_t<env_t> env;
    std::vector<scoped_ptr_t<op_t> > ops;

    // The stamp (see `stamped_msg_t`) associated with our `changefeed_stamp_t`
    // read.  We use these to make sure we don't see changes from writes before
    // our subscription.
    std::map<uuid_u, uint64_t> orig_stamps, next_stamps;
    keyspec_t::range_t spec;
    boost::optional<std::map<store_key_t, uint64_t> > store_keys;
    boost::optional<key_range_t> store_key_range;
    state_t state, sent_state;
    std::vector<datum_t> artificial_initial_vals;
    bool artificial_include_initial;

    auto_drainer_t *get_drainer() final { return &drainer; }
    auto_drainer_t drainer;
};

class limit_sub_t : public subscription_t {
    struct limit_change_t {
        datum_t old_d, new_d;
        boost::optional<size_t> old_offset, new_offset;
    };
public:
    // Throws QL exceptions.
    limit_sub_t(rdb_context_t *_rdb_context,
                const auth::user_context_t &_user_context,
                feed_t *_feed,
                configured_limits_t _limits,
                const datum_t &_squash,
                bool _include_offsets,
                bool _include_states,
                bool _include_types,
                keyspec_t::limit_t _spec)
        : subscription_t(_rdb_context,
                         _user_context,
                         _feed,
                         _limits,
                         _squash,
                         _include_states,
                         _include_types),
          uuid(generate_uuid()),
          need_init(-1),
          got_init(0),
          spec(std::move(_spec)),
          gt(limit_order_t(spec.range.sorting)),
          item_queue(gt),
          active_data(gt),
          include_initial(false),
          include_offsets(_include_offsets) {
        _feed->add_limit_sub(this, uuid);
    }

    virtual ~limit_sub_t() {
        destructor_cleanup(std::bind(&feed_t::del_limit_sub, feed, this, uuid));
    }

    feed_type_t cfeed_type() const final { return feed_type_t::orderby_limit; }

    void maybe_start() {
        // When we later support not always returning the initial set, that
        // logic should go here.
        if (need_init == got_init) {
            ASSERT_NO_CORO_WAITING;
            if (include_initial) {
                if (include_states) els.push_back(maybe_add_type(initializing_datum(),
                                                                 change_type_t::STATE));
                size_t i = 0;
                for (auto it = active_data.rbegin(); it != active_data.rend(); ++it) {
                    std::map<datum_string_t, datum_t> m;
                    m[datum_string_t("new_val")] = (**it)->second.second;
                    if (include_offsets) {
                        m[datum_string_t("new_offset")] =
                            datum_t(static_cast<double>(i++));
                    }
                    els.push_back(maybe_add_type(datum_t(std::move(m)),
                                                 change_type_t::INITIAL));
                }
            }
            if (include_states) els.push_back(maybe_add_type(ready_datum(),
                                                             change_type_t::STATE));

            if (!squash) {
                decltype(queued_changes) changes;
                changes.swap(queued_changes);
                for (const auto &pair : changes) {
                    note_change(pair.first, pair.second);
                }
            }
            maybe_signal_cond();
        }
    }

    // For limit changefeeds, the easiest way to effectively squash changes is
    // to delay applying them to the active data set until our timer is up.
    virtual void apply_queued_changes() {
        ASSERT_NO_CORO_WAITING;
        if (queued_changes.size() != 0 && need_init == got_init) {
            guarantee(squash);
            decltype(queued_changes) changes;
            changes.swap(queued_changes);

            std::vector<limit_change_t> limit_changes;
            limit_changes.reserve(changes.size()); // Important to keep iterators valid.
            // This has to be a multimap because of multi-indexes.
            std::multimap<datum_t, decltype(limit_changes.begin()),
                          std::function<bool(const datum_t &, const datum_t &)> >
                new_val_index(
                    [](const datum_t &a, const datum_t &b) {
                        if (!a.has() || !b.has()) {
                            return a.has() < b.has();
                        } else {
                            return a < b;
                        }
                    });

            // We do things this way rather than simply diffing the active sets
            // because it's easier to avoid irrational intermediate states.
            for (const auto &change_pair : changes) {
                limit_change_t lc
                    = note_change_impl(change_pair.first, change_pair.second);
                if (lc.old_d.has() || lc.new_d.has()) {
                    auto it = new_val_index.find(lc.old_d);
                    decltype(limit_changes.begin()) limit_changes_it;
                    if (it == new_val_index.end()) {
                        limit_changes.push_back(lc);
                        limit_changes_it = limit_changes.end()-1;
                    } else {
                        limit_changes_it = it->second;
                        limit_changes_it->new_d = lc.new_d;
                        new_val_index.erase(it);
                    }
                    new_val_index.insert(std::make_pair(lc.new_d, limit_changes_it));
                }
            }

            for (auto &&lc : limit_changes) {
                if (!((lc.old_d.has() && lc.new_d.has()
                       && lc.old_d == lc.new_d
                       && lc.old_offset == lc.new_offset)
                      || (!lc.old_d.has() && !lc.new_d.has()))) {
                    push_el(std::move(lc));
                }
            }

            guarantee(queued_changes.size() == 0);
        }
    }

    void init(const std::vector<std::pair<std::string, std::pair<datum_t, datum_t> > >
              &start_data) {
#ifndef NDEBUG
        nap(randint(250)); // Nap up to 250ms to test queueing.
#endif
        got_init += 1;
        for (const auto &pair : start_data) {
            auto it = item_queue.insert(pair).first;
            active_data.insert(it);
            if (active_data.size() > spec.limit) {
                auto old_ft = active_data.begin();
                for (auto ft = active_data.begin(); ft != active_data.end(); ++ft) {
                    old_ft = ft;
                }
                size_t erased = active_data.erase(*active_data.begin());
                guarantee(erased == 1);
            }
            guarantee(active_data.size() == spec.limit
                      || (active_data.size() < spec.limit
                          && active_data.size() == item_queue.size()));
        }
        maybe_start();
    }

    void push_el(limit_change_t lc) {
        // Empty changes should have been caught above us.
        guarantee(lc.old_d.has() || lc.new_d.has());
        if (lc.old_d.has() && lc.new_d.has()) {
            rassert(lc.old_d != lc.new_d || lc.old_offset != lc.new_offset);
        }

        datum_t el = vals_to_change(
            lc.old_d.has() ? std::move(lc.old_d) : datum_t::null(),
            lc.new_d.has() ? std::move(lc.new_d) : datum_t::null(),
            false,
            false,
            include_types,
            include_offsets,
            std::move(lc.old_offset),
            std::move(lc.new_offset));
        els.push_back(std::move(el));
    }

    virtual void note_change(
        const boost::optional<std::string> &old_key,
        const boost::optional<item_t> &new_val) {
        ASSERT_NO_CORO_WAITING;

        // If we aren't done initializing, or if we're squashing, just queue up
        // the change.  If we're initializing, we're done; the change will be
        // sent in `maybe_start`.  If we're squashing and we're done
        // initializing, there might be a coroutine blocking on more data in
        // `get_els`, so we call `maybe_signal_cond` to possibly wake it up.
        if (need_init != got_init || squash) {
            queued_changes.push_back(std::make_pair(old_key, new_val));
        } else {
            limit_change_t lc = note_change_impl(old_key, new_val);
            if (lc.old_d.has() || lc.new_d.has()) {
                push_el(std::move(lc));
            }
        }
        if (need_init == got_init) {
            maybe_signal_cond();
        }
    }

    template<class T>
    boost::optional<size_t> slow_active_offset(const T &it) {
        size_t i = 0;
        for (auto ft = active_data.begin();
             ft != active_data.end();
             ++ft, ++i) {
            if (*ft == it) {
                return active_data.size() - (i + 1);
            }
        }
        return boost::none;
    }

    limit_change_t note_change_impl(
        const boost::optional<std::string> &old_key,
        const boost::optional<item_t> &new_val) {
        ASSERT_NO_CORO_WAITING;

        boost::optional<item_t> old_send, new_send;
        boost::optional<size_t> old_offset, new_offset;
        if (old_key) {
            auto it = item_queue.find_id(*old_key);
            guarantee(it != item_queue.end());
            if (include_offsets) {
                old_offset = slow_active_offset(it);
            }
            size_t erased = active_data.erase(it);
            if (erased != 0) {
                // The old value was in the set.
                if (include_offsets) {
                    guarantee(old_offset);
                }
                old_send = **it;
            } else {
                if (include_offsets) {
                    guarantee(!old_offset);
                }
            }
            item_queue.erase(it);
        }
        if (new_val) {
            auto pair = item_queue.insert(*new_val);
            auto it = pair.first;
            guarantee(pair.second);
            bool insert;
            if (active_data.size() == 0) {
                insert = false;
            } else {
                guarantee(active_data.size() != 0);
                insert = !gt(it, *active_data.begin());
            }
            if (insert) {
                // The new value is in the old set bounds (and thus in the set).
                active_data.insert(it);
                if (include_offsets) {
                    new_offset = slow_active_offset(it);
                    guarantee(new_offset);
                }
                new_send = **it;
            }
        }
        if (active_data.size() > spec.limit) {
            // The old value wasn't in the set, but the new value is, and a
            // value has to leave the set to make room.
            auto last = *active_data.begin();
            guarantee(new_send && !old_send && !old_offset);
            if (include_offsets) {
                // We subtract two here because `new_val` was already inserted.
                old_offset = active_data.size() - 2;
            }
            old_send = **last;
            active_data.erase(last);
        } else if (active_data.size() < spec.limit) {
            // The set is too small.
            if (new_send) {
                // The set is too small because there aren't enough rows in the table.
                guarantee(active_data.size() == item_queue.size());
            } else if (active_data.size() < item_queue.size()) {
                // The set is too small because the new value wasn't in the old
                // set bounds, so we need to add the next best element.
                auto it = active_data.size() == 0
                    ? item_queue.end()
                    : *active_data.begin();

                guarantee(it != item_queue.begin());
                --it;
                active_data.insert(it);
                if (include_offsets) {
                    new_offset = active_data.size() - 1;
                }
                new_send = **it;
            }
        }
        guarantee(active_data.size() == spec.limit
                  || active_data.size() == item_queue.size());

        datum_t old_d = old_send ? (*old_send).second.second : datum_t();
        datum_t new_d = new_send ? (*new_send).second.second : datum_t();
        if (old_d.has() && new_d.has() && old_d == new_d && old_offset == new_offset) {
            old_d = new_d = datum_t();
            old_offset = new_offset = boost::none;
        }
        return limit_change_t{old_d, new_d, old_offset, new_offset};
    }

    virtual bool has_el() { return els.size() != 0; }
    virtual datum_t pop_el() {
        guarantee(has_el());
        datum_t ret = std::move(els.front());
        els.pop_front();
        return ret;
    }

    counted_t<datum_stream_t> to_stream(
        env_t *env,
        std::string table,
        namespace_interface_t *nif,
        const client_t::addr_t &addr,
        counted_t<datum_stream_t> maybe_src,
        scoped_ptr_t<subscription_t> &&self,
        backtrace_id_t bt) final {
        assert_thread();
        r_sanity_check(self.get() == this);
        // `.limit().changes()` is currently only allowed on a single range.
        r_sanity_check(
            spec.range.datumspec.visit<bool>(
                [](const datum_range_t &) { return true; },
                [](const std::map<datum_t, uint64_t> &) { return false; }));
        include_initial = maybe_src.has();
        read_response_t read_resp;
        nif->read(
            env->get_user_context(),
            read_t(changefeed_limit_subscribe_t(
                       addr,
                       uuid,
                       spec,
                       std::move(table),
                       env->get_all_optargs(),
                       env->get_user_context(),
                       spec.range.sindex
                           ? region_t::universe()
                           : region_t(
                               spec.range.datumspec.covering_range().to_primary_keyrange())),
                   profile_bool_t::DONT_PROFILE,
                   read_mode_t::SINGLE),
            &read_resp,
            order_token_t::ignore,
            env->interruptor);
        auto resp = boost::get<changefeed_limit_subscribe_response_t>(
            &read_resp.response);
        if (resp == NULL) {
            auto err_resp = boost::get<rget_read_response_t>(&read_resp.response);
            guarantee(err_resp != NULL);
            auto err = boost::get<exc_t>(&err_resp->result);
            guarantee(err != NULL);
            throw *err;
        }
        guarantee(need_init == -1);
        need_init = resp->shards;
        stop_addrs = std::move(resp->limit_addrs);
        guarantee(need_init > 0);
        maybe_start();
        return make_counted<stream_t<subscription_t> >(std::move(self), bt);
    }
    NORETURN virtual counted_t<datum_stream_t> to_artificial_stream(
        const uuid_u &, const std::string &, const std::vector<datum_t> &,
        bool, scoped_ptr_t<subscription_t> &&, backtrace_id_t) {
        crash("Cannot start a limit subscription on an artificial table.");
    }

    uuid_u uuid;
    int64_t need_init, got_init;
    keyspec_t::limit_t spec;

    limit_order_t gt;
    item_queue_t item_queue;
    typedef item_queue_t::iterator data_it_t;
    typedef std::function<bool(const data_it_t &, const data_it_t &)> data_it_lt_t;
    std::set<data_it_t, data_it_lt_t> active_data;

    std::deque<datum_t> els;
    std::vector<std::pair<boost::optional<std::string>, boost::optional<item_t> > >
        queued_changes;
    std::vector<server_t::limit_addr_t> stop_addrs;
    bool include_initial;
    bool include_offsets;

    auto_drainer_t *get_drainer() final { return &drainer; }
    auto_drainer_t drainer;
};

void real_feed_t::stop_limit_sub(limit_sub_t *sub) {
    for (const auto &addr : sub->stop_addrs) {
        send(manager, addr,
             mailbox.get_address(), sub->spec.range.sindex, sub->uuid);
    }
}

class msg_visitor_t : public boost::static_visitor<void> {
public:
    msg_visitor_t(feed_t *_feed, const auto_drainer_t::lock_t *_lock,
                  uuid_u _server_uuid, uint64_t _stamp)
        : feed(_feed), lock(_lock), server_uuid(_server_uuid), stamp(_stamp) {
        guarantee(feed != nullptr);
        guarantee(lock != nullptr);
        guarantee(lock->has_lock());
    }
    void operator()(const msg_t::limit_start_t &msg) const {
        feed->on_limit_sub(
            msg.sub, *lock,
            [&msg](limit_sub_t *sub) { sub->init(msg.start_data); });
    }
    void operator()(const msg_t::limit_change_t &msg) const {
        feed->on_limit_sub(
            msg.sub, *lock,
            [&msg](limit_sub_t *sub) { sub->note_change(msg.old_key, msg.new_val); });
    }
    void operator()(const msg_t::limit_stop_t &msg) const {
        feed->on_limit_sub(
            msg.sub, *lock,
            [&msg](limit_sub_t *sub) {
                sub->stop(std::make_exception_ptr(msg.exc), detach_t::NO);
            });
    }
    void operator()(const msg_t::change_t &change) const {
        datum_t null = datum_t::null();

        feed->each_range_sub(*lock, [&](range_sub_t *sub) {
            datum_t new_val = null, old_val = null;
            if (!sub->active()) return;
            bool trivial = false;
            if (sub->has_ops()) {
                if (change.new_val.has()) {
                    if (boost::optional<datum_t> d = sub->apply_ops(change.new_val)) {
                        new_val = *d;
                    }
                }
                if (!sub->active()) return;
                if (change.old_val.has()) {
                    if (boost::optional<datum_t> d = sub->apply_ops(change.old_val)) {
                        old_val = *d;
                    }
                }
                if (!sub->active()) return;
                // Duplicate values are caught before being written to disk and
                // don't generate a `mod_report`, but if we have transforms the
                // values might have changed.
                trivial = (new_val == old_val);
            } else {
                guarantee(change.old_val.has() || change.new_val.has());
                if (change.new_val.has()) {
                    new_val = change.new_val;
                }
                if (change.old_val.has()) {
                    old_val = change.old_val;
                }
            }
            ASSERT_NO_CORO_WAITING;
            boost::optional<std::string> sindex = sub->sindex();
            if (sindex) {
                std::vector<indexed_datum_t> old_idxs, new_idxs;
                auto old_it = change.old_indexes.find(*sindex);
                if (old_it != change.old_indexes.end()) {
                    for (const auto &idx : old_it->second) {
                        for (size_t i = 0; i < sub->copies(idx.first); ++i) {
                            old_idxs.push_back(indexed_datum_t(old_val, idx.second));
                        }
                    }
                }
                auto new_it = change.new_indexes.find(*sindex);
                if (new_it != change.new_indexes.end()) {
                    for (const auto &idx : new_it->second) {
                        for (size_t i = 0; i < sub->copies(idx.first); ++i) {
                            new_idxs.push_back(indexed_datum_t(new_val, idx.second));
                        }
                    }
                }
                while (old_idxs.size() > 0 && new_idxs.size() > 0) {
                    if (!trivial) {
                        sub->add_el(server_uuid, stamp, change.pkey, sindex,
                                    std::move(old_idxs.back()),
                                    std::move(new_idxs.back()));
                    }
                    old_idxs.pop_back();
                    new_idxs.pop_back();
                }
                while (old_idxs.size() > 0) {
                    guarantee(new_idxs.size() == 0);
                    if (old_val != null) {
                        sub->add_el(server_uuid, stamp, change.pkey, sindex,
                                    std::move(old_idxs.back()),
                                    boost::none);
                    }
                    old_idxs.pop_back();
                }
                while (new_idxs.size() > 0) {
                    guarantee(old_idxs.size() == 0);
                    if (new_val != null) {
                        sub->add_el(server_uuid, stamp, change.pkey, sindex,
                                    boost::none,
                                    std::move(new_idxs.back()));
                    }
                    new_idxs.pop_back();
                }
            } else {
                if (!trivial) {
                    for (size_t i = 0; i < sub->copies(change.pkey); ++i) {
                        sub->add_el(server_uuid, stamp, change.pkey, sindex,
                                    indexed_datum_t(old_val, boost::none),
                                    indexed_datum_t(new_val, boost::none));
                    }
                }
            }
        });
        feed->on_point_sub(
            change.pkey,
            *lock,
            std::bind(
                &point_sub_t::add_el,
                ph::_1,
                std::cref(server_uuid),
                stamp,
                change.pkey,
                boost::none,
                change.old_val.has()
                    ? boost::optional<indexed_datum_t>(
                        indexed_datum_t(change.old_val, boost::none))
                    : boost::none,
                change.new_val.has()
                    ? boost::optional<indexed_datum_t>(
                        indexed_datum_t(change.new_val, boost::none))
                    : boost::none));
    }
    void operator()(const msg_t::stop_t &) const {
        feed->abort_feed();
    }
private:
    feed_t *feed;
    const auto_drainer_t::lock_t *lock;
    uuid_u server_uuid;
    uint64_t stamp;
};

void msg_visit(feed_t *feed,
               const auto_drainer_t::lock_t *lock,
               uuid_u server_uuid,
               uint64_t stamp,
               const msg_t::op_t &op) {
    msg_visitor_t visitor(feed, lock, server_uuid, stamp);
    boost::apply_visitor(visitor, op);
    feed->update_stamps(server_uuid, stamp);
}

void real_feed_t::mailbox_cb(signal_t *, stamped_msg_t msg) {
    // We stop receiving messages when detached (we're only receiving
    // messages because we haven't managed to get a message to the
    // stop mailboxes for some of the primary replicas yet).  This also stops
    // us from trying to handle a message while waiting on the auto
    // drainer. Because we acquire the auto drainer, we don't pay any
    // attention to the mailbox's signal.
    if (!detached) {
        auto_drainer_t::lock_t lock(&drainer);

        // We wait for the write to complete and the queues to be ready.
        wait_any_t wait_any(&queues_ready, lock.get_drain_signal());
        wait_any.wait_lazily_unordered();
        if (detached) return;
        if (!lock.get_drain_signal()->is_pulsed()) {
            // We don't need a lock for this because the set of `uuid_u`s never
            // changes after it's initialized.
            auto it = queues.find(msg.server_uuid);
            guarantee(it != queues.end());
            queue_t *queue = it->second.get();
            guarantee(queue != NULL);

            rwlock_in_line_t spot(&queue->lock, access_t::write);
            spot.write_signal()->wait_lazily_unordered();
            if (detached) return;

            // Add us to the queue.
            guarantee(msg.stamp >= queue->next);
            queue->map.push(std::move(msg));

            // Read as much as we can from the queue (this enforces ordering.)
            while (queue->map.size() != 0 && queue->map.top().stamp == queue->next) {
                if (detached) return;
                const stamped_msg_t &curmsg = queue->map.top();
                msg_visit(this, &lock,
                          curmsg.server_uuid, curmsg.stamp, curmsg.submsg.op);
                queue->map.pop();
                queue->next += 1;
            }
        }
    }
}

class splice_stream_t : public stream_t<range_sub_t> {
public:
    template<class... Args>
    splice_stream_t(counted_t<datum_stream_t> _src, Args &&... args)
        : stream_t(std::forward<Args>(args)...),
          read_once(false),
          cached_ready(false),
          src(std::move(_src)) {
        r_sanity_check(src.has());
        for (const auto &p : sub->get_orig_stamps()) {
            stamped_ranges.insert(std::make_pair(p.first, stamped_range_t(p.second)));
        }
    }

private:
    std::vector<datum_t> next_stream_batch(env_t *env, const batchspec_t &bs) final {
        std::vector<datum_t> ret;
        batcher_t batcher = bs.to_batcher();

        while (ret.size() == 0) {
            r_sanity_check(!batcher.should_send_batch());
            // If there's nothing left to read, behave like a normal feed.  `ready`
            // should only be called after we've confirmed `is_exhausted` returns
            // true.
            if (src->is_exhausted() && ready()) {
                // This will send the `ready` state as its first doc.
                return stream_t::next_stream_batch(env, bs);
            }

            // We have to do a little song and dance to make sure we've read at
            // least once before deciding whether or not to discard changes, because
            // otherwise we don't know the `skey_version`.  We can remove this hack
            // once we're no longer backwards-compatible with pre-1.16 (I think?)
            // skey versions.
            if (read_once) {
                while (sub->has_change_val() && !batcher.should_send_batch()) {
                    change_val_t cv = sub->pop_change_val();
                    // Note that `discard` updates the `stamped_ranges`.
                    datum_t el = change_val_to_change(
                        cv,
                        cv.old_val && discard(
                            cv.pkey,
                            cv.source_stamp, *cv.old_val),
                        cv.new_val && discard(
                            cv.pkey,
                            cv.source_stamp, *cv.new_val),
                        sub->include_types);
                    if (el.has()) {
                        batcher.note_el(el);
                        ret.push_back(std::move(el));
                    }
                }
                maybe_skip_to_feed();
                remove_outdated_ranges();
            } else {
                if (sub->include_states) {
                    ret.push_back(sub->maybe_add_type(
                                      state_datum(state_t::INITIALIZING),
                                      change_type_t::STATE));
                }
            }
            if (!src->is_exhausted() && !batcher.should_send_batch()) {
                // Sorting must be UNORDERED for our last_read range calculation to work.
                batchspec_t new_bs = bs.with_lazy_sorting_override(sorting_t::UNORDERED);
                std::vector<datum_t> batch = src->next_batch(env, new_bs);
                update_ranges();
                r_sanity_check(active_state);
                read_once = true;
                if (batch.size() == 0) {
                    r_sanity_check(src->is_exhausted());
                } else {
                    ret.reserve(ret.size() + batch.size());
                    for (auto &&datum : batch) {
                        datum_t cv = vals_to_change(datum_t(), std::move(datum), true);
                        if (cv.has()) {
                            ret.push_back(
                                sub->maybe_add_type(std::move(cv), change_type_t::INITIAL));
                        }
                    }
                }
            } else {
                if (ret.size() == 0) {
                    // If we've exhausted the stream but aren't ready yet then
                    // we nap for 50ms to wait for changes.  The other
                    // alternatives would be to send back empty batches or to
                    // have more complicated logic to block until a change is
                    // available.  This shouldn't matter too much because this
                    // case should be rare in practice, and napping more than
                    // once should be extremely rare.
                    nap(50, env->interruptor);
                }
            }
        }

        r_sanity_check(ret.size() != 0);
        return ret;
    }

    bool discard(const store_key_t &pkey,
                 const std::pair<uuid_u, uint64_t> &source_stamp,
                 const indexed_datum_t &val) {
        store_key_t key;
        if (val.btree_index_key) {
            key = store_key_t(*val.btree_index_key);
        } else {
            key = pkey;
        }

        auto it = stamped_ranges.find(source_stamp.first);
        r_sanity_check(it != stamped_ranges.end());
        it->second.next_expected_stamp = source_stamp.second + 1;
        if (key < it->second.left_fencepost) return false;
        if (key >= it->second.get_right_fencepost()) return true;
        // `ranges` should be extremely small
        for (const auto &pair : it->second.ranges) {
            if (pair.first.contains_key(key)) {
                return source_stamp.second < pair.second;
            }
        }
        // If we get here then there's a gap in the ranges.
        r_sanity_fail();
    }

    void maybe_skip_to_feed() {
        const std::map<uuid_u, uint64_t> *sub_stamps = &sub->get_next_stamps();
        boost::optional<std::map<uuid_u, uint64_t> > feed_stamps;
        for (auto &&pair : stamped_ranges) {
            auto it = sub_stamps->find(pair.first);
            r_sanity_check(it != sub_stamps->end());
            uint64_t sub_stamp = it->second;
            // If we've consumed all the changes that the subscription has seen,
            // we can jump ahead to whatever stamp the parent feed says is the
            // latest it's decided whether or not to pass to the subscription.
            if (pair.second.next_expected_stamp >= sub_stamp) {
                if (!feed_stamps) {
                    feed_stamps = sub->parent_feed()->get_stamps();
                }
                auto ft = feed_stamps->find(pair.first);
                if (ft != feed_stamps->end()) {
                    pair.second.next_expected_stamp =
                        std::max(pair.second.next_expected_stamp,
                                 ft->second + 1);
                }
            }
        }
    }

    void update_ranges() {
        active_state = src->get_active_state();
        r_sanity_check(active_state);
        for (const auto &pair : active_state->shard_last_read_stamps) {
            add_range(pair.first, pair.second.first, pair.second.second);
        }
    }

    void add_range(uuid_u uuid, key_range_t read_range, uint64_t stamp) {
        // Safe because we never generate `store_key_t::max()`.
        if (read_range.right.unbounded) {
            read_range.right.unbounded = false;
            read_range.right.internal_key = store_key_t::max();
        }
        auto it = stamped_ranges.find(uuid);
        r_sanity_check(it != stamped_ranges.end());
        if (it->second.ranges.size() == 0) {
            it->second.left_fencepost = read_range.left;
            it->second.ranges.push_back(std::make_pair(std::move(read_range), stamp));
        } else if (it->second.ranges.back().second == stamp) {
            it->second.ranges.back().first.right = read_range.right;
        } else {
            it->second.ranges.push_back(std::make_pair(std::move(read_range), stamp));
        }
    }

    void remove_outdated_ranges() {
        for (auto &&pair : stamped_ranges) {
            auto *ranges = &pair.second.ranges;
            while (ranges->size() > 0) {
                uint64_t read_stamp = ranges->front().second;
                if (pair.second.next_expected_stamp >= read_stamp) {
                    pair.second.left_fencepost = ranges->front().first.right.key();
                    ranges->pop_front();
                } else {
                    break;
                }
            }
        }
    }

    const reql_version_t &reql_version() const {
        r_sanity_check(active_state);
        r_sanity_check(active_state->reql_version);
        return *(active_state->reql_version);
    }

    bool ready() {
        // It's OK to cache this because we only ever call `ready` once we're
        // done doing reads.
        if (!cached_ready) {
            remove_outdated_ranges();
            for (const auto &pair : stamped_ranges) {
                if (pair.second.ranges.size() != 0) {
                    return cached_ready;
                }
            }
            sub->maybe_enable_squashing();
            cached_ready = true;
        }
        return cached_ready;
    }

    bool read_once, cached_ready;
    counted_t<datum_stream_t> src;
    boost::optional<active_state_t> active_state;
    std::map<uuid_u, stamped_range_t> stamped_ranges;
};

subscription_t::subscription_t(
    rdb_context_t *_rdb_context,
    const auth::user_context_t &_user_context,
    feed_t *_feed,
    configured_limits_t _limits,
    const datum_t &_squash,
    bool _include_states,
    bool _include_types)
    : skipped(0),
      feed(_feed),
      limits(std::move(_limits)),
      squash(_squash.as_bool()),
      include_states(_include_states),
      include_types(_include_types),
      mid_batch(false),
      min_interval(_squash.get_type() == datum_t::R_NUM ? _squash.as_num() : 0.0),
      rdb_context(_rdb_context),
      user_context(_user_context),
      cond(NULL),
      queue_nearly_full_cond(NULL) {
    guarantee(feed != NULL);
}

subscription_t::~subscription_t() { }

void subscription_t::set_notes(response_t *res) const {
    if (include_states) res->add_note(Response::INCLUDES_STATES);
}

std::vector<datum_t>
subscription_t::get_els(batcher_t *batcher,
                        return_empty_normal_batches_t return_empty_normal_batches,
                        const signal_t *interruptor) {
    assert_thread();
    guarantee(cond == NULL); // Can't get while blocking.
    auto_drainer_t::lock_t lock(get_drainer());

    std::vector<datum_t> ret;

    // We wait for data if we don't have any or if we're squashing and not
    // in the middle of a logical batch.
    if (!exc && skipped == 0 && (!has_el() || (!mid_batch && min_interval > 0.0))) {
        scoped_ptr_t<signal_timer_t> batch_timer;
        if (batcher->get_batch_type() == batch_type_t::NORMAL_FIRST) {
            batch_timer = make_scoped<signal_timer_t>(0);
        } else if (return_empty_normal_batches == return_empty_normal_batches_t::YES) {
            batch_timer = make_scoped<signal_timer_t>(batcher->microtime_left() / 1000);
        }
        // If we have to wait, wait.
        if (min_interval > 0.0
            && batcher->get_batch_type() != batch_type_t::NORMAL_FIRST) {
            signal_timer_t squash_timer(min_interval * 1000);
            cond_t wait_for_nearly_full_queue;
            queue_nearly_full_cond = &wait_for_nearly_full_queue;
            try {
                wait_any_t any_interruptor(interruptor, &squash_timer);
                // Make sure to wait on `wait_for_nearly_full_queue` because we
                // don't trust `queue_nearly_full_cond` to not be `NULL`
                // already(although I'm not sure why we don't trust that).
                wait_interruptible(&wait_for_nearly_full_queue, &any_interruptor);
            } catch (const interrupted_exc_t &e) {
                queue_nearly_full_cond = NULL;
                // If we were really interrupted, rethrow.
                if (!squash_timer.is_pulsed()) throw e;
            }
            r_sanity_check(queue_nearly_full_cond == NULL);
        }
        // If we still don't have data, wait for data with a timeout.  (Note
        // that if we're squashing, we started the timeout *before* waiting
        // on `min_timer`.)
        apply_queued_changes();
        while (!has_el() && !exc && skipped == 0) {
            cond_t wait_for_data;
            cond = &wait_for_data;
            try {
                // We don't need to wait on the drain signal because the interruptor
                // will be pulsed if we're shutting down.  Not that `cond` might
                // already be reset by the time we get here, so make sure to wait on
                // `&wait_for_data`.
                if (batch_timer.has()) {
                    wait_any_t any_interruptor(interruptor, batch_timer.get());
                    wait_interruptible(&wait_for_data, &any_interruptor);
                } else {
                    wait_interruptible(&wait_for_data, interruptor);
                }
            } catch (const interrupted_exc_t &e) {
                cond = NULL;
                if (batch_timer.has() && batch_timer->is_pulsed()) {
                    r_sanity_check(ret.size() == 0);
                    return ret;
                }
                throw e;
            }
            r_sanity_check(cond == NULL);
            apply_queued_changes();
        }
    }

    if (exc) {
        std::rethrow_exception(exc);
    } else if (skipped != 0) {
        ret.push_back(
            datum_t(
                std::map<datum_string_t, datum_t>{
                    {datum_string_t("error"), datum_t(
                            datum_string_t(
                                strprintf("Changefeed cache over array size limit, "
                                          "skipped %zu elements.", skipped)))}}));
        skipped = 0;
    } else if (has_el()) {
        while (has_el() && !batcher->should_send_batch(ignore_latency_t::YES)) {
            datum_t el = pop_el();
            batcher->note_el(el);
            ret.push_back(std::move(el));
        }
        // We're in the middle of one logical batch if we stopped early because
        // of the batcher rather than because we ran out of elements.  (This
        // matters for squashing.)
        mid_batch = batcher->should_send_batch();
    } else {
        r_sanity_check(false);
    }

    // FIXME changefeeds on artificial tables
    if (feed != nullptr &&
            !feed->get_table_id().is_nil() &&
            feed->get_table_meta_client() != nullptr &&
            rdb_context != nullptr) {
        try {
            table_basic_config_t table_basic_config;
            feed->get_table_meta_client()->get_name(
                feed->get_table_id(), &table_basic_config);

            user_context.require_read_permission(
                rdb_context, table_basic_config.database, feed->get_table_id());
        } catch (no_such_table_exc_t const &no_such_table_exc) {
            stop(
                std::make_exception_ptr(
                    datum_exc_t(base_exc_t::OP_FAILED, no_such_table_exc.what())),
                detach_t::NO);
        } catch (auth::permission_error_t const permission_error) {
            stop(
                std::make_exception_ptr(
                    datum_exc_t(base_exc_t::PERMISSION_ERROR, permission_error.what())),
                detach_t::NO);
        }
    }

    r_sanity_check(ret.size() != 0);
    return ret;
}

void subscription_t::stop(std::exception_ptr _exc, detach_t detach) {
    assert_thread();
    if (detach == detach_t::YES) {
        feed = NULL;
    }
    exc = std::move(_exc);
    maybe_signal_cond();
}

void subscription_t::maybe_signal_cond() THROWS_NOTHING {
    assert_thread();
    if (cond != NULL) {
        ASSERT_NO_CORO_WAITING;
        cond->pulse();
        cond = NULL;
    }
}

void subscription_t::maybe_signal_queue_nearly_full_cond() THROWS_NOTHING {
    assert_thread();
    if (queue_nearly_full_cond != NULL) {
        ASSERT_NO_CORO_WAITING;
        queue_nearly_full_cond->pulse();
        queue_nearly_full_cond = NULL;
    }
}

void subscription_t::destructor_cleanup(std::function<void()> del_sub) THROWS_NOTHING {
    // This error is only sent if we're getting destroyed while blocking.
    stop(std::make_exception_ptr(
             datum_exc_t(base_exc_t::OP_FAILED,
                         "Subscription destroyed (shutting down?).")),
         detach_t::NO);
    if (feed != NULL) {
        del_sub();
    } else {
        // We only get here if we were detached.
        guarantee(exc);
    }
}
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(
        sorting_t, int8_t,
        sorting_t::UNORDERED, sorting_t::DESCENDING);

keyspec_t::~keyspec_t() { }

RDB_MAKE_SERIALIZABLE_4_FOR_CLUSTER(
    keyspec_t::range_t, transforms, sindex, sorting, datumspec);
RDB_MAKE_SERIALIZABLE_0_FOR_CLUSTER(keyspec_t::empty_t);
RDB_MAKE_SERIALIZABLE_2_FOR_CLUSTER(keyspec_t::limit_t, range, limit);
RDB_MAKE_SERIALIZABLE_1_FOR_CLUSTER(keyspec_t::point_t, key);

void feed_t::add_sub_with_lock(
    rwlock_t *rwlock, const std::function<void()> &f) THROWS_NOTHING {
    on_thread_t th(home_thread());
    // This check should be true because `add_sub_with_lock` is only called by
    // the `X_sub_t` constructors, which should only be called by `new_sub`,
    // which is called while holding `feeds_lock`, which prevents the feed from
    // being detached.
    guarantee(!detached);
    num_subs += 1;
    auto_drainer_t::lock_t lock = get_drainer_lock();
    rwlock_in_line_t spot(rwlock, access_t::write);
    // TODO: In some cases there's work we could do with just the read signal.
    spot.write_signal()->wait_lazily_unordered();
    f();
}

template<class Map, class Key, class Sub>
void map_add_sub(Map *map, const Key &key, Sub *sub) THROWS_NOTHING {
    auto it = map->find(key);
    if (it == map->end()) {
        auto pair = std::make_pair(key, decltype(it->second)(get_num_threads()));
        it = map->insert(std::move(pair)).first;
    }
    auto pair = (it->second)[sub->home_thread().threadnum].insert(sub);
    guarantee(pair.second);
}

void feed_t::del_sub_with_lock(
    rwlock_t *rwlock, const std::function<size_t()> &f) THROWS_NOTHING {
    on_thread_t th(home_thread());
    {
        // Note that we proceed deleting the subscription, even if the `feed_t` has been
        // detached. This is important because the destructor of a subscription might
        // call `del_sub` in order to remove itself from the map, and not going
        // through with it would leave an invalid pointer in the map. `stop_subs` might
        // then try to use that pointer and cause memory corruption or crash.
        auto_drainer_t::lock_t lock = get_drainer_lock();
        rwlock_in_line_t spot(rwlock, access_t::write);
        spot.write_signal()->wait_lazily_unordered();
        size_t erased = f();
        if (erased == 0) {
            guarantee(detached);
            return;
        }
    }
    guarantee(num_subs > 0);
    num_subs -= 1;
    if (num_subs == 0) {
        // It's possible that by the time we get the lock to remove the feed,
        // another subscriber might have already found the feed and subscribed.
        maybe_remove_feed();
    }
}

template<class Map, class Key, class Sub>
size_t map_del_sub(Map *map, const Key &key, Sub *sub) THROWS_NOTHING {
    auto subvec_it = map->find(key);
    if (subvec_it == map->end()) {
        return 0;
    }
    size_t erased = (subvec_it->second)[sub->home_thread().threadnum].erase(sub);
    // If there are no more subscribers, remove the key from the map.
    auto it = subvec_it->second.begin();
    for (; it != subvec_it->second.end(); ++it) {
        if (it->size() != 0) {
            break;
        }
    }
    if (it == subvec_it->second.end()) {
        map->erase(subvec_it);
    }
    return erased;
}

// If this throws we might leak the increment to `num_subs`.
void feed_t::add_point_sub(point_sub_t *sub, const store_key_t &key) THROWS_NOTHING {
    add_sub_with_lock(&point_subs_lock, [this, sub, &key]() {
            map_add_sub(&point_subs, key, sub);
        });
}

// Can't throw because it's called in a destructor.
void feed_t::del_point_sub(point_sub_t *sub, const store_key_t &key) THROWS_NOTHING {
    del_sub_with_lock(&point_subs_lock, [this, sub, &key]() {
            return map_del_sub(&point_subs, key, sub);
        });
}

// If this throws we might leak the increment to `num_subs`.
void feed_t::add_range_sub(range_sub_t *sub) THROWS_NOTHING {
    add_sub_with_lock(&range_subs_lock, [this, sub]() {
            auto pair = range_subs[sub->home_thread().threadnum].insert(sub);
            guarantee(pair.second);
        });
}

// Can't throw because it's called in a destructor.
void feed_t::del_range_sub(range_sub_t *sub) THROWS_NOTHING {
    del_sub_with_lock(&range_subs_lock, [this, sub]() {
            return range_subs[sub->home_thread().threadnum].erase(sub);
        });
}

// If this throws we might leak the increment to `num_subs`.
void feed_t::add_empty_sub(empty_sub_t *sub) THROWS_NOTHING {
    add_sub_with_lock(&empty_subs_lock, [this, sub]() {
        auto pair = empty_subs[sub->home_thread().threadnum].insert(sub);
        guarantee(pair.second);
    });
}

// Can't throw because it's called in a destructor.
void feed_t::del_empty_sub(empty_sub_t *sub) THROWS_NOTHING {
    del_sub_with_lock(&empty_subs_lock, [this, sub]() {
        return empty_subs[sub->home_thread().threadnum].erase(sub);
    });
}

// If this throws we might leak the increment to `num_subs`.
void feed_t::add_limit_sub(limit_sub_t *sub, const uuid_u &sub_uuid) THROWS_NOTHING {
    add_sub_with_lock(&limit_subs_lock, [this, sub, &sub_uuid]() {
            map_add_sub(&limit_subs, sub_uuid, sub);
        });
}

// Can't throw because it's called in a destructor.
void feed_t::del_limit_sub(limit_sub_t *sub, const uuid_u &sub_uuid) THROWS_NOTHING {
    del_sub_with_lock(&limit_subs_lock, [this, sub, &sub_uuid]() {
            stop_limit_sub(sub);
            return map_del_sub(&limit_subs, sub_uuid, sub);
        });
}

template<class Sub>
void feed_t::each_sub_in_vec(
    const std::vector<std::set<Sub *> > &vec,
    rwlock_in_line_t *spot,
    const auto_drainer_t::lock_t &lock,
    const std::function<void(Sub *)> &f) THROWS_NOTHING {
    assert_thread();
    guarantee(lock.has_lock());
    spot->read_signal()->wait_lazily_unordered();

    std::vector<int> subscription_threads;
    for (int i = 0; i < get_num_threads(); ++i) {
        if (vec[i].size() != 0) {
            subscription_threads.push_back(i);
        }
    }
    pmap(subscription_threads.size(),
         [&f, &vec, &subscription_threads](int i) {
             guarantee(vec[subscription_threads[i]].size() != 0);
             on_thread_t th((threadnum_t(subscription_threads[i])));
             for (Sub *sub : vec[subscription_threads[i]]) {
                 f(sub);
             }
         });
}

void feed_t::each_range_sub(
    const auto_drainer_t::lock_t &lock,
    const std::function<void(range_sub_t *)> &f) THROWS_NOTHING {
    assert_thread();
    rwlock_in_line_t spot(&range_subs_lock, access_t::read);
    each_sub_in_vec(range_subs, &spot, lock, f);
}

void feed_t::each_point_sub_cb(const std::function<void(point_sub_t *)> &f, int i) {
    on_thread_t th((threadnum_t(i)));
    for (auto const &pair : point_subs) {
        for (point_sub_t *sub : pair.second[i]) {
            f(sub);
        }
    }
}

void feed_t::each_point_sub_with_lock(
    rwlock_in_line_t *spot,
    const std::function<void(point_sub_t *)> &f) THROWS_NOTHING {
    spot->read_signal()->wait_lazily_unordered();
    pmap(get_num_threads(),
         std::bind(&feed_t::each_point_sub_cb,
                   this,
                   std::cref(f),
                   ph::_1));
}

void feed_t::each_limit_sub_cb(const std::function<void(limit_sub_t *)> &f, int i) {
    on_thread_t th((threadnum_t(i)));
    for (auto const &pair : limit_subs) {
        for (limit_sub_t *sub : pair.second[i]) {
            f(sub);
        }
    }
}

void feed_t::each_limit_sub_with_lock(
    rwlock_in_line_t *spot,
    const std::function<void(limit_sub_t *)> &f) THROWS_NOTHING {
    spot->read_signal()->wait_lazily_unordered();
    pmap(get_num_threads(),
         std::bind(&feed_t::each_limit_sub_cb,
                   this,
                   std::cref(f),
                   ph::_1));

}

void feed_t::on_point_sub(
    store_key_t key,
    const auto_drainer_t::lock_t &lock,
    const std::function<void(point_sub_t *)> &f) THROWS_NOTHING {
    assert_thread();
    guarantee(lock.has_lock());
    rwlock_in_line_t spot(&point_subs_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();

    auto point_sub = point_subs.find(key);
    if (point_sub != point_subs.end()) {
        each_sub_in_vec(point_sub->second, &spot, lock, f);
    }
}

void feed_t::on_limit_sub(
    const uuid_u &sub_uuid,
    const auto_drainer_t::lock_t &lock,
    const std::function<void(limit_sub_t *)> &f) THROWS_NOTHING {
    assert_thread();
    guarantee(lock.has_lock());
    rwlock_in_line_t spot(&limit_subs_lock, access_t::read);
    spot.read_signal()->wait_lazily_unordered();

    auto limit_sub = limit_subs.find(sub_uuid);
    if (limit_sub != limit_subs.end()) {
        each_sub_in_vec(limit_sub->second, &spot, lock, f);
    }
}

bool feed_t::can_be_removed() {
    assert_thread();
    return num_subs == 0;
}

// This should only be called after the feed has been removed from the client,
// because otherwise there could be a race condition where a new sub is added in
// the middle.
void feed_t::stop_subs(const auto_drainer_t::lock_t &lock) {
    assert_thread();
    const char *msg = "Changefeed aborted (unavailable).";
    auto f = std::bind(&subscription_t::stop,
                       ph::_1,
                       std::make_exception_ptr(
                           datum_exc_t(base_exc_t::OP_FAILED, msg)),
                       detach_t::YES);
    {
        rwlock_in_line_t spot(&range_subs_lock, access_t::write);
        spot.write_signal()->wait_lazily_unordered();
        each_sub_in_vec<range_sub_t>(range_subs, &spot, lock, f);
        for (auto &&set : range_subs) {
            num_subs -= set.size();
            set.clear();
        }
    }
    {
        rwlock_in_line_t spot(&empty_subs_lock, access_t::write);
        spot.write_signal()->wait_lazily_unordered();
        each_sub_in_vec<empty_sub_t>(empty_subs, &spot, lock, f);
        for (auto &&set : empty_subs) {
            num_subs -= set.size();
            set.clear();
        }
    }
    {
        rwlock_in_line_t spot(&point_subs_lock, access_t::write);
        spot.write_signal()->wait_lazily_unordered();
        each_point_sub_with_lock(&spot, f);
        for (auto &&pair : point_subs) {
            for (auto &&set : pair.second) {
                num_subs -= set.size();
            }
        }
        point_subs.clear();
    }
    {
        rwlock_in_line_t spot(&limit_subs_lock, access_t::write);
        spot.write_signal()->wait_lazily_unordered();
        each_limit_sub_with_lock(&spot, f);
        for (auto &&pair : limit_subs) {
            for (auto &&set : pair.second) {
                num_subs -= set.size();
            }
        }
        limit_subs.clear();
    }
    guarantee(num_subs == 0);
}

feed_t::feed_t(namespace_id_t const &_table_id, table_meta_client_t *_table_meta_client)
  : detached(false),
    num_subs(0),
    empty_subs(get_num_threads()),
    range_subs(get_num_threads()),
    table_id(_table_id),
    table_meta_client(_table_meta_client) { }

feed_t::~feed_t() {
    guarantee(num_subs == 0);
    guarantee(detached);
}

client_t::client_t(
        mailbox_manager_t *_manager,
        const std::function<
            namespace_interface_access_t(
                const namespace_id_t &,
                signal_t *)
            > &_namespace_source) :
    manager(_manager),
    namespace_source(_namespace_source)
{
    guarantee(manager != NULL);
}
client_t::~client_t() { }

scoped_ptr_t<subscription_t> new_sub(
    env_t *env,
    feed_t *feed,
    const streamspec_t &ss) {
    struct spec_visitor_t : public boost::static_visitor<subscription_t *> {
        explicit spec_visitor_t(env_t *_env, feed_t *_feed, const streamspec_t *_ss)
            : env(_env), feed(_feed), ss(_ss) { }
        subscription_t *operator()(const keyspec_t::range_t &range) const {
            rcheck_datum(!ss->include_offsets, base_exc_t::LOGIC,
                         "Cannot include offsets for range subs.");
            return new range_sub_t(
                env->get_rdb_ctx(),
                env->get_user_context(),
                feed,
                ss->limits,
                ss->squash,
                ss->include_states,
                ss->include_types,
                env,
                range);
        }
        subscription_t *operator()(const keyspec_t::empty_t &) const {
            rcheck_datum(!ss->include_offsets, base_exc_t::LOGIC,
                         "Cannot include offsets for empty subs.");
            return new empty_sub_t(
                env->get_rdb_ctx(),
                env->get_user_context(),
                feed,
                ss->limits,
                ss->squash,
                ss->include_states,
                ss->include_types);
        }
        subscription_t *operator()(const keyspec_t::limit_t &limit) const {
            return new limit_sub_t(
                env->get_rdb_ctx(),
                env->get_user_context(),
                feed,
                ss->limits,
                ss->squash,
                ss->include_offsets,
                ss->include_states,
                ss->include_types,
                limit);
        }
        subscription_t *operator()(const keyspec_t::point_t &point) const {
            rcheck_datum(!ss->include_offsets, base_exc_t::LOGIC,
                         "Cannot include offsets for point subs.");
            return new point_sub_t(
                env->get_rdb_ctx(),
                env->get_user_context(),
                feed,
                ss->limits,
                ss->squash,
                ss->include_states,
                ss->include_types,
                point.key);
        }
        env_t *env;
        feed_t *feed;
        const streamspec_t *ss;
    };
    return scoped_ptr_t<subscription_t>(
        boost::apply_visitor(spec_visitor_t(env, feed, &ss), ss.spec));
}

streamspec_t::streamspec_t(counted_t<datum_stream_t> _maybe_src,
                           std::string _table_name,
                           bool _include_offsets,
                           bool _include_states,
                           bool _include_types,
                           configured_limits_t _limits,
                           datum_t _squash,
                           keyspec_t::spec_t _spec) :
    maybe_src(std::move(_maybe_src)),
    table_name(std::move(_table_name)),
    include_offsets(std::move(_include_offsets)),
    include_states(std::move(_include_states)),
    include_types(std::move(_include_types)),
    limits(std::move(_limits)),
    squash(std::move(_squash)),
    spec(std::move(_spec)) { }

counted_t<datum_stream_t> client_t::new_stream(
    env_t *env,
    const streamspec_t &ss,
    const namespace_id_t &table_id,
    backtrace_id_t bt,
    table_meta_client_t *table_meta_client) {
    bool is_second_try = false;
    uuid_u last_feed_uuid;
    for (;;) {
        try {
            scoped_ptr_t<subscription_t> sub;
            boost::variant<scoped_ptr_t<range_sub_t>, scoped_ptr_t<point_sub_t> > presub;
            addr_t addr;
            {
                threadnum_t old_thread = get_thread_id();
                cross_thread_signal_t interruptor(env->interruptor, home_thread());
                on_thread_t th(home_thread());
                scoped_ptr_t<real_feed_t> destroy;
                // If the `client_t` is being destroyed, we're shutting down, so we
                // consider it an interruption.
                auto_drainer_t::lock_t lock(&drainer, throw_if_draining_t::YES);
                rwlock_in_line_t spot(&feeds_lock, access_t::write);
                spot.read_signal()->wait_lazily_unordered();
                auto feed_it = feeds.find(table_id);

                if (is_second_try) {
                    guarantee(!last_feed_uuid.is_unset());
                    if (feed_it != feeds.end()
                        && feed_it->second->get_table_id() == last_feed_uuid) {
                        // We enter this branch if we got a `RESUMABLE_OP_FAILED`
                        // exception, tried again, and found the same feed (which is
                        // presumably in a broken state and needs to be replaced).
                        // We want to destroy the feed after the lock is released,
                        // because it may be expensive.
                        spot.write_signal()->wait_lazily_unordered();
                        destroy.swap(feed_it->second);
                        destroy->mark_detached();
                        feeds.erase(feed_it);
                        feed_it = feeds.end();
                        destroy->stop_subs(destroy->get_drainer_lock());
                    }
                }

                if (feed_it == feeds.end()) {
                    spot.write_signal()->wait_lazily_unordered();
                    namespace_interface_access_t access =
                        namespace_source(table_id, &interruptor);
                    // Even though we have the user's feed here, multiple
                    // users may share a feed_t, and this code path will
                    // only be run for the first one.  Rather than mess
                    // about, just use the defaults.
                    auto val = make_scoped<real_feed_t>(
                        lock, this, manager, access.get(), table_id, &interruptor,
                        table_meta_client);
                    feed_it = feeds.insert(
                        std::make_pair(table_id, std::move(val))).first;
                }

                guarantee(feed_it != feeds.end());
                real_feed_t *feed = feed_it->second.get();
                last_feed_uuid = feed->get_table_id();
                addr = feed->get_addr();

                // We need to do this while holding `feeds_lock` to make sure
                // the feed isn't destroyed before we subscribe to it.  If you
                // want to change this behavior to make it more efficient, make
                // sure `feed_t::stop_subs` remains correct.
                on_thread_t th2(old_thread);
                sub = new_sub(env, feed, ss);
            }
            namespace_interface_access_t access =
                namespace_source(table_id, env->interruptor);
            return sub->to_stream(env, ss.table_name, access.get(),
                                  addr, ss.maybe_src, std::move(sub), bt);
        } catch (const cannot_perform_query_exc_t &e) {
            rfail_datum(base_exc_t::OP_FAILED,
                        "cannot subscribe to table `%s`: %s",
                        ss.table_name.c_str(), e.what());
        } catch (const base_exc_t &e) {
            if (e.get_type() == base_exc_t::RESUMABLE_OP_FAILED) {
                if (is_second_try) {
                    // We don't want multiple layers trying to resume the same
                    // operation.
                    e.rethrow_with_type(base_exc_t::OP_FAILED);
                } else {
                    is_second_try = true;
                }
            } else {
                throw;
            }
        }
    }
}

void client_t::maybe_remove_feed(
    const auto_drainer_t::lock_t &lock, const uuid_u &uuid) {
    assert_thread();
    lock.assert_is_holding(&drainer);
    scoped_ptr_t<real_feed_t> destroy;
    rwlock_in_line_t spot(&feeds_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    auto feed_it = feeds.find(uuid);
    // The feed might have disappeared because it may have been detached while
    // we held the lock, in which case we don't need to do anything.  The feed
    // might also have gotten a new subscriber, in which case we don't want to
    // remove it yet.
    if (feed_it != feeds.end() && feed_it->second->can_be_removed()) {
        // We want to destroy the feed after the lock is released, because it
        // may be expensive.
        destroy.swap(feed_it->second);
        feeds.erase(feed_it);
    }
}

scoped_ptr_t<real_feed_t> client_t::detach_feed(
        const auto_drainer_t::lock_t &lock, real_feed_t *expected_feed) {
    assert_thread();
    lock.assert_is_holding(&drainer);
    scoped_ptr_t<real_feed_t> ret;
    rwlock_in_line_t spot(&feeds_lock, access_t::write);
    spot.write_signal()->wait_lazily_unordered();
    // The feed might have been removed in `maybe_remove_feed`, in which case
    // there's nothing to detach.
    // It's also possible that the feed had been removed and a new feed has since been
    // added for this table uuid, so we need to compare the pointer to `expected_feed`.
    auto feed_it = feeds.find(expected_feed->get_table_id());
    if (feed_it != feeds.end() && feed_it->second.get_or_null() == expected_feed) {
        ret.swap(feed_it->second);
        ret->mark_detached();
        feeds.erase(feed_it);
    }
    return ret;
}

class artificial_feed_t : public feed_t {
public:
    explicit artificial_feed_t(artificial_t *_parent)
        : feed_t(nil_uuid(), nullptr),
          parent(_parent) { }
    ~artificial_feed_t() { detached = true; }
    virtual auto_drainer_t::lock_t get_drainer_lock() { return drainer.lock(); }
    virtual void maybe_remove_feed() { parent->maybe_remove(); }

    void abort_feed() final {
        stop_subs(get_drainer_lock());
        maybe_remove_feed();
    }

    NORETURN virtual void stop_limit_sub(limit_sub_t *) {
        crash("Limit subscriptions are not supported on artificial feeds.");
    }
private:
    artificial_t *parent;
    auto_drainer_t drainer;
};

artificial_t::artificial_t()
    : stamp(0), uuid(generate_uuid()), feed(make_scoped<artificial_feed_t>(this)) { }
artificial_t::~artificial_t() { }

counted_t<datum_stream_t> artificial_t::subscribe(
    env_t *env,
    const streamspec_t &ss,
    const std::string &primary_key_name,
    const std::vector<datum_t> &initial_values,
    backtrace_id_t bt) {
    // It's OK not to switch threads here because `feed.get()` can be called
    // from any thread and `new_sub` ends up calling `feed_t::add_sub_with_lock`
    // which does the thread switch itself.  If you later change this to switch
    // threads, make sure that the `subscription_t` and `stream_t` are allocated
    // on the thread you want to use them on.
    guarantee(feed.has());
    const streamspec_t *unsquashed_ss = &ss;
    scoped_ptr_t<streamspec_t> dup;
    if (ss.squash != datum_t::boolean(false)) {
        dup = make_scoped<streamspec_t>(ss);
        dup->squash = datum_t::boolean(false);
        unsquashed_ss = &*dup;
    }
    scoped_ptr_t<subscription_t> sub = new_sub(env, feed.get(), *unsquashed_ss);
    return sub->to_artificial_stream(
        uuid, primary_key_name, initial_values,
        unsquashed_ss->maybe_src.has(), std::move(sub), bt);
}

void artificial_t::send_all(const msg_t &msg) {
    assert_thread();
    // We acquire `stamp_mutex` to ensure that multiple calls to `msg_visit` don't
    // interleave, or `feed->update_stamps` that's called from `msg_visit` might update
    // stamps in the wrong order.
    new_mutex_acq_t stamp_acq(&stamp_mutex);
    if (auto *change = boost::get<msg_t::change_t>(&msg.op)) {
        guarantee(change->old_val.has() || change->new_val.has());
        if (change->old_val.has() && change->new_val.has()) {
            rassert(change->old_val != change->new_val);
        }
    }
    auto_drainer_t::lock_t lock = feed->get_drainer_lock();
    msg_visit(feed.get(), &lock, uuid, stamp++, msg.op);
}

bool artificial_t::can_be_removed() {
    assert_thread();
    return feed->can_be_removed();
}

} // namespace changefeed
} // namespace ql
