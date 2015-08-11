package com.rethinkdb.model;

import org.json.simple.JSONArray;

import java.util.Optional;

public class Profile {

    public static Optional<Profile> fromJSONArray(JSONArray profileObj) {
        if(profileObj == null){
            return Optional.empty();
        }
        throw new RuntimeException("fromJSONArray not implemented");
    }
}
