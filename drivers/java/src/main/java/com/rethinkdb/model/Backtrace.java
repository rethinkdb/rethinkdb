package com.rethinkdb.model;

import org.json.simple.JSONArray;

import java.util.Optional;

public class Backtrace {

    private JSONArray rawBacktrace;

    private Backtrace(JSONArray rawBacktrace){
        this.rawBacktrace = rawBacktrace;
    }

    public static Optional<Backtrace> fromJSONArray(JSONArray rawBacktrace) {
        if(rawBacktrace == null || rawBacktrace.size() == 0){
            return Optional.empty();
        }else{
            return Optional.of(new Backtrace(rawBacktrace));
        }
    }

    public JSONArray getRawBacktrace(){
        return rawBacktrace;
    }
}
