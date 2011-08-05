#include "protocol/redis/redis_util.hpp"

status_result redis_error(const char *msg) {
    boost::shared_ptr<status_result_struct> result(new status_result_struct);
    result->status = ERROR;
    result->msg = msg;
    return result;
}
