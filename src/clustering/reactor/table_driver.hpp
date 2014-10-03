// Copyright 2010-2014 RethinkDB, all rights reserved.
#ifndef CLUSTERING_REACTOR_TABLE_DRIVER_HPP_
#define CLUSTERING_REACTOR_TABLE_DRIVER_HPP_

class table_driver_business_card_t {
public:
    typedef mailbox_t<void(
        namespace_id_t,
        table_raft_instance_id_t,
        raft_config_t,
        table_raft_state_t
        )> force_mailbox_t;

    typedef mailbox_t<void(
        namespace_id_t,
        table_raft_instance_id_t
        )> recruit_mailbox_t;

    std::map<namespace_id_t, table_raft_business_card_t> rafts;
};

class table_driver_t {
public:
    table_driver_t(
        mailbox_manager_t *_mailbox_manager,
        clone_ptr_t<watchable_t<change_tracking_map_t<
            peer_id_t, table_driver_business_card_t> > > _directory_view);

    clone_ptr_t<watchable_t<table_driver_business_card_t> > get_business_card() {
        return business_card.get_watchable();
    }

private:
    mailbox_manager_t *mailbox_manager;
    clone_ptr_t<watchable_t<change_tracking_map_t<
        peer_id_t, table_driver_business_card_t> > > directory_view;

    watchable_variable_t<table_driver_business_card_t> business_card;

    
};

#endif /* CLUSTERING_REACTOR_TABLE_DRIVER_HPP_ */

