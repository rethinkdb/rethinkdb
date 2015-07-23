package com.rethinkdb.net;

import com.rethinkdb.proto.TermType;
import com.rethinkdb.proto.Version;
import com.rethinkdb.proto.QueryType;
import com.rethinkdb.ast.Query;
import com.rethinkdb.ast.helper.OptArgs;
import com.rethinkdb.model.GlobalOptions;
import com.rethinkdb.response.Response;
import com.rethinkdb.response.DBResultFactory;
import com.rethinkdb.ast.RqlAst;
import com.rethinkdb.RethinkDBException;
import com.rethinkdb.RethinkDBConstants;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.atomic.AtomicLong;
import java.util.*;

public class Connection {
    private static final Logger logger =
        LoggerFactory.getLogger(Connection.class);

    private static final AtomicLong tokenGenerator = new AtomicLong();

    public final String hostname;
    public final int port;
    public final int timeout;

    private final String authKey;

    private String dbName;


    public Connection() {
        throw new RuntimeException("constructor not implemented");
    }
    public Connection(String hostname) {
        throw new RuntimeException("constructor not implemented");
    }
    public Connection(String hostname, int port) {
        throw new RuntimeException("constructor not implemented");
    }
    public Connection(String hostname, int port, String authKey) {
        throw new RuntimeException("constructor not implemented");
    }
    public Connection(String hostname, int port, String authKey, int timeout) {
        throw new RuntimeException("constructor not implemented");
    }
}
