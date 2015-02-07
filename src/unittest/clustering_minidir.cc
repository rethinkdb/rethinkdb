// Copyright 2010-2013 RethinkDB, all rights reserved.
#include "unittest/gtest.hpp"

#include "clustering/generic/minidir.hpp"
#include "unittest/clustering_utils.hpp"
#include "unittest/unittest_utils.hpp"

namespace unittest {

/* `SinglePeer` tests sending messages across a minidir on a single peer. */
TPTEST(ClusteringMinidir, Register) {
    simple_mailbox_cluster_t cluster;

    minidir_read_manager_t<std::string, std::string> reader1(
        cluster.get_mailbox_manager());

    watchable_map_var_t<std::string, std::string> writer1_data;
    writer1_data.set_key("hello", "world");
    watchable_map_var_t<uuid_u, minidir_bcard_t<std::string, std::string> >
        writer1_readers;
    writer1_readers.set_key(generate_uuid(), reader1.get_bcard());
    minidir_write_manager_t<std::string, std::string> writer1(
        cluster.get_mailbox_manager(),
        &writer1_data,
        &writer1_readers);

    let_stuff_happen();
    ASSERT_EQ(writer1_data.get_all(), reader1.get_values()->get_all());

    writer1_data.set_key("foo", "bar");
    let_stuff_happen();
    ASSERT_EQ(writer1_data.get_all(), reader1.get_values()->get_all());

    {
        minidir_read_manager_t<std::string, std::string> reader2(
            cluster.get_mailbox_manager());
        uuid_u reader2_id = generate_uuid();
        writer1_readers.set_key(reader2_id, reader2.get_bcard());

        let_stuff_happen();
        ASSERT_EQ(writer1_data.get_all(), reader2.get_values()->get_all());

        writer1_data.delete_key("hello");
        let_stuff_happen();
        ASSERT_EQ(writer1_data.get_all(), reader1.get_values()->get_all());
        ASSERT_EQ(writer1_data.get_all(), reader2.get_values()->get_all());

        watchable_map_var_t<std::string, std::string> writer2_data;
        writer2_data.set_key("blah", "baz");
        watchable_map_var_t<std::string, std::string> writer2_readers;
        writer2_readers.set_key(generate_uuid(), reader1.get_bcard());
        writer2_readers.set_key(generate_uuid(), reader2.get_bcard());
        minidir_write_manager_t<std::string, std::string> writer2(
            cluster.get_mailbox_manager(),
            &writer2_data,
            &writer2_readers);

        let_stuff_happen();
        std::map<std::string, std::string> expected = writer1_data.get_all();
        expected["blah"] = "baz";
        ASSERT_EQ(expected, reader1.get_values()->get_all());
        ASSERT_EQ(expected, reader2.get_values()->get_all());

        writer1_readers.delete_key(reader2_id);
        let_stuff_happen();
        ASSERT_EQ(writer2_data.get_all(), reader2.get_values()->get_all());

        /* `writer2` goes out of scope here; it should send a disconnect to `reader1` */
    }

    let_stuff_happen();
    ASSERT_EQ(writer1_data.get_all(), reader1.get_values()->get_all());
}

/* RSI: Test multi-peer scenarios */

}   /* namespace unittest */
