#ifndef REMOTE_SERVER_H
#define REMOTE_SERVER_H

#include "coke/net/basic_server.h"
#include "remote/task.h"

namespace remote {

using RemoteServerContext = coke::ServerContext<RemoteRequest, RemoteResponse>;
using RemoteReplyResult = coke::NetworkReplyResult;

constexpr coke::ServerParams REMOTE_SERVER_PARAMS_DEFAULT = {
    .transport_type         = TT_TCP,
    .max_connections        = 2000,
    .peer_response_timeout  = 10 * 1000,
    .receive_timeout        = -1,
    .keep_alive_timeout     = 60 * 1000,
    .request_size_limit     = (size_t)-1,
    .ssl_accept_timeout     = 10 * 1000,
};

struct RemoteServerParams : public coke::ServerParams {
    RemoteServerParams() : coke::ServerParams(REMOTE_SERVER_PARAMS_DEFAULT) { }
    ~RemoteServerParams() = default;
};

class Server : public coke::BasicServer<RemoteRequest, RemoteResponse> {
    using Base = coke::BasicServer<RemoteRequest, RemoteResponse>;

public:
    Server(const RemoteServerParams &params, ProcessorType co_proc)
        : Base(params, std::move(co_proc))
    { }

    Server(ProcessorType co_proc)
        : Base(RemoteServerParams(), std::move(co_proc))
    { }
};

} // namespace remote

#endif //REMOTE_SERVER_H
