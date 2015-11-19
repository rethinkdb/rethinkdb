package com.rethinkdb.net;

import com.rethinkdb.ast.Query;
import com.rethinkdb.gen.exc.ReqlRuntimeError;
import com.rethinkdb.gen.proto.ResponseType;

import java.nio.ByteBuffer;
import java.util.*;
import java.util.concurrent.TimeoutException;


public abstract class Cursor<T> implements Iterator<T>, Iterable<T> {

    // public immutable members
    public final long token;

    // immutable members
    protected final Connection connection;
    protected final Query query;
    protected final boolean _isFeed;

    // mutable members
    protected Deque<T> items = new ArrayDeque<>();
    protected int outstandingRequests = 0;
    protected int threshold = 1;
    protected Optional<RuntimeException> error = Optional.empty();

    public Cursor(Connection connection, Query query, Response firstResponse) {
        this.connection = connection;
        this.query = query;
        this.token = query.token;
        this._isFeed = firstResponse.isFeed();
        connection.addToCache(query.token, this);
        maybeSendContinue();
        extendInternal(firstResponse);
    }

    public void close() {
        if (!error.isPresent()) {
            error = Optional.of(new NoSuchElementException());
            if (connection.isOpen()) {
                outstandingRequests += 1;
                connection.stop(this);
            }
        }
    }

    public int bufferedSize() {
        return items.size();
    }

    public ArrayList<T> bufferedItems() {
        return new ArrayList<>(items);
    }

    public boolean isFeed() {
        return this._isFeed;
    }

    void extend(ByteBuffer responseBuffer) {
        outstandingRequests -= 1;
        maybeSendContinue();
        Response res = Response.parseFrom(query.token, responseBuffer);
        extendInternal(res);
    }

    private void extendInternal(Response response) {
        threshold = response.data.size();
        if(!error.isPresent()){
            if(response.isPartial()){
                items.addAll(response.data);
            } else if(response.isSequence()) {
                items.addAll(response.data);
                error = Optional.of(new NoSuchElementException());
            } else {
                error = Optional.of(response.makeError(query));
            }
        }
        if(outstandingRequests == 0 && error.isPresent()) {
            connection.removeFromCache(response.token);
        }
    }

    protected void maybeSendContinue() {
        if(!error.isPresent()
                && items.size() < threshold
                && outstandingRequests == 0) {
            outstandingRequests += 1;
            connection.continue_(this);
        }
    }

    void setError(String errMsg) {
        if(!error.isPresent()){
            error = Optional.of(new ReqlRuntimeError(errMsg));
            Response dummyResponse = Response
                    .make(query.token, ResponseType.SUCCESS_SEQUENCE)
                    .build();
            extendInternal(dummyResponse);
        }
    }

    public static <T> Cursor<T> create(Connection connection, Query query, Response firstResponse, Optional<Class<T>> pojoClass) {
        return new DefaultCursor<T>(connection, query, firstResponse, pojoClass);
    }


    public T next() {
        try {
            return getNext(Optional.empty());
        }catch(TimeoutException toe) {
            throw new RuntimeException("Timeout can't happen here");
        }
    }

    public T next(long timeout) throws TimeoutException {
        return getNext(Optional.of(timeout));
    }

    public Iterator<T> iterator(){
        return this;
    }

    /**
     * Iterates over all elements of this cursor and returns them as a list
     * @return The list of this cursor's elements
     */
    public List<T> toList() {
        List<T> list = new ArrayList<T>();
        forEachRemaining(list::add);
        return list;
    }

    // Abstract methods
    abstract T getNext(Optional<Long> timeout) throws TimeoutException;

    private static class DefaultCursor<T> extends Cursor<T> {
        private final Optional<Class<T>> pojoClass;
        public final Converter.FormatOptions fmt;

        public DefaultCursor(Connection connection, Query query, Response firstResponse, Optional<Class<T>> pojoClass) {
            super(connection, query, firstResponse);

            this.pojoClass = pojoClass;
            fmt = new Converter.FormatOptions(query.globalOptions);
        }

        /* This isn't great, but the Java iterator protocol relies on hasNext,
         so it must be implemented in a reasonable way */
        public boolean hasNext(){
            if(items.size() > 0){
                return true;
            }
            if(error.isPresent()){
                return false;
            }
            if(_isFeed){
                return true;
            }
            maybeSendContinue();
            connection.readResponse(query);
            return items.size() > 0;
        }

        @SuppressWarnings("unchecked")
        T getNext(Optional<Long> timeout) throws TimeoutException {
            while(items.size() == 0) {
                maybeSendContinue();
                error.ifPresent(exc -> {
                    throw exc;
                });
                connection.readResponse(query, timeout.map(Util::deadline));
            }

            Object value = Converter.convertPseudotypes(items.pop(), fmt);
            return Util.convertToPojo(value, pojoClass);
        }
    }
}
