package com.rethinkdb.ast;

import com.rethinkdb.ast.helper.Arguments;
import com.rethinkdb.ast.helper.OptArgs;
import com.rethinkdb.proto.TermType;

import java.util.*;

import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

/** Base class for all reql queries.
 */
public class ReqlAst {

    protected final ReqlAst prev;
    protected final TermType termType;
    protected final Arguments args;
    protected final OptArgs optargs;

    protected ReqlAst(ReqlAst prev, TermType termType, Arguments args, OptArgs optargs) {
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

    protected ReqlAst(TermType termType, Arguments args) {
        this(null, termType, args, null);
    }

    protected JSONArray build() {
        // Create a JSON object from the Ast
        JSONArray list = new JSONArray();
        list.add(termType.value);
        if (args != null) {
            for(ReqlAst arg: args){
                list.add(arg.build());
            }
        }
        if (optargs != null) {
            JSONObject joptargs = new JSONObject();
            for(Map.Entry<String, ReqlAst> entry: optargs.entrySet()){
                joptargs.put(entry.getKey(), entry.getValue().build());
            }
            list.add(joptargs);
        }
        return list;
    }
}
