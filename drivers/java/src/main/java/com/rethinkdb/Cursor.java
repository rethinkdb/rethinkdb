package com.rethinkdb;

import com.rethinkdb.proto.ResponseType;
import com.rethinkdb.response.DBResultFactory;
import com.rethinkdb.response.Response;
import com.rethinkdb.ast.Query;
import com.rethinkdb.RethinkDBException;

import java.util.*;


public class Cursor<T> implements Iterator<T> {

    protected final RethinkDBConnection connection;
    protected final Query query;
    protected List<T> items = new ArrayList<>();
    protected int outstandingRequests = 1;
    protected int threshold = 0;
    protected RethinkDBException error = null;

    public Cursor(RethinkDBConnection connection, Query query) {
        this.connection = connection;
        this.query = query;
        connection.addToCache(query.token, this);
    }

    public void close() {
        if (error == null && connection.isOpen()) {
            outstandingRequests += 1;
            connection.stop(this);
        }
    }

    @Override
    public boolean hasNext() {
        // if (currentBatch.size() > 0) {
        //     return true;
        // } else if (!isClosed()) {
        //     if (isFeed) {
        //         return true;
        //     } else if (response.isPartial()) {
        //         loadNextBatch();
        //         return hasNext();
        //     }
        // }
        // return false;
        throw new RuntimeException("not implemented yet");
    }

    @Override
    public T next() {
        // if (currentBatch.size() > 0) {
        //     return _next();
        // } else if (!isClosed()){
        //     while(!ended && currentBatch.size() == 0){
        //         loadNextBatch();
        //     }
        //     if(currentBatch.size() != 0) {
        //         return _next();
        //     }
        // }
        throw new NoSuchElementException("There are no more elements to get");
    }

    @Override
    public void remove() {
        throw new UnsupportedOperationException();
    }
}
