package com.rethinkdb.net;

import com.rethinkdb.gen.exc.ReqlAuthError;
import com.rethinkdb.gen.exc.ReqlDriverError;
import com.rethinkdb.gen.proto.Protocol;
import com.rethinkdb.gen.proto.Version;
import org.json.simple.JSONObject;

import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.util.Optional;

import static com.rethinkdb.net.Crypto.*;
import static com.rethinkdb.net.Util.toJSON;
import static com.rethinkdb.net.Util.toUTF8;

public class Handshake {
    static final Version VERSION = Version.V1_0;
    static final Long SUB_PROTOCOL_VERSION = 0L;
    static final Protocol PROTOCOL = Protocol.JSON;


    private static final String CLIENT_KEY = "Client Key";
    private static final String SERVER_KEY = "Server Key";

    private final String username;
    private final String password;
    private ProtocolState state;

    private interface ProtocolState {
        ProtocolState nextState(String response);
        Optional<ByteBuffer> toSend();
        boolean isFinished();
    }

    private class InitialState implements ProtocolState {
        private final String nonce;
        private final String username;
        private final byte[] password;

        InitialState(String username, String password) {
            this.username = username;
            this.password = toUTF8(password);
            this.nonce = makeNonce();
        }

        @Override
        public ProtocolState nextState(String response) {
            if (response != null) {
                throw new ReqlDriverError("Unexpected response");
            }
            // We could use a json serializer, but it's fairly straightforward
            ScramAttributes clientFirstMessageBare = ScramAttributes.create()
                    .username(username)
                    .nonce(nonce);
            byte[] jsonBytes = toUTF8(
                "{" +
                    "\"protocol_version\":" + SUB_PROTOCOL_VERSION + "," +
                    "\"authentication_method\":\"SCRAM-SHA-256\"," +
                    "\"authentication\":" + "\"n,," + clientFirstMessageBare + "\"" +
                "}"
            );
            ByteBuffer msg = Util.leByteBuffer(
                    Integer.BYTES +    // size of VERSION
                    jsonBytes.length + // json auth payload
                    1                  // terminating null byte
            ).putInt(VERSION.value)
             .put(jsonBytes)
             .put(new byte[1]);
            return new WaitingForProtocolRange(
                    nonce, password, clientFirstMessageBare, msg);
        }

        @Override
        public Optional<ByteBuffer> toSend() {
            return Optional.empty();
        }

        @Override
        public boolean isFinished() {
            return false;
        }
    }

    private class WaitingForProtocolRange implements ProtocolState {
        private final String nonce;
        private final ByteBuffer message;
        private final ScramAttributes clientFirstMessageBare;
        private final byte[] password;

        WaitingForProtocolRange(
                String nonce,
                byte[] password,
                ScramAttributes clientFirstMessageBare,
                ByteBuffer message) {
            this.nonce = nonce;
            this.password = password;
            this.clientFirstMessageBare = clientFirstMessageBare;
            this.message = message;
        }

        @Override
        public ProtocolState nextState(String response) {
            JSONObject json = toJSON(response);
            throwIfFailure(json);
            Long minVersion = (Long) json.get("min_protocol_version");
            Long maxVersion = (Long) json.get("max_protocol_version");
            if (SUB_PROTOCOL_VERSION < minVersion || SUB_PROTOCOL_VERSION > maxVersion) {
                throw new ReqlDriverError(
                        "Unsupported protocol version " + SUB_PROTOCOL_VERSION +
                                ", expected between " + minVersion + " and " + maxVersion);
            }
            return new WaitingForAuthResponse(nonce, password, clientFirstMessageBare);
        }

        @Override
        public Optional<ByteBuffer> toSend() {
            return Optional.of(message);
        }

        @Override
        public boolean isFinished() {
            return false;
        }
    }

    private class WaitingForAuthResponse implements ProtocolState {
        private final String nonce;
        private final byte[] password;
        private final ScramAttributes clientFirstMessageBare;

        WaitingForAuthResponse(
                String nonce, byte[] password, ScramAttributes clientFirstMessageBare) {
            this.nonce = nonce;
            this.password = password;
            this.clientFirstMessageBare = clientFirstMessageBare;
        }

