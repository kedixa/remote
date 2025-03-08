#ifndef REMOTE_COMMON_H
#define REMOTE_COMMON_H

#include <cstdint>
#include <string>
#include <vector>

#include "msgpack.hpp"

namespace remote {

enum : uint32_t {
    CMD_INVOKE = 0,
    CMD_RETURN = 1,
    CMD_JUMP = 2,
    CMD_JUMP_TRUE = 3,
    CMD_JUMP_FALSE = 4,
};

using ArgID = uint32_t;

constexpr ArgID INDETERMINATE_ID = 0;
constexpr ArgID FIRST_ID = 1;

struct Command {
    uint32_t type{0};
    ArgID ret_id{INDETERMINATE_ID};
    std::size_t label{(std::size_t)-1};
    std::string name;
    std::vector<ArgID> arg_ids;

    MSGPACK_DEFINE(type, ret_id, label, name, arg_ids);
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
