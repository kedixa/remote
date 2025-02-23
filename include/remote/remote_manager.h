#ifndef REMOTE_REMOTE_MANAGER_H
#define REMOTE_REMOTE_MANAGER_H

#include <cctype>
#include <map>
#include <string>
#include <vector>

#include "remote/remote_common.h"

namespace remote {

class RemoteArg {
public:
    RemoteArg(RemoteArg &&) = default;

    RemoteArg(const RemoteArg &) = default;
    RemoteArg &operator=(const RemoteArg &) = delete;

    int get_id() const { return id; }

private:
    explicit RemoteArg(int id) : id(id) { }

private:
    int id;

    friend class RemoteManager;
};

struct RemoteManager {
    static bool check_name(const std::string &name) {
        if (name.empty())
            return false;

        for (char c : name) {
            if (!std::isalnum(c) && c != '_' && c != '/')
                return false;
        }

        return true;
    }

public:
    RemoteManager() = default;

    RemoteManager(const RemoteManager &) = delete;

    ~RemoteManager() = default;

    template<typename U>
    RemoteArg arg(U &&u) {
        RemoteArg a(cur_id++);
        std::string str;
        PackStream stream(str);

        msgpack::pack(stream, std::forward<U>(u));
        data[a.get_id()] = std::move(str);

        return a;
    }

    template<typename... Args>
    RemoteArg remote(const std::string &name, Args &&... args) {
        RemoteFunction func;

        auto handle_args = [this, &func] <typename U> (U &&u) {
            if constexpr (std::is_same_v<std::decay_t<U>, RemoteArg>)
                func.arg_ids.push_back(u.get_id());
            else
                func.arg_ids.push_back(arg(std::forward<U>(u)).get_id());
        };

        if (!check_name(name)) {
            // TODO
        }

        (handle_args(std::forward<Args>(args)), ...);

        RemoteArg ret(cur_id++);
        func.return_id = ret.get_id();
        func.name = name;
        funcs.push_back(std::move(func));

        return ret;
    }

    void set_return_args(const std::vector<RemoteArg> &rets) {
        return_args.clear();

        for (auto &r : rets)
            return_args.push_back(r.get_id());
    }

    template<typename T>
    T get_return_value(RemoteArg arg) {
        auto it = return_data.find(arg.get_id());
        if (it == return_data.end())
            throw std::runtime_error("arg not found");

        const std::string &str = it->second;
        auto handle = msgpack::unpack(str.data(), str.size());
        return handle.get().as<T>();
    }

private:
    std::map<int, std::string> data;
    std::map<int, std::string> return_data;
    std::vector<RemoteFunction> funcs;
    std::vector<int> return_args;
    int cur_id{0};

    friend class RemoteClient;
};

} // namespace remote

#endif //REMOTE_REMOTE_MANAGER_H
