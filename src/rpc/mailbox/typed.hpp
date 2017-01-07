// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef RPC_MAILBOX_TYPED_HPP_
#define RPC_MAILBOX_TYPED_HPP_

#include <functional>
#include <tuple>

#include "containers/archive/versioned.hpp"
#include "containers/archive/tuple.hpp"
#include "rpc/serialize_macros.hpp"
#include "rpc/mailbox/mailbox.hpp"
#include "rpc/semilattice/joins/macros.hpp"

template <class...> class mailbox_t;

template <class... Args>
class mailbox_addr_t {
public:
    bool operator<(const mailbox_addr_t<Args...> &other) const {
        return addr < other.addr;
    }
    bool is_nil() const { return addr.is_nil(); }
    peer_id_t get_peer() const { return addr.get_peer(); }

    friend class mailbox_t<Args...>;

    RDB_MAKE_ME_SERIALIZABLE_1(mailbox_addr_t, addr);
    RDB_MAKE_ME_EQUALITY_COMPARABLE_1(mailbox_addr_t<Args...>, addr);

private:
    template <class... Args2>
    friend void send(mailbox_manager_t *, mailbox_addr_t<Args2...>, const Args2 &... args);

    raw_mailbox_t::address_t addr;
};

template <class... Args>
class mailbox_t {
    class read_impl_t : public mailbox_read_callback_t {
    public:
        explicit read_impl_t(mailbox_t<Args...> *_parent) : parent(_parent) { }
        template <size_t... Is>
        void read_helper(signal_t *interruptor, std::tuple<Args...> &&tup, rindex_sequence<Is...>) {
            parent->fun(interruptor, std::move(std::get<Is>(tup))...);
        }
        void read(read_stream_t *stream, signal_t *interruptor) {
            std::tuple<Args...> args;
            archive_result_t res = deserialize<cluster_version_t::CLUSTER>(stream, &args);
            if (bad(res)) { throw fake_archive_exc_t(); }
            read_helper(interruptor, std::move(args), make_rindex_sequence<sizeof...(Args)>());
        }
    private:
        mailbox_t<Args...> *parent;
    };

    read_impl_t reader;
public:
    typedef mailbox_addr_t<Args...> address_t;

    mailbox_t(mailbox_manager_t *manager,
              const std::function< void(signal_t *, Args...)> &f) :
        reader(this), fun(f), mailbox(manager, &reader)
        { }

    void begin_shutdown() {
        mailbox.begin_shutdown();
    }

    address_t get_address() const {
        address_t a;
        a.addr = mailbox.get_address();
        return a;
    }

private:
    template <class... Args2>
    friend void send(mailbox_manager_t *, mailbox_addr_t<Args2...>, const Args2 &... args);

    std::function< void(signal_t *, Args...) > fun;
    raw_mailbox_t mailbox;
};

template <class... Args>
class mailbox_write_impl : public mailbox_write_callback_t {
private:
    const std::tuple<Args...> args;
public:
    explicit mailbox_write_impl(const Args &... _args) : args(_args...) { }
    void write(DEBUG_VAR cluster_version_t cluster_version, write_message_t *wm) {
        rassert(cluster_version == cluster_version_t::CLUSTER);
        serialize<cluster_version_t::CLUSTER>(wm, args);
    }
#ifdef ENABLE_MESSAGE_PROFILER
    const char *message_profiler_tag() const {
        static const std::string tag =
            strprintf("mailbox<%s>", typeid(std::tuple<Args...>).name());
        return tag.c_str();
    }
#endif
};

template <class... Args>
void send(mailbox_manager_t *src, mailbox_addr_t<Args...> dest, const Args &... args) {
    mailbox_write_impl<Args...> writer(args...);
    send_write(src, dest.addr, &writer);
}

#endif // RPC_MAILBOX_TYPED_HPP_
