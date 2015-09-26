package com.rethinkdb.ast;

import com.rethinkdb.gen.exc.ReqlDriverError;
import com.rethinkdb.gen.proto.TermType;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.net.Connection;
import com.rethinkdb.net.ConnectionInstance;
import org.json.simple.JSONArray;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

/** Base class for all reql queries.
 */
public class ReqlAst {

    protected final TermType termType;
    protected final Arguments args;
    protected final OptArgs optargs;

    protected ReqlAst(TermType termType, Arguments args, OptArgs optargs) {
        if(termType == null){
            throw new ReqlDriverError("termType can't be null!");
        }
        this.termType = termType;
        this.args = args != null ? args : new Arguments();
        this.optargs = optargs != null ? optargs : new OptArgs();
    }

    protected Object build() {
        // Create a JSON object from the Ast
        JSONArray list = new JSONArray();
        list.add(termType.value);
        if (args.size() > 0) {
            list.add(args.stream()
                    .map(ReqlAst::build)
                    .collect(Collectors.toCollection(JSONArray::new)));
        }else {
            list.add(new JSONArray());
        }
        if (optargs.size() > 0) {
            list.add(buildOptarg(optargs));
        }
        return list;
    }

    public static Map<String, Object> buildOptarg(OptArgs opts){
        Map<String, Object> result = new HashMap<>( opts.size() );
        opts.forEach( (name, arg) -> result.put( name, arg.build() ) );
        return result;
    }

    public <T> T run(Connection<? extends ConnectionInstance> conn,
                     OptArgs runOpts) {
        return conn.run(this, runOpts);
    }

    public <T> T run(Connection<? extends ConnectionInstance> conn) {
        return conn.run(this, new OptArgs());
    }

    public <T> T run(Connection<? extends ConnectionInstance> conn, OptArgs runOpts, Class<T> pojoClass) {
        return Util.toPojo(conn.run(this, runOpts), pojoClass);
    }

    public <T> T run(Connection<? extends ConnectionInstance> conn, Class<T> pojoClass) {
        return Util.toPojo(conn.run(this, new OptArgs()), pojoClass);
    }

    public <T> List<T> runList(Connection<? extends ConnectionInstance> conn, OptArgs runOpts, Class<T> pojoClass) {
        return Util.toPojoList(conn.run(this, runOpts), pojoClass);
    }

    public <T> List<T> runList(Connection<? extends ConnectionInstance> conn, Class<T> pojoClass) {
        return Util.toPojoList(conn.run(this, new OptArgs()), pojoClass);
    }

    public void runNoReply(Connection conn){
        conn.runNoReply(this, new OptArgs());
    }

    public void runNoReply(Connection conn, OptArgs globalOpts){
        conn.runNoReply(this, globalOpts);
    }

    @Override
    public String toString() {
        return "ReqlAst{" +
                "termType=" + termType +
                ", args=" + args +
                ", optargs=" + optargs +
                '}';
    }
}
