#ifndef REMOTE_COMMAND_BUILDER_H
#define REMOTE_COMMAND_BUILDER_H

#include <cctype>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "remote/common.h"

namespace remote {

class Arg;
class CommandBuilder;

class ArgWrapper {
public:
    ~ArgWrapper() = default;

private:
    ArgWrapper(CommandBuilder *b, std::size_t cid)
        : builder(b), cmd_id(cid)
    { }

    ArgWrapper(const ArgWrapper &) = delete;
    ArgWrapper &operator=(const ArgWrapper &) = delete;

    CommandBuilder *builder;
    std::size_t cmd_id;

    friend Arg;
    friend CommandBuilder;
};

class Arg {
public:
    Arg(ArgWrapper &&w) noexcept;

    Arg(Arg &&) = default;
    Arg &operator=(const Arg &) = delete;

    Arg &operator= (ArgWrapper &&w) noexcept;

    ArgID get_id() const { return id; }

private:
    explicit Arg(ArgID id) : id(id) { }

private:
    ArgID id;

    friend CommandBuilder;
};

class CommandBuilder {
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
    CommandBuilder() = default;
    ~CommandBuilder() = default;

    CommandBuilder(const CommandBuilder &) = delete;

    template<typename U>
    Arg arg(U &&u) {
        std::string str;
        PackStream stream(str);
        ArgID id = next_id();

        msgpack::pack(stream, std::forward<U>(u));
        data[id] = std::move(str);

        return Arg(id);
    }

    template<typename... Args>
    ArgWrapper remote(const std::string &name, Args &&... args) {
        Command cmd;

        auto handle_args = [this, &cmd] <typename U> (U &&u) {
            if constexpr (std::is_same_v<std::decay_t<U>, Arg>)
                cmd.arg_ids.push_back(u.get_id());
            else if constexpr (std::is_same_v<std::decay_t<U>, ArgWrapper>)
                cmd.arg_ids.push_back(Arg(std::forward<U>(u)).get_id());
            else
                cmd.arg_ids.push_back(this->arg(std::forward<U>(u)).get_id());
        };

        if (!check_name(name)) {
            // TODO
        }

        (handle_args(std::forward<Args>(args)), ...);

        cmd.type = CMD_INVOKE;
        cmd.ret_id = INDETERMINATE_ID;
        cmd.name = name;
        cmds.push_back(std::move(cmd));

        return ArgWrapper(this, cmds.size() - 1);
    }

    template<typename WhileBody>
    void remote_while(const Arg &arg, WhileBody &&body) {
        std::size_t label_start = cmds.size();

        Command cmd1;
        cmd1.type = CMD_JUMP_FALSE;
        cmd1.arg_ids.push_back(arg.get_id());
        cmds.push_back(std::move(cmd1));

        body();

        Command cmd2;
        cmd2.type = CMD_JUMP;
        cmd2.label = label_start;

        cmds.push_back(std::move(cmd2));
        cmds[label_start].label = cmds.size();
    }

    template<typename WhileBody>
    void remote_while(const ArgWrapper &arg, WhileBody &&body) = delete;

    template<typename Condition, typename WhileBody>
        requires std::is_invocable_r_v<Arg, Condition>
    void remote_while(Condition &&cond, WhileBody &&body) {
        std::size_t cond_start = cmds.size();

        Arg arg = cond();

        std::size_t label_start = cmds.size();
        Command cmd1;
        cmd1.type = CMD_JUMP_FALSE;
        cmd1.arg_ids.push_back(arg.get_id());
        cmds.push_back(std::move(cmd1));

        body();

        Command cmd2;
        cmd2.type = CMD_JUMP;
        cmd2.label = cond_start;

        cmds.push_back(std::move(cmd2));
        cmds[label_start].label = cmds.size();
    }

    void remote_return() {
        Command cmd;
        cmd.type = CMD_RETURN;
        cmds.push_back(std::move(cmd));
    }

    void set_return_ids(const std::vector<ArgID> &rets) {
        return_ids = rets;
    }

    template<typename... Args>
    void set_return_args(const Args &... args) {
        set_return_ids({args.get_id()...});
    }

    template<typename T>
    T get_return_value(const Arg &arg) {
        return get_return_value<T>(arg.get_id());
    }

    template<typename T>
    T get_return_value(ArgID id) {
        auto it = return_data.find(id);
        if (it == return_data.end())
            throw std::runtime_error("arg not found");

        const std::string &str = it->second;
        auto handle = msgpack::unpack(str.data(), str.size());
        return handle.get().as<T>();
    }

private:
    ArgID next_id() {
        return cur_id++;
    }

    void confirm_ret_id(std::size_t cmd_id, ArgID ret_id) {
        cmds[cmd_id].ret_id = ret_id;
    }

private:
    std::map<ArgID, std::string> data;
    std::map<ArgID, std::string> return_data;
    std::vector<Command> cmds;
    std::vector<ArgID> return_ids;
    ArgID cur_id{FIRST_ID};

    friend Arg;
    friend class Client;
};

inline Arg::Arg(ArgWrapper &&w) noexcept {
    id = w.builder->next_id();
    w.builder->confirm_ret_id(w.cmd_id, id);
}

inline Arg &Arg::operator=(ArgWrapper &&w) noexcept {
    w.builder->confirm_ret_id(w.cmd_id, id);
    return *this;
}

} // namespace remote

#endif // REMOTE_COMMAND_BUILDER_H
