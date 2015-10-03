import com.rethinkdb.RethinkDB;
import com.rethinkdb.gen.exc.ReqlError;
import com.rethinkdb.gen.exc.ReqlQueryLogicError;
import com.rethinkdb.model.MapObject;
import com.rethinkdb.net.Connection;
import junit.framework.Assert;
import org.junit.*;
import org.junit.rules.ExpectedException;

import java.util.Arrays;
import java.util.Date;
import java.util.List;
import java.util.Map;

import static org.junit.Assert.assertEquals;

public class RethinkDBTest{

    public static final RethinkDB r = RethinkDB.r;
    Connection<?> conn;
    public static final String dbName = "javatests";
    public static final String tableName = "atest";

    @Rule
    public ExpectedException expectedEx = ExpectedException.none();

    @BeforeClass
    public static void oneTimeSetUp() throws Exception {
        Connection<?> conn = TestingFramework.createConnection();
        try{
            r.dbCreate(dbName).run(conn);
        }catch(ReqlError e){}
        try {
            r.db(dbName).wait_().run(conn);
            r.db(dbName).tableCreate(tableName).run(conn);
            r.db(dbName).table(tableName).wait_().run(conn);
        }catch(ReqlError e){}
        conn.close();
    }

    @AfterClass
    public static void oneTimeTearDown() throws Exception {
        Connection<?> conn = TestingFramework.createConnection();
        try {
            r.db(dbName).tableDrop(tableName).run(conn);
        }catch(ReqlError e){}
        try {
            r.dbDrop(dbName).run(conn);
        }catch(ReqlError e){}
        conn.close();
    }

    @Before
    public void setUp() throws Exception {
        conn = TestingFramework.createConnection();
        r.db(dbName).table(tableName).delete().run(conn);
    }

    @After
    public void tearDown() throws Exception {
        conn.close();
    }

    @Test
    public void testBooleans() throws Exception {
        Boolean t = r.expr(true).run(conn);
        Assert.assertEquals(t.booleanValue(), true);

        Boolean f = r.expr(false).run(conn);
        Assert.assertEquals(f.booleanValue(), false);

        String trueType = r.expr(true).typeOf().run(conn);
        Assert.assertEquals(trueType, "BOOL");

        String falseString = r.expr(false).coerceTo("string").run(conn);
        Assert.assertEquals(falseString, "false");

        Boolean boolCoerce = r.expr(true).coerceTo("bool").run(conn);
        Assert.assertEquals(boolCoerce.booleanValue(), true);
    }

    @Test
    public void testNull() {
        Object o = r.expr(null).run(conn);
        Assert.assertEquals(o, null);

        String nullType = r.expr(null).typeOf().run(conn);
        Assert.assertEquals(nullType, "NULL");

        String nullCoerce = r.expr(null).coerceTo("string").run(conn);
        Assert.assertEquals(nullCoerce, "null");

        Object n = r.expr(null).coerceTo("null").run(conn);
        Assert.assertEquals(n, null);
    }

    @Test
    public void testString() {
        String str = r.expr("str").run(conn);
        Assert.assertEquals(str, "str");

        String unicode = r.expr("こんにちは").run(conn);
        Assert.assertEquals(unicode, "こんにちは");

        String strType = r.expr("foo").typeOf().run(conn);
        Assert.assertEquals(strType, "STRING");

        String strCoerce = r.expr("foo").coerceTo("string").run(conn);
        Assert.assertEquals(strCoerce, "foo");

        Number nmb12 = r.expr("-1.2").coerceTo("NUMBER").run(conn);
        Assert.assertEquals(nmb12, -1.2);

        Long nmb10 = r.expr("0xa").coerceTo("NUMBER").run(conn);
        Assert.assertEquals(nmb10.longValue(), 10L);
    }

    @Test
    public void testDate() {
        Date date = new Date();
        Date result = r.expr(date).run(conn);
        Assert.assertEquals(date.getTime(), result.getTime());
    }

    @Test
    public void testCoerceFailureDoubleNegative() {
        expectedEx.expect(ReqlQueryLogicError.class);
        expectedEx.expectMessage("Could not coerce `--1.2` to NUMBER.");
        r.expr("--1.2").coerceTo("NUMBER").run(conn);
    }

    @Test
    public void testCoerceFailureTrailingNegative() {
        expectedEx.expect(ReqlQueryLogicError.class);
        expectedEx.expectMessage("Could not coerce `-1.2-` to NUMBER.");
        r.expr("-1.2-").coerceTo("NUMBER").run(conn);
    }

    @Test
    public void testCoerceFailureInfinity() {
        expectedEx.expect(ReqlQueryLogicError.class);
        expectedEx.expectMessage("Non-finite number: inf");
        r.expr("inf").coerceTo("NUMBER").run(conn);
    }

    @Test
    public void testSplitEdgeCases() {
        List<String> emptySplitNothing = r.expr("").split().run(conn);
        Assert.assertEquals(emptySplitNothing, Arrays.asList());

        List<String> nullSplit = r.expr("").split(null).run(conn);
        Assert.assertEquals(nullSplit, Arrays.asList());

        List<String> emptySplitSpace = r.expr("").split(" ").run(conn);
        Assert.assertEquals(Arrays.asList(""), emptySplitSpace);

        List<String> emptySplitEmpty = r.expr("").split("").run(conn);
        assertEquals(Arrays.asList(), emptySplitEmpty);

        List<String> emptySplitNull5 = r.expr("").split(null, 5).run(conn);
        assertEquals(Arrays.asList(), emptySplitNull5);

        List<String> emptySplitSpace5 = r.expr("").split(" ", 5).run(conn);
        assertEquals(Arrays.asList(""), emptySplitSpace5);

        List<String> emptySplitEmpty5 = r.expr("").split("", 5).run(conn);
        assertEquals(Arrays.asList(), emptySplitEmpty5);
    }

    @Test
    public void testSplitWithNullOrWhitespace() {
        List<String> extraWhitespace = r.expr("aaaa bbbb  cccc ").split().run(conn);
        assertEquals(Arrays.asList("aaaa", "bbbb", "cccc"), extraWhitespace);

        List<String> extraWhitespaceNull = r.expr("aaaa bbbb  cccc ").split(null).run(conn);
        assertEquals(Arrays.asList("aaaa", "bbbb", "cccc"), extraWhitespaceNull);

        List<String> extraWhitespaceSpace = r.expr("aaaa bbbb  cccc ").split(" ").run(conn);
        assertEquals(Arrays.asList("aaaa", "bbbb", "", "cccc", ""), extraWhitespaceSpace);

        List<String> extraWhitespaceEmpty = r.expr("aaaa bbbb  cccc ").split("").run(conn);
        assertEquals(Arrays.asList("a", "a", "a", "a", " ",
                "b", "b", "b", "b", " ", " ", "c", "c", "c", "c", " "), extraWhitespaceEmpty);
    }

    @Test
    public void testSplitWithString() {
        List<String> b = r.expr("aaaa bbbb  cccc ").split("b").run(conn);
        assertEquals(Arrays.asList("aaaa ", "", "", "", "  cccc "), b);
    }

    @Test
    public void testTableInsert(){
        MapObject foo = new MapObject()
                .with("hi", "There")
                .with("yes", 7)
                .with("no", null );
        Map<String, Object> result = r.db(dbName).table(tableName).insert(foo).run(conn);
        assertEquals(result.get("inserted"), 1L);
    }

}