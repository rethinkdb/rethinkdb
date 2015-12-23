package com.rethinkdb.net;


import com.rethinkdb.gen.ast.Datum;
import com.rethinkdb.gen.exc.ReqlDriverError;
import com.rethinkdb.model.GroupedResult;
import com.rethinkdb.model.MapObject;
import com.rethinkdb.model.OptArgs;

import java.time.Instant;
import java.time.OffsetDateTime;
import java.time.ZoneOffset;
import java.util.Base64;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

public class Converter {

    private static final Base64.Decoder b64decoder = Base64.getMimeDecoder();
    private static final Base64.Encoder b64encoder = Base64.getMimeEncoder();


    public static final String PSEUDOTYPE_KEY = "$reql_type$";

    public static final String TIME = "TIME";
    public static final String GROUPED_DATA = "GROUPED_DATA";
    public static final String GEOMETRY = "GEOMETRY";
    public static final String BINARY = "BINARY";

    /* Compact way of keeping these flags around through multiple recursive
    passes */
    public static class FormatOptions{
        public final boolean rawTime;
        public final boolean rawGroups;
        public final boolean rawBinary;

        public FormatOptions(OptArgs args){
            this.rawTime = ((Datum)args.getOrDefault("time_format",
                    new Datum("native"))).datum.equals("raw");
            this.rawBinary = ((Datum)args.getOrDefault("binary_format",
                    new Datum("native"))).datum.equals("raw");
            this.rawGroups = ((Datum)args.getOrDefault("group_format",
                    new Datum("native"))).datum.equals("raw");
        }
    }

    @SuppressWarnings("unchecked")
    public static Object convertPseudotypes(Object obj, FormatOptions fmt){
        if(obj instanceof List) {
            return ((List) obj).stream()
                    .map(item -> convertPseudotypes(item, fmt))
                    .collect(Collectors.toList());
        } else if(obj instanceof Map) {
            Map<String, Object> mapobj = (Map) obj;
            if(mapobj.containsKey(PSEUDOTYPE_KEY)){
                return convertPseudo(mapobj, fmt);
            }
            return mapobj.entrySet().stream()
                    .collect(
                            HashMap::new,
                            (map, entry) -> map.put(
                                    entry.getKey(),
                                    convertPseudotypes(entry.getValue(), fmt)
                            ),
                            HashMap<String, Object>::putAll
                    );
        } else {
            return obj;
        }
    }

    public static Object convertPseudo(Map<String, Object> value, FormatOptions fmt) {
        if(value == null){
            return null;
        }
        String reqlType = (String) value.get(PSEUDOTYPE_KEY);
        switch (reqlType) {
            case TIME:
                return fmt.rawTime ? value : getTime(value);
            case GROUPED_DATA:
                return fmt.rawGroups ? value : getGrouped(value);
            case BINARY:
                return fmt.rawBinary ? value : getBinary(value);
            case GEOMETRY:
                // Nothing specific here
                return value;
            default:
                // Just leave unknown pseudo-types alone
                return value;
        }
    }

    @SuppressWarnings("unchecked")
    private static List<GroupedResult> getGrouped(Map<String, Object> value) {
        return ((List<List<Object>>) value.get("data")).stream()
                .map(g -> new GroupedResult(g.remove(0), g))
                .collect(Collectors.toList());
    }

    private static OffsetDateTime getTime(Map<String, Object> obj) {
        try {
            ZoneOffset offset = ZoneOffset.of((String) obj.get("timezone"));
            Double epochTime = ((Number) obj.get("epoch_time")).doubleValue();
            Instant timeInstant = Instant.ofEpochMilli(((Double) (epochTime * 1000.0)).longValue());
            return OffsetDateTime.ofInstant(timeInstant, offset);
        } catch (Exception ex) {
            throw new ReqlDriverError("Error handling date", ex);
        }
    }

    @SuppressWarnings("unchecked")
    private static byte[] getBinary(Map<String, Object> value) {
        String str = (String) value.get("data");
        return b64decoder.decode(str);
    }

    public static Map<String,Object> toBinary(byte[] data){
        MapObject mob = new MapObject();
        mob.with("$reql_type$", BINARY);
        mob.with("data", b64encoder.encodeToString(data));
        return mob;
    }
}
