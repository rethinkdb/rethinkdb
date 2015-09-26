package com.rethinkdb.model;

import java.util.HashMap;

public class MapObject extends HashMap {

    public MapObject() {
    }

    public MapObject with(Object key, Object value) {
        put(key, value);
        return this;
    }
}