        @Override
        public ProtocolState nextState(String response) {
            JSONObject json = toJSON(response);
            throwIfFailure(json);
            String serverFirstMessage = (String) json.get("authentication");
            ScramAttributes serverAuth = ScramAttributes.from(serverFirstMessage);
            if (!serverAuth.nonce().startsWith(nonce)) {
                throw new ReqlAuthError("Invalid nonce from server");
            }
            ScramAttributes clientFinalMessageWithoutProof = ScramAttributes.create()
                    .headerAndChannelBinding("biws")
                    .nonce(serverAuth.nonce());

            // SaltedPassword := Hi(Normalize(password), salt, i)
            byte[] saltedPassword = pbkdf2(
                    password, serverAuth.salt(), serverAuth.iterationCount());

            // ClientKey := HMAC(SaltedPassword, "Client Key")
            byte[] clientKey = hmac(saltedPassword, CLIENT_KEY);

            // StoredKey := H(ClientKey)
            byte[] storedKey = sha256(clientKey);

            // AuthMessage := client-first-message-bare + "," +
            //                server-first-message + "," +
            //                client-final-message-without-proof
            String authMessage =
                    clientFirstMessageBare + "," +
                    serverFirstMessage + "," +
                    clientFinalMessageWithoutProof;

            // ClientSignature := HMAC(StoredKey, AuthMessage)
            byte[] clientSignature = hmac(storedKey, authMessage);

            // ClientProof := ClientKey XOR ClientSignature
            byte[] clientProof = xor(clientKey, clientSignature);

            // ServerKey := HMAC(SaltedPassword, "Server Key")
            byte[] serverKey = hmac(saltedPassword, SERVER_KEY);

            // ServerSignature := HMAC(ServerKey, AuthMessage)
            byte[] serverSignature = hmac(serverKey, authMessage);

            ScramAttributes auth = clientFinalMessageWithoutProof
                    .clientProof(clientProof);
            byte[] authJson = toUTF8("{\"authentication\":\"" + auth + "\"}");
            ByteBuffer message = Util.leByteBuffer(authJson.length + 1)
                    .put(authJson)
                    .put(new byte[1]);
            return new WaitingForAuthSuccess(serverSignature, message);
        }

        @Override
        public Optional<ByteBuffer> toSend() {
            return Optional.empty();
        }

        @Override
        public boolean isFinished() {
            return false;
        }
    }

    private class WaitingForAuthSuccess implements ProtocolState {
        private final byte[] serverSignature;
        private final ByteBuffer message;

        public WaitingForAuthSuccess(byte[] serverSignature, ByteBuffer message) {
            this.serverSignature = serverSignature;
            this.message = message;
        }

        @Override
        public ProtocolState nextState(String response) {
            JSONObject json = toJSON(response);
            throwIfFailure(json);
            ScramAttributes auth = ScramAttributes
                    .from((String) json.get("authentication"));
            if (!MessageDigest.isEqual(auth.serverSignature(), serverSignature)) {
                throw new ReqlAuthError("Invalid server signature");
            }
            return new HandshakeSuccess();
        }

        @Override
        public Optional<ByteBuffer> toSend() {
            return Optional.of(message);
        }

        @Override
        public boolean isFinished() {
            return false;
        }
    }

    private class HandshakeSuccess implements ProtocolState {

        @Override
        public ProtocolState nextState(String response) {
            return this;
        }

        @Override
        public Optional<ByteBuffer> toSend() {
            return Optional.empty();
        }

        @Override
        public boolean isFinished() {
            return true;
        }
    }

    private void throwIfFailure(JSONObject json) {
        if (!(boolean) json.get("success")) {
            Long errorCode = (Long) json.get("error_code");
            if (errorCode >= 10 && errorCode <= 20) {
                throw new ReqlAuthError((String) json.get("error"));
            } else {
                throw new ReqlDriverError((String) json.get("error"));
            }
        }
    }

    public Handshake(String username, String password) {
        this.username = username;
        this.password = password;
        this.state = new InitialState(username, password);
    }

    public void reset() {
        this.state = new InitialState(this.username, this.password);
    }

    public Optional<ByteBuffer> nextMessage(String response) {
        this.state = this.state.nextState(response);
        return this.state.toSend();
    }

    public boolean isFinished() {
        return this.state.isFinished();
    }
}
