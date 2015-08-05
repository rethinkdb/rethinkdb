package com.rethinkdb.net;

import com.rethinkdb.ReqlDriverError;
import com.rethinkdb.ReqlError;
import com.rethinkdb.response.Response;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Optional;


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
            Optional<Integer> timeout) {
        SocketWrapper sock = new SocketWrapper(hostname, port, timeout);
        sock.connect(handshake);
        socket = Optional.of(sock);
    }

    public boolean isOpen() {
        return socket.map(SocketWrapper::isOpen).orElse(false);
    }

    public <T> void close() {
        closing = true;
        for (Cursor cursor : cursorCache.values()) {
            cursor.setError("Connection is closed.");
        }
        cursorCache.clear();
        socket.ifPresent(SocketWrapper::close);
    }

    void addToCache(long token, Cursor cursor) {
        cursorCache.put(token, cursor);
    }

    void removeFromCache(long token){
        cursorCache.remove(token);
    }

    Response readResponse(long token) {
        return readResponse(token, Optional.empty());
    }

    Response readResponse(long token, Optional<Integer> deadline) {
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

            Response res = Response.parseFrom(resToken, resBuf);

            Optional<Cursor> cursor = Optional.ofNullable(
                    cursorCache.get(resToken));
            cursor.ifPresent(c -> c.extend(res));

            if(res.token == token) {
                return res;
            }else if(closing || cursor.isPresent()) {
                close();
                throw new ReqlDriverError("Unexpected response received");
            }
        }
    }
}
