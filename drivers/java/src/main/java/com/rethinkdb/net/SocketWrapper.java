package com.rethinkdb.net;

import com.rethinkdb.gen.exc.ReqlDriverError;

import javax.net.SocketFactory;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Optional;

public class SocketWrapper {
    // networking stuff
    private Socket socket = null;
    private SocketFactory socketFactory = SocketFactory.getDefault();
    private SSLSocket sslSocket = null;
    private OutputStream writeStream = null;
    private DataInputStream readStream = null;

    // options
    private Optional<SSLContext> sslContext = Optional.empty();
    private Optional<Long> timeout = Optional.empty();
    private final String hostname;
    private final int port;

    SocketWrapper(String hostname,
                  int port,
                  Optional<SSLContext> sslContext,
                  Optional<Long> timeout) {
        this.hostname = hostname;
        this.port = port;
        this.sslContext = sslContext;
        this.timeout = timeout;
    }

    /**
     * @param handshake
     */
    void connect(Handshake handshake) {
        final Optional<Long> deadline = timeout.map(Util::deadline);
        try {
            handshake.reset();
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
                readStream = new DataInputStream(sslSocket.getInputStream());
                writeStream = sslSocket.getOutputStream();

                // execute SSL handshake
                sslSocket.startHandshake();
            } else {
                writeStream = socket.getOutputStream();
                readStream = new DataInputStream(socket.getInputStream());
            }

            // execute RethinkDB handshake

            // initialize handshake
            Optional<ByteBuffer> toWrite = handshake.nextMessage(null);
            // Sit in the handshake until it's completed. Exceptions will be thrown if
            // anything goes wrong.
            while(!handshake.isFinished()) {
                if (toWrite.isPresent()) {
                    write(toWrite.get());
                }
                String serverMsg = readNullTerminatedString(deadline);
                toWrite = handshake.nextMessage(serverMsg);
            }
        } catch (IOException e) {
            throw new ReqlDriverError("Connection timed out.", e);
        }
    }

    void write(ByteBuffer buffer) {
        try {
            buffer.flip();
            writeStream.write(buffer.array());
        } catch (IOException e) {
            throw new ReqlDriverError(e);
        }
    }

    /**
     * Tries to read a null-terminated string from the socket. This operation may timeout if a timeout is specified.
     *
     * @param deadline an optional timeout.
     * @return a string.
     * @throws IOException
     */
    private String readNullTerminatedString(Optional<Long> deadline)
            throws IOException {
        final StringBuilder sb = new StringBuilder();
        char c;
        // set deadline instant
        final Optional<Long> deadlineInstant = deadline.isPresent() ? Optional.of(System.currentTimeMillis() + deadline.get()) : Optional.empty();
        while ((c = (char) this.readStream.readByte()) != '\0') {
            // is there a deadline?
            if (deadlineInstant.isPresent()) {
                // have we timed-out?
                if (deadlineInstant.get() < System.currentTimeMillis()) { // reached time-out
                    throw new ReqlDriverError("Connection timed out.");
                }
            }
            sb.append(c);
        }

        return sb.toString();
    }

    /**
     * Tries to read a {@link Response} from the socket. This operation is blocking.
     *
     * @return a {@link Response}.
     * @throws IOException
     */
    Response read() throws IOException {
        final ByteBuffer header = readBytesToBuffer(12);
        final long token = header.getLong();
        final int responseLength = header.getInt();
        return Response.parseFrom(token, readBytesToBuffer(responseLength).order(ByteOrder.LITTLE_ENDIAN));
    }

    private ByteBuffer readBytesToBuffer(int bufsize) throws IOException {
        byte[] buf = new byte[bufsize];
        int bytesRead = 0;
        while (bytesRead < bufsize) {
            final int res = this.readStream.read(buf, bytesRead, bufsize - bytesRead);
            if (res == -1) {
                throw new ReqlDriverError("Reached the end of the read stream.");
            } else {
                bytesRead += res;
            }
        }
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN);
    }

    public Optional<Integer> clientPort() {
        Optional<Integer> ret;
        if (socket != null) {
            ret = Optional.ofNullable(socket.getLocalPort());
        } else {
            ret = Optional.empty();
        }
        return ret;
    }

    public Optional<SocketAddress> clientAddress() {
        return Optional.ofNullable(socket.getLocalSocketAddress());
    }
    /**
     * Tells whether we have a working connection or not.
     *
     * @return true if connection is connected and open, false otherwise.
     */
    boolean isOpen() {
        return socket == null ? false : socket.isConnected() && !socket.isClosed();
    }

    /**
     * Close connection.
     */
    void close() {
        // if needed, disconnect from server
        if (socket != null && isOpen())
            try {
                socket.close();
            } catch (IOException e) {
                throw new ReqlDriverError(e);
            }
    }
}
