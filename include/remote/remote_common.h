#ifndef REMOTE_REMOTE_COMMON_H
#define REMOTE_REMOTE_COMMON_H

#include <string>
#include <vector>

#include "msgpack.hpp"

namespace remote {

struct RemoteFunction {
    std::string name;
    int return_id;
    std::vector<int> arg_ids;

    MSGPACK_DEFINE(name, return_id, arg_ids);
};

struct PackStream {
    PackStream &write(const char *buf, size_t len) {
        data.append(buf, len);
        return *this;
    }

    std::string &data;
};

} // namespace remote

#endif //REMOTE_REMOTE_COMMON_H
