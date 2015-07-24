package com.rethinkdb.net;

import java.util.*;
import java.nio.ByteBuffer;

import com.rethinkdb.RethinkDBException;
import com.rethinkdb.ReqlDriverError;
import com.rethinkdb.proto.QueryType;
import com.rethinkdb.proto.ResponseType;
import com.rethinkdb.ast.Query;
import com.rethinkdb.response.Response;


public class ConnectionInstance<C extends Connection> {

    private Optional<C> parent = Optional.empty();
    private HashMap<Long, Cursor> cursorCache = new HashMap<>();
    private Optional<SocketWrapper> socket = Optional.empty();
    private boolean closing = false;
    private Optional<ByteBuffer> headerInProgress = Optional.empty();

    public ConnectionInstance(C parent) {
        this.parent = Optional.ofNullable(parent);
    }

    public C connect(Optional<Integer> timeout) {
        String parentHost = this.parent.get().hostname;
        int parentPort = this.parent.get().port;
        socket = Optional.of(new SocketWrapper(parentHost, parentPort, timeout));
        return parent.get();
    }

    public boolean isOpen() {
        return socket.map(SocketWrapper::isOpen).orElse(false);
    }

    public void close(boolean noreplyWait, long token) {
        closing = true;
        for(Cursor cursor: cursorCache.values()) {
            cursor.error("Connection is closed.");
        }
        cursorCache.clear();
        try {
            if(noreplyWait) {
                Query noreplyWaitQuery = new Query(QueryType.NOREPLY_WAIT, token);
                runQuery(noreplyWaitQuery, false);
            }
        } finally {
            socket.ifPresent(SocketWrapper::close);
        }
    }

    private Optional<Object> runQuery(Query query, boolean noreply) {
        socket.orElseThrow(() -> new ReqlDriverError("No socket open."))
            .sendall(query.serialize());
        if(noreply){
            return Optional.empty();
        }

        Response res = readResponse(query.token);

        // TODO: This logic needs to move into the Response class
        if(res.isAtom()){
            return Optional.of(
                (Object) Response.convertPseudotypes(res.data.get(), res.profile));
        } else if(res.isPartial() || res.isSequence()) {
            Cursor cursor = Cursor.empty(this, query);
            cursor.extend(res);
            return Optional.of((Object) cursor);
        } else if(res.isWaitComplete()) {
            return Optional.empty();
        } else {
            throw res.makeError(query);
        }
    }

    void addToCache(long token, Cursor cursor) {
        cursorCache.put(token, cursor);
    }

    Response readResponse(long token) {
        return readResponse(token, Optional.empty());
    }

    Response readResponse(long token, Optional<Integer> deadline) {
        SocketWrapper sock = socket.orElseThrow(() ->
            new RethinkDBException("Socket not open"));
        while(true) {
            if(!headerInProgress.isPresent()) {
                headerInProgress = Optional.of(sock.recvall(12, deadline));
            }
            long resToken = headerInProgress.get().getLong();
            int resLen = headerInProgress.get().getInt();
            ByteBuffer resBuf = sock.recvall(resLen, deadline);
            headerInProgress = Optional.empty();

            Response res = Response.parseFrom(resToken, resBuf);

            Optional<Cursor> cursor = Optional.ofNullable(cursorCache.get(resToken));
            cursor.ifPresent(c -> c.extend(res));

            if(res.token == token) {
                return res;
            }else if(closing || cursor.isPresent()) {
                close(false, token);
                throw new ReqlDriverError("Unexpected response received");
            }
        }
    }
}
