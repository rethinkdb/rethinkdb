package com.rethinkdb.net;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class Util {
    public static int getTimestamp() {
        return (int)(System.nanoTime() / 1_000_000L);
    }

    public static int deadline(int timeout){
        return getTimestamp() + timeout;
    }

    public static ByteBuffer leByteBuffer(int capacity) {
        return ByteBuffer.allocate(capacity)
                .order(ByteOrder.LITTLE_ENDIAN);
    }

}
