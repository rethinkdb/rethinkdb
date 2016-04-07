// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/auth/scram_authenticator.hpp"

#include "clustering/administration/auth/authentication_error.hpp"
#include "clustering/administration/auth/password.hpp"
#include "clustering/administration/auth/username.hpp"
#include "crypto/base64.hpp"
#include "crypto/error.hpp"
#include "crypto/hash.hpp"
#include "crypto/hmac.hpp"
#include "crypto/random.hpp"

namespace auth {

scram_authenticator_t::scram_authenticator_t(
        clone_ptr_t<watchable_t<auth_semilattice_metadata_t>> auth_watchable)
    : base_authenticator_t(auth_watchable),
      m_state(state_t::FIRST_MESSAGE) {
}

std::string scram_authenticator_t::next_message(std::string const &message)
        THROWS_ONLY(authentication_error_t) {
    try {
        switch (m_state) {
            case state_t::FIRST_MESSAGE: {
                if (message.find("n,") != 0) {
                    throw authentication_error_t(15, "Channel binding is not supported");
                }
                size_t client_first_message_bare_offset = message.find(',', 2);
                if (client_first_message_bare_offset == std::string::npos) {
                    throw authentication_error_t(10, "Invalid encoding");
                }
                m_client_first_message_bare =
                    message.substr(client_first_message_bare_offset + 1);

                std::string client_nonce;
                std::map<char, std::string> attributes =
                    split_attributes(m_client_first_message_bare);
                for (auto const &attribute : attributes) {
                    switch (attribute.first) {
                        case 'n':
                            m_username = saslname_decode(attribute.second);
                            break;
                        case 'r':
                            client_nonce = attribute.second;
                            break;
                        case 'm':
                            throw authentication_error_t(11, "Extensions not supported");
                        default:
                            throw authentication_error_t(10, "Invalid encoding");
                    }
                }
                if (attributes.count('n') == 0 || attributes.count('r') == 0) {
                    throw authentication_error_t(10, "Invalid encoding");
                }

                m_auth_watchable->apply_read(
                    [&](auth_semilattice_metadata_t const *auth_metadata) {
                        auto user = auth_metadata->m_users.find(m_username);
                        if (user == auth_metadata->m_users.end() ||
                                !static_cast<bool>(user->second.get_ref())) {
                            m_is_user_known = false;
                            m_password =
                                password_t::generate_password_for_unknown_user();
                        } else {
                            m_is_user_known = true;
                            m_password = user->second.get_ref()->get_password();
                        }
                });

                std::array<unsigned char, 18> server_nonce = crypto::random_bytes<18>();
                m_nonce = client_nonce + crypto::base64_encode(server_nonce);

                m_server_first_message =
                    "r=" + m_nonce +
                    ",s=" + crypto::base64_encode(m_password.get_salt()) +
                    ",i=" + std::to_string(m_password.get_iteration_count());

                m_state = state_t::FINAL_MESSAGE;

                return m_server_first_message;
            }
            case state_t::FINAL_MESSAGE: {
                if (!m_is_user_known) {
                    throw authentication_error_t(17, "Unknown user");
                }

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

                std::map<char, std::string> attributes = split_attributes(message);
                for (auto const &attribute : attributes) {
                    switch (attribute.first) {
                        case 'c':
                            if (attribute.second != "biws") {
                                throw authentication_error_t(10, "Invalid encoding");
                            }
                            break;
                        case 'r':
                            if (attribute.second != m_nonce) {
                                // There's no specific error for invalid nonce
                                throw authentication_error_t(20, "Other error");
                            }
                            break;
                        case 'p':
                            if (attribute.second !=
                                    crypto::base64_encode(client_proof)) {
                                throw authentication_error_t(12, "Wrong password");
                            }
                            m_state = state_t::AUTHENTICATED;
                            break;
                        case 'm':
                            throw authentication_error_t(11, "Extensions not supported");
                        default:
                            throw authentication_error_t(10, "Invalid encoding");
                    }
                }
                if (attributes.count('c') == 0 ||
                        attributes.count('r') == 0 ||
                        attributes.count('p') == 0) {
                    throw authentication_error_t(10, "Invalid encoding");
                }

                // ServerKey := HMAC(SaltedPassword, "Server Key")
                std::array<unsigned char, SHA256_DIGEST_LENGTH> server_key =
                    crypto::hmac_sha256(m_password.get_hash(), "Server Key");

                // ServerSignature := HMAC(ServerKey, AuthMessage)
                std::array<unsigned char, SHA256_DIGEST_LENGTH> server_signature =
                    crypto::hmac_sha256(server_key, auth_message);

                return "v=" + crypto::base64_encode(server_signature);
            }
            case state_t::ERROR:
                throw authentication_error_t(
                    20, "A previous error occured, no more messages expected.");
            case state_t::AUTHENTICATED:
                throw authentication_error_t(
                    20, "Already authenticated, no more messages expected.");
            default:
                unreachable();
        }
    } catch (...) {
        m_state = state_t::ERROR;
        throw;
    }
}

/* virtual */ username_t scram_authenticator_t::get_authenticated_username() const
        THROWS_ONLY(authentication_error_t) {
    if (m_state == state_t::AUTHENTICATED) {
        return m_username;
    } else {
        throw authentication_error_t(20, "No authenticated user");
    }
}

/* static */ std::map<char, std::string> scram_authenticator_t::split_attributes(
        std::string const &message) {
    std::map<char, std::string> attributes;

    size_t attribute_offset = 0;
    while (attribute_offset < message.size()) {
        size_t equals_sign_offset = message.find('=', attribute_offset);
        if (equals_sign_offset == std::string::npos) {
            throw authentication_error_t(10, "Invalid encoding");
        }
        if ((equals_sign_offset - attribute_offset) != 1) {
            throw authentication_error_t(10, "Invalid encoding");
        }

        char key = message[attribute_offset];
        if (attributes.count(key) != 0) {
            throw authentication_error_t(10, "Invalid encoding");
        }

        size_t value_offset = equals_sign_offset + 1;
        size_t comma_offset = message.find(',', value_offset);
        if (comma_offset == std::string::npos) {
            attributes.insert(std::make_pair(key, message.substr(value_offset)));
            attribute_offset = message.size();
        } else {
            attributes.insert(std::make_pair(
                key, message.substr(value_offset, comma_offset - value_offset)));
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
                    throw authentication_error_t(18, "Invalid username encoding");
                }

                if (saslname[offset + 1] == '2' && saslname[offset + 2] == 'C') {
                    username.push_back(',');
                } else if (saslname[offset + 1] == '3' && saslname[offset + 2] == 'D') {
                    username.push_back('=');
                } else {
                    throw authentication_error_t(18, "Invalid username encoding");
                }

                offset += 2;
            default:
                username.push_back(saslname[offset]);
                break;
        }
    }

    try {
        // Note that the `username_t` constructor applies SASLPrep, and may throw
        return username_t(username);
    } catch (crypto::error_t const &) {
        throw authentication_error_t(18, "Invalid username encoding");
    }
}

}  // namespace auth
