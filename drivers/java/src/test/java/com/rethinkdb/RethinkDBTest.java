package com.rethinkdb;

import com.fasterxml.jackson.databind.ObjectMapper;
import com.rethinkdb.annotations.IgnoreNullFields;
import com.rethinkdb.gen.exc.ReqlError;
import com.rethinkdb.gen.exc.ReqlQueryLogicError;
import com.rethinkdb.model.MapObject;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.net.Connection;
import com.rethinkdb.net.Cursor;
import net.jodah.concurrentunit.Waiter;
import org.junit.*;
import org.junit.rules.ExpectedException;

import java.beans.BeanInfo;
import java.beans.Introspector;
import java.beans.PropertyDescriptor;
import java.lang.reflect.Array;
import java.lang.reflect.Method;
import java.time.OffsetDateTime;
import java.util.*;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicInteger;

import static com.rethinkdb.TestingCommon.smartDeepEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

public class RethinkDBTest{

    public static final RethinkDB r = RethinkDB.r;
    Connection conn;
    public static final String dbName = "javatests";
    public static final String tableName = "atest";

    @Rule
    public ExpectedException expectedEx = ExpectedException.none();

    @BeforeClass
    public static void oneTimeSetUp() throws Exception {
        Connection conn = TestingFramework.createConnection();
        try{
            r.dbCreate(dbName).run(conn);
        } catch(ReqlError e){}
        try {
            r.db(dbName).wait_().run(conn);
            r.db(dbName).tableCreate(tableName).run(conn);
            r.db(dbName).table(tableName).wait_().run(conn);
        } catch(ReqlError e){}
        conn.close();
    }

