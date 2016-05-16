import base64
import binascii
import hashlib
import hmac
import random
import struct
import sys
import threading

from . import ql2_pb2
from .errors import *

try:
    xrange
except NameError:
    xrange = range

class HandshakeV0_4(object):
    VERSION = ql2_pb2.VersionDummy.Version.V0_4
    PROTOCOL = ql2_pb2.VersionDummy.Protocol.JSON

    def __init__(self, host, port, auth_key):
        self._host = host
        self._port = port
        self._auth_key = auth_key.encode("ascii")

        self._state = 0

    def reset(self):
        self._state = 0

    def next_message(self, response):
        if self._state == 0:
            if response is not None:
                raise ReqlDriverError("Unexpected response")
            self._state = 1
            return \
                struct.pack("<2L", self.VERSION, len(self._auth_key)) + \
                self._auth_key + \
                struct.pack("<L", self.PROTOCOL)
        elif self._state == 1:
            if response is None:
                raise ReqlDriverError("Expected response")
            self._state = 2
            if response != b"SUCCESS":
                # This is an error case, we attempt to decode `response` as UTF-8 with
                # with fallbacks to get something useful
                message = None
                try:
                    message = response.decode("utf-8", errors="ignore").strip()
                except TypeError:
                    try:
                        message = response.decode("utf-8").strip()
                    except UnicodeError:
                        message = repr(response).strip()
                if message == "ERROR: Incorrect authorization key.":
                    raise ReqlAuthError("Incorrect authentication key.", self._host, self._port)
                else:
                    raise ReqlDriverError(
                        "Server dropped connection with message: \"%s\"" % (message, ))
            return None
        else:
            raise ReqlDriverError("Unexpected handshake state")

