// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "clustering/administration/servers/name_server.hpp"

/* `server_priority_less()` decides which of two servers should get priority in a name
collision. It returns `true` if the priority of the first server is less than the
priority of the second server. */
static bool server_priority_less(
        const change_tracking_map_t<peer_id_t, cluster_directory_metadata_t> &directory,
        machine_id_t machine1, machine_id_t machine2)
{
    time_t startup_time_1 = 0, startup_time_2 = 0;
    for (auto it = directory.get_inner().begin();
              it != directory.get_inner().end();
            ++it) {
        if (it->second.machine_id == machine1) {
            guarantee(it->second.server_name_business_card, "We shouldn't be in a name "
                "conflict with a proxy because proxies don't have names");
            startup_time_1 = it->second.server_name_business_card.get().startup_time;
        }
        if (it->second.machine_id == machine2) {
            guarantee(it->second.server_name_business_card, "We shouldn't be in a name "
                "conflict with a proxy because proxies don't have names");
            startup_time_2 = it->second.server_name_business_card.get().startup_time;
        }
    }
    return (startup_time_1 > startup_time_2) ||
        ((startup_time_1 == startup_time_2) && (machine1 < machine2));
}

/* `make_renamed_name()` generates an alternative name based on the input name.
Ordinarily it just appends `_renamed`. But if the name already has a `_renamed` suffix,
then it starts appending numbers; i.e. `_renamed2`, `_renamed3`, and so on. */
static name_string_t make_renamed_name(const name_string_t &name) {
    static const std::string suffix = "_renamed";
    std::string str = name.str();

    /* Parse the previous name. In particular, determine if it already has a suffix and
    what the number associated with that suffix is. */
    size_t num_end_digits = 0;
    while (num_end_digits < str.length() &&
           str[str.length()-1-num_end_digits] > '0' &&
           str[str.length()-1-num_end_digits] < '9') {
        num_end_digits++;
    }
    bool has_suffix =
        (str.length() >= num_end_digits + suffix.length()) &&
        (str.compare((str.length() - num_end_digits - suffix.length()), 
                     suffix.length(),
                     suffix) == 0);
    uint64_t end_digits_value = 0;   /* Note: the initial value will never be used */
    if (has_suffix) {
        if (num_end_digits > 0) {
            bool ok = strtou64_strict(str.substr(str.length() - num_end_digits),
                10, &end_digits_value);
            guarantee(ok, "A string made of digits should parse as an integer");
        } else {
            /* Make sure that after `_renamed` the next one is `_renamed2` */
            end_digits_value = 1;
        }
    }

    /* Generate the new name */
    std::string new_str;
    if (!has_suffix) {
        new_str = str + suffix;
    } else {
        new_str = strprintf("%s%s%" PRIu64, str.c_str(), suffix.c_str(),
            end_digits_value+1);
    }
    name_string_t new_name;
    bool ok = new_name.assign_value(new_str);
    guarantee(ok, "A valid name plus some letters and digits should be a valid name");

    guarantee(new_name != name);
    return new_name;
}

server_name_server_t::server_name_server_t(
        mailbox_manager_t *_mailbox_manager,
        machine_id_t _my_machine_id,
        clone_ptr_t<watchable_t<change_tracking_map_t<peer_id_t,
            cluster_directory_metadata_t> > > _directory_view,
        boost::shared_ptr<semilattice_readwrite_view_t<machines_semilattice_metadata_t> >
            _semilattice_view) :
    mailbox_manager(_mailbox_manager),
    my_machine_id(_my_machine_id),
    startup_time(get_secs()),
    directory_view(_directory_view),
    semilattice_view(_semilattice_view),
    rename_mailbox(mailbox_manager,
        [this](const name_string_t &name, const mailbox_t<void()>::address_t &addr) {
            this->on_rename_request(name, addr);
        }),
    semilattice_subs([this]() {
        /* We have to call `on_semilattice_change()` in a coroutine because it might
        make changes to the semilattices. */
        auto_drainer_t::lock_t keepalive(&drainer);
        coro_t::spawn_sometime([this, keepalive /* important to capture */]() {
            this->on_semilattice_change();
            });
        })
{
    /* Find our entry in the machines semilattice map and determine our current name from
    it. */
    machines_semilattice_metadata_t sl_metadata = semilattice_view->get();
    auto it = sl_metadata.machines.find(my_machine_id);
    guarantee(it != sl_metadata.machines.end(), "We should already have an entry in the "
        "semilattices");
    if (it->second.is_deleted()) {
        permanently_removed_cond.pulse();
    } else {
        my_name = it->second.get_ref().name.get_ref();
    }

    /* Check for name collisions. */
    semilattice_subs.reset(semilattice_view);
    on_semilattice_change();
}

