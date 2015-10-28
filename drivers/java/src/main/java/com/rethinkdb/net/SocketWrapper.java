package com.rethinkdb.net;

import com.rethinkdb.gen.exc.ReqlDriverError;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;
import java.net.StandardSocketOptions;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.concurrent.TimeoutException;

public class SocketWrapper {
    private SocketChannel socketChannel;
    private Optional<Long> timeout = Optional.empty();

    public final String hostname;
    public final int port;

    public SocketWrapper(String hostname, int port,
                         Optional<Long> timeout) {
        this.hostname = hostname;
        this.port = port;
        this.timeout = timeout;
        try {
            this.socketChannel = SocketChannel.open();
        } catch (IOException e) {
            throw new ReqlDriverError(e);
        }
    }

    public void connect(ByteBuffer handshake) throws TimeoutException {
        Optional<Long> deadline = timeout.map(Util::deadline);
        try {
            socketChannel.configureBlocking(true);
            socketChannel.setOption(StandardSocketOptions.TCP_NODELAY, true);
            socketChannel.setOption(StandardSocketOptions.SO_KEEPALIVE, true);
            socketChannel.socket().connect(
                    new InetSocketAddress(hostname, port),
                    timeout.orElse(0L).intValue());
            socketChannel.write(handshake);
            String msg = readNullTerminatedString(deadline);
            if (!msg.equals("SUCCESS")) {
                throw new ReqlDriverError(
                        "Server dropped connection with message: \"%s\"", msg);
            }
        } catch (SocketTimeoutException ste) {
            throw new TimeoutException("Connect timed out.");
        } catch (IOException e) {
            throw new ReqlDriverError(e);
        }
    }

    public void write(ByteBuffer buffer) {
        try {
            buffer.flip();
            while (buffer.hasRemaining()) {
                socketChannel.write(buffer);
            }
        } catch (IOException e) {
            throw new ReqlDriverError(e);
        }
    }

    private String readNullTerminatedString(Optional<Long> deadline)
            throws IOException {
        ByteBuffer byteBuf = Util.leByteBuffer(1);
        List<Byte> bytelist = new ArrayList<>();
        while(true) {
            int bytesRead = socketChannel.read(byteBuf);
            if(bytesRead == 0){
                continue;
            }
            if(bytesRead == -1) {
                throw new ReqlDriverError("Read -1 bytes on socket.");
            }

            deadline.ifPresent(d -> {
                if(d <= System.currentTimeMillis()) {
                    throw new ReqlDriverError("Connection timed out.");
                }
            });
            if(byteBuf.get(0) == (byte)0) {
                byte[] raw = new byte[bytelist.size()];
                for(int i = 0; i < raw.length; i++) {
                    raw[i] = bytelist.get(i);
                }
                return new String(raw, StandardCharsets.UTF_8);
            } else {
                bytelist.add(byteBuf.get(0));
            }
            byteBuf.flip();
        }
    }

    public ByteBuffer recvall(int bufsize, Optional<Long> deadline) throws TimeoutException {
        ByteBuffer buf = Util.leByteBuffer(bufsize);
        int bytesRead = 0;
        while(bytesRead < bufsize) try {
            if (deadline.isPresent()) {
                long timeout = Math.max(0L, deadline.get() - System.currentTimeMillis());
                socketChannel.socket().setSoTimeout((int) timeout);
            }
            bytesRead += socketChannel.read(buf);
        } catch (SocketTimeoutException ste) {
            throw new TimeoutException("Read timed out." + ste.getMessage());
        } catch (IOException ex) {
            throw new ReqlDriverError(ex);
        } finally {
            try {
                socketChannel.socket().setSoTimeout(0);
            }catch (SocketException se){
                throw new ReqlDriverError(se);
            }
        }
        buf.flip();
        return buf;
    }

    public boolean isOpen() {
        return socketChannel.isOpen();
    }

    public void close() {
        try {
            socketChannel.close();
        } catch (IOException e) {
            throw new ReqlDriverError(e);
        }
    }
}
