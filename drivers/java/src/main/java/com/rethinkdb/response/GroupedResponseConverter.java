package com.rethinkdb.response;

import com.rethinkdb.RethinkDBException;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class GroupedResponseConverter {
    public static <K,V> Map<K, V>  convert(Map<String, Object> map) {
        if (!"GROUPED_DATA".equals(map.get("$reql_type$"))) {
            throw new RethinkDBException("Expected grouped data but found something else");
        }

        Map<K, V> result = new HashMap<K, V>();
        List<List> data = (List<List>) map.get("data");
        for (List group : data) {
            result.put((K)group.get(0), (V)group.get(1));
        }
        return result;
    }
}
