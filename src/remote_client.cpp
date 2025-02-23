#include "remote/remote_client.h"
#include "remote/remote_task.h"

#include "coke/basic_awaiter.h"

namespace remote {

class RemoteAwaiter : public coke::BasicAwaiter<void> {
public:
    explicit RemoteAwaiter(RemoteTask *task) {
        task->set_callback([info = this->get_info()] (RemoteTask *task) {
            auto *awaiter = info->get_awaiter<RemoteAwaiter>();
            awaiter->done();
        });

        this->set_task(task);
    }
};

coke::Task<std::pair<int,int>>
RemoteClient::call(RemoteManager &m) {
    RemoteTask *task;
    task = create_remote_task(params.host, params.port, params.retry_max);
    task->set_send_timeout(params.send_timeout);
    task->set_receive_timeout(params.receive_timeout);
    task->set_keep_alive(params.keep_alive_timeout);

    std::string msg;
    PackStream stream(msg);

    msgpack::pack(stream, m.data);
    msgpack::pack(stream, m.funcs);
    msgpack::pack(stream, m.return_args);

    auto *req = task->get_req();
    req->set_type(0);
    req->set_value(std::move(msg));

    co_await RemoteAwaiter(task);

    int state = task->get_state();
    int error = task->get_error();

    if (state != WFT_STATE_SUCCESS)
        co_return std::make_pair(state, error);

    auto *resp = task->get_resp();
    std::string *value = resp->get_value();
    auto handle = msgpack::unpack(value->data(), value->size());

    m.return_data.clear();
    handle.get().convert(m.return_data);

    co_return std::make_pair(0, 0);
}

} // namespace remote
