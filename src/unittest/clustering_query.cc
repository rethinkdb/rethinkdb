// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/administration/admin_op_exc.hpp"
#include "clustering/query_routing/primary_query_client.hpp"
#include "clustering/query_routing/primary_query_server.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "unittest/mock_store.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

class query_counter_t : public primary_query_server_t::query_callback_t {
public:
    query_counter_t() : should_succeed(true), num_writes(0), num_reads(0) { }
    bool on_write(
            const write_t &request,
            fifo_enforcer_sink_t::exit_write_t *exiter,
            order_token_t order_token,
            UNUSED signal_t *interruptor,
            write_response_t *response_out,
            admin_err_t *error_out) {
        order_sink.check_out(order_token);
        exiter->end();
        EXPECT_TRUE(boost::get<dummy_write_t>(&request.write) != nullptr);
        ++num_writes;
        if (should_succeed) {
            *response_out = write_response_t(dummy_write_response_t());
        } else {
            *error_out = admin_err_t{
                "query_counter_t write failed",
                query_state_t::FAILED};
        }
        return should_succeed;
    }
    bool on_read(
            const read_t &request,
            fifo_enforcer_sink_t::exit_read_t *exiter,
            order_token_t order_token,
            UNUSED signal_t *interruptor,
            read_response_t *response_out,
            admin_err_t *error_out) {
        order_sink.check_out(order_token);
        exiter->end();
        EXPECT_TRUE(boost::get<dummy_read_t>(&request.read) != nullptr);
        ++num_reads;
        if (should_succeed) {
            *response_out = read_response_t(dummy_read_response_t());
        } else {
            *error_out = admin_err_t{
                "query_counter_t read failed",
                query_state_t::FAILED};
        }
        return should_succeed;
    }
    bool should_succeed;
    int num_writes, num_reads;
    order_sink_t order_sink;
};

/* The `ReadWrite` test sends some reads and writes to some shards via a
`primary_query_client_t`. */

TPTEST(ClusteringQuery, ReadWrite) {
    order_source_t order_source;
    simple_mailbox_cluster_t cluster;
    query_counter_t query_counter;
    primary_query_server_t server(
        cluster.get_mailbox_manager(), region_t::universe(), &query_counter);

    /* Set up a client */
    cond_t non_interruptor;
    primary_query_client_t client(
        cluster.get_mailbox_manager(),
        server.get_bcard(),
        &non_interruptor);

    /* Send a write to the namespace */
    {
        fifo_enforcer_sink_t::exit_write_t token;
        client.new_write_token(&token);
        write_t write;
        write.write = dummy_write_t();
        write_response_t res;
        client.write(
            write,
            &res,
            order_source.check_in("ClusteringQuery.ReadWrite.write"),
            &token,
            &non_interruptor);
        EXPECT_TRUE(boost::get<dummy_write_response_t>(&res.response) != nullptr);
        EXPECT_EQ(1, query_counter.num_writes);
    }

    /* Send a read to the namespace */
    {
        fifo_enforcer_sink_t::exit_read_t token;
        client.new_read_token(&token);
        read_t read;
        read.read = dummy_read_t();
        read_response_t res;
        client.read(
            read,
            &res,
            order_source.check_in("ClusteringQuery.ReadWrite.read").with_read_mode(),
            &token,
            &non_interruptor);
        EXPECT_TRUE(boost::get<dummy_read_response_t>(&res.response) != nullptr);
        EXPECT_EQ(1, query_counter.num_reads);
    }
}

TPTEST(ClusteringQuery, Failure) {
    order_source_t order_source;
    simple_mailbox_cluster_t cluster;
    query_counter_t query_counter;
    query_counter.should_succeed = false;
    primary_query_server_t server(
        cluster.get_mailbox_manager(), region_t::universe(), &query_counter);

    /* Set up a client */
    cond_t non_interruptor;
    primary_query_client_t client(
        cluster.get_mailbox_manager(),
        server.get_bcard(),
        &non_interruptor);

    /* Send a write to the namespace */
    {
        fifo_enforcer_sink_t::exit_write_t token;
        client.new_write_token(&token);
        write_t write;
        write.write = dummy_write_t();
        write_response_t res;
        try {
            client.write(
                write,
                &res,
                order_source.check_in("ClusteringQuery.ReadWrite.write"),
                &token,
                &non_interruptor);
            ADD_FAILURE() << "Expected an exception to be thrown.";
        } catch (const cannot_perform_query_exc_t &) {
            /* This is expected. */
        }
        EXPECT_EQ(1, query_counter.num_writes);
    }

    /* Send a read to the namespace */
    {
        fifo_enforcer_sink_t::exit_read_t token;
        client.new_read_token(&token);
        read_t read;
        read.read = dummy_read_t();
        read_response_t res;
        try {
            client.read(
                read,
                &res,
                order_source.check_in("ClusteringQuery.ReadWrite.read").with_read_mode(),
                &token,
                &non_interruptor);
            ADD_FAILURE() << "Expected an exception to be thrown.";
        } catch (const cannot_perform_query_exc_t &) {
            /* This is expected. */
        }
        EXPECT_EQ(1, query_counter.num_reads);
    }
}

}   /* namespace unittest */

