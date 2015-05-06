package com.rethinkdb;

import com.rethinkdb.response.Response;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.SocketChannel;

public class SocketChannelFacade {
    private SocketChannel socketChannel;

    public void connect(String hostname, int port) {
        try {
            socketChannel = SocketChannel.open();
            socketChannel.configureBlocking(true);
            socketChannel.connect(new InetSocketAddress(hostname, port));
        } catch (IOException e) {
            throw new RethinkDBException(e);
        }
    }

    private void _write(ByteBuffer buffer) {
        try {
            buffer.flip();
            while (buffer.hasRemaining()) {
                socketChannel.write(buffer);
            }
        } catch (IOException e) {
            throw new RethinkDBException(e);
        }
    }

    private ByteBuffer _read(int i, boolean strict) {
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
        _write(buffer);
    }

    public void writeStringWithLength(String s) {
        writeLEInt(s.length());

        ByteBuffer buffer = ByteBuffer.allocate(s.length());
        buffer.put(s.getBytes());
        _write(buffer);
    }

    public String readString() {
        return new String(_read(5000, false).array());
    }

    public void write(byte[] bytes) {
        writeLEInt(bytes.length);

        ByteBuffer buffer = ByteBuffer.allocate(bytes.length);
        buffer.put(bytes);

        _write(buffer);
    }

    public Response read() {
        try {
            ByteBuffer datalen = ByteBuffer.allocate(4);
            datalen.order(ByteOrder.LITTLE_ENDIAN);
            int bytesRead = socketChannel.read(datalen);
            if (bytesRead != 4) {
                throw new RethinkDBException("Error receiving data, expected 4 bytes but received " + bytesRead);
            }
            datalen.flip();
            int len = datalen.getInt();

            ByteBuffer buf = ByteBuffer.allocate(len);
            bytesRead = 0;
            while (bytesRead != len) {
                bytesRead += socketChannel.read(buf);
            }
            buf.flip();
            return Response.parseFrom(buf.array());
        }
        catch (IOException ex) {
            throw new RethinkDBException("IO Exception ",ex);
        }
    }

    public boolean isClosed(){
        return !socketChannel.isOpen();
    }

    public void close() {
        try {
            socketChannel.close();
        } catch (IOException e) {
            throw new RethinkDBException(e);
        }
    }
}
