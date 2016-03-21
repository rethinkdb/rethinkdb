// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/scram_authenticator.hpp"

#include "errors.hpp"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "clustering/administration/auth/base64.hpp"
#include "clustering/administration/auth/crypto/hash.hpp"
#include "clustering/administration/auth/crypto/hmac.hpp"
#include "clustering/administration/auth/crypto/random.hpp"
#include "clustering/administration/auth/password.hpp"
#include "clustering/administration/auth/username.hpp"

namespace auth {

scram_authenticator_t::scram_authenticator_t(
        clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable)
    : m_auth_watchable(auth_watchable) {
}

std::string scram_authenticator_t::first_message(std::string const &message) {
    // Note that we define this up here since we might need it in the error handling
    std::string client_nonce;

    try {
        if (message.find("n,") != 0) {
            // FIXME "server-does-support-channel-binding"
        }
        size_t client_first_message_bare_offset = message.find(',', 2);
        if (client_first_message_bare_offset == std::string::npos) {
            // FIXME "invalid-encoding"
        }
        m_client_first_message_bare = message.substr(client_first_message_bare_offset + 1);

        username_t username;
        std::map<char, std::string> attributes =
            scram_authenticator_t::split_attributes(m_client_first_message_bare);
        for (auto const &attribute : attributes) {
            switch (attribute.first) {
                case 'n':
                    username = saslname_decode(attribute.second);
                    break;
                case 'r':
                    client_nonce = attribute.second;
                    break;
                case 'm':
                    // FIXME "extensions-not-supported"
                    break;
                default:
                    ; // FIXME "invalid-encoding"
            }
        }
        if (attributes.size() != 2) {
            ; // FIXME "invalid-encoding"
        }

        m_auth_watchable->apply_read(
            [&](auth_semilattice_metadata_t const *auth_metadata) {
                auto iter = auth_metadata->m_users.find(auth::username_t(username));
                if (iter == auth_metadata->m_users.end() ||
                        !static_cast<bool>(iter->second.get_ref())) {
                    // FIXME "unknown-user"
                }
                // FIXME assume optional is filled
                m_password = iter->second.get_ref().get().get_password().get();
        });

        std::array<unsigned char, 18> server_nonce = crypto::random_bytes<18>();
        m_nonce = client_nonce + base64_encode(server_nonce);

        m_server_first_message =
            "r=" + m_nonce +
            ",s=" + base64_encode(m_password.get_salt()) +
            ",i=" + std::to_string(m_password.get_iteration_count());
    } catch (...) {
        // FIXME
    }

    return m_server_first_message;
}

std::string scram_authenticator_t::final_message(std::string const &message) {
    // FIXME handle error if set

    try {
        // ClientKey := HMAC(SaltedPassword, "Client Key")
        std::array<unsigned char, SHA256_DIGEST_LENGTH> client_key =
            crypto::hmac_sha256(m_password.get_hash(), "Client Key");

        // StoredKey := H(ClientKey)
        std::array<unsigned char, SHA256_DIGEST_LENGTH> stored_key =
            crypto::sha256(client_key);

        /* AuthMessage := client-first-message-bare + "," +
                          server-first-message + "," +
                          client-final-message-without-proof */
        std::string auth_message =
            m_client_first_message_bare + "," +
            m_server_first_message + "," +
            message.substr(0, message.find(",p="));

        // ClientSignature := HMAC(StoredKey, AuthMessage)
        std::array<unsigned char, SHA256_DIGEST_LENGTH> client_signature =
            crypto::hmac_sha256(stored_key, auth_message);

        // ClientProof := ClientKey XOR ClientSignature
        std::array<unsigned char, SHA256_DIGEST_LENGTH> client_proof;
        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
            client_proof[i] = (client_key[i] ^ client_signature[i]);
        }

        std::map<char, std::string> attributes =
            scram_authenticator_t::split_attributes(message);
        for (auto const &attribute : attributes) {
            switch (attribute.first) {
                case 'c':
                    if (attribute.second != "biws") {
                        // FIXME "invalid-encoding"
                    }
                    break;
                case 'r':
                    if (attribute.second != m_nonce) {
                        // FIXME "other-error", there's no message for invalid nonce
                    }
                    break;
                case 'p':
                    if (attribute.second != auth::base64_encode(client_proof)) {
                        // FIXME "invalid-proof"
                    }
                    break;
                case 'm':
                    // FIXME "extensions-not-supported"
                    break;
                default:
                    ; // FIXME "invalid-encoding"
            }
        }
        if (attributes.size() != 3) {
            ; // FIXME "invalid-encoding"
        }

        // ServerKey := HMAC(SaltedPassword, "Server Key")
        std::array<unsigned char, SHA256_DIGEST_LENGTH> server_key =
            crypto::hmac_sha256(m_password.get_hash(), "Server Key");

        // ServerSignature := HMAC(ServerKey, AuthMessage)
        std::array<unsigned char, SHA256_DIGEST_LENGTH> server_signature =
            crypto::hmac_sha256(server_key, auth_message);

        return "v=" + auth::base64_encode(server_signature);
    } catch (...) {
        // FIXME
    }

    return "";
}

/* static */ std::map<char, std::string> scram_authenticator_t::split_attributes(
        std::string const &message) {
    std::map<char, std::string> attributes;

    size_t attribute_offset = 0;
    while (attribute_offset < message.size()) {
        size_t equals_sign_offset = message.find('=', attribute_offset);
        if (equals_sign_offset == std::string::npos) {
            // FIXME "invalid-encoding"
        }
        if ((equals_sign_offset - attribute_offset) != 1) {
            // FIXME "invalid-encoding"
        }

        char key = message[attribute_offset];
        if (attributes.count(key) != 0) {
            // FIXME "invalid-encoding"
        }

        size_t value_offset = equals_sign_offset + 1;
        size_t comma_offset = message.find(',', value_offset);
        if (comma_offset == std::string::npos) {
            attributes.emplace(key, message.substr(value_offset));
            attribute_offset = message.size();
        } else {
            attributes.emplace(
                key, message.substr(value_offset, comma_offset - value_offset));
            attribute_offset = comma_offset + 1;
        }
    }

    return attributes;
}

/* static */ username_t scram_authenticator_t::saslname_decode(
        std::string const &saslname) {
    std::string username;
    username.reserve(saslname.size());

    for (size_t offset = 0; offset < saslname.size(); ++offset) {
        switch (saslname[offset]) {
            case '=':
                if (offset + 2 >= saslname.size()) {
                    // FIXME "invalid-encoding"
                }

                if (saslname[offset + 1] == '2' && saslname[offset + 2] == 'C') {
                    username.push_back(',');
                } else if (saslname[offset + 1] == '3' && saslname[offset + 2] == 'D') {
                    username.push_back('=');
                } else {
                    // FIXME "invalid-username-encoding"
                }

                offset += 2;
            default:
                username.push_back(saslname[offset]);
                break;
        }
    }

    try {
        return username_t(username); // Note the `username_t` constructor applies SASLPrep
    } catch (...) {
        ; // FIXME "invalid-username-encoding"
    }

    return username_t(); // FIXME
}

}  // namespace auth
