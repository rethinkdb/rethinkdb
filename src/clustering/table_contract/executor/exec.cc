// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/executor/exec.hpp"

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(contract_execution_bcard_t,
    remote_replicator_server, replica, peer);

