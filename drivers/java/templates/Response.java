package com.rethinkdb.response;

import com.rethinkdb.proto.ResponseType;
import com.rethinkdb.proto.ResponseNote;
import org.json.simple.*;
import org.json.simple.

import java.util.*;
import java.nio.ByteBuffer;

private class ResponseHandler extends ContentHandler {
    public Optional<ResponseType> responseType = Optional.empty();
    public Optional<Backtrace> backtrace = Optional.empty();
    public Optional<Profile> profile = Optional.empty();
    public Optional<ArrayList<ResponseNote>> notes = Optional.empty();
    public Optional<JSONArray> response = Optional.empty();

    private boolean atTopLevel = true;
    private boolean inProfile = false;
    private boolean inBacktrace = false;
    private boolean inNotes = false;
    private boolean inResponse = false;

    private ArrayList<ContentHandler> parserStack;

    public ResponseHandler() {
        this.p = new TopLevelParser(parserStack);
    }

}

private interface SubParser extends ContentHandler {
    SubParser nextParser() throws ParseException;

    default boolean successfullyEnded() {
        return false;
    }

    // Provide default erroring implementations for subparsers
    default void startJSON() throws ParseException, IOException {}
    default void endJSON() throws ParseException, IOException {}
    default boolean startArray() throws ParseException, IOException {
        throw new ParseException("start of Array not expected here");
    }
    default boolean endArray() throws ParseException, IOException {
        throw new ParseException("end of Array not expected here");
    }
    default boolean startObject() throws ParseException, IOException {
        throw new ParseException("start of Object not expected here");
    }
    default boolean endObject() throws ParseException, IOException {
        throw new ParseException("end of Object not expected here");
    }
    default boolean primitive() throws ParseException, IOException {
        throw new ParseException("primitive not expected here");
    }
    default boolean startObjectEntry(String key) throws ParseException, IOException {
        throw new ParseException("start of object entry not expected here");
    }
    default boolean endObjectEntry() throws ParseException, IOException {
        throw new ParseException("end of object entry not expected here");
    }
}

private class TopLevelParser implements SubParser {
    private Optional<SubParser> _nextParser = Optional.empty();
    private boolean ended = false;

    public Optional<ArrayList<ResponseNote>> responseNotes = Optional.empty();
    public Optional<ResponseType> responseType = Optional.empty();
    public Optional<Backtrace> backtrace = Optional.empty();
    public Optional<Profile> profile = Optional.empty();

    public TopLevelParser() {
        //TODO: accept global optargs to pass to the ResultParser
    }

    @Override
    public SubParser nextParser() throws ParseException {
        return _nextParser.orElseThrow(
             () -> new ParseException("No keys found in top level!"));
    }

    @Override
    public boolean successfullyEnded() {
        return ended;
    }

    @Override
    public boolean startObject() throws ParseException, IOException {
        return true;
    }

    @Override
    public boolean endObject() throws ParseException, IOException {
        ended = true;
        return false;
    }

    @Override
    public boolean startObjectEntry(String key)
                   throws ParseException, IOException {
        switch(key) {
            case "t":
                _nextParser = new ResponseTypeParser(this);
                break;
            case "p":
                _nextParser = new ProfileParser(this);
                break;
            case "n":
                _nextParser = new NotesParser(this);
                break;
            case "b":
                _nextParser = new BacktraceParser(this);
                break;
            case "r":
                _nextParser = new ResultParser(this);
                break;
            default:
                throw new ParseException(String.format(
                    "%s is an illegal top-level response key", key))
        }
        return false; // Stop temporarily.
    }
}

private class BacktraceParser implements SubParser {
    private final TopLevelParser parent;
    public BacktraceParser(TopLevelParser parent) {
        this.parent = parent;
    }
    @Override
    public SubParser nextParser() {
        return parent;
    }
    // TODO: implement parsing backtraces
}

private class ProfileParser implements SubParser {
    private final TopLevelParser parent;
    public ProfileParser(TopLevelParser parent) {
        this.parent = parent;
    }
    @Override
    public SubParser nextParser() {
        return parent;
    }
    // TODO: implement parsing profiles
}

private class NotesParser implements SubParser {
    private final TopLevelParser parent;
    private currentlyInArray = false;

    public final ArrayList<ResponseNote> result = new ArrayList<>();

    public NotesParser(TopLevelParser parent) {
        this.parent = parent;
    }

    @Override
    public SubParser nextParser() {
        return parent;
    }

    @Override
    public boolean primitive(Object value) throws ParseException, IOException {
        if(currentlyInArray) {
            if(value instanceof Number) {
                result.push(ResponseNote.fromInt(value.intValue()));
                return true;
            }else{
                throw new ParseException("Only ints expected in ResponseNotes array");
            }
        }else{
            throw new ParseException("Didn't expect a primitive here");
        }
    }

