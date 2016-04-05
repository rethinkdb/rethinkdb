package com.rethinkdb.net;

import com.rethinkdb.gen.exc.ReqlAuthError;

import java.util.Optional;

public class ScramAttributes {
    Optional<String> _authIdentity = Optional.empty(); // a
    Optional<String> _username = Optional.empty();     // n
    Optional<String> _nonce = Optional.empty();        // r
    Optional<String> _headerAndChannelBinding = Optional.empty(); // c
    Optional<byte[]> _salt = Optional.empty(); // s
    Optional<Integer> _iterationCount = Optional.empty(); // i
    Optional<String> _clientProof = Optional.empty(); // p
    Optional<byte[]> _serverSignature = Optional.empty(); // v
    Optional<String> _error = Optional.empty(); // e
    Optional<String> _originalString = Optional.empty();


    public static ScramAttributes create() {
        return new ScramAttributes();
    }

    static ScramAttributes from(ScramAttributes other) {
        ScramAttributes out = new ScramAttributes();
        out._authIdentity = other._authIdentity;
        out._username = other._username;
        out._nonce = other._nonce;
        out._headerAndChannelBinding = other._headerAndChannelBinding;
        out._salt = other._salt;
        out._iterationCount = other._iterationCount;
        out._clientProof = other._clientProof;
        out._serverSignature = other._serverSignature;
        out._error = other._error;
        return out;
    }

    static ScramAttributes from(String input) {

        ScramAttributes sa = new ScramAttributes();
        sa._originalString = Optional.of(input);
        for (String section : input.split(",")) {
            String[] keyVal = section.split("=", 2);
            sa.setAttribute(keyVal[0], keyVal[1]);
        }
        return sa;
    }

    private void setAttribute(String key, String val) {
        switch (key) {
            case "a":
                _authIdentity = Optional.of(val);
                break;
            case "n":
                _username = Optional.of(val);
                break;
            case "r":
                _nonce = Optional.of(val);
                break;
            case "m":
                throw new ReqlAuthError("m field disallowed");
            case "c":
                _headerAndChannelBinding = Optional.of(val);
                break;
            case "s":
                _salt = Optional.of(Crypto.fromBase64(val));
                break;
            case "i":
                _iterationCount = Optional.of(Integer.parseInt(val));
                break;
            case "p":
                _clientProof = Optional.of(val);
                break;
            case "v":
                _serverSignature = Optional.of(Crypto.fromBase64(val));
                break;
            case "e":
                _error = Optional.of(val);
                break;
            default:
                // Supposed to ignore unexpected fields
        }
    }

    public String toString() {
        if (_originalString.isPresent()) {
            return _originalString.get();
        }
        String output = "";
        if (_username.isPresent()) {
            output += ",n=" + _username.get();
        }
        if (_nonce.isPresent()) {
            output += ",r=" + _nonce.get();
        }
        if (_headerAndChannelBinding.isPresent()) {
            output += ",c=" + _headerAndChannelBinding.get();
        }
        if (_clientProof.isPresent()) {
            output += ",p=" + _clientProof.get();
        }
        if (output.startsWith(",")) {
            return output.substring(1);
        } else {
            return output;
        }
    }

    // Setters with coercion
    ScramAttributes username(String username) {
        ScramAttributes next = ScramAttributes.from(this);
        next._username = Optional.of(username.replace("=", "=3D").replace(",", "=2C"));
        return next;
    }

    ScramAttributes nonce(String nonce) {
        ScramAttributes next = ScramAttributes.from(this);
        next._nonce = Optional.of(nonce);
        return next;
    }

    ScramAttributes headerAndChannelBinding(String hacb) {
        ScramAttributes next = ScramAttributes.from(this);
        next._headerAndChannelBinding = Optional.of(hacb);
        return next;
    }

    ScramAttributes clientProof(byte[] clientProof) {
        ScramAttributes next = ScramAttributes.from(this);
        next._clientProof = Optional.of(Crypto.toBase64(clientProof));
        return next;
    }

    // Getters
    String authIdentity() { return _authIdentity.get(); }
    String username() { return _username.get(); }
    String nonce() { return _nonce.get(); }
    String headerAndChannelBinding() { return _headerAndChannelBinding.get(); }
    byte[] salt() { return _salt.get(); }
    Integer iterationCount() { return _iterationCount.get(); }
    String clientProof() { return _clientProof.get(); }
    byte[] serverSignature() { return _serverSignature.get(); }
    String error() { return _error.get(); }
}
