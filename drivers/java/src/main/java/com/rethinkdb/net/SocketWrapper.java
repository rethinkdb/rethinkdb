package com.rethinkdb.net;

import com.rethinkdb.response.Response;
import com.rethinkdb.RethinkDBException;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.SocketChannel;
import java.net.StandardSocketOptions;
import java.net.SocketException;
import java.util.*;

public class SocketWrapper {
    private SocketChannel socketChannel;
    private Optional<Integer> timeout = Optional.empty();
    private Optional<ByteBuffer> readBuffer = Optional.empty();

    public final String hostname;
    public final int port;

    public SocketWrapper(String hostname, int port, Optional<Integer> timeout) {
        this.hostname = hostname;
        this.port = port;
        this.timeout = timeout;
        try {
            socketChannel = SocketChannel.open();
            socketChannel.configureBlocking(true);
            socketChannel.setOption(StandardSocketOptions.TCP_NODELAY, true);
            socketChannel.connect(new InetSocketAddress(hostname, port));
        } catch (IOException e) {
            throw new RethinkDBException(e);
        }
    }

    public void setTimeout(int timeout) {
        try {
            socketChannel.socket().setSoTimeout(timeout);
        } catch (SocketException se) {
            throw new RethinkDBException(se);
        }
    }

    public void sendall(ByteBuffer buf) {
        throw new RuntimeException("sendall not implemented");
    }

    public void write(ByteBuffer buffer) {
        try {
            buffer.flip();
            while (buffer.hasRemaining()) {
                socketChannel.write(buffer);
            }
        } catch (IOException e) {
            throw new RethinkDBException(e);
        }
    }

    private ByteBuffer readBytes(int i, boolean strict) {
        ByteBuffer buffer = ByteBuffer.allocate(i);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        try {
            int totalRead = 0;
            int read = socketChannel.read(buffer);
            totalRead += read;

            while (strict && read != 0 && read != i) {
                read = socketChannel.read(buffer);
                totalRead+=read;
            }

            if (totalRead != i && strict) {
                throw new RethinkDBException("Error receiving data, expected " + i + " bytes but received " + totalRead);
            }

            buffer.flip();
            return buffer;
        } catch (IOException e) {
            throw new RethinkDBException(e);
        }
    }

    public void writeLEInt(int i) {
        ByteBuffer buffer = ByteBuffer.allocate(4);
        buffer.order(ByteOrder.LITTLE_ENDIAN);
        buffer.putInt(i);
        write(buffer);
    }

    public void writeStringWithLength(String s) {
        writeLEInt(s.length());

        ByteBuffer buffer = ByteBuffer.allocate(s.length());
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
        ByteBuffer buf = ByteBuffer.allocate(bufsize);
        buf.order(ByteOrder.LITTLE_ENDIAN);
        try {
            int bytesRead = socketChannel.read(buf);
            if (bytesRead != bufsize) {
                do {
                    bytesRead += socketChannel.read(buf);
                } while(bytesRead < bufsize);
            }
        } catch(IOException ex) {
            // TODO: Throw the correct exception here
            throw new RethinkDBException("IO Exception ", ex);
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
            throw new RethinkDBException(e);
        }
    }
}
