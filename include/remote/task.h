#ifndef REMOTE_TASK_H
#define REMOTE_TASK_H

#include "workflow/WFTaskFactory.h"
#include "workflow/TLVMessage.h"

namespace remote {

using RemoteRequest = protocol::TLVRequest;
using RemoteResponse = protocol::TLVResponse;
using RemoteTask = WFNetworkTask<RemoteRequest, RemoteResponse>;

inline RemoteTask *
create_remote_task(const std::string &host, int port, int retry_max) {
    using Factory = WFNetworkTaskFactory<RemoteRequest, RemoteResponse>;
    return Factory::create_client_task(TT_TCP, host, port, retry_max,
                                       nullptr);
}

} // namespace remote

#endif //REMOTE_TASK_H
