package com.rethinkdb.net;

import com.rethinkdb.gen.exc.ReqlDriverError;

import javax.crypto.Mac;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;
import javax.crypto.spec.SecretKeySpec;
import javax.net.ssl.SSLContext;
import javax.net.ssl.TrustManagerFactory;
import java.io.IOException;
import java.io.InputStream;
import java.security.*;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.security.spec.InvalidKeySpecException;
import java.util.Base64;
import java.util.Optional;

import static com.rethinkdb.net.Util.fromUTF8;
import static com.rethinkdb.net.Util.toUTF8;

class Crypto {

    private static final String DEFAULT_SSL_PROTOCOL = "TLSv1.2";
    private static final String HMAC_SHA_256 = "HmacSHA256";
    private static final String PBKDF2_ALGORITHM = "PBKDF2WithHmacSHA256";

    private static final Base64.Encoder encoder = Base64.getEncoder();
    private static final Base64.Decoder decoder = Base64.getDecoder();
    private static final SecureRandom secureRandom = new SecureRandom();
    private static final int NONCE_BYTES = 18;


    static byte[] sha256(byte[] clientKey) {
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            return digest.digest(clientKey);
        } catch (NoSuchAlgorithmException e) {
            throw new ReqlDriverError(e);
        }
    }

    static byte[] hmac(byte[] key, String string) {
        try {
            Mac mac = Mac.getInstance(HMAC_SHA_256);
            SecretKeySpec secretKey = new SecretKeySpec(key, HMAC_SHA_256);
            mac.init(secretKey);
            return mac.doFinal(toUTF8(string));
        } catch (InvalidKeyException | NoSuchAlgorithmException e) {
            throw new ReqlDriverError(e);
        }
    }

    static byte[] pbkdf2(byte[] password, byte[] salt, Integer iterationCount) {
        PBEKeySpec spec = new PBEKeySpec(
                fromUTF8(password).toCharArray(), salt, iterationCount, 256);
        SecretKeyFactory skf;
        try {
            skf = SecretKeyFactory.getInstance(PBKDF2_ALGORITHM);
            return skf.generateSecret(spec).getEncoded();
        } catch (NoSuchAlgorithmException | InvalidKeySpecException e) {
            throw new ReqlDriverError(e);
        }
    }

    static String makeNonce() {
        byte[] rawNonce = new byte[NONCE_BYTES];
        secureRandom.nextBytes(rawNonce);
        return toBase64(rawNonce);
    }

    static byte[] xor(byte[] a, byte[] b) {
        if (a.length != b.length) {
            throw new ReqlDriverError("arrays must be the same length");
        }
        byte[] result = new byte[a.length];
        for (int i = 0; i < result.length; i++) {
            result[i] = (byte) (a[i] ^ b[i]);
        }
        return result;
    }

    static String toBase64(byte[] bytes) {
        return fromUTF8(encoder.encode(bytes));
    }

    static byte[] fromBase64(String string) {
        return decoder.decode(string);
    }

    static Optional<SSLContext> handleCertfile(
            Optional<InputStream> certFile, Optional<SSLContext> sslContext) {
        if (certFile.isPresent()) {
            try {
                final CertificateFactory cf = CertificateFactory.getInstance("X.509");
                final X509Certificate caCert = (X509Certificate) cf.generateCertificate(certFile.get());

                final TrustManagerFactory tmf = TrustManagerFactory
                        .getInstance(TrustManagerFactory.getDefaultAlgorithm());
                KeyStore ks = KeyStore.getInstance(KeyStore.getDefaultType());
                ks.load(null); // You don't need the KeyStore instance to come from a file.
                ks.setCertificateEntry("caCert", caCert);
                tmf.init(ks);

                final SSLContext ssc = SSLContext.getInstance(DEFAULT_SSL_PROTOCOL);
                ssc.init(null, tmf.getTrustManagers(), null);
                return Optional.of(ssc);
            } catch (IOException | CertificateException | NoSuchAlgorithmException | KeyStoreException | KeyManagementException e) {
                throw new ReqlDriverError(e);
            }
        } else {
            return sslContext;
        }
    }
}
