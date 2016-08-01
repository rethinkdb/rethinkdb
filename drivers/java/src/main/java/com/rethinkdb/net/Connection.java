package com.rethinkdb.net;

import com.rethinkdb.ast.Query;
import com.rethinkdb.ast.ReqlAst;
import com.rethinkdb.gen.ast.Db;
import com.rethinkdb.gen.exc.ReqlDriverError;
import com.rethinkdb.gen.proto.Protocol;
import com.rethinkdb.gen.proto.Version;
import com.rethinkdb.model.Arguments;
import com.rethinkdb.model.OptArgs;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import javax.net.ssl.SSLContext;
import java.io.IOException;
import java.io.InputStream;
import java.io.Closeable;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.locks.ReentrantLock;

public class Connection implements Closeable {
    // logger
    private static final Logger log = LoggerFactory.getLogger(Connection.class);

    // public immutable
    public final String hostname;
    public final int port;

    private final AtomicLong nextToken = new AtomicLong();

    // private mutable
    private Optional<String> dbname;
    private Optional<Long> connectTimeout;
    private Optional<SSLContext> sslContext;
    private final Handshake handshake;

    // network stuff
    Optional<SocketWrapper> socket = Optional.empty();

    private Map<Long, Cursor> cursorCache = new ConcurrentHashMap<>();

    // execution stuff
    private ExecutorService exec;
    private final Map<Long, CompletableFuture<Response>> awaiters = new ConcurrentHashMap<>();
    private Exception awaiterException = null;
    private final ReentrantLock lock = new ReentrantLock();

    public Connection(Builder builder) {
        dbname = builder.dbname;
        if (builder.authKey.isPresent() && builder.user.isPresent()) {
            throw new ReqlDriverError("Either `authKey` or `user` can be used, but not both.");
        }
        String user = builder.user.orElse("admin");
        String password = builder.password.orElse(builder.authKey.orElse(""));
        handshake = new Handshake(user, password);
        hostname = builder.hostname.orElse("localhost");
        port = builder.port.orElse(28015);
        // is certFile provided? if so, it has precedence over SSLContext
        this.sslContext = Crypto.handleCertfile(builder.certFile, builder.sslContext);
        connectTimeout = builder.timeout;
    }

    public static Builder build() {
        return new Builder();
    }

    public Optional<String> db() {
        return dbname;
    }

    public void connect() throws TimeoutException {
        connect(Optional.empty());
    }

    public Connection reconnect() {
        try {
            return reconnect(false, Optional.empty());
        } catch (TimeoutException toe) {
            throw new RuntimeException("Timeout can't happen here.");
        }
    }

    public Connection reconnect(boolean noreplyWait, Optional<Long> timeout) throws TimeoutException {
        if (!timeout.isPresent()) {
            timeout = connectTimeout;
        }
        close(noreplyWait);
        connect(timeout);
        return this;
    }

    void connect(Optional<Long> timeout) throws TimeoutException {
        final SocketWrapper sock = new SocketWrapper(hostname, port, sslContext, timeout.isPresent() ? timeout : connectTimeout);
        sock.connect(handshake);
        socket = Optional.of(sock);

        // start response pump
        exec = Executors.newSingleThreadExecutor();
        exec.submit((Runnable) () -> {
            // pump responses until canceled
            while (true) {
                // validate socket is open
                if (!isOpen()) {
                    awaiterException = new IOException("The socket is closed, exiting response pump.");
                    this.close();
                    break;
                }

                // read response and send it to whoever is waiting, if anyone
                try {
                    final Response response = this.socket.orElseThrow(() -> new ReqlDriverError("No socket available.")).read();
                    final CompletableFuture<Response> awaiter = awaiters.remove(response.token);
                    if (awaiter != null) {
                        awaiter.complete(response);
                    }
                } catch (Exception e) {
                    awaiterException = e;
                    this.close();
                    break;
                }
            }
        });
    }

    public Optional<Integer> clientPort() {
        return socket.map(SocketWrapper::clientPort).orElse(Optional.empty());
    }

