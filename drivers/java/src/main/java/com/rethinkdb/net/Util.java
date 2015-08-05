package com.rethinkdb.net;

public class Util {
    public static int getTimestamp() {
        return (int)(System.nanoTime() / 1_000_000L);
    }

    public static int deadline(int timeout){
        return getTimestamp() + timeout;
    }
}