server_name_business_card_t server_name_server_t::get_business_card() {
    server_name_business_card_t bcard;
    bcard.rename_addr = rename_mailbox.get_address();
    bcard.startup_time = startup_time;
    return bcard;
}

void server_name_server_t::rename_me(const name_string_t &new_name) {
    ASSERT_FINITE_CORO_WAITING;
    if (new_name != my_name) {
        my_name = new_name;
        machines_semilattice_metadata_t metadata = semilattice_view->get();
        deletable_t<machine_semilattice_metadata_t> *entry =
            &metadata.machines.at(my_machine_id);
        if (!entry->is_deleted()) {
            entry->get_mutable()->name = entry->get_ref().name.make_new_version(
                new_name, my_machine_id);
            semilattice_view->join(metadata);
        }
    }
}

void server_name_server_t::on_rename_request(const name_string_t &new_name,
                                             mailbox_t<void()>::address_t ack_addr) {
    if (!permanently_removed_cond.is_pulsed()) {
        logINF("Changed server's name from `%s` to `%s`.",
            my_name.c_str(), new_name.c_str());
        rename_me(new_name);
        on_semilattice_change();   /* check if we caused a name collision */
    }

    /* Send an acknowledgement to the server that initiated the request */
    auto_drainer_t::lock_t keepalive(&drainer);
    coro_t::spawn_sometime([this, keepalive /* important to capture */, ack_addr]() {
        send(this->mailbox_manager, ack_addr);
    });
}

void server_name_server_t::retag_me(const std:set<name_string_t> &new_tags) {
    ASSERT_FINITE_CORO_WAITING;
    if (new_tags != my_tags) {
        my_tags = new_tags;
        machines_semilattice_metadata_t metadata = semilattice_view->get();
        deletable_t<machine_semilattice_metadata_t> *entry =
            &metadata.machines.at(my_machine_id);
        if (!entry->is_deleted()) {
            entry->get_mutable()->tags = entry->get_ref().tags.make_new_version(
                new_tags, my_machine_id);
            semilattice_view->join(metadata);
        }
    }
}

void server_name_server_t::on_retag_request(const std::set<name_string_t> &new_tags,
                                            mailbox_t<void()>::address_t ack_addr) {
    if (!permanently_removed_cond.is_pulsed()) {
        retag_me(new_tags);
    }

    /* Send an acknowledgement to the server that initiated the request */
    auto_drainer_t::lock_t keepalive(&drainer);
    coro_t::spawn_sometime([this, keepalive /* important to capture */, ack_addr]() {
        send(this->mailbox_manager, ack_addr);
    });
}

void server_name_server_t::on_semilattice_change() {
    ASSERT_FINITE_CORO_WAITING;
    machines_semilattice_metadata_t sl_metadata = semilattice_view->get();
    guarantee(sl_metadata.machines.count(my_machine_id) == 1);

    /* Check if we've been permanently removed */
    if (sl_metadata.machines.at(my_machine_id).is_deleted()) {
        if (!permanently_removed_cond.is_pulsed()) {
            logERR("This server has been permanently removed from the cluster. Please "
                "stop the process, erase its data files, and start a fresh RethinkDB "
                "instance.");
            my_name = name_string_t();
            permanently_removed_cond.pulse();
        }
        return;
    }

    /* Check for any name collisions */
    bool must_rename_myself = false;
    std::multiset<name_string_t> names_in_use;
    for (auto it = sl_metadata.machines.begin();
              it != sl_metadata.machines.end();
            ++it) {
        if (it->second.is_deleted()) continue;
        name_string_t name = it->second.get_ref().name.get_ref();
        names_in_use.insert(name);
        if (it->first == my_machine_id) {
            guarantee(name == my_name);
        } else if (name == my_name) {
            directory_view->apply_read(
                [&](const change_tracking_map_t<peer_id_t,
                        cluster_directory_metadata_t> *directory) {
                    must_rename_myself = server_priority_less(
                        *directory, my_machine_id, it->first);
                });
        }
    }

    if (must_rename_myself) {
        /* There's a name collision, and we're the one being renamed. */
        name_string_t new_name = my_name;
        while (names_in_use.count(new_name) != 0) {
            /* Keep trying new names until we find one that isn't in use. If our original
            name is `foo`, repeated calls to `make_renamed_name` will yield
            `foo_renamed`, `foo_renamed2`, `foo_renamed3`, etc. */
            new_name = make_renamed_name(new_name);
        }
        logWRN("Automatically changed server's name from `%s` to `%s` to fix a name "
            "collision.", my_name.c_str(), new_name.c_str());
        rename_me(new_name);
    }
}

