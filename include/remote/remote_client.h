#ifndef REMOTE_REMOTE_CLIENT_H
#define REMOTE_REMOTE_CLIENT_H

#include "remote/remote_manager.h"
#include "coke/task.h"

namespace remote {

struct RemoteClientParams {
    std::string host;
    int port                = 5300;
    int retry_max           = 0;
    int send_timeout        = -1;
    int receive_timeout     = -1;
    int keep_alive_timeout  = 60 * 1000;
};

class RemoteClient {
public:
    explicit RemoteClient(const RemoteClientParams &params)
        : params(params)
    { }

    coke::Task<std::pair<int,int>> call(RemoteManager &m);

private:
    RemoteClientParams params;
};

} // namespace remote

#endif // REMOTE_REMOTE_CLIENT_H
