package com.rethinkdb.ast;

import com.rethinkdb.RethinkDBConnection;
import com.rethinkdb.ast.helper.Arguments;
import com.rethinkdb.ast.helper.OptArgs;
import com.rethinkdb.proto.TermType;

import java.util.*;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

/** Base class for all reql queries.
 */
public class RqlAst {

    private TermType termType;
    protected Arguments args = new Arguments();
    protected OptArgs optargs = new OptArgs();

    // ** Protected Methods ** //

    protected RqlAst(TermType termType, Arguments args) {
        this(termType, args, new OptArgs());
    }

    protected RqlAst(TermType termType) {
        this(termType, new Arguments(), new OptArgs());
    }

    protected RqlAst(TermType termType, Arguments args, OptArgs optargs) {
        this(null, termType, args, optargs);
    }
    public RqlAst(RqlAst previous, TermType termType, Arguments args, OptArgs optargs) {
        this.termType = termType;

        init(previous, args, optargs);
    }

    protected void init(RqlAst previous, Arguments args, OptArgs optargs) {
        if (previous != null && previous.termType != null) {
            this.args.add(previous);
        }

        if (args != null) {
            this.args.addAll(args);
        }

        if (optargs != null) {
            this.optargs.putAll(optargs);
        }
    }

    protected TermType getTermType() {
        return termType;
    }

    protected Arguments getArguments() {
        return args;
    }

    protected OptArgs getOptArgs() {
        return optargs;
    }

    protected JSONArray build() {
        // Create a JSON object from the Ast
        JSONArray list = new JSONArray();
        list.add(termType.value);
        if (args != null) {
            for(RqlAst arg: args){
                list.add(arg.build());
            }
        }
        if (optargs != null) {
            JSONObject joptargs = new JSONObject();
            for(Map.Entry<String, RqlAst> entry: optargs.entrySet()){
                joptargs.put(entry.getKey(), entry.getValue().build());
            }
            list.add(joptargs);
        }
        return list;
    }
}