    public Optional<SocketAddress> clientAddress() {
        return socket.map(SocketWrapper::clientAddress).orElse(Optional.empty());
    }

    public boolean isOpen() {
        return socket.map(SocketWrapper::isOpen).orElse(false);
    }

    @Override
    public void close() {
        close(false);
    }

    public void close(boolean shouldNoreplyWait) {
        // disconnect
        try {
            if (shouldNoreplyWait) {
                noreplyWait();
            }
        } finally {
            // reset token
            nextToken.set(0);

            // clear cursor cache
            for (Cursor cursor : cursorCache.values()) {
                cursor.setError("Connection is closed.");
            }
            cursorCache.clear();

            // handle current awaiters
            this.awaiters.values().stream().forEach(awaiter -> {
                // what happened?
                if (this.awaiterException != null) { // an exception
                    awaiter.completeExceptionally(this.awaiterException);
                } else { // probably canceled
                    awaiter.cancel(true);
                }
            });
            awaiters.clear();

            // terminate response pump
            if (exec != null && !exec.isShutdown()) {
                exec.shutdown();
            }

            // close the socket
            socket.ifPresent(SocketWrapper::close);
        }

    }

    public void use(String db) {
        dbname = Optional.ofNullable(db);
    }

    public Optional<Long> timeout() {
        return connectTimeout;
    }

    /**
     * Writes a query and returns a completable future.
     * Said completable future value will eventually be set by the runnable response pump (see {@link #connect}).
     *
     * @param query    the query to execute.
     * @param deadline the timeout.
     * @return a completable future.
     */
    private Future<Response> sendQuery(Query query, Optional<Long> deadline) {
        // check if response pump is running
        if (!exec.isShutdown() && !exec.isTerminated()) {
            final CompletableFuture<Response> awaiter = new CompletableFuture<>();
            awaiters.put(query.token, awaiter);
            try {
                lock.lock();
                socket.orElseThrow(() -> new ReqlDriverError("No socket available."))
                        .write(query.serialize());
                return awaiter.toCompletableFuture();
            } finally {
                lock.unlock();
            }
        }

        // shouldn't be here
        throw new ReqlDriverError("Can't write query because response pump is not running.");
    }

    /**
     * Writes a query without waiting for a response
     *
     * @param query    the query to execute.
     */
    private void sendQueryNoreply(Query query) {
        // check if response pump is running
        if (!exec.isShutdown() && !exec.isTerminated()) {
            try {
                lock.lock();
                socket.orElseThrow(() -> new ReqlDriverError("No socket available."))
                        .write(query.serialize());
                return;
            } finally {
                lock.unlock();
            }
        }

        // shouldn't be here
        throw new ReqlDriverError("Can't write query because response pump is not running.");
    }


    void runQueryNoreply(Query query) {
        sendQueryNoreply(query);
    }

    <T> T runQuery(Query query) {
        return runQuery(query, Optional.empty());
    }

    <T, P> T runQuery(Query query, Optional<Class<P>> pojoClass) {
        return runQuery(query, pojoClass, Optional.empty());
    }

    /**
     * Runs a query and blocks until a response is retrieved.
     *
     * @param query
     * @param pojoClass
     * @param timeout
     * @param <T>
     * @param <P>
     * @return
     */
    <T, P> T runQuery(Query query, Optional<Class<P>> pojoClass, Optional<Long> timeout) {
        Response res = null;
        try {
            res = sendQuery(query, timeout).get();
        } catch (InterruptedException | ExecutionException e) {
            throw new ReqlDriverError(e);
        }

        if (res.isAtom()) {
            try {
                Converter.FormatOptions fmt = new Converter.FormatOptions(query.globalOptions);
                Object value = ((List) Converter.convertPseudotypes(res.data, fmt)).get(0);
                return Util.convertToPojo(value, pojoClass);
            } catch (IndexOutOfBoundsException ex) {
                throw new ReqlDriverError("Atom response was empty!", ex);
            }
        } else if (res.isPartial() || res.isSequence()) {
            Cursor cursor = Cursor.create(this, query, res, pojoClass);
            return (T) cursor;
        } else if (res.isWaitComplete()) {
            return null;
        } else {
            throw res.makeError(query);
        }
    }

