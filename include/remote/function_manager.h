#ifndef REMOTE_FUNCTION_MANAGER_H
#define REMOTE_FUNCTION_MANAGER_H

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <tuple>
#include <type_traits>
#include <unordered_map>

#include "remote/common.h"

namespace remote {

template<typename... Args>
using remove_cvref_tuple_t = std::tuple<std::remove_cvref_t<Args> ...>;

template<typename T>
constexpr bool is_non_const_lvalue_ref_v = std::is_lvalue_reference_v<T> &&
                                !std::is_const_v<std::remove_reference_t<T>>;


class FunctionManager {
public:
    using DataMap = std::map<ArgID, std::string>;
    using ArgList = std::vector<ArgID>;
    using Function = std::function<std::string(DataMap &data, const ArgList &args)>;

private:
    template<size_t... I, typename... Args>
    static void parse_to_tuple(std::tuple<Args...> &tp,
                               const std::vector<std::string_view> &args,
                               std::index_sequence<I...>)
    {
        auto parse = [&] <typename U> (U &arg, std::string_view str) {
            auto handle = msgpack::unpack(str.data(), str.size());
            handle.get().convert(arg);
        };

        (parse(std::get<I>(tp), args[I]), ...);
    }

    template<size_t... I, typename... Args>
    static void save_ref(std::tuple<Args...> &tp, DataMap &data,
                         const std::array<bool, sizeof...(I)> &is_ref,
                         const ArgList &arg_list, std::index_sequence<I...>)
    {
        auto save = [&] <typename U> (const U &arg, std::size_t i) {
            if (is_ref[i]) {
                std::string str;
                PackStream stream(str);
                msgpack::pack(stream, arg);
                data[arg_list[i]] = std::move(str);
            }
        };

        (save(std::get<I>(tp), I), ...);
    }


    template<typename R, typename... Args>
    static std::string call_func(const std::function<R(Args...)> &func,
                                 DataMap &data, const ArgList &arg_list)
    {
        using Tuple = remove_cvref_tuple_t<Args...>;

        constexpr std::size_t arg_size = sizeof...(Args);
        constexpr auto index_seq = std::make_index_sequence<arg_size>();
        constexpr std::array<bool, arg_size> is_ref{
            is_non_const_lvalue_ref_v<Args>...
        };

        std::vector<std::string_view> args;
        std::string result_str;
        Tuple tp{};

        for (auto id : arg_list)
            args.push_back(data[id]);

        if constexpr (arg_size > 0)
            parse_to_tuple(tp, args, index_seq);

        if constexpr (std::is_void_v<R>) {
            std::apply(func, tp);
        }
        else {
            R result = std::apply(func, tp);
            PackStream stream(result_str);

            msgpack::pack(stream, result);
        }

        save_ref(tp, data, is_ref, arg_list, index_seq);

        return result_str;
    }

    bool test(const std::string &value) {
        auto handle = msgpack::unpack(value.data(), value.size());
        return handle.get().as<bool>();
    }

public:
    FunctionManager() = default;

    template<typename R, typename... Args>
    bool add(const std::string &name, std::function<R(Args...)> func) {
        Function proc_func = [func](DataMap &data, const ArgList &args) {
            return call_func(func, data, args);
        };

        return func_map.try_emplace(name, std::move(proc_func)).second;
    }

    template<typename R, typename... Args>
    bool add(const std::string &name, R(*func)(Args...)) {
        return add(name, std::function<R(Args...)>(func));
    }

    bool erase(const std::string &name) {
        return func_map.erase(name) == 1;
    }

    void invoke(DataMap &data, const std::vector<Command> &cmds) {
        std::size_t x = 0;
        std::size_t instructions = 0;
        std::size_t max_instructions = 100;

        while (instructions < max_instructions) {
            ++instructions;

            if (x >= cmds.size())
                break;

            const Command &cmd = cmds[x];

            switch (cmd.type) {
            case CMD_INVOKE:
            {
                auto it = func_map.find(cmd.name);
                if (it == func_map.end())
                    throw std::runtime_error("function not found");

                std::string s = (it->second)(data, cmd.arg_ids);
                data[cmd.ret_id] = std::move(s);
                ++x;
                break;
            }

            case CMD_RETURN:
                break;

            case CMD_JUMP:
                x = cmd.label;
                break;

            case CMD_JUMP_TRUE:
                if (test(data[cmd.arg_ids[0]]))
                    x = cmd.label;
                else
                    ++x;
                break;

            case CMD_JUMP_FALSE:
                if (test(data[cmd.arg_ids[0]]))
                    ++x;
                else
                    x = cmd.label;
                break;

            default:
                //break;
                throw std::runtime_error("unknown command type");
            }
        }
    }

private:
    std::unordered_map<std::string, Function> func_map;
};

} // namespace remote

#endif // REMOTE_FUNCTION_MANAGER_H
