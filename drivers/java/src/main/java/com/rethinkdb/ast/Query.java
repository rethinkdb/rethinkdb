package com.rethinkdb.ast;

import com.rethinkdb.proto.QueryType;
import com.rethinkdb.model.GlobalOptions;
import com.rethinkdb.net.Util;

import java.nio.ByteBuffer;
import java.util.*;

import org.json.simple.JSONArray;

/* An instance for a query that has been sent to the server. Keeps
 * track of its token, the args to .run() it was called with, and its
 * query type.
*/

public class Query {
    public final QueryType type;
    public final long token;
    public final Optional<ReqlAst> term;
    public final GlobalOptions globalOptions;

    public Query(QueryType type, long token, ReqlAst term, GlobalOptions globalOptions) {
        this.type = type;
        this.token = token;
        this.term = Optional.ofNullable(term);
        this.globalOptions = globalOptions;
    }

    public Query(QueryType type, long token) {
        this(type, token, null, new GlobalOptions());
    }

    public static Query stop(long token) {
        return new Query(QueryType.STOP, token, null, new GlobalOptions());
    }

    public static Query continue_(long token) {
        return new Query(QueryType.CONTINUE, token, null, new GlobalOptions());
    }

    public static Query start(long token, ReqlAst term, GlobalOptions globalOptions) {
        return new Query(QueryType.START, token, term, globalOptions);
    }

    public static Query noreplyWait(long token) {
        return new Query(QueryType.NOREPLY_WAIT, token, null, new GlobalOptions());
    }

    public ByteBuffer serialize() {
        JSONArray queryArr = new JSONArray();
        queryArr.add(type.value);
        term.ifPresent(t -> queryArr.add(t.build()));
        queryArr.add(globalOptions.toOptArgs());
        String queryJson = queryArr.toJSONString();
        ByteBuffer bb = Util.leByteBuffer(8 + 4 + queryJson.length())
            .putLong(token)
            .putInt(queryJson.length())
            .put(queryJson.getBytes());
        System.out.println("Sending: "+ Util.bufferToString(bb));
        return bb;
    }
}