class HandshakeV1_0(object):
    VERSION = ql2_pb2.VersionDummy.Version.V1_0
    PROTOCOL = ql2_pb2.VersionDummy.Protocol.JSON

    def __init__(self, json_decoder, json_encoder, host, port, username, password):
        self._json_decoder = json_decoder
        self._json_encoder = json_encoder
        self._host = host
        self._port = port
        self._username = \
            username.encode("utf-8").replace(b"=", b"=3D").replace(b",", b"=2C")
        try:
            self._password = bytes(password, "utf-8")
        except TypeError:
            self._password = bytes(password)

        # Python 2.7.7+ and Python 3.3+
        self._compare_digest = getattr(hmac, "compare_digest", None)
        if not self._compare_digest:
            self._compare_digest = self.__compare_digest

        # Python 2.7.8+ and Python 3.4+.
        self._pbkdf2_hmac = getattr(hashlib, "pbkdf2_hmac", None)
        if not self._pbkdf2_hmac:
            self._pbkdf2_hmac = HandshakeV1_0.__pbkdf2_hmac

        self._protocol_version = 0
        self._random = random.SystemRandom()
        self._state = 0

    def reset(self):
        self._r = None
        self._client_first_message_bare = None
        self._server_signature = None
        self._state = 0

    def next_message(self, response):
        if self._state == 0:
            if response is not None:
                raise ReqlDriverError("Unexpected response")

            # Using base64 encoding for printable characters
            self._r = base64.standard_b64encode(
                bytes(bytearray(self._random.getrandbits(8) for i in xrange(18))))

            self._client_first_message_bare = b"n=" + self._username + b",r=" + self._r

            # Here we send the version as well as the initial JSON as an optimization
            self._state = 1
            return struct.pack("<L", self.VERSION) + \
                self._json_encoder.encode({
                    "protocol_version": self._protocol_version,
                    "authentication_method": "SCRAM-SHA-256",
                    "authentication":
                        (b"n,," + self._client_first_message_bare).decode("ascii")
                }).encode("utf-8") + \
                b'\0'
        elif self._state == 1:
            response = response.decode("utf-8")
            if response.startswith("ERROR"):
                raise ReqlDriverError("Received an unexpected reply.  You may be attempting to connect to a RethinkDB server that is too old for this driver.  The minimum supported server version is 2.3.0.")
            json = self._json_decoder.decode(response)
            try:
                if json["success"] == False:
                    if 10 <= json["error_code"] <= 20:
                        raise ReqlAuthError(json["error"], self._host, self._port)
                    else:
                        raise ReqlDriverError(json["error"])

                min = json["min_protocol_version"]
                max = json["max_protocol_version"]
                if not min <= self._protocol_version <= max:
                    raise ReqlDriverError(
                        "Unsupported protocol version %d, expected between %d and %d" % (
                            self._protocol_version, min, max))
            except KeyError as key_error:
                raise ReqlDriverError("Missing key: %s" % (key_error, ))

            # We've already sent the initial JSON above, and only support a single
            # protocol version at the moment thus we simply read the next response
            self._state = 2
            return ""
        elif self._state == 2:
            json = self._json_decoder.decode(response.decode("utf-8"))
            server_first_message = r = salt = i = None
            try:
                if json["success"] == False:
                    if 10 <= json["error_code"] <= 20:
                        raise ReqlAuthError(json["error"], self._host, self._port)
                    else:
                        raise ReqlDriverError(json["error"])

                server_first_message = json["authentication"].encode("ascii")
                authentication = dict(
                    x.split(b"=", 1) for x in server_first_message.split(b","))

                r = authentication[b"r"]
                if not r.startswith(self._r):
                    raise ReqlAuthError("Invalid nonce from server", self._host, self._port)
                salt = base64.standard_b64decode(authentication[b"s"])
                i = int(authentication[b"i"])
            except KeyError as key_error:
                raise ReqlDriverError("Missing key: %s" % (key_error, ))

            client_final_message_without_proof = b"c=biws,r=" + r

            # SaltedPassword := Hi(Normalize(password), salt, i)
            salted_password = self._pbkdf2_hmac("sha256", self._password, salt, i)

            # ClientKey := HMAC(SaltedPassword, "Client Key")
            client_key = hmac.new(
                salted_password, b"Client Key", hashlib.sha256).digest()

            # StoredKey := H(ClientKey)
            stored_key = hashlib.sha256(client_key).digest()

            # AuthMessage := client-first-message-bare + "," +
            #                server-first-message + "," +
            #                client-final-message-without-proof
            auth_message = b",".join((
                self._client_first_message_bare,
                server_first_message,
                client_final_message_without_proof))

            # ClientSignature := HMAC(StoredKey, AuthMessage)
            client_signature = hmac.new(
                stored_key, auth_message, hashlib.sha256).digest()

            # ClientProof := ClientKey XOR ClientSignature
            client_proof = struct.pack(
                "32B",
                *(l ^ r for l, r in zip(
                    struct.unpack("32B", client_key),
                    struct.unpack("32B", client_signature))))

            # ServerKey := HMAC(SaltedPassword, "Server Key")
            server_key = hmac.new(
                salted_password, b"Server Key", hashlib.sha256).digest()

            # ServerSignature := HMAC(ServerKey, AuthMessage)
            self._server_signature = hmac.new(
                server_key, auth_message, hashlib.sha256).digest()

            self._state = 3
            return self._json_encoder.encode({
                "authentication": (
                        client_final_message_without_proof +
                        b",p=" + base64.standard_b64encode(client_proof)
                    ).decode("ascii")
            }).encode("utf-8") + \
            b'\0'
        elif self._state == 3:
            json = self._json_decoder.decode(response.decode("utf-8"))
            v = None
            try:
                if json["success"] == False:
                    if 10 <= json["error_code"] <= 20:
                        raise ReqlAuthError(json["error"], self._host, self._port)
                    else:
                        raise ReqlDriverError(json["error"])

                authentication = dict(
                    x.split(b"=", 1)
                        for x in json["authentication"].encode("ascii").split(b","))

                v = base64.standard_b64decode(authentication[b"v"])
            except KeyError as key_error:
                raise ReqlDriverError("Missing key: %s" % (key_error, ))

            if not self._compare_digest(v, self._server_signature):
                raise ReqlAuthError("Invalid server signature", self._host, self._port)

            self._state = 4
            return None
        else:
            raise ReqlDriverError("Unexpected handshake state");

    @staticmethod
    def __compare_digest(a, b):
        if sys.version_info[0] == 3:
            def xor_bytes(a, b):
                return a ^ b
        else:
            def xor_bytes(a, b, _ord=ord):
                return _ord(a) ^ _ord(b)

        left = None
        right = b
        if len(a) == len(b):
            left = a
            result = 0
        if len(a) != len(b):
            left = b
            result = 1

        for l, r in zip(left, right):
            result |= xor_bytes(l, r)

        return result == 0

    class thread_local_cache(threading.local):
        def __init__(self):
            self.cache = {}

        def set(self, key, val):
            self.cache[key] = val

        def get(self, key):
            return self.cache.get(key)

    pbkdf2_cache = thread_local_cache()

    @staticmethod
    def __pbkdf2_hmac(hash_name, password, salt, iterations):
        assert hash_name == "sha256", hash_name

        def from_bytes(value, hexlify=binascii.hexlify, int=int):
            return int(hexlify(value), 16)

        def to_bytes(value, unhexlify=binascii.unhexlify):
            try:
                return unhexlify(bytes("%064x" % value, "ascii"))
            except TypeError:
                return unhexlify(bytes("%064x" % value))

        cache_key = (password, salt, iterations)

        cache_result = HandshakeV1_0.pbkdf2_cache.get(cache_key)

        if cache_result is not None:
            return cache_result

        mac = hmac.new(password, None, hashlib.sha256)

        def digest(msg, mac=mac):
            mac_copy = mac.copy()
            mac_copy.update(msg)
            return mac_copy.digest()

        t = digest(salt + b"\x00\x00\x00\x01")
        u = from_bytes(t)
        for c in xrange(iterations - 1):
            t = digest(t)
            u ^= from_bytes(t)

        u = to_bytes(u)
        HandshakeV1_0.pbkdf2_cache.set(cache_key, u)
        return u
