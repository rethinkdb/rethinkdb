package com.rethinkdb.net;

import com.rethinkdb.gen.exc.ReqlDriverError;
import com.rethinkdb.ast.Query;
import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.gen.ast.Datum;
import com.rethinkdb.gen.proto.Protocol;
import com.rethinkdb.gen.proto.Version;
import com.rethinkdb.model.OptArgs;

import java.nio.ByteBuffer;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicLong;
import java.util.function.Supplier;

public class Connection<C extends ConnectionInstance> {
    // public immutable
    public final String hostname;
    public final int port;

    private final AtomicLong nextToken = new AtomicLong();
    private final Supplier<C> instanceMaker;

    // private mutable
    private Optional<String> dbname;
    private Optional<Integer> connectTimeout;
    private ByteBuffer handshake;
    private Optional<C> instance = Optional.empty();

    private Connection(Builder<C> builder) {
        dbname = builder.dbname;
        String authKey = builder.authKey.orElse("");
        handshake = Util.leByteBuffer(4 + 4 + authKey.length() + 4)
            .putInt(Version.V0_4.value)
            .putInt(authKey.length())
            .put(authKey.getBytes())
            .putInt(Protocol.JSON.value);
        handshake.flip();
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

    public Connection<C> reconnect() throws TimeoutException {
        return reconnect(false, Optional.empty());
    }

    public Connection<C> reconnect(boolean noreplyWait, Optional<Integer> timeout) throws TimeoutException  {
        if(!timeout.isPresent()){
            timeout = connectTimeout;
        }
        close(noreplyWait);
        C inst = instanceMaker.get();
        instance = Optional.of(inst);
        inst.connect(hostname, port, handshake, timeout);
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

    Response readResponse(long token, Optional<Integer> deadline) {
        return checkOpen().readResponse(token, deadline);
    }
    void runQueryNoreply(Query query){
        ConnectionInstance inst = checkOpen();
        inst.socket
                .orElseThrow(() -> new ReqlDriverError("No socket open."))
                .write(query.serialize());
    }

    @SuppressWarnings("unchecked")
    <T> T runQuery(Query query) {
        ConnectionInstance inst = checkOpen();
        inst.socket
                .orElseThrow(() -> new ReqlDriverError("No socket open."))
                .write(query.serialize());

        Response res = inst.readResponse(query.token);
        if(res.isAtom()) {
            try {
                Converter.FormatOptions fmt =
                        new Converter.FormatOptions(query.globalOptions);
                return (T) ((List)
                        Converter.convertPseudotypes(res.data,fmt)).get(0);
            } catch (IndexOutOfBoundsException ex){
                throw new ReqlDriverError("Atom response was empty!", ex);
            }
        } else if(res.isPartial() || res.isSequence()) {
            Cursor cursor = Cursor.empty(this, query);
            cursor.extend(res);
            return (T) cursor;
        } else if(res.isWaitComplete()) {
            return null;
        } else {
            throw res.makeError(query);
        }
    }

    public void noreplyWait() {
        runQuery(Query.noreplyWait(newToken()));
    }

    private void setDefaultDB(OptArgs globalOpts){
        if (!globalOpts.containsKey("db") && dbname.isPresent()) {
            globalOpts.with("db", dbname.get());
        }
    }

    public <T> T run(ReqlAst term, OptArgs globalOpts) {
        setDefaultDB(globalOpts);
        Query q = Query.start(newToken(), term, globalOpts);
        if(globalOpts.containsKey("noreply")) {
            throw new ReqlDriverError(
                    "Don't provide the noreply option as an optarg. "+
                            "Use `.runNoReply` instead of `.run`");
        }
        return runQuery(q);
    }

    public void runNoReply(ReqlAst term, OptArgs globalOpts){
        setDefaultDB(globalOpts);
        runQueryNoreply(Query.start(newToken(), term, globalOpts));
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

        public Connection<T> connect() throws TimeoutException {
            Connection<T> conn = new Connection<>(this);
            conn.reconnect();
            return conn;
        }
    }
}
