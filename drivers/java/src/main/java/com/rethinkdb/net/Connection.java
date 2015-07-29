package com.rethinkdb.net;

import com.rethinkdb.proto.TermType;
import com.rethinkdb.proto.Version;
import com.rethinkdb.proto.QueryType;
import com.rethinkdb.proto.Protocol;
import com.rethinkdb.ast.Query;
import com.rethinkdb.ast.helper.OptArgs;
import com.rethinkdb.model.GlobalOptions;
import com.rethinkdb.response.Response;
import com.rethinkdb.response.DBResultFactory;
import com.rethinkdb.ast.RqlAst;
import com.rethinkdb.ast.Util;
import com.rethinkdb.ast.gen.Db;
import com.rethinkdb.ReqlDriverError;
import com.rethinkdb.RethinkDBConstants;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.util.concurrent.atomic.AtomicLong;
import java.nio.ByteBuffer;
import java.util.*;
import java.util.function.Supplier;
import java.lang.InstantiationException;
import java.lang.reflect.InvocationTargetException;

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
        this.dbname = builder.dbname;
        this.authKey = builder.authKey.orElse("");
        this.handshake = Util.leByteBuffer(4 + 4 + this.authKey.length() + 4)
            .putInt(Version.V0_4.value)
            .putInt(this.authKey.length())
            .put(this.authKey.getBytes())
            .putInt(Protocol.JSON.value);
        this.hostname = builder.hostname.orElse("localhost");
        this.port = builder.port.orElse(28015);
        this.connectTimeout = builder.timeout;

        this.instanceMaker = builder.instanceMaker;
    }

    public static Builder<ConnectionInstance> build() {
        return new Builder(ConnectionInstance::new);
    }

    public Optional<String> db() {
        return dbname;
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
        return instance.map(c -> c.isOpen()).orElse(false);
    }

    public C checkOpen() {
        if(instance.map(c -> !c.isOpen()).orElse(true)){
            throw new ReqlDriverError("Connection is closed.");
        } else {
            return instance.get();
        }
    }

    public void close(boolean noreplyWait) {
        instance.ifPresent(inst -> {
            long noreplyToken = newToken();
            instance = Optional.empty();
            nextToken.set(0);
            inst.close(noreplyWait, noreplyToken);
        });
    }

    private long newToken() {
        return nextToken.incrementAndGet();
    }

    void noreplyWait() {
        checkOpen().runQuery(Query.noreplyWait(newToken()));
    }

    Optional<Object> start(RqlAst term, GlobalOptions globalOpts) {
        C inst = checkOpen();
        if(!globalOpts.db().isPresent()){
            dbname.ifPresent(db -> {
              globalOpts.db(db);
            });
        }
        Query q = Query.start(newToken(), term, globalOpts);
        return inst.runQuery(q, globalOpts.noreply().orElse(false));
    }

    void continue_(Cursor cursor) {
        checkOpen().runQueryNoreply(Query.continue_(cursor.token));
    }

    void stop(Cursor cursor) {
        checkOpen().runQueryNoreply(Query.stop(cursor.token));
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
