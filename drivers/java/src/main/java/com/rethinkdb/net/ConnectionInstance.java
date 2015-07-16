import java.util.*;

import com.rethinkdb.proto.QueryType;
import com.rethinkdb.proto.ResponseType;
import com.rethinkdb.ast.Query;
import com.rethinkdb.ast.Profile;


public class ConnectionInstance<C extends Connection> {

    private Optional<C> parent = Optional.empty();
    private HashMap<Long, Cursor> cursorCache = new HashMap<>();
    private Optional<SocketWrapper> socket = Optional.empty();
    private boolean closing = false;
    // TODO: when adding async, _header_in_progress may be needed

    public ConnectionInstance(C parent) {
        this.parent = Optional.ofNullable(parent);
    }

    public C connect(int timeout) {
        socket = Optional.of(new SocketWrapper(this, timeout));
        return parent;
    }

    public isOpen() {
        return socket.map(s -> s.isOpen()).orElse(false);
    }

    public void close(boolean noreplyWait, long token) {
        closing = true;
        for(Cursor cursor: cursorCache.values()) {
            cursor.error("Connection is closed.");
        }
        cursorCache.clear();
        try {
            if(noreplyWait) {
                Query noreplyQuery = new Query(QueryType.NOREPLY_WAIT, token);
                self.runQuery(noreplyQuery, false);
            }
        } finally {
            socket.close();
        }
    }

    public Response runQuery(Query query, boolean noreply) {
        socket.sendall(query.serialize());
        if(noreply){
            return Response.empty();
        }

        SocketResponse res = readResponse(query.token);

        // TODO: This logic needs to move into the Response class
        if(res.isAtom()){
            return Response.ofAtom(
                convertPseudotypes(res.data[0], query),
                res.profile);
        } else if(res.isPartial() || res.isSequence()) {
            Cursor cursor = new DefaultCursor(this, query);
            cursor.extend(res);
            return Response.ofCursor(cursor, res.profile);
        } else if(res.isWaitComplete()) {
            return Response.empty();
        } else {
            return Response.ofError(res.makeError(query))
        }
    }

    private Response readResponse(long token, deadline) {
        while(true) {

        }
    }
}
