package com.rethinkdb.net;

import com.rethinkdb.gen.exc.ReqlDriverError;

import javax.net.SocketFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.locks.ReentrantLock;

public class SocketWrapper {
    final ReentrantLock mu = new ReentrantLock();

    private Socket socket = null;

    private SocketFactory socketFactory = SocketFactory.getDefault();
    private SSLSocket sslSocket = null;

    protected OutputStream writeStream = null;
    protected InputStream readStream = null;

    // options
    private Optional<InputStream> certFile = Optional.empty();
    private Optional<SSLContext> sslContext = Optional.empty();
    private Optional<Long> timeout = Optional.empty();
    private final String hostname;
    private final int port;

    public SocketWrapper(String hostname,
                         int port,
                         Optional<SSLContext> sslContext,
                         Optional<Long> timeout) {
        this.hostname = hostname;
        this.port = port;
        this.sslContext = sslContext;
        this.timeout = timeout;
    }

    public void connect(ByteBuffer handshake) throws TimeoutException {
        Optional<Long> deadline = timeout.map(Util::deadline);

        mu.lock();
        try {
            // establish connection
            final InetSocketAddress addr = new InetSocketAddress(hostname, port);
            socket = socketFactory.createSocket();
            socket.connect(addr, timeout.orElse(0L).intValue());
            socket.setTcpNoDelay(true);
            socket.setKeepAlive(true);

            // should we secure the connection?
            if (sslContext.isPresent()) {
                socketFactory = sslContext.get().getSocketFactory();
                SSLSocketFactory sslSf = (SSLSocketFactory) socketFactory;
                sslSocket = (SSLSocket) sslSf.createSocket(socket,
                        socket.getInetAddress().getHostAddress(),
                        socket.getPort(),
                        true);

                // replace input/output streams
                readStream = sslSocket.getInputStream();
                writeStream = sslSocket.getOutputStream();

                // execute SSL handshake
                sslSocket.startHandshake();
            } else {
                writeStream = socket.getOutputStream();
                readStream = socket.getInputStream();
            }
            writeStream.write(handshake.array());
            final String msg = readNullTerminatedString(deadline);
            if (!msg.equals("SUCCESS")) {
                throw new ReqlDriverError(
                        "Server dropped connection with message: \"%s\"", msg);
            }
        } catch (SocketTimeoutException ste) {
            throw new TimeoutException("Connect timed out.");
        } catch (IOException e) {
            throw new ReqlDriverError(e);
        } finally {
            mu.unlock();
        }
    }

    public void write(ByteBuffer buffer) {
        try {
            buffer.flip();
            writeStream.write(buffer.array());
        } catch (IOException e) {
            throw new ReqlDriverError(e);
        }
    }

    private String readNullTerminatedString(Optional<Long> deadline)
            throws IOException {
        byte[] buf = new byte[1];
        List<Byte> bytelist = new ArrayList<>();
        while (true) {
            int bytesRead = readStream.read(buf);
            if (bytesRead == 0) {
                continue;
            }
            if (bytesRead == -1) {
                throw new ReqlDriverError("Read -1 bytes on socket.");
            }

            deadline.ifPresent(d -> {
                if (d <= System.currentTimeMillis()) {
                    throw new ReqlDriverError("Connection timed out.");
                }
            });
            if (buf[0] == (byte) 0) {
                byte[] raw = new byte[bytelist.size()];
                for (int i = 0; i < raw.length; i++) {
                    raw[i] = bytelist.get(i);
                }
                return new String(raw, StandardCharsets.UTF_8);
            } else {
                bytelist.add(buf[0]);
            }
        }
    }

    public ByteBuffer recvall(int bufsize, Optional<Long> deadline) throws TimeoutException {
        byte[] buf = new byte[bufsize];
        int bytesRead = 0;
        while (bytesRead < bufsize) try {
            if (deadline.isPresent()) {
                Long timeout = Math.max(0L, deadline.get() - System.currentTimeMillis());
                socket.setSoTimeout(timeout.intValue());
            }
            bytesRead += readStream.read(buf);
        } catch (SocketTimeoutException ste) {
            throw new TimeoutException("Read timed out." + ste.getMessage());
        } catch (IOException ex) {
            throw new ReqlDriverError(ex);
        } finally {
            try {
                socket.setSoTimeout(0);
            } catch (SocketException se) {
                throw new ReqlDriverError(se);
            }
        }
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN);
    }

    public boolean isOpen() {
        return socket == null ? false : socket.isConnected();
    }

    public void close() {
        if (socket != null)
            try {
                socket.close();
            } catch (IOException e) {
                throw new ReqlDriverError(e);
            }
    }
}
