package com.rethinkdb.ast;

import com.rethinkdb.gen.proto.QueryType;
import com.rethinkdb.model.OptArgs;
import com.rethinkdb.net.Util;

import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.util.*;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import org.json.simple.JSONArray;

/* An instance for a query that has been sent to the server. Keeps
 * track of its token, the args to .run() it was called with, and its
 * query type.
*/

public class Query {
    public final QueryType type;
    public final long token;
    public final Optional<ReqlAst> term;
    public final OptArgs globalOptions;

    static final Logger logger = LoggerFactory.getLogger(Query.class);

    public Query(QueryType type, long token, ReqlAst term, OptArgs globalOptions) {
        this.type = type;
        this.token = token;
        this.term = Optional.ofNullable(term);
        this.globalOptions = globalOptions;
    }

    public Query(QueryType type, long token) {
        this(type, token, null, new OptArgs());
    }

    public static Query stop(long token) {
        return new Query(QueryType.STOP, token, null, new OptArgs());
    }

    public static Query continue_(long token) {
        return new Query(QueryType.CONTINUE, token, null, new OptArgs());
    }

    public static Query start(long token, ReqlAst term, OptArgs globalOptions) {
        return new Query(QueryType.START, token, term, globalOptions);
    }

    public static Query noreplyWait(long token) {
        return new Query(QueryType.NOREPLY_WAIT, token, null, new OptArgs());
    }

    public ByteBuffer serialize() {
        JSONArray queryArr = new JSONArray();
        queryArr.add(type.value);
        term.ifPresent(t -> queryArr.add(t.build()));
        if(!globalOptions.isEmpty()) {
            queryArr.add(ReqlAst.buildOptarg(globalOptions));
        }
        String queryJson = queryArr.toJSONString();
        byte[] queryBytes = queryJson.getBytes(StandardCharsets.UTF_8);
        ByteBuffer bb = Util.leByteBuffer(Long.BYTES + Integer.BYTES + queryBytes.length)
            .putLong(token)
            .putInt(queryBytes.length)
            .put(queryBytes);
        logger.debug("JSON Send: Token: {} {}", token, queryJson);
        return bb;
    }
}
