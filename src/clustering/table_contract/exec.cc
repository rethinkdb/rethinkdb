// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/table_contract/exec.hpp"

/* RSI(raft): This should be `SINCE_v1_N`, where `N` is the version number at which Raft
is released */
RDB_IMPL_SERIALIZABLE_3_SINCE_v1_16(contract_execution_bcard_t,
    broadcaster, replier, peer);