    @AfterClass
    public static void oneTimeTearDown() throws Exception {
        Connection conn = TestingFramework.createConnection();
        try {
            r.dbDrop(dbName).run(conn);
        } catch(ReqlError e){}
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
        OffsetDateTime date = OffsetDateTime.now();
        OffsetDateTime result = r.expr(date).run(conn);
        Assert.assertEquals(date, result);
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

    @Test
    public void testDbGlobalArgInserted() {
        final String tblName = "test_global_optargs";

        try {
            r.dbCreate("test").run(conn);
        } catch (Exception e) {}
        try{
            r.db("test").tableDrop(tblName).run(conn);
        } catch (Exception e) {}

        try {
            r.dbDrop("optargs").run(conn);
        } catch (Exception e) {}
        try {
            r.dbDrop("conn_default").run(conn);
        } catch (Exception e) {}
        r.expr(r.array("optargs", "conn_default")).forEach(r::dbCreate).run(conn);
        r.expr(r.array("test", "optargs", "conn_default")).forEach(dbName ->
                        r.db(dbName).tableCreate(tblName).do_((unused) ->
                                        r.db(dbName).table(tblName).insert(r.hashMap("dbName", dbName).with("id", 1))
                        )
        ).run(conn);

        try {
            // no optarg set, no default db
            conn.use(null);
            Map x1 = r.table(tblName).get(1).run(conn);
            assertEquals("test", x1.get("dbName"));

            // no optarg set, default db set
            conn.use("conn_default");
            Map x2 = r.table(tblName).get(1).run(conn);
            assertEquals("conn_default", x2.get("dbName"));

            // optarg set, no default db
            conn.use(null);
            Map x3 = r.table(tblName).get(1).run(conn, OptArgs.of("db", "optargs"));
            assertEquals("optargs", x3.get("dbName"));

            // optarg set, default db
            conn.use("conn_default");
            Map x4 = r.table(tblName).get(1).run(conn, OptArgs.of("db", "optargs"));
            assertEquals("optargs", x4.get("dbName"));

        } finally {
            conn.use(null);
            r.expr(r.array("optargs", "conn_default")).forEach(r::dbDrop).run(conn);
            r.db("test").tableDrop(tblName).run(conn);
        }
    }

    @Test
    public void testFilter() {
        r.db(dbName).table(tableName).insert(new MapObject().with("field", "123")).run(conn);
        r.db(dbName).table(tableName).insert(new MapObject().with("field", "456")).run(conn);

        Cursor<Map<String, String>> allEntries = r.db(dbName).table(tableName).run(conn);
        assertEquals(2, allEntries.toList().size());

        // The following won't work, because r.row is not implemented in the Java driver. Use lambda syntax instead
        // Cursor<Map<String, String>> oneEntryRow = r.db(dbName).table(tableName).filter(r.row("field").eq("456")).run(conn);
        // assertEquals(1, oneEntryRow.toList().size());

        Cursor<Map<String, String>> oneEntryLambda = r.db(dbName).table(tableName).filter(table -> table.getField("field").eq("456")).run(conn);
        assertEquals(1, oneEntryLambda.toList().size());
    }

    @Test
    public void testTableSelectOfPojo() {
        TestPojo pojo = new TestPojo("foo", new TestPojoInner(42L, true));
        Map<String, Object> pojoResult = r.db(dbName).table(tableName).insert(pojo).run(conn);
        assertEquals(1L, pojoResult.get("inserted"));

        String key = (String) ((List) pojoResult.get("generated_keys")).get(0);
        TestPojo result = r.db(dbName).table(tableName).get(key).run(conn, TestPojo.class);

        assertEquals("foo", result.getStringProperty());
        assertTrue(42L == result.getPojoProperty().getLongProperty());
        assertEquals(true, result.getPojoProperty().getBooleanProperty());
    }

    @Test(expected = ClassCastException.class)
    public void testTableSelectOfPojo_withNoPojoClass_throwsException() {
        TestPojo pojo = new TestPojo("foo", new TestPojoInner(42L, true));
        Map<String, Object> pojoResult = r.db(dbName).table(tableName).insert(pojo).run(conn);
        assertEquals(1L, pojoResult.get("inserted"));

        String key = (String) ((List) pojoResult.get("generated_keys")).get(0);
        TestPojo result = r.db(dbName).table(tableName).get(key).run(conn /* TestPojo.class is not specified */);
    }

    @Test
    public void testTableSelectOfPojoCursor() {
        TestPojo pojoOne = new TestPojo("foo", new TestPojoInner(42L, true));
        TestPojo pojoTwo = new TestPojo("bar", new TestPojoInner(53L, false));
        Map<String, Object> pojoOneResult = r.db(dbName).table(tableName).insert(pojoOne).run(conn);
        Map<String, Object> pojoTwoResult = r.db(dbName).table(tableName).insert(pojoTwo).run(conn);
        assertEquals(1L, pojoOneResult.get("inserted"));
        assertEquals(1L, pojoTwoResult.get("inserted"));

        Cursor<TestPojo> cursor = r.db(dbName).table(tableName).run(conn, TestPojo.class);
        List<TestPojo> result = cursor.toList();
        assertEquals(2, result.size());

        TestPojo pojoOneSelected = "foo".equals(result.get(0).getStringProperty()) ? result.get(0) : result.get(1);
        TestPojo pojoTwoSelected = "bar".equals(result.get(0).getStringProperty()) ? result.get(0) : result.get(1);

        assertEquals("foo", pojoOneSelected.getStringProperty());
        assertTrue(42L == pojoOneSelected.getPojoProperty().getLongProperty());
        assertEquals(true, pojoOneSelected.getPojoProperty().getBooleanProperty());

        assertEquals("bar", pojoTwoSelected.getStringProperty());
        assertTrue(53L == pojoTwoSelected.getPojoProperty().getLongProperty());
        assertEquals(false, pojoTwoSelected.getPojoProperty().getBooleanProperty());
    }

    @Test
    public void testTableSelectOfPojoCursor_ManyTypeMappings() {
        TestPojo pojoOne = new TestPojo("foo", new TestPojoInner(42L, true));
        Map<String, Object> pojoOneResult = r.db(dbName).table(tableName).insert(pojoOne).run(conn);
        assertEquals(1L, pojoOneResult.get("inserted"));

        Cursor<TestPojo> cursor = r.db(dbName).table(tableName).run(conn, TestPojo.class);
        List<TestPojo> result = cursor.toList();

        TestPojo pojoOneSelected = result.get(0);

        compareMostPropertiesOfPojo(pojoOneSelected, pojoOne);

        assertTrue( smartDeepEquals(pojoOneSelected.getPojoListProperty(), pojoOne.getPojoListProperty()));
    }

    @Test
    public void testTableSelectOfPojoCursor_ConvertStringToOtherTypes() {
        TestPojo pojoOne = new TestPojo("foo", new TestPojoInner(42L, true));
        //pojoOne.getEnumProperty().toString() lowercase output.
        //pojoOne.getEnumProperty().Name() will output uppercase,
        //uppercase is more easy to be converted back because auto-generated Enum use such convention.
        //By default, when storing enum to db, we store the .Name()
        MapObject map = r.hashMap("stringProperty",pojoOne.getStringProperty().toString())
                .with("enumProperty", pojoOne.getEnumProperty().toString())
                .with("enumInnerLowerCaseProperty", pojoOne.getEnumInnerLowerCaseProperty().toString()) //store "xxx"
                .with("enumInnerUpperCaseProperty", pojoOne.getEnumInnerUpperCaseProperty().toString()) //store "XXX"
                .with("offsetDateTimeProperty", pojoOne.getOffsetDateTimeProperty().toString())
                .with("localDateTimeProperty", pojoOne.getLocalDateTimeProperty().toString())
                .with("zonedDateTimeProperty", pojoOne.getZonedDateTimeProperty().toString())
                .with("localDateProperty", pojoOne.getLocalDateProperty().toString())
                .with("localTimeProperty", pojoOne.getLocalTimeProperty().toString())
                .with("dateProperty", pojoOne.getDateProperty().toString())
                .with("doubleProperty", pojoOne.getDoubleProperty().toString())
                .with("primitiveDoubleProperty", String.valueOf(pojoOne.getPrimitiveDoubleProperty()))
                .with("floatProperty", pojoOne.getFloatProperty().toString())
                .with("primitiveFloatProperty", String.valueOf(pojoOne.getPrimitiveFloatProperty()))
                .with("integerProperty", pojoOne.getIntegerProperty().toString())
                .with("primitiveIntegerProperty", String.valueOf(pojoOne.getPrimitiveIntegerProperty()))
                .with("longProperty", pojoOne.getLongProperty().toString())
                .with("primitiveLongProperty", String.valueOf(pojoOne.getPrimitiveLongProperty()))
                .with("shortProperty", pojoOne.getShortProperty().toString())
                .with("primitiveShortProperty", String.valueOf(pojoOne.getPrimitiveShortProperty()))
                .with("byteProperty", pojoOne.getByteProperty().toString())
                .with("primitiveByteProperty", String.valueOf(pojoOne.getPrimitiveByteProperty()))
                .with("booleanProperty", pojoOne.getBooleanProperty().toString())
                .with("primitiveBooleanProperty", String.valueOf(pojoOne.getPrimitiveBooleanProperty()))
                .with("bigDecimalProperty", pojoOne.getBigDecimalProperty().toString())
                .with("bigIntegerProperty", pojoOne.getBigIntegerProperty().toString())
                ;
        Map<String, Object> pojoOneResult = r.db(dbName).table(tableName).insert(map).run(conn);
        assertEquals(1L, pojoOneResult.get("inserted"));

        Cursor<TestPojo> cursor = r.db(dbName).table(tableName).run(conn, TestPojo.class);
        List<TestPojo> result = cursor.toList();
        assertEquals(1, result.size());

        TestPojo pojoOneSelected = result.get(0);

        compareMostPropertiesOfPojo(pojoOneSelected, pojoOne);
    }

    @Test
    public void testSaveBeanAsMapThenSelectAsBean() {
        TestPojo pojoOne = new TestPojo("foo", new TestPojoInner(42L, true));
        ObjectMapper m = new ObjectMapper();

        Map<String, Object> map = m.convertValue(pojoOne, Map.class);

        Map<String, Object> pojoOneResult = r.db(dbName).table(tableName).insert(map).run(conn);
        assertEquals(1L, pojoOneResult.get("inserted"));

        Cursor<TestPojo> cursor = r.db(dbName).table(tableName).run(conn, TestPojo.class);
        List<TestPojo> result = cursor.toList();
        assertEquals(1, result.size());

        TestPojo pojoOneSelected = result.get(0);

        compareMostPropertiesOfPojo(pojoOneSelected, pojoOne);

        assertTrue( smartDeepEquals(pojoOneSelected.getPojoListProperty(), pojoOne.getPojoListProperty()));
    }

    private void compareMostPropertiesOfPojo(TestPojo pojoOneSelected, TestPojo pojoOne) {
        assertEquals(pojoOneSelected.getEnumProperty(), pojoOne.getEnumProperty());
        assertEquals(pojoOneSelected.getEnumInnerLowerCaseProperty(), pojoOne.getEnumInnerLowerCaseProperty());
        assertEquals(pojoOneSelected.getEnumInnerUpperCaseProperty(), pojoOne.getEnumInnerUpperCaseProperty());

        assertEquals(pojoOneSelected.getOffsetDateTimeProperty(), pojoOne.getOffsetDateTimeProperty());
        assertEquals(pojoOneSelected.getLocalDateTimeProperty(), pojoOne.getLocalDateTimeProperty());
        assertEquals(pojoOneSelected.getLocalDateProperty(), pojoOne.getLocalDateProperty());
        assertEquals(pojoOneSelected.getLocalTimeProperty(), pojoOne.getLocalTimeProperty());
        assertEquals(pojoOneSelected.getZonedDateTimeProperty().toInstant(), pojoOne.getZonedDateTimeProperty().toInstant());
        assertEquals(pojoOneSelected.getDateProperty().toString(), pojoOne.getDateProperty().toString());

        assertEquals(pojoOneSelected.getDoubleProperty(), pojoOne.getDoubleProperty());
        assertTrue(pojoOneSelected.getPrimitiveDoubleProperty() == pojoOne.getPrimitiveDoubleProperty());
        assertEquals(pojoOneSelected.getFloatProperty(), pojoOne.getFloatProperty());
        assertTrue(pojoOneSelected.getPrimitiveFloatProperty() == pojoOne.getPrimitiveFloatProperty());
        assertEquals(pojoOneSelected.getIntegerProperty(), pojoOne.getIntegerProperty());
        assertTrue(pojoOneSelected.getPrimitiveIntegerProperty() == pojoOne.getPrimitiveIntegerProperty());
        assertEquals(pojoOneSelected.getLongProperty(), pojoOne.getLongProperty());
        assertTrue(pojoOneSelected.getPrimitiveLongProperty() == pojoOne.getPrimitiveLongProperty());
        assertEquals(pojoOneSelected.getShortProperty(), pojoOne.getShortProperty());
        assertTrue(pojoOneSelected.getPrimitiveShortProperty() == pojoOne.getPrimitiveShortProperty());
        assertEquals(pojoOneSelected.getByteProperty(), pojoOne.getByteProperty());
        assertTrue(pojoOneSelected.getPrimitiveByteProperty() == pojoOne.getPrimitiveByteProperty());
        assertEquals(pojoOneSelected.getBooleanProperty(), pojoOne.getBooleanProperty());
        assertTrue(pojoOneSelected.getPrimitiveBooleanProperty() == pojoOne.getPrimitiveBooleanProperty());

        int scale = pojoOneSelected.getBigDecimalProperty().scale();
        java.math.BigDecimal sampleBigDecimal = pojoOne.getBigDecimalProperty().setScale(scale, java.math.BigDecimal.ROUND_HALF_UP);
        assertEquals(pojoOneSelected.getBigDecimalProperty(), sampleBigDecimal);

        assertEquals(pojoOneSelected.getBigIntegerProperty(), pojoOne.getBigIntegerProperty());
    }

    @Test
    public void testDeepCompare() {
        assertEquals(false, smartDeepEquals("hihihih", "xx"));
        assertEquals(true, smartDeepEquals("hihihih", "hihihih"));
        assertEquals(true, smartDeepEquals(Long.valueOf(123456789), 123456789L));

        TestPojo pojo1 = new TestPojo("s1", null);
        TestPojo pojo2 = new TestPojo("s1", null);

        assertEquals(false, pojo1.equals(pojo2));
        assertEquals(true, smartDeepEquals(pojo1, pojo2));

        pojo1 = new TestPojo("s1", new TestPojoInner(11L, true));
        pojo2 = new TestPojo("s1", new TestPojoInner(11L, true));

        assertEquals(false, pojo1.equals(pojo2));
        assertEquals(true, smartDeepEquals(pojo1, pojo2));

        pojo1.getPojoListProperty().set(0, new TestPojoInner(99L, true));
        assertEquals(false, smartDeepEquals(pojo1.getPojoListProperty(), pojo2.getPojoListProperty()));
        assertEquals(false, smartDeepEquals(pojo1, pojo2));

        List<TestPojo> list1 = Arrays.asList(new TestPojo("s1", new TestPojoInner(11L, true)), new TestPojo("s2", new TestPojoInner(22L, true)));
        List<TestPojo> list2 = Arrays.asList(new TestPojo("s1", new TestPojoInner(11L, true)), new TestPojo("s2", new TestPojoInner(22L, true)));

        ArrayList list3 = new ArrayList();
        ArrayList<TestPojo> list4 = new ArrayList<>();
        list3.add(new TestPojo("s1", new TestPojoInner(11L, true)));
        list4.add(new TestPojo("s1", new TestPojoInner(11L, true)));
        list3.add(new TestPojo("s2", new TestPojoInner(22L, true)));
        list4.add(new TestPojo("s2", new TestPojoInner(22L, true)));

        assertEquals(false, list1.equals(list2));
        assertEquals(true, smartDeepEquals(list1, list2));

        assertEquals(false, list1.equals(list3));
        assertEquals(true, smartDeepEquals(list1, list3));

        assertEquals(false, list1.equals(list4));
        assertEquals(true, smartDeepEquals(list1, list4));

        assertEquals(false, list3.equals(list4));
        assertEquals(true, smartDeepEquals(list3, list4));

        TestPojo[] ary1 = new TestPojo[]{new TestPojo("s1", new TestPojoInner(11L, true)), new TestPojo("s2", new TestPojoInner(22L, true))};
        TestPojo[] ary2 = new TestPojo[]{new TestPojo("s1", new TestPojoInner(11L, true)), new TestPojo("s2", new TestPojoInner(22L, true))};
        Object[] ary3 = new Object[]{new TestPojo("s1", new TestPojoInner(11L, true)), new TestPojo("s2", new TestPojoInner(22L, true))};

        assertEquals(false, ary1.equals(ary2));
        assertEquals(true, smartDeepEquals(ary1, ary2));

        assertEquals(false, ary1.equals(ary3));
        assertEquals(true, smartDeepEquals(ary1, ary3));
    }

    @IgnoreNullFields
    public static class TestPojoIgnoreNull {
        private Object notNullProperty;
        private Object nullProperty;

        public TestPojoIgnoreNull() {
        }

        public TestPojoIgnoreNull(Object notNullProperty, Object nullProperty) {
            this.notNullProperty = notNullProperty;
            this.nullProperty = nullProperty;
        }

        public Object getNotNullProperty() {
            return notNullProperty;
        }

        public void setNotNullProperty(Object notNullProperty) {
            this.notNullProperty = notNullProperty;
        }

        public Object getNullProperty() {
            return nullProperty;
        }

        public void setNullProperty(Object nullProperty) {
            this.nullProperty = nullProperty;
        }
    }

    @Test
    public void testSaveObjectIgnoreNullProperties() {
        TestPojoIgnoreNull pojoOne = new TestPojoIgnoreNull("foo", null);

        Map<String, Object> pojoOneResult = r.db(dbName).table(tableName).insert(pojoOne).run(conn);
        assertEquals(1L, pojoOneResult.get("inserted"));

        Cursor cursor = r.db(dbName).table(tableName).run(conn);
        List result = cursor.toList();
        assertEquals(1, result.size());

        Object _pojoOneSelected = result.get(0);
        assertTrue(_pojoOneSelected instanceof Map);

        Map<String, Object> pojoOneSelected = (Map<String, Object>)_pojoOneSelected;

        assertTrue(!pojoOneSelected.containsKey("nullProperty"));
        assertEquals(pojoOneSelected.get("notNullProperty"), pojoOne.getNotNullProperty());
    }

    @Test
    public void testSaveObjectIgnoreNullPropertiesNested() {
        TestPojoIgnoreNull pojoOne = new TestPojoIgnoreNull("foo", null);
        TestPojoIgnoreNull pojoOneInner = new TestPojoIgnoreNull("foo inner", null);
        pojoOne.setNotNullProperty(pojoOneInner);

        Map<String, Object> pojoOneResult = r.db(dbName).table(tableName).insert(pojoOne).run(conn);
        assertEquals(1L, pojoOneResult.get("inserted"));

        Cursor cursor = r.db(dbName).table(tableName).run(conn);
        List result = cursor.toList();
        assertEquals(1, result.size());

        Object _pojoOneSelected = result.get(0);
        assertTrue(_pojoOneSelected instanceof Map);

        Map<String, Object> pojoOneSelected = (Map<String, Object>)_pojoOneSelected;

        assertTrue(!pojoOneSelected.containsKey("nullProperty"));

        Object _pojoOneInnerSelected = pojoOneSelected.get("notNullProperty");
        assertTrue(_pojoOneInnerSelected instanceof Map);

        Map<String, Object> pojoOneInnerSelected = (Map<String, Object>)_pojoOneInnerSelected;

        assertTrue(!pojoOneInnerSelected.containsKey("nullProperty"));
        assertEquals(pojoOneInnerSelected.get("notNullProperty"), pojoOneInner.getNotNullProperty());
    }

    @Test
    public void testTableSelect_convertStringToDate() {
        Locale.setDefault(Locale.JAPAN);
        _convertStringToDate("2016-03-16", new String[]{
                "Wed Mar 16 00:00:00 JST 2016"
                ,"2016-03-16T00:00+09:00"
                ,"2016-03-16T00:00"
                ,"2016-03-16"
                ,"2016-03-16T00:00+09:00[Asia/Tokyo]"
        });
        _convertStringToDate("2016-03-16T22:43:05", new String[]{
                "Wed Mar 16 22:43:05 JST 2016"
                ,"2016-03-16T22:43:05+09:00"
                ,"2016-03-16T22:43:05"
                ,"2016-03-16"
                ,"2016-03-16T22:43:05+09:00[Asia/Tokyo]"
        });
        _convertStringToDate("2016-03-16T22:43:05.002", new String[]{
                "Wed Mar 16 22:43:05 JST 2016"
                ,"2016-03-16T22:43:05.002+09:00"
                ,"2016-03-16T22:43:05.002"
                ,"2016-03-16"
                ,"2016-03-16T22:43:05.002+09:00[Asia/Tokyo]"
        });
        _convertStringToDate("2016-03-16T22:43:05+09:00", new String[]{
                "Wed Mar 16 22:43:05 JST 2016"
                ,"2016-03-16T22:43:05+09:00"
                ,"2016-03-16T22:43:05"
                ,"2016-03-16"
                ,"2016-03-16T22:43:05+09:00[Asia/Tokyo]"
        });
        _convertStringToDate("2016-03-16T22:43:05.002+09:00", new String[]{
                "Wed Mar 16 22:43:05 JST 2016"
                ,"2016-03-16T22:43:05.002+09:00"
                ,"2016-03-16T22:43:05.002"
                ,"2016-03-16"
                ,"2016-03-16T22:43:05.002+09:00[Asia/Tokyo]"
        });
        _convertStringToDate("2016-03-16T22:43:05.002+09:00[Asia/Tokyo]", new String[]{
                "Wed Mar 16 22:43:05 JST 2016"
                ,"2016-03-16T22:43:05.002+09:00"
                ,"2016-03-16T22:43:05.002"
                ,"2016-03-16"
                ,"2016-03-16T22:43:05.002+09:00[Asia/Tokyo]"
        });
        _convertStringToDate(1458135785002L, new String[]{
                "Wed Mar 16 22:43:05 JST 2016"
                ,"2016-03-16T22:43:05.002+09:00"
                ,"2016-03-16T22:43:05.002"
                ,"2016-03-16"
                ,"2016-03-16T22:43:05.002+09:00[Asia/Tokyo]"
        });
        _convertStringToDate("Wed Mar 16 22:43:05 JST 2016", new String[]{
                "Wed Mar 16 22:43:05 JST 2016"
                ,"2016-03-16T22:43:05+09:00"
                ,"2016-03-16T22:43:05"
                ,"2016-03-16"
                ,"2016-03-16T22:43:05+09:00[Asia/Tokyo]"
        });
    }

    void _convertStringToDate(Object value, String[] expects) {
        r.db(dbName).table(tableName).delete().run(conn);
        r.db(dbName).table(tableName).insert(r.hashMap()
                .with("dateProperty", value)
                .with("offsetDateTimeProperty", value)
                .with("localDateTimeProperty", value)
                .with("localDateProperty", value)
                .with("zonedDateTimeProperty", value)
        ).run(conn);
        TestPojo pojoSelected = ((Cursor<TestPojo>)r.db(dbName).table(tableName).run(conn, TestPojo.class)).next();

        assertEquals(expects[0], pojoSelected.getDateProperty().toString());
        assertEquals(expects[1], pojoSelected.getOffsetDateTimeProperty().toString());
        assertEquals(expects[2], pojoSelected.getLocalDateTimeProperty().toString());
        assertEquals(expects[3], pojoSelected.getLocalDateProperty().toString());
        assertEquals(expects[4], pojoSelected.getZonedDateTimeProperty().toString());
    }

    @Test(expected = ClassCastException.class)
    public void testTableSelectOfPojoCursor_withNoPojoClass_throwsException() {
        TestPojo pojoOne = new TestPojo("foo", new TestPojoInner(42L, true));
        TestPojo pojoTwo = new TestPojo("bar", new TestPojoInner(53L, false));
        Map<String, Object> pojoOneResult = r.db(dbName).table(tableName).insert(pojoOne).run(conn);
        Map<String, Object> pojoTwoResult = r.db(dbName).table(tableName).insert(pojoTwo).run(conn);
        assertEquals(1L, pojoOneResult.get("inserted"));
        assertEquals(1L, pojoTwoResult.get("inserted"));

        Cursor<TestPojo> cursor = r.db(dbName).table(tableName).run(conn /* TestPojo.class is not specified */);
        List<TestPojo> result = cursor.toList();

        TestPojo pojoSelected = result.get(0);
    }

    @Test(timeout=20000)
    public void testConcurrentWrites() throws TimeoutException, InterruptedException {
        final int total = 500;
        final AtomicInteger writeCounter = new AtomicInteger(0);
        final Waiter waiter = new Waiter();
        for (int i = 0; i < total; i++)
            new Thread(() -> {
                final TestPojo pojo = new TestPojo("writezz", new TestPojoInner(10L, true));
                final Map<String, Object> result = r.db(dbName).table(tableName).insert(pojo).run(conn);
                waiter.assertEquals(1L, result.get("inserted"));
                writeCounter.getAndIncrement();
                waiter.resume();
            }).start();

        waiter.await(5000, total);

        assertEquals(total, writeCounter.get());
    }

    @Test(timeout=20000)
    public void testConcurrentReads() throws TimeoutException {
        final int total = 500;
        final AtomicInteger readCounter = new AtomicInteger(0);

        // write to the database and retrieve the id
        final TestPojo pojo = new TestPojo("readzz", new TestPojoInner(10L, true));
        final Map<String, Object> result = r.db(dbName).table(tableName).insert(pojo).optArg("return_changes", true).run(conn);
        final String id = ((List) result.get("generated_keys")).get(0).toString();

        final Waiter waiter = new Waiter();
        for (int i = 0; i < total; i++)
            new Thread(() -> {
                // make sure there's only one
                final Cursor<TestPojo> cursor = r.db(dbName).table(tableName).run(conn, TestPojo.class);
                assertEquals(1, cursor.toList().size());
                // read that one
                final TestPojo readPojo = r.db(dbName).table(tableName).get(id).run(conn, TestPojo.class);
                waiter.assertNotNull(readPojo);
                // assert inserted values
                waiter.assertEquals("readzz", readPojo.getStringProperty());
                waiter.assertEquals(10L, readPojo.getPojoProperty().getLongProperty());
                waiter.assertEquals(true, readPojo.getPojoProperty().getBooleanProperty());
                readCounter.getAndIncrement();
                waiter.resume();
            }).start();

        waiter.await(10000, total);

        assertEquals(total, readCounter.get());
    }

    @Test(timeout=20000)
    public void testConcurrentCursor() throws TimeoutException, InterruptedException {
        final int total = 500;
        final Waiter waiter = new Waiter();
        for (int i = 0; i < total; i++)
            new Thread(() -> {
                final TestPojo pojo = new TestPojo("writezz", new TestPojoInner(10L, true));
                final Map<String, Object> result = r.db(dbName).table(tableName).insert(pojo).run(conn);
                waiter.assertEquals(1L, result.get("inserted"));
                waiter.resume();
            }).start();

        waiter.await(2500, total);

        final Cursor<TestPojo> all = r.db(dbName).table(tableName).run(conn);
        assertEquals(total, all.toList().size());
    }

    @Test
    public void testQueryTimeout() {
        Cursor c = r.db(dbName).table(tableName).changes().run(conn);
        try {
            c.next(10*1000);
            fail();
        } catch (TimeoutException e) {
            e = e;
        }
    }
}

