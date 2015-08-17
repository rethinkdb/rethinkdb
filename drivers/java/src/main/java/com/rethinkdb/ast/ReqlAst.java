package com.rethinkdb.ast;

import com.rethinkdb.ReqlDriverError;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.GlobalOptions;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.net.Connection;
import com.rethinkdb.proto.TermType;

import java.lang.reflect.Array;
import java.util.*;
import java.util.stream.Collectors;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

/** Base class for all reql queries.
 */
public class ReqlAst {

    protected final Optional<ReqlAst> prev;
    protected final TermType termType;
    protected final Arguments args;
    protected final OptArgs optargs;

    protected ReqlAst(ReqlAst prev, TermType termType, Arguments args, OptArgs optargs) {
        this.prev = Optional.ofNullable(prev);
        if(termType == null){
            throw new ReqlDriverError("termType can't be null!");
        }
        this.termType = termType;
        this.args = new Arguments();
        if(prev != null){
            this.args.add(prev);
        }
        if(args != null){
            this.args.addAll(args);
        }
        this.optargs = optargs != null ? optargs : new OptArgs();
    }

    protected ReqlAst(TermType termType, Arguments args) {
        this(null, termType, args, null);
    }

    protected java.lang.Object build() {
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
            JSONObject joptargs = new JSONObject();
            for(Map.Entry<String, ReqlAst> entry: optargs.entrySet()){
                joptargs.put(entry.getKey(), entry.getValue().build());
            }
            list.add(joptargs);
        }
        return list;
    }

    public Optional<Object> run(Connection conn, GlobalOptions g) {
        return conn.run(this, g);
    }

    public Optional<Object> run(Connection conn) {
        return conn.run(this, new GlobalOptions());
    }

    @Override
    public String toString() {
        return "ReqlAst{" +
                "prev=" + prev +
                ", termType=" + termType +
                ", args=" + args +
                ", optargs=" + optargs +
                '}';
    }
}
