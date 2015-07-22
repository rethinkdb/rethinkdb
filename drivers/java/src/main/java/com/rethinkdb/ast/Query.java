package com.rethinkdb.ast;

import com.rethinkdb.proto.QueryType;
import com.rethinkdb.ast.RqlAst;
import com.rethinkdb.ast.helper.OptArgs;
import com.rethinkdb.model.GlobalOptions;

import java.nio.ByteBuffer;
import java.util.*;

import org.json.simple.JSONObject;
import org.json.simple.JSONArray;

/* An instance for a query that has been sent to the server. Keeps
 * track of its token, the args to .run() it was called with, and its
 * query type.
*/

public class Query {
    public final QueryType type;
    public final long token;
    public final Optional<RqlAst> term;
    public final GlobalOptions globalOptions;

    public Query(QueryType type, long token, RqlAst term, GlobalOptions globalOptions) {
        this.type = type;
        this.token = token;
        this.term = Optional.ofNullable(term);
        this.globalOptions = globalOptions;
    }

    public Query(QueryType type, long token) {
        this(type, token, null, GlobalOptions.default_());
    }

    public static Query stop(long token) {
        return new Query(QueryType.STOP, token, null, GlobalOptions.default_());
    }

    public static Query continue_(long token) {
        return new Query(QueryType.CONTINUE, token, null, GlobalOptions.default_());
    }

    public static Query start(long token, RqlAst term, GlobalOptions globalOptions) {
        return new Query(QueryType.START, token, term, globalOptions);
    }

    public static Query noreplyWait(long token) {
        return new Query(QueryType.NOREPLY_WAIT, token, null, GlobalOptions.default_());
    }

    public ByteBuffer serialize() {
        JSONArray queryArr = new JSONArray();
        queryArr.add(type.value);
        if (term != null) {
            queryArr.add(term.build());
        }
        queryArr.add(globalOptions.toMap());
        String queryJson = queryArr.toJSONString();
        return Util.leByteBuffer(8 + 4 + queryJson.length())
            .putLong(token)
            .putInt(queryJson.length())
            .put(queryJson.getBytes());
    }
}
