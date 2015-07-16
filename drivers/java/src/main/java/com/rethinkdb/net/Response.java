// A class that encapsulates the fact that a query can return an atom,
// a cursor, or an error (or nothing if noreply is requested)

import org.json.simple.*;

public class Response<C extends Cursor, E extends Error> {
    public final Optional<JSONValue> atom;
    public final Optional<C> cursor;
    public final Optional<E> error;
    public final Optional<Profile> profile;

    private Response(JSONValue atom, C cursor, E error, Optional<Profile> profile) {
        this.atom = Optional.ofNullable(atom);
        this.cursor = Optional.ofNullable(cursor);
        this.error = Optional.ofNullable(error);
        this.profile = profile;
    }

    public static Response<Cursor, Error> ofAtom(JSONValue atom, Optional<Profile> profile) {
        return new Response(atom, (Cursor) null, (Error) null, profile);
    }

    public static <C> Response<C, Error> ofCursor(C cursor, Optional<Profile> profile) {
        return new Response(null, cursor, (Error) null, profile);
    }

    public static <E> Response<Cursor, E> ofError(E error) {
        return new Response(null, (Cursor) null, error, Optional.empty());
    }

    public static Response<Cursor, Error> empty() {
        return new Response(null, (Cursor) null, (Error) null, Optional.empty());
    }
}
