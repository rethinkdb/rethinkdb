package com.rethinkdb;

import com.rethinkdb.proto.TermType;
import com.rethinkdb.proto.Version;
import com.rethinkdb.proto.QueryType;
import com.rethinkdb.ast.Query;
import com.rethinkdb.ast.helper.OptArgs;
import com.rethinkdb.response.Response;
import com.rethinkdb.response.DBResultFactory;
import com.rethinkdb.ast.RqlAst;
import com.rethinkdb.RethinkDBException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.atomic.AtomicLong;
import java.util.*;

public class Connection {
    private static final Logger logger =
        LoggerFactory.getLogger(Connection.class);

    private static final AtomicLong tokenGenerator = new AtomicLong();

    private String hostname;
    private String authKey;
    private int port;
    private int timeout;
    private String dbName = null;
    protected Map<Long, Cursor> cursorCache = new HashMap<>();

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
        socket.writeLEInt(Version.V0_2.value);

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

    public boolean isOpen() {
        return !isClosed();
    }

    protected void assertOpen() {
        if (isClosed()){
            throw new RethinkDBException("Connection is closed");
        }
    }

    protected long nextToken() {
        return tokenGenerator.getAndIncrement();
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

    private <T> Response execute(Query query) {
        // logger.debug("running {} ", query);
        // socket.write(query.toByteArray());
        // return socket.read();
        socket.write(query.serialize());
        return socket.read();
    }

    void addToCache(long token, Cursor cursor) {
        cursorCache.put(token, cursor);
    }

    Query startQuery(RqlAst query, OptArgs globalOptargs) {
        execute(Query.start(nextToken(), query, globalOptargs));
    }

    void stop(Cursor cursor) {
        assertOpen();
        execute(Query.stop(cursor.token));
    }

}
