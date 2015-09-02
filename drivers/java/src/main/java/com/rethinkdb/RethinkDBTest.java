package com.rethinkdb;

import com.rethinkdb.gen.exc.ReqlError;
import com.rethinkdb.gen.exc.ReqlOpFailedError;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.net.Connection;
import com.rethinkdb.net.Cursor;
import junit.framework.TestCase;
import org.json.simple.JSONObject;

import java.util.Date;
import java.util.List;

public class RethinkDBTest extends TestCase {

    public static final RethinkDB r = RethinkDB.r;
    Connection<?> conn;
    public final String dbName = "javatests";
    public final String tableName = "atest";

    public void setUp() throws Exception {
        super.setUp();
        conn = r.connection()
                .hostname("newton")
                .port(31157)
                .connect();
        try{
            r.dbCreate(dbName);
        }catch(ReqlError e){}
        try {
            r.db(dbName).wait_().run(conn);
            r.db(dbName).tableCreate(tableName).run(conn);
            r.db(dbName).table(tableName).wait_().run(conn);
        }catch(ReqlError e){}
    }

    public void tearDown() throws Exception {
        try {
            r.db(dbName).tableDrop(tableName).run(conn);
        }catch(ReqlError e){}
        try {
            r.dbDrop(dbName).run(conn);
        }catch(ReqlError e){}
        conn.close();
    }

    public void testBooleans() throws Exception {
        Boolean t = r.expr(true).run(conn);
        assertEquals(t.booleanValue(), true);

        Boolean f = r.expr(false).run(conn);
        assertEquals(t.booleanValue(), false);

        String trueType = r.expr(true).typeOf().run(conn);
        assertEquals(trueType, "BOOL");

        String falseString = r.expr(false).coerceTo("string").run(conn);
        assertEquals(falseString, "false");

        Boolean boolCoerce = r.expr(true).coerceTo("bool").run(conn);
        assertEquals(boolCoerce.booleanValue(), true);
    }

    public void testNull() throws Exception {
        Object o = r.expr(null).run(conn);
        assertEquals()
    }

}