package com.rethinkdb.ast;

import com.rethinkdb.proto.QueryType;
import com.rethinkdb.ast.RqlAst;
import com.rethinkdb.ast.helper.OptArgs;

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
    public final RqlAst term;
    public final long token;
    public final OptArgs globalOptargs;

    public Query(QueryType type, long token, RqlAst term, OptArgs globalOptargs) {
        this.type = type;
        this.token = token;
        this.term = term;
        this.globalOptargs = globalOptargs;
    }

    public static Query stop(long token) {
        return new Query(QueryType.STOP, token, null, null);
    }

    public static Query continue_(long token) {
        return new Query(QueryType.CONTINUE, token, null, null);
    }

    public static Query stop(long token, RqlAst term, OptArgs globalOptargs) {
        return new Query(QueryType.START, token, term, globalOptargs);
    }

    public static Query noreplyWait(long token) {
        return new Query(QueryType.NOREPLY_WAIT, token, null, null);
    }

    public ByteBuffer serialize() {
        JSONArray queryArr = new JSONArray();
        queryArr.add(type.value);
        if (term != null) {
            queryArr.add(term.build());
        }
        if (globalOptargs != null) {
            queryArr.add(globalOptargs);
        }
        String queryJson = queryArr.toJSONString();
        return Util.leByteBuffer(8 + 4 + queryJson.length())
            .putLong(token)
            .putInt(queryJson.length())
            .put(queryJson.getBytes());
    }
}
