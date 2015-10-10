//package gen;

import com.rethinkdb.RethinkDB;
import com.rethinkdb.gen.exc.*;
import com.rethinkdb.gen.ast.*;
import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.model.MapObject;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.net.Connection;
import com.rethinkdb.net.Cursor;
import junit.framework.TestCase;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertArrayEquals;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.junit.*;
import org.junit.rules.ExpectedException;

import java.util.Arrays;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.time.OffsetDateTime;
import java.time.ZoneOffset;
import java.time.Instant;
import java.util.stream.LongStream;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.concurrent.TimeoutException;
import java.util.regex.Pattern;
import java.util.Collections;
import java.nio.charset.StandardCharsets;

public class ${module_name} {
    Logger logger = LoggerFactory.getLogger(${module_name}.class);
    public static final RethinkDB r = RethinkDB.r;
    %for var_name in table_var_names:
    public static final Table ${var_name} = r.db("test").table("${var_name}");
    %endfor

    Connection<?> conn;
    public String hostname = TestingFramework.getConfig().getHostName();
    public int port = TestingFramework.getConfig().getPort();

    @Before
    public void setUp() throws Exception {
        conn = TestingFramework.createConnection();
        try {
            r.dbCreate("test").run(conn);
            r.db("test").wait_().run(conn);
        }catch (Exception e){}
        %for var_name in table_var_names:
        try {
            r.db("test").tableCreate("${var_name}").run(conn);
            r.db("test").table(${var_name}).wait_().run(conn);
        }catch (Exception e){}
        %endfor
    }

    @After
    public void tearDown() throws Exception {
        %for var_name in table_var_names:
        r.db("test").tableDrop("${var_name}").run(conn);
        %endfor
        r.dbDrop("test").run(conn);
        conn.close();
    }

    // Python test conversion compatibility definitions

    public int len(List array) {
        return array.size();
    }

    static class Lst {
        final List lst;
        public Lst(List lst) {
            this.lst = lst;
        }

        public boolean equals(Object other) {
            return lst.equals(other);
        }
    }

    static class Bag {
        final List lst;
        public Bag(List lst) {
            Collections.sort(lst);
            this.lst = lst;
        }

        public boolean equals(Object other) {
            if(!(other instanceof List)) {
                return false;
            }
            List otherList = (List) other;
            Collections.sort(otherList);
            return lst.equals(otherList);
        }
    }

    Bag bag(List lst) {
        return new Bag(lst);
    }

    static class Partial {}

    static class PartialLst extends Partial {
        final List lst;
        public PartialLst(List lst){
            this.lst = lst;
        }

        public boolean equals(Object other) {
            if(!(other instanceof List)) {
                return false;
            }
            List otherList = (List) other;
            if(lst.size() > otherList.size()){
                return false;
            }
            for(Object item: lst) {
                if(otherList.indexOf(item) == -1){
                    return false;
                }
            }
            return true;
        }
    }

    PartialLst partial(List lst) {
        return new PartialLst(lst);
    }

    static class Dct {
        final Map dct;
        public Dct(Map dct){
            this.dct = dct;
        }

        public boolean equals(Object other) {
            return dct.equals(other);
        }
    }

    static class PartialDct extends Partial {
        final Map dct;
        public PartialDct(Map dct){
            this.dct = dct;
        }

        public boolean equals(Object other) {
            if(!(other instanceof Map)) {
                return false;
            }
            Set otherEntries = ((Map) other).entrySet();
            return otherEntries.containsAll(dct.entrySet());
        }
    }
    PartialDct partial(Map dct) {
        return new PartialDct(dct);
    }

    static class ArrLen {
        final int length;
        final Object thing;
        public ArrLen(int length, Object thing) {
            this.length = length;
            this.thing = thing;
        }

        public boolean equals(Object other) {
            if(!(other instanceof List)){
                return false;
            }
            List otherList = (List) other;
            if(length != otherList.size()) {
                return false;
            }
            if(thing == null) {
                return true;
            }
            for(Object item: otherList) {
                if(!item.equals(thing)){
                    return false;
                }
            }
            return true;
        }
    }

    ArrLen arrlen(Long length, Object thing) {
        return new ArrLen(length.intValue(), thing);
    }

    static class Uuid {
        public boolean equals(Object other) {
            if(!(other instanceof String)) {
                return false;
            }
            return Pattern.matches(
            "[a-z0-9]{8}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{12}",
            (String) other);
        }
    }

    Uuid uuid() {
        return new Uuid();
    }

    static class IntCmp {
        final Long nbr;
        public IntCmp(Long nbr) {
            this.nbr = nbr;
        }
        public boolean equals(Object other) {
            return nbr.equals(other);
        }
    }

    IntCmp int_cmp(Long nbr) {
        return new IntCmp(nbr);
    }

    static class FloatCmp {
        final Double nbr;
        public FloatCmp(Double nbr) {
            this.nbr = nbr;
        }
        public boolean equals(Object other) {
            return nbr.equals(other);
        }

    }

    FloatCmp float_cmp(Double nbr) {
        return new FloatCmp(nbr);
    }

    static class Err {
        public final Class clazz;
        public final String message;
        public final Pattern inRegex = Pattern.compile(
            "^(?<message>[^\n]*?)(?: in)?:\n.*$",
            Pattern.DOTALL);
        public final Pattern assertionRegex = Pattern.compile(
            "^(?<message>[^\n]*?)\nFailed assertion:.*$",
            Pattern.DOTALL);

        public Err(String classname, String message) {
            String clazzname = "com.rethinkdb.gen.exc." + classname;
            try {
                this.clazz = Class.forName(clazzname);
            } catch (ClassNotFoundException cnfe) {
                throw new RuntimeException("Bad exception class: "+clazzname, cnfe);
            }
            this.message = message;
        }

