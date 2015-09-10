import com.rethinkdb.RethinkDB;
import com.rethinkdb.gen.exc.*;
import com.rethinkdb.gen.ast.*;
import com.rethinkdb.model.MapObject;
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
    public static final Table ${table_var_name} = r.db("test").table("atest");

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
    }

    @After
    public void tearDown() throws Exception {
        ${table_var_name}.delete().run(conn);
        conn.close();
    }

    @Test
    public void test() throws Exception {
        %for rendered, expected, item in defs_and_test:
        %if type(item) == Def:

        ${rendered}
        %elif type(item) == Query:
        {
            Object expected = ${expected};
            Object obtained = ${rendered}.run(conn);
            assertEquals(expected, obtained);
        }
        %endif
        %endfor
    }
}
