package gen;

import com.rethinkdb.RethinkDB;
import com.rethinkdb.gen.exc.*;
import com.rethinkdb.gen.ast.*;
import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.model.MapObject;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.net.Connection;
import junit.framework.Assert;
import junit.framework.TestCase;
import static org.junit.Assert.assertEquals;

import org.junit.*;
import org.junit.rules.ExpectedException;

import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.IntStream;
import java.util.stream.Collectors;
import java.util.concurrent.TimeoutException;
import java.util.regex.Pattern;
import java.util.Collections;



public class ${module_name} {

    public static final RethinkDB r = RethinkDB.r;
    %for var_name in table_var_names:
    public static final Table ${var_name} = r.db("test").table("${var_name}");
    %endfor

    Connection<?> conn;
    public String hostname = "newton";
    public int port = 31157;

    public ExpectedException expectedEx = ExpectedException.none();

    @Before
    public void setUp() throws Exception {
        conn = r.connection()
            .hostname(hostname)
            .port(port)
            .connect();
        %for var_name in table_var_names:
        r.db("test").tableCreate("${var_name}").run(conn);
        %endfor
    }

    @After
    public void tearDown() throws Exception {
        %for var_name in table_var_names:
        r.db("test").tableDrop("${var_name}").run(conn);
        %endfor
        conn.close();
    }

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

    static class PartialLst {
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

    static class PartialDct {
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

    ArrLen arrlen(int length, Object thing) {
        return new ArrLen(length, thing);
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
        final Integer nbr;
        public IntCmp(Integer nbr) {
            this.nbr = nbr;
        }
        public boolean equals(Object other) {
            return nbr.equals(other);
        }
    }

    IntCmp int_cmp(Integer nbr) {
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

        public Err(String classname, String message) {
            try {
                this.clazz = Class.forName(classname);
            } catch (ClassNotFoundException cnfe) {
                throw new RuntimeException("Bad exception class", cnfe);
            }
            this.message = message;
        }
    }

    Err err(String classname, String message) {
        return new Err(classname, message);
    }

    Err err(String classname, String message, List _unused) {
        return err(classname, message);
    }

    @Test
    public void test() throws Exception {
        <%rendered_something = False %>\
        %for item in defs_and_test:
        %if type(item) == JavaDef:
        <%rendered_something = True %>\

        // ${item.testfile} #${item.test_num}
        // ${item.line.original}
        ${item.line.java}
        %elif type(item) == JavaQuery:
        <%rendered_something = True %>
        {
            // ${item.testfile} #${item.test_num}
            // ${item.expected_line.original}
            ${item.expected_type} expected = ${item.expected_line.java};
            // ${item.line.original}
            Object obtained = ${item.line.java}
            %if item.runopts:
                .run(conn, new OptArgs()
              %for key, val in item.runopts.items():
                    .with("${key}", ${val})
              %endfor
                );
            %else:
                .run(conn);
            %endif
            assertEquals(expected, obtained);
        }
        %endif
        %endfor
        %if not rendered_something:
        <% raise EmptyTemplate() %>
        %endif
    }
}
