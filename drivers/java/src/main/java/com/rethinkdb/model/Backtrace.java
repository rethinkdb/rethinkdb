package com.rethinkdb.model;

import org.json.simple.JSONArray;

import java.util.Optional;

public class Backtrace {
    public static Optional<Backtrace> fromJSONArray(JSONArray object) {
        if(object == null || object.size() == 0){
            return Optional.empty();
        }
        throw new RuntimeException("fromJSONArray not implemented");
    }
}
