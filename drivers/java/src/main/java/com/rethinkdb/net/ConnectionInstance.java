package com.rethinkdb.net;

import java.util.*;

import com.rethinkdb.proto.QueryType;
import com.rethinkdb.proto.ResponseType;
import com.rethinkdb.ast.Query;
import com.rethinkdb.response.Response;


public class ConnectionInstance<C extends Connection> {

    private Optional<C> parent = Optional.empty();
    private HashMap<Long, Cursor> cursorCache = new HashMap<>();
    private Optional<SocketWrapper> socket = Optional.empty();
    private boolean closing = false;
    // TODO: when adding async, _header_in_progress may be needed

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
        socket.sendall(query.serialize());
        if(noreply){
            return Optional.empty();
        }

        Response res = readResponse(query.token);

        // TODO: This logic needs to move into the Response class
        if(res.isAtom()){
            return Optional.of((Object) convertPseudotypes(res.data[0], res.profile));
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

    private Response readResponse(long token) {
        throw new RuntimeException("readResponse is not implemented");
    }
}
