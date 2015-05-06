package com.rethinkdb.proto;

/* Class for creating a request to the server using the builder pattern */
public class QueryBuilder {
    private QueryBuilder(){}

    public byte[] toByteArray() {
        throw new RuntimeException("toByteArray not implemented");
    }
}
