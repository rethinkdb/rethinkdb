package com.rethinkdb;

import com.rethinkdb.net.Connection;
import com.rethinkdb.net.Cursor;
import junit.framework.TestCase;
import org.json.simple.JSONArray;

import java.util.Map;
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
        Optional<Object> res = r.table("foo")
                .map(m -> m.bracket("id")).run(conn);
        assert(res.isPresent());
    }
}