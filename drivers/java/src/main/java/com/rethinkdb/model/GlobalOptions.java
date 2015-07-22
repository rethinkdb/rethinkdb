package com.rethinkdb.model;

import java.util.*;

public class GlobalOptions {
    private GlobalOptions() {
    }

    public static GlobalOptions default_() {
        return new GlobalOptions();
    }

    public Map<String, Object> toMap() {
        throw new RuntimeException("toMap not implemented yet");
    }
}
