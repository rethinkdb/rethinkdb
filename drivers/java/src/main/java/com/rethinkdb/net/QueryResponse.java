// A class that encapsulates the fact that a query can return an atom,
// a cursor, or nothing if noreply was requested

import org.json.simple.*;

public class QueryResponse<C extends Cursor> {
    public final Optional<JSONValue> atom;
    public final Optional<C> cursor;
    public final Optional<Profile> profile;

    private Response(JSONValue atom, C cursor, E error, Optional<Profile> profile) {
        this.atom = Optional.ofNullable(atom);
        this.cursor = Optional.ofNullable(cursor);
        this.profile = profile;
    }

    public static Response<Cursor> ofAtom(JSONValue atom, Optional<Profile> profile) {
        return new Response(atom, (Cursor) null, profile);
    }

    public static <C> Response<C> ofCursor(C cursor, Optional<Profile> profile) {
        return new Response(null, cursor, profile);
    }

    public static Response<Cursor> empty() {
        return new Response(null, (Cursor) null, Optional.empty());
    }
}
