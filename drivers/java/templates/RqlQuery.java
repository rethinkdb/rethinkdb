package com.rethinkdb.ast.query;

import com.google.common.collect.Lists;
import com.rethinkdb.Cursor;
import com.rethinkdb.RethinkDBConnection;
import com.rethinkdb.ast.helper.Arguments;
import com.rethinkdb.ast.helper.OptionalArguments;
import com.rethinkdb.ast.query.gen.Date;
import com.rethinkdb.model.Durability;
import com.rethinkdb.model.RqlFunction;
import com.rethinkdb.ast.query.gen.*;
import com.rethinkdb.model.RqlFunction2;
import com.rethinkdb.proto.TermType
import com.rethinkdb.response.GroupedResponseConverter;

import java.security.Key;
import java.util.*;

public class RqlQuery {

    public static RqlQuery R = new RqlQuery(null, null);

    private TermType termType;
    protected List<RqlQuery> args = new ArrayList<RqlQuery>();
    protected java.util.Map<String, RqlQuery> optionalArgs =
        new HashMap<String, RqlQuery>();

    // ** Protected Methods ** //

    protected RqlQuery(TermType termType, List<Object> args) {
        this(termType, args, new HashMap<String, Object>());
    }

    protected RqlQuery(TermType termType) {
        this(termType, new ArrayList<Object>(), new HashMap<String, Object>());
    }

    protected RqlQuery(TermType termType, List<Object> args, java.util.Map<String, Object> optargs) {
        this(null, termType, args, optargs);
    }
    public RqlQuery(RqlQuery previous, TermType termType, List<Object> args, Map<String, Object> optargs) {
        this.termType = termType;

        init(previous, args, optargs);
    }

    protected void init(RqlQuery previous, List<Object> args, Map<String, Object> optionalArgs) {
        if (previous != null && previous.termType != null) {
            this.args.add(previous);
        }

        if (args != null) {
            for (Object arg : args) {
                this.args.add(RqlUtil.toRqlQuery(arg));
            }
        }

        if (optionalArgs != null) {
            for (Map.Entry<String, Object> kv : optionalArgs.entrySet()) {
                this.optionalArgs.put(kv.getKey(), RqlUtil.toRqlQuery(kv.getValue()));
            }
        }
    }

    protected TermType getTermType() {
        return termType;
    }

    protected List<RqlQuery> getArgs() {
        return args;
    }

    protected Map<String, RqlQuery> getOptionalArgs() {
        return optionalArgs;
    }


    /* Public API */
    public RqlQuery opt(Object... optargs){

        for(Object optargs: optargs){

        }
    }

    /* Query level terms */
% for term, info in meta.iteritems():
    % if 'query' in info.get('include_in', ['query']):
    public ${camel(term)} ${info.get('alias', dromedary(term))}(Object... fields) {
        return new ${camel(term)}(this, new Arguments(fields), new Optargs());
    }

    % endif
% endfor
}
