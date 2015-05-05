package com.rethinkdb.ast.helper;

import java.util.HashMap;
import java.util.List;

public class OptArgs extends HashMap<String, Object> {

    public OptArgs() {
        super();
    }

    public OptArgs with(String key, Object value) {
        if (value != null) {
            put(key, value);
        }
        return this;
    }

    public OptArgs with(String key, List<Object> value) {
        if (value != null) {
            put(key, value);
        }
        return this;
    }

}