        public boolean equals(Object other) {
            if(other.getClass() != clazz) {
                System.out.println("Classes didn't match: "
                                   + clazz + " vs. " + other.getClass());
                return false;
            }
            String otherMessage = ((Exception) other).getMessage();
            otherMessage = inRegex.matcher(otherMessage)
## Thise ${} syntax is double escaped to avoid confusing mako
                .replaceFirst("${'${message}:'}");
            otherMessage = assertionRegex.matcher(otherMessage)
                .replaceFirst("${'${message}'}");
            return message.equals(otherMessage);
        }
    }

    Err err(String classname, String message) {
        return new Err(classname, message);
    }

    Err err(String classname, String message, List _unused) {
        return err(classname, message);
    }

    static class ErrRegex {
        public final Class clazz;
        public final String message_rgx;

        public ErrRegex(String classname, String message_rgx) {
            String clazzname = "com.rethinkdb.gen.exc." + classname;
            try {
                this.clazz = Class.forName(clazzname);
            } catch (ClassNotFoundException cnfe) {
                throw new RuntimeException("Bad exception class: "+clazzname, cnfe);
            }
            this.message_rgx = message_rgx;
        }

        public boolean equals(Object other) {
            if(other.getClass() != clazz) {
                return false;
            }
            return Pattern.matches(message_rgx, ((Exception)other).getMessage());
        }
    }

    ErrRegex err_regex(String classname, String message_rgx) {
        return new ErrRegex(classname, message_rgx);
    }

    ErrRegex err_regex(String classname, String message_rgx, Object dummy) {
        // Some invocations pass a stack frame as a third argument
        return new ErrRegex(classname, message_rgx);
    }

    ReqlExpr fetch(ReqlExpr query, long limit) {
        return query.limit(limit).coerceTo("ARRAY");
    }
    ArrayList fetch(Cursor cursor, long limit) {
        long total = 0;
        ArrayList result = new ArrayList((int) limit);
        for(long i = 0; i < limit; i++) {
            if(!cursor.hasNext()){
                break;
            }
            result.set((int) i, cursor.next());
        }
        return result;
    }

    Object runOrCatch(Object query, OptArgs runopts) {
        if(query == null) {
            return null;
        }
        if(query instanceof Cursor) {
            ArrayList ret = new ArrayList();
            ((Cursor)query).forEachRemaining(ret::add);
            return ret;
        }
        try {
            return ((ReqlAst)query).run(conn, runopts);
        } catch (Exception e) {
            return e;
        }
    }

    LongStream range(long start, long stop) {
        return LongStream.range(start, stop);
    }

    List list(LongStream str) {
        return str.boxed().collect(Collectors.toList());
    }

    static class sys {
        static class floatInfo {
            public static final Double min = Double.MIN_VALUE;
            public static final Double max = Double.MAX_VALUE;
        }
    }

    ZoneOffset PacificTimeZone() {
        return ZoneOffset.ofHours(-7);
    }

    ZoneOffset UTCTimeZone() {
        return ZoneOffset.ofHours(0);
    }

    static class datetime {
        static OffsetDateTime fromtimestamp(double seconds, ZoneOffset offset) {
            Instant inst = Instant.ofEpochMilli(
                (new Double(seconds * 1000)).longValue());
            return OffsetDateTime.ofInstant(inst, offset);
        }

        static OffsetDateTime now() {
            return OffsetDateTime.now();
        }
    }

    static class ast {
        static ZoneOffset rqlTzinfo(String offset) {
            if(offset.equals("00:00")){
                offset = "Z";
            }
            return ZoneOffset.of(offset);
        }
    }

    Double float_(Double nbr) {
        return nbr;
    }

    Object wait_(long length) {
        try {
            Thread.sleep(length * 1000);
        }catch(InterruptedException ie) {}
        return null;
    }

    // Autogenerated tests below

    <%rendered_vars = set() %>\
    @Test
    public void test() throws Exception {
        <%rendered_something = False %>\
        %for item in defs_and_test:
        %if type(item) == JavaDef:
        <%rendered_something = True %>\

        // ${item.testfile} #${item.test_num}
        // ${item.line.original}
        %if item.varname in rendered_vars:
        ${item.varname} = ${item.value};
        %else:
        ${item.line.java}
        <%rendered_vars.add(item.varname)%>\
        %endif
        %elif type(item) == JavaQuery:
        <%rendered_something = True %>
        {
            // ${item.testfile} #${item.test_num}
            /* ${item.expected_line.original} */
            %if item.expected_type == 'Long':
            Long expected_ = new Long(${item.expected_line.java});
            %else:
            ${item.expected_type} expected_ = ${item.expected_line.java};
            %endif
            /* ${item.line.original} */
            Object obtained = runOrCatch(${item.line.java},
                                          new OptArgs()
            %if item.runopts:
              %for key, val in item.runopts.items():
                                          .with("${key}", ${val})
              %endfor
            %endif
                                          );
            try {
            %if item.expected_type.endswith('[]'):
                assertArrayEquals(expected_, (${item.expected_type}) obtained);
            %elif item.expected_type == 'Double':
                assertEquals((double) expected_,
                             ((Number) obtained).doubleValue(),
                             0.00000000001);
            %else:
                assertEquals(expected_, obtained);
            %endif
            } catch (Throwable ae) {
                if(obtained instanceof Throwable) {
                    ((Throwable) obtained).printStackTrace();
                    ae.addSuppressed((Throwable) obtained);
                }
                throw ae;
            }
        }
        %endif
        %endfor
        %if not rendered_something:
        <% raise EmptyTemplate() %>
        %endif
    }
}