    @Override
    public boolean startArray() throws ParseException, IOException {
        currentlyInArray = true;
        return true;
    }

    @Override
    public boolean endArray() throws ParseException, IOException {
        currentlyInArray = false;
        parent.responseNotes = result;
        // Done getting response notes, back to TopLevelParser
        return false;
    }

}

private class ResponseTypeParser implements SubParser {
    public Optional<ResponseType> result = Optional.empty();

    @Override
    public boolean primitive(Object value) throws ParseException, IOException {
        if(value instanceof Number){
            result = Optional.of(ResponseType.fromValue(value));
            return false;
        }else {
            throw new ParseException("ResponseType may only be a number");
        }
    }
}

private class ResultParser implements SubParser {

    private Optional<JSONArray> result = Optional.empty();
    // TODO: Parse reql results, converting pseudotypes as we go
    @Override
    public void startJSON() throws ParseException, IOException {
    }

    @Override
    public void endJSON() throws ParseException, IOException {
    }

    @Override
    public boolean primitive(Object value) throws ParseException, IOException {
        return true;
    }

    @Override
    public boolean startArray() throws ParseException, IOException {
        return true;
    }

    @Override
    public boolean startObject() throws ParseException, IOException {
        return true;
    }

    @Override
    public boolean startObjectEntry(String key) throws ParseException, IOException {
        return true;
    }

    @Override
    public boolean endArray() throws ParseException, IOException {
        return true;
    }

    @Override
    public boolean endObject() throws ParseException, IOException {
        return true;
    }

    @Override
    public boolean endObjectEntry() throws ParseException, IOException {
        return true;
    }
}

public class Response {
    public final long token;
    public final ResponseType responseType;
    public final ArrayList<ResponseNote> responseNotes;

    public static Response parseFrom(token,
                                     InputStream stream,
                                     GlobalOptions globalOpts) {
        JSONParser parser = new JSONParser();
        Reader codepointReader = InputStreamReader(stream, "UTF-8");

        final TopLevelParser tlparser = new TopLevelParser();
        SubParser currentParser = tlparser;
        try{
            while(!currentParser.isEnd()){
                parser.parse(codepointReader, , true);
                if(finder.isFound()){
                    finder.setFound(false);
                    System.out.println("found id:");
                    System.out.println(finder.getValue());
                }
            }
        }
        catch(ParseException pe){
            pe.printStackTrace();
        }
        parser.parse(codepointReader, new ResponseHandler(globalOpts));
    }

    public static Response parseFrom(token, byte[] buf) {
        return Response.parseFrom(token, ByteArrayInputStream(buf));
    }

    public Response(long token,
                    ResponseType responseType,
                    ArrayList<ResponseNote> responseNotes) {
        this.token = token;
        this.responseType = responseType;
        this.responseNotes = responseNotes;
    }

    public static Response<Cursor> empty() {
        return new Response(null, (Cursor) null, Optional.empty());
    }

    public Datum getResponse() {
        throw new RuntimeException("getResponse not implemented");
    }

    public Datum getResponse(int x) {
        throw new RuntimeException("getResponse:1 not implemented");
    }

    public <T> List<T> getResponseList() {
        throw new RuntimeException("getResponseList not implemented");
    }

    public boolean isWaitComplete() {
        return responseType == ResponseType.WAIT_COMPLETE;
    }

    /* Autogenerated methods below */

    /* Whether the response is any kind of feed */
    public boolean isFeed() {
        return ${"\n            || ".join(
                "responseNotes.contains(ResponseNote.{})".format(note_name)
                for note_name in proto['Response']['ResponseNote'].keys()
                if note_name.endswith('_FEED'))};
    }

    /* If the response has a given kind of note (only used by feeds currently) */
    % for note_name in proto['Response']['ResponseNote'].keys():
    public boolean ${"is"+camel(note_name) if note_name.endswith('_FEED') else dromedary(note_name)}() {
        return responseNotes.contains(ResponseNote.${note_name});
    }
    % endfor

    /* Whether the response is any kind of error */
    public boolean isError() {
        return ${"\n            || ".join(
                "responseType == ResponseType.{}".format(error_type)
                for error_type in proto['Response']['ResponseType'].keys()
                if error_type.endswith('_ERROR'))};
    }

    /* What type of success the response contains */
    % for success_type in proto['Response']['ResponseType'].keys():
        % if success_type.startswith('SUCCESS_'):
    public boolean ${"is"+camel(success_type[8:])}() {
        return responseType == ResponseType.${success_type};
    }
        % endif
    % endfor
}
