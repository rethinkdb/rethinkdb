package com.rethinkdb;

public class RethinkDBException extends RuntimeException {
    public RethinkDBException() {
    }

    public RethinkDBException(String message) {
        super(message);
    }

    public RethinkDBException(String message, Throwable cause) {
        super(message, cause);
    }

    public RethinkDBException(Throwable cause) {
        super(cause);
    }
}
