#ifndef REMOTE_COMMON_H
#define REMOTE_COMMON_H

#include <cstdint>
#include <string>
#include <vector>

#include "msgpack.hpp"

namespace remote {

enum : uint32_t {
    CMD_INVOKE = 0,
};

using ArgID = uint32_t;

constexpr ArgID INDETERMINATE_ID = 0;
constexpr ArgID FIRST_ID = 1;

struct Command {
    uint32_t type;
    ArgID ret_id;
    std::string name;
    std::vector<ArgID> arg_ids;

    MSGPACK_DEFINE(type, ret_id, name, arg_ids);
};

struct PackStream {
    PackStream &write(const char *buf, size_t len) {
        data.append(buf, len);
        return *this;
    }

    std::string &data;
};

} // namespace remote

#endif // REMOTE_COMMON_H
