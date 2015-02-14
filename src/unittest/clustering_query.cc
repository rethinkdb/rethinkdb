// Copyright 2010-2014 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/immediate_consistency/branch/broadcaster.hpp"
#include "clustering/immediate_consistency/branch/listener.hpp"
#include "clustering/immediate_consistency/branch/replier.hpp"
#include "clustering/immediate_consistency/query/master.hpp"
#include "clustering/immediate_consistency/query/master_access.hpp"
#include "unittest/branch_history_manager.hpp"
#include "unittest/clustering_utils.hpp"
#include "rdb_protocol/protocol.hpp"
#include "unittest/mock_store.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

class query_counter_t : public master_t::query_callback_t {
public:
    query_counter_t() : should_succeed(true), num_writes(0), num_reads(0) { }
    bool on_write(
            const write_t &request,
            fifo_enforcer_sink_t::exit_write_t *exiter,
            order_token_t order_token,
            UNUSED signal_t *interruptor,
            write_response_t *response_out,
            std::string *error_out) {
        order_sink.check_out(order_token);
        exiter->end();
        EXPECT_TRUE(boost::get<dummy_write_t>(&request.write) != nullptr);
        ++num_writes;
        if (should_succeed) {
            *response_out = write_response_t(dummy_write_response_t());
        } else {
            *error_out = "query_counter_t write failed";
        }
        return should_succeed;
    }
    bool on_read(
            const read_t &request,
            fifo_enforcer_sink_t::exit_read_t *exiter,
            order_token_t order_token,
            UNUSED signal_t *interruptor,
            read_response_t *response_out,
            std::string *error_out) {
        order_sink.check_out(order_token);
        exiter->end();
        EXPECT_TRUE(boost::get<dummy_read_t>(&request.read) != nullptr);
        ++num_reads;
        if (should_succeed) {
            *response_out = read_response_t(dummy_read_response_t());
        } else {
            *error_out = "query_counter_t read failed";
        }
        return should_succeed;
    }
    bool should_succeed;
    int num_writes, num_reads;
    order_sink_t order_sink;
};

/* The `ReadWrite` test sends some reads and writes to some shards via a
`master_access_t`. */

TPTEST(ClusteringQuery, ReadWrite) {
    order_source_t order_source;
    simple_mailbox_cluster_t cluster;
    query_counter_t query_counter;
    master_t master(cluster.get_mailbox_manager(), region_t::universe(), &query_counter);

    /* Set up a master access */
    cond_t non_interruptor;
    master_access_t master_access(
        cluster.get_mailbox_manager(),
        master.get_business_card(),
        &non_interruptor);

    /* Send a write to the namespace */
    {
        fifo_enforcer_sink_t::exit_write_t token;
        master_access.new_write_token(&token);
        write_t write;
        write.write = dummy_write_t();
        write_response_t res;
        master_access.write(
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
        master_access.new_read_token(&token);
        read_t read;
        read.read = dummy_read_t();
        read_response_t res;
        master_access.read(
            read,
            &res,
            order_source.check_in("ClusteringQuery.ReadWrite.read"),
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
    master_t master(cluster.get_mailbox_manager(), region_t::universe(), &query_counter);

    /* Set up a master access */
    cond_t non_interruptor;
    master_access_t master_access(
        cluster.get_mailbox_manager(),
        master.get_business_card(),
        &non_interruptor);

    /* Send a write to the namespace */
    {
        fifo_enforcer_sink_t::exit_write_t token;
        master_access.new_write_token(&token);
        write_t write;
        write.write = dummy_write_t();
        write_response_t res;
        try {
            master_access.write(
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
        master_access.new_read_token(&token);
        read_t read;
        read.read = dummy_read_t();
        read_response_t res;
        try {
            master_access.read(
                read,
                &res,
                order_source.check_in("ClusteringQuery.ReadWrite.read"),
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

