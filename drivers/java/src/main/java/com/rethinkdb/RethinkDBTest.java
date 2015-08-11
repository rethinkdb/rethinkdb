package com.rethinkdb;

import com.rethinkdb.net.Connection;
import junit.framework.TestCase;

import java.util.Optional;

public class RethinkDBTest extends TestCase {

    public static final RethinkDB r = RethinkDB.r;

    public void setUp() throws Exception {
        super.setUp();
        System.out.println("Set up!"); //RSI
    }

    public void tearDown() throws Exception {
        System.out.println("Tear down!"); //RSI
    }

    public void testBuildConnection() throws Exception {
        Connection conn = r.connection()
                .hostname("newton")
                .port(31157)
                .connect();
        Optional<Object> res = r.table("foo").limit(1).run(conn);
        Long val = (Long) res.get();
        assert(val >= 3 && val <= 6);
    }
}