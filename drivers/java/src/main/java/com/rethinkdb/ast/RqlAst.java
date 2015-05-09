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

    protected final RqlAst prev;
    protected final TermType termType;
    protected final Arguments args;
    protected final OptArgs optargs;

    protected RqlAst(RqlAst prev, TermType termType, Arguments args, OptArgs optargs) {
        this.prev = prev;
        if (termType == null) {
            throw new RuntimeException("termType can't be null.");
        }
        this.termType = termType;
        if (args != null) {
            this.args = args;
        } else {
            this.args = new Arguments();
        }
        if (optargs != null) {
            this.optargs = optargs;
        } else {
            this.optargs = new OptArgs();
        }
    }

    protected RqlAst(TermType termType, Arguments args) {
        this(null, termType, args, null);
    }

    // protected RqlAst(TermType termType, Object[] args) {

    // }

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
