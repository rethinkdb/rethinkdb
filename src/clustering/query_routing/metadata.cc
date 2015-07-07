// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/query_routing/metadata.hpp"

#include "containers/archive/boost_types.hpp"
#include "containers/archive/stl_types.hpp"
#include "containers/archive/versioned.hpp"

RDB_IMPL_SERIALIZABLE_4_FOR_CLUSTER(
        primary_query_bcard_t::read_request_t,
        read, order_token, fifo_token, cont_addr);

RDB_IMPL_SERIALIZABLE_4_FOR_CLUSTER(
        primary_query_bcard_t::write_request_t, write, order_token, fifo_token, cont_addr);

RDB_IMPL_SERIALIZABLE_2_FOR_CLUSTER(primary_query_bcard_t, region, multi_client);

RDB_IMPL_EQUALITY_COMPARABLE_2(primary_query_bcard_t, region, multi_client);

RDB_IMPL_SERIALIZABLE_1_FOR_CLUSTER(direct_query_bcard_t, read_mailbox);
RDB_IMPL_EQUALITY_COMPARABLE_1(direct_query_bcard_t, read_mailbox);

RDB_IMPL_SERIALIZABLE_3_FOR_CLUSTER(table_query_bcard_t, region, primary, direct);
RDB_IMPL_EQUALITY_COMPARABLE_3(table_query_bcard_t, region, primary, direct);

