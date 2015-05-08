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
    public final int token;
    public final OptArgs globalOptargs;

    public Query(QueryType type, int token, RqlAst term, OptArgs globalOptargs) {
        this.type = type;
        this.token = token;
        this.term = term;
        this.globalOptargs = globalOptargs;
    }

    public ByteBuffer serialize() {
        JSONArray queryArr = new JSONArray();
        queryArr.add(type.value);
        queryArr.add(term.build());
        queryArr.add(globalOptargs);
        String queryJson = queryArr.toJSONString();
        return Util.leByteBuffer(8 + 4 + queryJson.length())
            .putLong(token)
            .putInt(queryJson.length())
            .put(queryJson.getBytes());
    }
}
