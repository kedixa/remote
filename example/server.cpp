#include <atomic>
#include <csignal>
#include <iostream>
#include <string>
#include <map>
#include <mutex>

#include "remote/function_manager.h"
#include "remote/server.h"
#include "coke/coke.h"

std::atomic<bool> run_flag{true};

std::map<std::string, std::string> kv;
std::mutex kv_mtx;
std::atomic<std::size_t> id{0};

remote::FunctionManager fm;

void sighandler(int) {
    run_flag.store(false, std::memory_order_relaxed);
    run_flag.notify_all();
}

coke::Task<> process(remote::RemoteServerContext ctx) {
    remote::RemoteRequest &req = ctx.get_req();
    remote::RemoteResponse &resp = ctx.get_resp();
    std::string *input = req.get_value();

    std::map<remote::ArgID, std::string> data;
    std::map<remote::ArgID, std::string> return_data;
    std::vector<remote::Command> cmds;
    std::vector<remote::ArgID> return_ids;

    std::size_t off = 0;
    auto hdl = msgpack::unpack(input->data(), input->size(), off);
    hdl.get().convert(data);
    hdl = msgpack::unpack(input->data(), input->size(), off);
    hdl.get().convert(cmds);
    hdl = msgpack::unpack(input->data(), input->size(), off);
    hdl.get().convert(return_ids);

    fm.invoke(data, cmds);

    for (auto ret_id : return_ids)
        return_data[ret_id] = data[ret_id];

    std::string str;
    remote::PackStream stream(str);
    msgpack::pack(stream, return_data);
    resp.set_value(std::move(str));

    co_return;
}

void register_functions() {
    fm.add("kv/set", +[](const std::string &key, const std::string &value) {
        std::lock_guard<std::mutex> lock(kv_mtx);
        kv[key] = value;
        std::cout << "kv/set: " << key << " " << value << std::endl;
    });

    fm.add("kv/get", +[](const std::string &key) {
        std::lock_guard<std::mutex> lock(kv_mtx);
        auto it = kv.find(key);
        std::cout << "kv/get: " << key << std::endl;
        return it == kv.end() ? std::string() : it->second;
    });

    fm.add("kv/del", +[](const std::string &key) {
        std::lock_guard<std::mutex> lock(kv_mtx);
        kv.erase(key);
        std::cout << "kv/del: " << key << std::endl;
    });

    fm.add("kedixa/to_int", +[](const std::string &str) {
        std::cout << "kedixa/to_int: " << str << std::endl;
        return std::stoi(str);
    });

    fm.add("kedixa/add", +[](int a, int b) {
        std::cout << "kedixa/add: " << a << " " << b << std::endl;
        return a + b;
    });

    fm.add("kedixa/to_string", +[](int x) {
        std::cout << "kedixa/to_string: " << x << std::endl;
        return std::to_string(x);
    });

    fm.add("kedixa/append", +[](std::string &str, const std::string &append) {
        std::cout << "kedixa/append: " << std::quoted(str) << ' '
                  << std::quoted(append) << std::endl;
        str.append(append);
    });

    fm.add("kedixa/next_id", +[]() {
        std::size_t x = id.fetch_add(1, std::memory_order_relaxed);
        std::cout << "kedixa/next_id: " << x << std::endl;
        return x;
    });
}

int main() {
    signal(SIGTERM, sighandler);
    signal(SIGINT, sighandler);

    register_functions();

    remote::Server server(process);

    if (server.start(5300) == 0) {
        std::cout << "Server started" << std::endl;
        run_flag.wait(true, std::memory_order_relaxed);
        server.shutdown();
        server.wait_finish();
    }
    else {
        std::cerr << "Server start failed" << std::endl;
    }

    return 0;
}
