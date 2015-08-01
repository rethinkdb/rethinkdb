package com.rethinkdb.net;

import com.rethinkdb.proto.Version;
import com.rethinkdb.proto.Protocol;
import com.rethinkdb.ast.Query;
import com.rethinkdb.model.GlobalOptions;
import com.rethinkdb.ast.RqlAst;
import com.rethinkdb.ast.Util;
import com.rethinkdb.ReqlDriverError;
import com.rethinkdb.response.Response;

import java.util.concurrent.atomic.AtomicLong;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.function.Supplier;

public class Connection<C extends ConnectionInstance> {
    // public immutable
    public final String hostname;
    public final int port;

    // private immutable
    private final String authKey;
    private final AtomicLong nextToken = new AtomicLong();
    private final Supplier<C> instanceMaker;

    // private mutable
    private Optional<String> dbname;
    private Optional<Integer> connectTimeout;
    private ByteBuffer handshake;
    private Optional<C> instance = Optional.empty();

    private Connection(Builder<C> builder) {
        dbname = builder.dbname;
        authKey = builder.authKey.orElse("");
        handshake = Util.leByteBuffer(4 + 4 + this.authKey.length() + 4)
            .putInt(Version.V0_4.value)
            .putInt(authKey.length())
            .put(authKey.getBytes())
            .putInt(Protocol.JSON.value);
        hostname = builder.hostname.orElse("localhost");
        port = builder.port.orElse(28015);
        connectTimeout = builder.timeout;

        instanceMaker = builder.instanceMaker;
    }

    public static Builder<ConnectionInstance> build() {
        return new Builder<>(ConnectionInstance::new);
    }

    public Optional<String> db() {
        return dbname;
    }

    void addToCache(long token, Cursor cursor){
        instance.ifPresent(i -> i.addToCache(token, cursor));
        instance.orElseThrow(() ->
                new ReqlDriverError(
                        "Can't add to cache when not connected."));
    }

    void removeFromCache(long token) {
        instance.ifPresent(i -> i.removeFromCache(token));
    }

    public void use(String db) {
        dbname = Optional.of(db);
    }

    public Optional<Integer> timeout() {
        return connectTimeout;
    }

    public Connection<C> reconnect() {
        return reconnect(false, Optional.empty());
    }

    public Connection<C> reconnect(boolean noreplyWait, Optional<Integer> timeout) {
        if(!timeout.isPresent()){
            timeout = connectTimeout;
        }
        close(noreplyWait);
        C inst = instanceMaker.get();
        instance = Optional.of(inst);
        inst.connect(hostname, port, timeout);
        return this;
    }

    public boolean isOpen() {
        return instance.map(ConnectionInstance::isOpen).orElse(false);
    }

    public C checkOpen() {
        if(instance.map(c -> !c.isOpen()).orElse(true)){
            throw new ReqlDriverError("Connection is closed.");
        } else {
            return instance.get();
        }
    }

    public void close(boolean shouldNoreplyWait) {
        instance.ifPresent(inst -> {
            try {
                if(shouldNoreplyWait) {
                    noreplyWait();
                }
            }finally {
                instance = Optional.empty();
                nextToken.set(0);
                inst.close();
            }
        });
    }

    private long newToken() {
        return nextToken.incrementAndGet();
    }

    Optional<Object> runQuery(Query query, boolean noreply) {
        ConnectionInstance inst = checkOpen();
        inst.socket
                .orElseThrow(() -> new ReqlDriverError("No socket open."))
                .sendall(query.serialize());
        if(noreply){
            return Optional.empty();
        }

        Response res = inst.readResponse(query.token);

        // TODO: This logic needs to move into the Response class
        if(res.isAtom()){
            return Optional.of(
                    Response.convertPseudotypes(res.data.get(), res.profile));
        } else if(res.isPartial() || res.isSequence()) {
            Cursor cursor = Cursor.empty(this, query);
            cursor.extend(res);
            return Optional.of(cursor);
        } else if(res.isWaitComplete()) {
            return Optional.empty();
        } else {
            throw res.makeError(query);
        }
    }

    Optional<Object> runQuery(Query query) {
        return runQuery(query, false);
    }

    void runQueryNoreply(Query query) {
        runQuery(query, true);
    }

    public void noreplyWait() {
        runQuery(Query.noreplyWait(newToken()));
    }

    Optional<Object> start(RqlAst term, GlobalOptions globalOpts) {
        C inst = checkOpen();
        if (!globalOpts.db().isPresent()) {
            dbname.ifPresent(globalOpts::db);
        }
        Query q = Query.start(newToken(), term, globalOpts);
        return runQuery(q, globalOpts.noreply().orElse(false));
    }

    void continue_(Cursor cursor) {
        runQueryNoreply(Query.continue_(cursor.token));
    }

    void stop(Cursor cursor) {
        runQueryNoreply(Query.stop(cursor.token));
    }

    public static class Builder<T extends ConnectionInstance> {
        private final Supplier<T> instanceMaker;
        private Optional<String> hostname = Optional.empty();
        private Optional<Integer> port = Optional.empty();
        private Optional<String> dbname = Optional.empty();
        private Optional<String> authKey = Optional.empty();
        private Optional<Integer> timeout = Optional.empty();

        public Builder(Supplier<T> instanceMaker) {
            this.instanceMaker = instanceMaker;
        }
        public Builder<T> hostname(String val)
            { hostname = Optional.of(val); return this; }
        public Builder<T> port(int val)
            { port     = Optional.of(val); return this; }
        public Builder<T> db(String val)
            { dbname   = Optional.of(val); return this; }
        public Builder<T> authKey(String val)
            { authKey  = Optional.of(val); return this; }
        public Builder<T> timeout(int val)
            { timeout  = Optional.of(val); return this; }

        public Connection<T> connect() {
            Connection<T> conn = new Connection<>(this);
            conn.reconnect();
            return conn;
        }
    }
}
