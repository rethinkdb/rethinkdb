package com.rethinkdb.model;

import java.util.List;

public class GroupedResult<G,V> {
    public final G group;
    public final List<V> values;

    public GroupedResult(G group, List<V> values){
        this.group = group;
        this.values = values;
    }
}
