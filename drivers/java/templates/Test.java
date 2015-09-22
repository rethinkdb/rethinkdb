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
import java.util.stream.IntStream;
import java.util.stream.Collectors;
import java.util.concurrent.TimeoutException;


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

    public Object bag(Object...) {
        throw new RuntimeException("Not implemented");
    }

    @Test
    public void test() throws Exception {
        <%rendered_something = False %>\
        %for item in defs_and_test:
        %if type(item) == JavaDef:
        <%rendered_something = True %>\

        // ${item.testfile} #${item.test_num}
        // ${item.original_line}
        ${item.java_line}
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
