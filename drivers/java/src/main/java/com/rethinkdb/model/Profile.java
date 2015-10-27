package com.rethinkdb.model;

import org.json.simple.JSONArray;

import java.util.Optional;

public class Profile {

    private JSONArray profileObj;

    private Profile(JSONArray profileObj){
        this.profileObj = profileObj;
    }

    public static Optional<Profile> fromJSONArray(JSONArray profileObj) {
        if(profileObj == null || profileObj.size() == 0){
            return Optional.empty();
        } else {
            return Optional.of(new Profile(profileObj));
        }
    }

    public JSONArray getProfileObj(){
        return profileObj;
    }
}
