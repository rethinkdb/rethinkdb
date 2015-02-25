/* RSI(raft): Finish these unit tests */
#if 0

/* `dummy_contract_executor_group_t` watches the contracts in the given Raft cluster
state, executes them for a collection of fake storage engines, and generates plausible
contract acks for all the servers. */
class dummy_contract_executor_group_t {
public:
    dummy_contract_executor_group_t(raft_member_t<table_raft_state_t> *_raft) :
            raft(_raft), raft_state_subs([this]() { on_raft_state_change(); }),
            timer(10, [this]() { run_all_contracts(); })
    {
        watchable_t<raft_member_t<table_raft_state_t>::state_and_config_t>::freeze_t
            freeze(raft->get_committed_state());
        raft_state_subs.reset(raft->get_committed_state(), &freeze);
    }

    watchable_map_t<std::pair<server_id_t, contract_id_t>, contract_ack_t>
            *get_contract_acks() {
        return &contract_acks;
    }

    void make_server_alive(const server_id_t &server) {
        ensure_server(server);
        servers.at(server).alive = true;
        run_all_contracts();
    }

    void make_server_dead(const server_id_t &server) {
        ensure_server(server);
        servers.at(server).alive = false;
        run_all_contracts();
    }

private:
    typedef watchable_map_var_t<std::pair<server_id_t, contract_id_t>,
        contract_ack_t>::entry_t contract_ack_entry_t;
    class server_info_t {
    public:
        region_map_t<version_t> storage;
        branch_history_t branch_history;
        bool alive;
        std::map<region_t, version_t> active_primaries;
        std::map<contract_id_t, scoped_ptr_t<contract_ack_entry_t> > contract_acks;
    };

    void ensure_server(const server_id_t &server) {
        if (servers.count(server) == 0) {
            servers[server].storage =
                region_map_t<version_t>(region_t::universe(), version_t::zero());
        }
    }

    /* For each contract, `run_contract()` will be invoked periodically. */
    void run_contract(const contract_id_t &contract_id, const region_t &region,
            const contract_t &contract) {
        /* Erase data for servers that are finished */
        for (auto &&server_pair : servers) {
            if (contract.replicas.count(server_pair.first) == 0) {
                /* Sometimes we randomly don't delete the data. We'll inevitably roll a
                zero on some call to `run_contract()`, so the data is guaranteed to be
                deleted eventually, but sometimes there will be a random delay in how
                long it takes to happen. */
                if (server_pair.second.alive && randint(0, 2) == 0) {
                    server_pair.second.storage.set(region, version_t::zero());
                    server_pair.second.contract_acks.insert(std::make_pair(
                        contract_id,
                        make_scoped<contract_ack_entry_t>(&contract_acks, contract_id,
                            contract_ack_t(contract_ack_t::state_t::nothing))));
                }
            }
        }
        if (static_cast<bool>(contract.primary)) {
            ensure_server(contract.primary->server);
            server_info_t *primary = &servers.at(contract.primary->server);
            if (primary->alive && randint(0, 2) == 0) {
                if (primary->active_primaries.count(region) == 0) {
                    branch_birth_certificate_t new_branch_info;
                    new_branch_info.region = region;
                    new_branch_info.origin = primary->storage.mask(region);
                    new_branch_info.initial_timestamp = state_timestamp_t::zero();
                    for (const auto &pair : new_branch_info.origin) {
                        new_branch_info.initial_timestamp = std::max(
                            new_branch_info.initial_timestamp, pair.second.timestamp);
                    }
                    branch_id_t new_branch_id = generate_uuid();
                    primary->branch_history.branches.insert(std::make_pair(
                        new_branch_id, new_branch_info));
                    primary->active_primaries.insert(std::make_pair(region,
                        version_t(new_branch_id, new_branch_info.initial_timestamp)));
                }
                if (contract.branch_id != primary->active_primaries.at(region).branch) {
                    if (primary->contract_acks.count(contract_id) == 0) {
                        
                    }
            
        }
        /* Have the primary request a branch ID if necessary */
    }

    void run_all_contracts() {
        raft->get_committed_state()->apply_read(
        [&](const raft_member_t<table_raft_state_t>::state_and_config_t &sc) {
            for (const auto &contract_pair : sc.state.contracts) {
                run_contract(contract_pair.first, contract_pair.second.first,
                    contract_pair.second.second);
            }
        });
    }

    void on_raft_state_change() {
        raft->get_committed_state()->apply_read(
        [&](const raft_member_t<table_raft_state_t>::state_and_config_t &sc) {
            /* Make a list of which servers are primary for which regions, and remove any
            active primaries that are no longer valid */
            std::map<region_t, server_id_t> active_primaries;
            for (const auto &contract_pair : sc.state.contracts) {
                if (static_cast<bool>(contract_pair.second.second.primary)) {
                    primaries[contract_pair.second.first] =
                        contract_pair.second.second.primary->server;
                }
            }
            for (auto &&server_pair : servers) {
                for (auto it = server_pair.second.active_primaries.begin();
                        it != server_pair.second.active_primaries.end();) {
                    if (active_primaries.count(it->first) == 0 ||
                            active_primaries[it->first] != server_pair.first) {
                        server_pair.second.active_primaries.erase(it++);
                    } else {
                        ++it;
                    }
                }
            }
            /* Remove contract acks for old contracts */
            for (auto &&server_pair : servers) {
                for (auto it = server_pair.second.contract_acks.begin();
                        it != server_pair.second.contract_acks.end();) {
                    if (sc.state.contracts.count(it->first) == 0) {
                        server_pair.second.contract_acks.erase(it++);
                    } else {
                        ++it;
                    }
                }
            }
        });
        run_all_contracts();
    }

    raft_member_t<table_raft_state_t> *raft;
    std::map<server_id_t, server_info_t> servers;
    region_map_t<version_t> acked_writes;
    watchable_map_var_t<std::pair<server_id_t, contract_id_t>, contract_ack_t>
        contract_acks;
    watchable_t<raft_member_t<table_raft_state_t>::state_and_config_t>::subscription_t
        raft_state_subs;
    repeating_timer_t timer;
}

TPTEST(ClusteringContractCoordinator, Static) {
    
}

#endif
