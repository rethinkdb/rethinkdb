package com.rethinkdb.net;

import com.rethinkdb.ReqlError;
import com.rethinkdb.ReqlRuntimeError;
import com.rethinkdb.proto.ResponseType;
import com.rethinkdb.response.Response;
import com.rethinkdb.ast.Query;
import org.json.simple.JSONArray;
import org.json.simple.JSONObject;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Optional;


public abstract class Cursor<T> implements Iterator<T> {

    // public immutable members
    public final long token;

    // immutable members
    protected final Connection connection;
    protected final Query query;

    // mutable members
    protected List<T> items = new ArrayList<>();
    protected int outstandingRequests = 1;
    protected int threshold = 0;
    protected Optional<ReqlError> error = Optional.empty();

    public Cursor(Connection connection, Query query) {
        this.connection = connection;
        this.query = query;
        this.token = query.token;
        connection.addToCache(query.token, this);
    }

    public void close() {
        if(!error.isPresent()){
            error = Optional.of(emptyError());
            if(connection.isOpen()){
                outstandingRequests += 1;
                connection.stop(this);
            }
        }
    }

    @Override
    public boolean hasNext() {
        throw new UnsupportedOperationException();
    }

    abstract public T next(int timeout);

    @Override
    public void remove() {
        throw new UnsupportedOperationException();
    }

    void extend(Response response) {
        outstandingRequests -= 1;
        threshold = response.data.map(JSONArray::size).orElse(0);
        if(!error.isPresent()){
            if(response.isPartial()){
                items.addAll(response.data.orElse(new JSONArray()));
            } else if(response.isSequence()) {
                items.addAll(response.data.orElse(new JSONArray()));
                error = Optional.of(emptyError());
            } else {
                error = Optional.of(response.makeError(query));
            }
        }
        maybeFetchBatch();
        if(outstandingRequests == 0 && error.isPresent()) {
            connection.removeFromCache(response.token);
        }
    }

    protected void maybeFetchBatch() {
        if(!error.isPresent()
                && items.size() <= threshold
                && outstandingRequests == 0) {
            outstandingRequests += 1;
            connection.continue_(this);
        }
    }

    void setError(String errMsg) {
        if(!error.isPresent()){
            error = Optional.of(new ReqlRuntimeError(errMsg));
            Response dummyResponse = new Response(query.token,
                    ResponseType.SUCCESS_SEQUENCE,
                    new ArrayList<>(),
                    Optional.empty(),
                    Optional.empty(),
                    Optional.of(new JSONArray())
                    );
            extend(dummyResponse);
        }
    }

    public static Cursor empty(Connection connection, Query query) {
        return new DefaultCursor(connection, query);
    }

    // Abstract methods
    abstract ReqlError emptyError();
    protected abstract T getNext();
    protected abstract T getNext(int deadline);

    private static class DefaultCursor extends Cursor {
        public DefaultCursor(Connection connection, Query query) {
            super(connection, query);
        }

        @Override
        ReqlError emptyError() {
            return null;
        }

        @Override
        protected Object getNext() {
            return null;
        }

        @Override
        protected Object getNext(int deadline) {
            return null;
        }

        @Override
        public Object next() {
            return null;
        }

        @Override
        public Object next(int timeout) {
            return null;
        }

    }
}
