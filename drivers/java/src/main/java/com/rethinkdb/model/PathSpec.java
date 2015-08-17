package com.rethinkdb.model;

import org.json.simple.JSONObject;

import java.util.ArrayDeque;

/*
        r.path("contact", "phone", "work");
        r.path("contact", "phone", r.paths("work", "home"));
        r.path("contact",
            r.paths(
                r.path("phone",
                    r.paths("work", "home")),
            r.paths(
                r.path("im", "skype")),
        );

*/
public class PathSpec {

    private final JSONObject root = new JSONObject();

    private PathSpec() {}

    public PathSpec path(String start, PathSpec... specs){
        throw new RuntimeException("not implemented");
    }

    public PathSpec path(String... strings){
        throw new RuntimeException("not implemented");
    }
}
