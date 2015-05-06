package com.rethinkdb;

import com.rethinkdb.proto.TermType;
import com.rethinkdb.proto.Version;
import com.rethinkdb.proto.QueryType;
import com.rethinkdb.proto.QueryBuilder;
import com.rethinkdb.response.Response;
import com.rethinkdb.response.DBResultFactory;
import com.rethinkdb.ast.RqlAst;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.List;
import java.util.concurrent.atomic.AtomicInteger;

public class RethinkDBConnection {
    private static final Logger logger = LoggerFactory.getLogger(RethinkDBConnection.class);

    private static final AtomicInteger tokenGenerator = new AtomicInteger();

    private String hostname;
    private String authKey;
    private int port;
    private int timeout;
    private String dbName = null;

    private SocketChannelFacade socket = new SocketChannelFacade();

    public RethinkDBConnection() {
        this(RethinkDBConstants.DEFAULT_HOSTNAME);
    }

    public RethinkDBConnection(String hostname) {
        this(hostname, RethinkDBConstants.DEFAULT_PORT);
    }

    public RethinkDBConnection(String hostname, int port) {
        this(hostname, port, RethinkDBConstants.DEFAULT_AUTHKEY);
    }

    public RethinkDBConnection(String hostname, int port, String authKey) {
        this(hostname, port, authKey, RethinkDBConstants.DEFAULT_TIMEOUT);
    }

    public RethinkDBConnection(String hostname, int port, String authKey, int timeout) {
        this.hostname = hostname;
        this.port = port;
        this.authKey = authKey;
        this.timeout = timeout;
        reconnect();
    }

    // TODO: use timeout
    public void reconnect() {
        socket.connect(hostname, port);
        socket.writeLEInt(Version.V0_2.value);
        socket.writeStringWithLength(authKey);

        String result = socket.readString();
        if (!result.startsWith(RethinkDBConstants.Protocol.SUCCESS)) {
            throw new RethinkDBException(result);
        }
    }

    public void close() {
        socket.close();
    }

    public boolean isClosed(){
        return socket.isClosed();
    }

    public void use(String dbName) {
        this.dbName = dbName;
    }

    protected Response getNext(long token) {
        return new Response();
        // TODO: send CONTINUE query
    }

    protected <T> void closeCursor(Cursor<T> cursor) {
        // TODO: close cursor
    }

    public <T> T run(RqlAst ast) {
        // TODO: build query & execute it.
        return null;
    }

    private QueryBuilder startQuery(RqlAst query) {
        // TODO send token and START

        throw new RuntimeException("startQuery not implemented");
    }

    private <T> Response execute(QueryBuilder query) {
        logger.debug("running {} ", query);
        socket.write(query.toByteArray());
        return socket.read();
    }

}
