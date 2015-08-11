package com.rethinkdb.net;

import com.rethinkdb.ReqlDriverError;
import com.rethinkdb.ReqlError;
import com.rethinkdb.ReqlRuntimeError;
import com.rethinkdb.response.Response;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.SocketTimeoutException;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;
import java.net.StandardSocketOptions;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.concurrent.TimeoutException;

public class SocketWrapper {
    private SocketChannel socketChannel;
    private Optional<Integer> timeout = Optional.empty();
    private Optional<ByteBuffer> readBuffer = Optional.empty();

    public final String hostname;
    public final int port;

    public SocketWrapper(String hostname, int port,
                         Optional<Integer> timeout) {
        this.hostname = hostname;
        this.port = port;
        this.timeout = timeout;
        try {
            this.socketChannel = SocketChannel.open();
        } catch (IOException e) {
            throw new ReqlRuntimeError(e);
        }
    }

    public void connect(ByteBuffer handshake) throws TimeoutException {
        Optional<Integer> deadline = timeout.map(Util::deadline);
        try {
            socketChannel.configureBlocking(true);
            socketChannel.setOption(StandardSocketOptions.TCP_NODELAY, true);
            socketChannel.socket().connect(
                    new InetSocketAddress(hostname, port),
                    timeout.orElse(0));
            socketChannel.write(handshake);
            String msg = readNullTerminatedString(deadline);
            if (!msg.equals("SUCCESS")) {
                throw new ReqlDriverError(
                        "Server dropped connection with message: \"%s\"", msg);
            }
        } catch (SocketTimeoutException ste) {
            throw new TimeoutException("Connect timed out.");
        } catch (IOException e) {
            throw new ReqlRuntimeError(e);
        }
    }

    public void write(ByteBuffer buffer) {
        try {
            buffer.flip();
            while (buffer.hasRemaining()) {
                socketChannel.write(buffer);
            }
        } catch (IOException e) {
            throw new ReqlError(e);
        }
    }

    private String readNullTerminatedString(Optional<Integer> deadline)
            throws IOException {
        ByteBuffer byteBuf = Util.leByteBuffer(1);
        List<Byte> bytelist = new ArrayList<>();
        while(true) {
            int bytesRead = socketChannel.read(byteBuf);
            if(bytesRead == 0){
                continue;
            }
            // Maybe we read -1? Throw an error

            deadline.ifPresent(d -> {
                if(d <= Util.getTimestamp()) {
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

    private ByteBuffer readBytes(int i, boolean strict) {
        ByteBuffer buffer = Util.leByteBuffer(i);
        try {
            int totalRead = 0;
            int read = socketChannel.read(buffer);
            totalRead += read;

            while (strict && read != 0 && read != i) {
                read = socketChannel.read(buffer);
                totalRead+=read;
            }

            if (totalRead != i && strict) {
                throw new ReqlError("Error receiving data, expected " + i + " bytes but received " + totalRead);
            }

            buffer.flip();
            return buffer;
        } catch (IOException e) {
            throw new ReqlError(e);
        }
    }

    public void writeLEInt(int i) {
        ByteBuffer buffer = Util.leByteBuffer(4);
        buffer.putInt(i);
        write(buffer);
    }

    public void writeStringWithLength(String s) {
        writeLEInt(s.length());

        ByteBuffer buffer = Util.leByteBuffer(s.length());
        buffer.put(s.getBytes());
        write(buffer);
    }

    public void write(byte[] bytes) {
        writeLEInt(bytes.length);
        write(ByteBuffer.wrap(bytes));
    }

    public ByteBuffer recvall(int bufsize) {
        return recvall(bufsize, Optional.empty());
    }

    public ByteBuffer recvall(int bufsize, Optional<Integer> deadline) {
        // TODO: make deadline work
        ByteBuffer buf = Util.leByteBuffer(bufsize);
        try {
            int bytesRead = socketChannel.read(buf);
            if (bytesRead != bufsize) {
                do {
                    bytesRead += socketChannel.read(buf);
                } while(bytesRead < bufsize);
            }
        } catch(IOException ex) {
            throw new ReqlDriverError(ex);
        }
        buf.flip();
        return buf;
    }

    public Response read() {
        long token = recvall(8).getLong();
        int responseLength = recvall(4).getInt();
        ByteBuffer responseBytes = recvall(responseLength);
        return Response.parseFrom(token, responseBytes);
    }

    public boolean isClosed(){
        return !socketChannel.isOpen();
    }

    public boolean isOpen() {
        return socketChannel.isOpen();
    }

    public void close() {
        try {
            socketChannel.close();
        } catch (IOException e) {
            throw new ReqlError(e);
        }
    }
}
