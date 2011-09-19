#include "redis/pubsub.hpp"

bool pubsub_runtime_t::subscribe(std::string &channel, uint64_t connection_id, redis_output_writer *out) {
    subs_lock.co_lock(rwi_write);

    std::list<subscription_t> subs = subscriptions[channel];

    // Check if we're already subscribed
    for(std::list<subscription_t>::iterator iter = subs.begin(); iter != subs.end(); ++iter) {
        if(iter->connection_id == connection_id) {
            // Already here, no need to do more
            subs_lock.unlock();
            return false;
        }
    }

    subscription_t new_sub;
    new_sub.connection_id = connection_id;
    new_sub.out = out;

    subs.push_back(new_sub);

    subs_lock.unlock();

    return true;
}

bool pubsub_runtime_t::subscribe_pattern(std::string &pattern, uint64_t connection_id, redis_output_writer *out) {
    subs_lock.co_lock(rwi_write);

    // Check for a duplicate pattern for this connection
    for(std::list<pattern_subscription_t>::iterator iter = pattern_subscriptions.begin();
            iter != pattern_subscriptions.end(); ++iter) {
        if(iter->connection_id == connection_id && iter->pattern == pattern) {
            // Already here, no need to do more
            subs_lock.unlock();
            return false;
        }
    }

    pattern_subscription_t psub;
    psub.connection_id = connection_id;
    psub.out = out;
    psub.pattern = pattern;

    pattern_subscriptions.push_back(psub);

    subs_lock.unlock();

    return true;
}

bool pubsub_runtime_t::unsubscribe(std::string &channel, uint64_t connection_id) {
    subs_lock.co_lock(rwi_write);

    bool unsubscribed = false;

    std::list<subscription_t> subs = subscriptions[channel];
    for(std::list<subscription_t>::iterator iter = subs.begin(); iter != subs.end(); ++iter) {
        if(iter->connection_id == connection_id) {
            subs.erase(iter);
            unsubscribed = true;
            break;
        }
    }

    subs_lock.unlock();

    return unsubscribed;
}

bool pubsub_runtime_t::unsubscribe_pattern(std::string &pattern, uint64_t connection_id) {
    subs_lock.co_lock(rwi_write);

    bool unsubscribed = false;

    for(std::list<pattern_subscription_t>::iterator iter = pattern_subscriptions.begin();
            iter != pattern_subscriptions.end(); ++iter) {
        if((iter->connection_id == connection_id) && (iter->pattern == pattern)) {
            pattern_subscriptions.erase(iter);
            unsubscribed = true;
            break;
        }
    }

    subs_lock.unlock();

    return unsubscribed;
}


int pubsub_runtime_t::publish(std::string &channel, std::string &message) {
    subs_lock.co_lock(rwi_write);

    int clients_recieved = 0;

    // Prepare the multi-bulk message
    std::vector<std::string> message_package;
    message_package.push_back(std::string("message"));
    message_package.push_back(channel);
    message_package.push_back(message);
    redis_protocol_t::redis_return_type output(message_package);

    std::list<subscription_t> subs = subscriptions[channel];
    for(std::list<subscription_t>::iterator iter = subs.begin(); iter != subs.end(); ++iter) {
        {
            on_thread_t thr(iter->out->home_thread());
            iter->out->output_response(output);
        }
        clients_recieved++;
    }

    // Now check pattern subscriptions

    for(std::list<pattern_subscription_t>::iterator iter = pattern_subscriptions.begin();
            iter != pattern_subscriptions.end(); ++iter) {
        if(pattern_matches(channel, iter->pattern)) {
            {
                on_thread_t thr(iter->out->home_thread());

                // The message here depends upon the pattern so we have to construct it here
                std::vector<std::string> message_package;
                message_package.push_back(std::string("pmessage"));
                message_package.push_back(iter->pattern);
                message_package.push_back(channel);
                message_package.push_back(message);
                redis_protocol_t::redis_return_type output(message_package);

                iter->out->output_response(output);
            }
            clients_recieved++;
        }
    }

    subs_lock.unlock();

    return clients_recieved;
}

bool match_pattern(const char *, const char *);

bool star_match(const char *chan, const char *patt) {
    assert(*patt == '*');
    ++patt;

    // If * is the last charater in the pattern than we automatically match the rest of the pattern
    if(*patt == '\0') return true;
    
    // This potentially requires backtracking. Luckily our recursive solution will backtrack for us!
    for(int i = 0; *(chan + i) != '\0'; ++i) {
        // Assume that the * consumes exactly i characters in the channel
        if(match_pattern(chan + i, patt)) return true;
    }

    return false;
}

bool group_match(const char *chan, const char *patt) {
    assert(*patt == '[');
    ++patt;

    std::vector<char> group;
    while(*patt != ']') {
        if(*patt == '\0') return false;
        group.push_back(*patt);
        ++patt;
    }

    ++patt;

    for(std::vector<char>::iterator iter = group.begin(); iter != group.end(); ++iter) {
        if(*iter == *chan) {
            return match_pattern(chan + 1, patt);
        }
    }

    return false;
}

bool match_pattern(const char *chan, const char *patt) {
    if(*chan == '\0' && *patt != '\0') return false;

    switch(*patt) {
    case '\0':
        return (*chan == '\0');
    case '*':
        return star_match(chan, patt);
    case '?':
        // Matches any character
        return match_pattern(chan + 1, patt + 1);
    case '[':
        // Match single character from group defined by [...]
        return group_match(chan, patt);
    case '\\':
        // Literally match next character in pattern
        patt++;
        // Intentional fallthrough
    default:
        // Litteral character match
        if(*patt != *chan) {
            return false;
        }
        return match_pattern(chan + 1, patt + 1);
    }
}

bool pubsub_runtime_t::pattern_matches(std::string &channel, std::string &pattern) {
    // I can't find an implementation of glob matching (existing libraries given a path will return
    // all files that match the pattern on that path) so I'm just going to do it manually. That is
    // apparently how redis does it and obviously I can't copy their code.

    const char *chan = channel.c_str();
    const char *patt = pattern.c_str();

    return match_pattern(chan, patt);
}
