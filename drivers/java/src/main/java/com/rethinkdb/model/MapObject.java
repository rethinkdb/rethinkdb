package com.rethinkdb.model;

import java.util.HashMap;

public class MapObject extends HashMap<String, Object> {

    public MapObject() {
    }

    public MapObject with(String key, Object value) {
        put(key, value);
        return this;
    }
}