    private long newToken() {
        return nextToken.incrementAndGet();
    }

    void addToCache(long token, Cursor cursor) {
        cursorCache.put(token, cursor);
    }

    void removeFromCache(long token) {
        cursorCache.remove(token);
    }

    public void noreplyWait() {
        runQuery(Query.noreplyWait(newToken()));
    }

    private void setDefaultDB(OptArgs globalOpts) {
        if (!globalOpts.containsKey("db") && dbname.isPresent()) {
            // Only override the db global arg if the user hasn't
            // specified one already and one is specified on the connection
            globalOpts.with("db", dbname.get());
        }
        if (globalOpts.containsKey("db")) {
            // The db arg must be wrapped in a db ast object
            globalOpts.with("db", new Db(Arguments.make(globalOpts.get("db"))));
        }
    }

    public <T, P> T run(ReqlAst term, OptArgs globalOpts, Optional<Class<P>> pojoClass) {
        return run(term, globalOpts, pojoClass, Optional.empty());
    }

    public <T, P> T run(ReqlAst term, OptArgs globalOpts, Optional<Class<P>> pojoClass, Optional<Long> timeout) {
        setDefaultDB(globalOpts);
        Query q = Query.start(newToken(), term, globalOpts);
        if (globalOpts.containsKey("noreply")) {
            throw new ReqlDriverError(
                    "Don't provide the noreply option as an optarg. " +
                            "Use `.runNoReply` instead of `.run`");
        }
        return runQuery(q, pojoClass, timeout);
    }

    public void runNoReply(ReqlAst term, OptArgs globalOpts) {
        setDefaultDB(globalOpts);
        globalOpts.with("noreply", true);
        runQueryNoreply(Query.start(newToken(), term, globalOpts));
    }

    Future<Response> continue_(Cursor cursor) {
        return sendQuery(Query.continue_(cursor.token), Optional.empty());
    }


    void stop(Cursor cursor) {
        // While the server does reply to the stop request, we ignore that reply.
        // This works because the response pump in `connect` ignores replies for which
        // no waiter exists.
        runQueryNoreply(Query.stop(cursor.token));
    }

    /**
     * Connection.Builder should be used to build a Connection instance.
     */
    public static class Builder implements Cloneable {
        private Optional<String> hostname = Optional.empty();
        private Optional<Integer> port = Optional.empty();
        private Optional<String> dbname = Optional.empty();
        private Optional<InputStream> certFile = Optional.empty();
        private Optional<SSLContext> sslContext = Optional.empty();
        private Optional<Long> timeout = Optional.empty();
        private Optional<String> authKey = Optional.empty();
        private Optional<String> user = Optional.empty();
        private Optional<String> password = Optional.empty();

        public Builder clone() throws CloneNotSupportedException {
            Builder c = (Builder)super.clone();
            c.hostname = hostname;
            c.port = port;
            c.dbname = dbname;
            c.certFile = certFile;
            c.sslContext = sslContext;
            c.timeout = timeout;
            c.authKey = authKey;
            c.user = user;
            c.password = password;
            return c;
        }

        public Builder hostname(String val) {
            hostname = Optional.of(val);
            return this;
        }

        public Builder port(int val) {
            port = Optional.of(val);
            return this;
        }

        public Builder db(String val) {
            dbname = Optional.of(val);
            return this;
        }

        public Builder authKey(String key) {
            authKey = Optional.of(key);
            return this;
        }

        public Builder user(String user, String password) {
            this.user = Optional.of(user);
            this.password = Optional.of(password);
            return this;
        }

        public Builder certFile(InputStream val) {
            certFile = Optional.of(val);
            return this;
        }

        public Builder sslContext(SSLContext val) {
            sslContext = Optional.of(val);
            return this;
        }

        public Builder timeout(long val) {
            timeout = Optional.of(val);
            return this;
        }

        public Connection connect() {
            final Connection conn = new Connection(this);
            conn.reconnect();
            return conn;
        }
    }
}
