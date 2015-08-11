package com.rethinkdb.response;

import com.rethinkdb.ReqlDriverError;
import com.rethinkdb.ReqlError;
import com.rethinkdb.ast.Query;
import com.rethinkdb.net.Util;
import com.rethinkdb.proto.ResponseNote;
import com.rethinkdb.proto.ResponseType;
import org.json.simple.JSONArray;
import org.json.simple.JSONObject;
import org.json.simple.JSONValue;
import org.json.simple.parser.ParseException;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.Optional;
import java.util.stream.Collectors;


public class Response {
    public final long token;
    public final ResponseType type;
    public final ArrayList<ResponseNote> notes;
    public final JSONArray data;
    public final Optional<Profile> profile;
    public final Optional<Backtrace> backtrace;

    public static Response parseFrom(long token, ByteBuffer buf) {
        System.out.println("Buffer: " + Util.bufferToString(buf)); //RSI
        InputStreamReader codepointReader =
            new InputStreamReader(new ByteArrayInputStream(buf.array()));
        JSONObject jsonResp;
        try {
            jsonResp = (JSONObject)
                    JSONValue.parseWithException(codepointReader);
        } catch (ParseException | IOException ex) {
            throw new ReqlDriverError("Couldn't parse server response", ex);
        }
        ResponseType responseType = ResponseType.fromValue(
                ((Long)jsonResp.get("t")).intValue()
        );
        ArrayList<Integer> responseNoteVals = (ArrayList<Integer>) jsonResp
            .getOrDefault("n", new ArrayList<>());
        ArrayList<ResponseNote> responseNotes = responseNoteVals
            .stream()
            .map(ResponseNote::fromValue)
            .collect(Collectors.toCollection(ArrayList::new));
        JSONArray data = (JSONArray) jsonResp.getOrDefault("r", new ArrayList<>());
        return new Response(token, responseType, data, responseNotes,
                            Optional.empty(),  // TODO implement profile
                            Optional.empty()); // TODO implement backtrace

    }

    public Response(long token,
                    ResponseType responseType,
                    JSONArray data,
                    ArrayList<ResponseNote> responseNotes,
                    Optional<Profile> profile,
                    Optional<Backtrace> backtrace
    ) {
        this.token = token;
        this.type = responseType;
        this.notes = responseNotes;
        this.profile = profile;
        this.backtrace = backtrace;
        this.data = data;
    }

    @Override
    public String toString() {
        return "Response{" +
                "token=" + token +
                ", type=" + type +
                ", notes=" + notes +
                ", data=" + data +
                ", profile=" + profile +
                ", backtrace=" + backtrace +
                '}';
    }

    public boolean isWaitComplete() {
        return type == ResponseType.WAIT_COMPLETE;
    }



    /* Whether the response is any kind of feed */
    public boolean isFeed() {
        return notes.stream().allMatch(ResponseNote::isFeed);
    }

    /* Whether the response is any kind of error */
    public boolean isError() {
        return type.isError();
    }

    public static JSONArray convertPseudotypes(
      JSONArray obj, Optional<Profile> profile) {
        return obj; // TODO remove pass-through
    }

    /* What type of success the response contains */
    public boolean isAtom() {
        return type == ResponseType.SUCCESS_ATOM;
    }
    public boolean isSequence() {
        return type == ResponseType.SUCCESS_SEQUENCE;
    }
    public boolean isPartial() {
        return type == ResponseType.SUCCESS_PARTIAL;
    }

    public ReqlError makeError(Query query) {
        throw new RuntimeException("makeError not implemented");
    }
}
