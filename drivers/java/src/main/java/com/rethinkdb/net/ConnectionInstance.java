package com.rethinkdb.net;

import com.rethinkdb.ast.Query;
import com.rethinkdb.gen.exc.*;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Optional;
import java.util.concurrent.TimeoutException;


public class ConnectionInstance {
    // package private
    Optional<SocketWrapper> socket = Optional.empty();

    // protected members
    protected HashMap<Long, Cursor> cursorCache = new HashMap<>();
    protected boolean closing = false;
    protected Optional<ByteBuffer> headerInProgress = Optional.empty();

    public ConnectionInstance() {
    }

    public void connect(
            String hostname,
            int port,
            ByteBuffer handshake,
            Optional<Long> timeout) throws TimeoutException {
        SocketWrapper sock = new SocketWrapper(hostname, port, timeout);
        sock.connect(handshake);
        socket = Optional.of(sock);
    }

    public boolean isOpen() {
        return socket.map(SocketWrapper::isOpen).orElse(false);
    }

    public void close() {
        closing = true;
        for (Cursor cursor : cursorCache.values()) {
            cursor.setError("Connection is closed.");
        }
        cursorCache.clear();
        socket.ifPresent(SocketWrapper::close);
        headerInProgress = Optional.empty();
    }

    void addToCache(long token, Cursor cursor) {
        cursorCache.put(token, cursor);
    }

    void removeFromCache(long token){
        cursorCache.remove(token);
    }

    Optional<Response> readResponse(Query query) {
        try {
            return readResponse(query, Optional.empty());
        } catch (TimeoutException toe) {
            throw new RuntimeException("Timeout exception can't happen here.");
        }
    }

    Optional<Response> readResponse(Query query, Optional<Long> deadline) throws TimeoutException {
        long token = query.token;
        SocketWrapper sock = socket.orElseThrow(() ->
            new ReqlError("Socket not open"));
        while(true) {
            if(!headerInProgress.isPresent()) {
                headerInProgress = Optional.of(sock.recvall(12, deadline));
            }
            long resToken = headerInProgress.get().getLong();
            int resLen = headerInProgress.get().getInt();
            ByteBuffer resBuf = sock.recvall(resLen, deadline);
            headerInProgress = Optional.empty();

            Optional<Cursor> cursor = Optional.ofNullable(cursorCache.get(resToken));
            if(cursor.isPresent()) {
                cursor.get().extend(resBuf);
                if(resToken == token) {
                    return Optional.empty();
                }
            }else if(resToken == token) {
                Response resp = Response.parseFrom(resToken, resBuf);
                return Optional.of(resp);
            }else if(!closing) {
                close();
                throw new ReqlDriverError("Unexpected response received: " +
                        Response.parseFrom(resToken, resBuf));
            }
        }
    }
}
