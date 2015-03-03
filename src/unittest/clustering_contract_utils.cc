// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "unittest/clustering_contract_utils.hpp"

namespace unittest {

region_map_t<version_t> quick_cpu_version_map(
        size_t which_cpu_subspace,
        std::initializer_list<quick_cpu_version_map_args_t> qvms) {
    std::vector<std::pair<region_t, version_t> > parts;
    for (const quick_cpu_version_map_args_t &qvm : qvms) {
        key_range_t range = quick_range(qvm.quick_range_spec);
        region_t region = region_intersection(
            region_t(range), cpu_sharding_subspace(which_cpu_subspace));
        version_t version;
        if (qvm.branch == nullptr) {
            guarantee(qvm.timestamp == 0);
            version = version_t::zero();
        } else {
            version = version_t(
                qvm.branch->branch_ids[which_cpu_subspace],
                state_timestamp_t::from_num(qvm.timestamp));
        }
        parts.push_back({ region, version });
    }
    return region_map_t<version_t>(parts.begin(), parts.end());
}

cpu_branch_ids_t quick_cpu_branch(
        branch_history_t *bhist,
        std::initializer_list<quick_cpu_version_map_args_t> origin) {
    cpu_branch_ids_t res;

    /* Compute the region of the new "branch" */
    res.range.left = quick_range(origin.begin()->quick_range_spec).left;
    res.range.right = key_range_t::right_bound_t(res.range.left);
    for (const quick_cpu_version_map_args_t &qvm : origin) {
        key_range_t range = quick_range(qvm.quick_range_spec);
        guarantee(key_range_t::right_bound_t(range.left) == res.range.right);
        res.range.right = range.right;
        if (qvm.branch != nullptr) {
            guarantee(region_is_superset(qvm.branch->range, range));
        }
    }

    /* Create birth certificates for the individual cpu-specific branches of the new
    "branch" */
    branch_birth_certificate_t bcs[CPU_SHARDING_FACTOR];
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        bcs[i].region = region_intersection(
            region_t(res.range), cpu_sharding_subspace(i));
        bcs[i].initial_timestamp = state_timestamp_t::zero();
        bcs[i].origin = quick_cpu_version_map(i, origin);
        for (const auto &pair : bcs[i].origin) {
            bcs[i].initial_timestamp = std::max(bcs[i].initial_timestamp,
                pair.second.timestamp);
        }
        
    }

    /* Register the branches */
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        branch_id_t bid = generate_uuid();
        bhist->branches.insert(std::make_pair(bid, bcs[i]));
        res.branch_ids[i] = bid;
    }

    return res;
}

static branch_id_t branch_or_nil(const cpu_branch_ids_t *branch, size_t i) {
    if (branch != nullptr) {
        return branch->branch_ids[i];
    } else {
        return nil_uuid();
    }
}

cpu_contracts_t quick_contract_simple(
        const std::set<server_id_t> &voters,
        const server_id_t &primary,
        const cpu_branch_ids_t *branch) {
    cpu_contracts_t res;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        res.contracts[i].replicas = res.contracts[i].voters = voters;
        res.contracts[i].primary =
            boost::make_optional(contract_t::primary_t { primary, boost::none } );
        res.contracts[i].branch = branch_or_nil(branch, i);
    }
    return res;
}
cpu_contracts_t quick_contract_extra_replicas(
        const std::set<server_id_t> &voters,
        const std::set<server_id_t> &extras,
        const server_id_t &primary,
        const cpu_branch_ids_t *branch) {
    cpu_contracts_t res;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        res.contracts[i].replicas = res.contracts[i].voters = voters;
        res.contracts[i].replicas.insert(extras.begin(), extras.end());
        res.contracts[i].primary =
            boost::make_optional(contract_t::primary_t { primary, boost::none } );
        res.contracts[i].branch = branch_or_nil(branch, i);
    }
    return res;
}
        
cpu_contracts_t quick_contract_no_primary(
        const std::set<server_id_t> &voters,   /* first voter is primary */
        const cpu_branch_ids_t *branch) {
    cpu_contracts_t res;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        res.contracts[i].replicas = res.contracts[i].voters = voters;
        res.contracts[i].primary = boost::none;
        res.contracts[i].branch = branch_or_nil(branch, i);
    }
    return res;
}
cpu_contracts_t quick_contract_hand_over(
        const std::set<server_id_t> &voters,
        const server_id_t &primary,
        const server_id_t &hand_over,
        const cpu_branch_ids_t *branch) {
    cpu_contracts_t res;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        res.contracts[i].replicas = res.contracts[i].voters = voters;
        res.contracts[i].primary =
            boost::make_optional(contract_t::primary_t {
                primary, boost::make_optional(hand_over) } );
        res.contracts[i].branch = branch_or_nil(branch, i);
    }
    return res;
}
cpu_contracts_t quick_contract_temp_voters(
        const std::set<server_id_t> &voters,
        const std::set<server_id_t> &temp_voters,
        const server_id_t &primary,
        const cpu_branch_ids_t *branch) {
    cpu_contracts_t res;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        res.contracts[i].replicas = res.contracts[i].voters = voters;
        res.contracts[i].replicas.insert(temp_voters.begin(), temp_voters.end());
        res.contracts[i].temp_voters = boost::make_optional(temp_voters);
        res.contracts[i].primary =
            boost::make_optional(contract_t::primary_t { primary, boost::none } );
        res.contracts[i].branch = branch_or_nil(branch, i);
    }
    return res;
}
cpu_contracts_t quick_contract_temp_voters_hand_over(
        const std::set<server_id_t> &voters,
        const std::set<server_id_t> &temp_voters,
        const server_id_t &primary,
        const server_id_t &hand_over,
        const cpu_branch_ids_t *branch) {
    cpu_contracts_t res;
    for (size_t i = 0; i < CPU_SHARDING_FACTOR; ++i) {
        res.contracts[i].replicas = res.contracts[i].voters = voters;
        res.contracts[i].replicas.insert(temp_voters.begin(), temp_voters.end());
        res.contracts[i].temp_voters = boost::make_optional(temp_voters);
        res.contracts[i].primary =
            boost::make_optional(contract_t::primary_t {
                primary, boost::make_optional(hand_over) } );
        res.contracts[i].branch = branch_or_nil(branch, i);
    }
    return res;
}

} /* namespace unittest */


