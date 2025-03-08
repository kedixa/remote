#ifndef REMOTE_CLIENT_H
#define REMOTE_CLIENT_H

#include "remote/command_builder.h"
#include "coke/task.h"

namespace remote {

struct ClientParams {
    std::string host;
    int port                = 5300;
    int retry_max           = 0;
    int send_timeout        = -1;
    int receive_timeout     = -1;
    int keep_alive_timeout  = 60 * 1000;
};

class Client {
public:
    explicit Client(const ClientParams &params)
        : params(params)
    { }

    coke::Task<std::pair<int,int>> call(CommandBuilder &b);

private:
    ClientParams params;
};

} // namespace remote

#endif // REMOTE_CLIENT_H
