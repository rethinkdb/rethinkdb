package com.rethinkdb;

import com.rethinkdb.gen.ast.ReqlExpr;
import com.rethinkdb.net.Connection;
import com.rethinkdb.net.ConnectionInstance;
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
        Connection<? extends ConnectionInstance> conn = r.connection()
                .hostname("newton")
                .port(31157)
                .connect();
        Long res = r.expr(3).run(conn);
        assert (res.equals(3L));
        Cursor resCursor = r.range(3, 9).map(r.range(2, 8), r.range(1, 7),
                (x, y, z) -> x.mul(y, z)).run(conn);
        assert (resCursor.hasNext());

        Boolean thing = r.expr(false).run(conn);
    }

}