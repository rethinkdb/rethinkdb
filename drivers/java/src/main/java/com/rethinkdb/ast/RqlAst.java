package com.rethinkdb.ast;

import com.rethinkdb.Cursor;
import com.rethinkdb.RethinkDBConnection;
import com.rethinkdb.ast.helper.Arguments;
import com.rethinkdb.ast.helper.OptionalArguments;
import com.rethinkdb.proto.TermType;

import java.security.Key;
import java.util.*;

/** Base class for all reql queries.
 */
public class RqlAst {
    public static RqlAst R = new RqlAst(null, null);

    private TermType termType;
    protected List<RqlAst> args = new ArrayList<>();
    protected java.util.Map<String, RqlAst> optargs =
        new HashMap<>();
        // ** Protected Methods ** //

    protected RqlAst(TermType termType, List<Object> args) {
        this(termType, args, new HashMap<String, Object>());
    }

    protected RqlAst(TermType termType) {
        this(termType, new ArrayList<Object>(), new HashMap<String, Object>());
    }

    protected RqlAst(TermType termType, List<Object> args, java.util.Map<String, Object> optargs) {
        this(null, termType, args, optargs);
    }
    public RqlAst(RqlAst previous, TermType termType, List<Object> args, Map<String, Object> optargs) {
        this.termType = termType;

        init(previous, args, optargs);
    }

    protected void init(RqlAst previous, List<Object> args, Map<String, Object> optargs) {
        if (previous != null && previous.termType != null) {
            this.args.add(previous);
        }

        if (args != null) {
            for (Object arg : args) {
                this.args.add(RqlUtil.toRqlAst(arg));
            }
        }

        if (optargs != null) {
            for (Map.Entry<String, Object> kv : optargs.entrySet()) {
                this.optargs.put(kv.getKey(),
                                 RqlUtil.toRqlAst(kv.getValue()));
            }
        }
    }

    protected TermType getTermType() {
        return termType;
    }

    protected List<RqlAst> getArgs() {
        return args;
    }

    protected Map<String, RqlAst> getOptArgs() {
        return optargs;
    }

    protected Map<String, Object> build() {
        return null; // TODO: fill me in
    }
}
