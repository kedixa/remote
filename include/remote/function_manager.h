#ifndef REMOTE_FUNCTION_MANAGER_H
#define REMOTE_FUNCTION_MANAGER_H

#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <tuple>
#include <type_traits>
#include <unordered_map>

#include "remote/remote_common.h"

namespace remote {

template<typename... Args>
using remove_cvref_tuple_t = std::tuple<std::remove_cvref_t<Args> ...>;

template<typename T>
constexpr bool is_non_const_lvalue_ref_v = std::is_lvalue_reference_v<T> &&
                                !std::is_const_v<std::remove_reference_t<T>>;


class FunctionManager {
public:
    using DataMap = std::map<int, std::string>;
    using ArgList = std::vector<int>;
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

    void invoke(DataMap &data, const std::vector<RemoteFunction> &funcs) {
        for (const auto &func : funcs) {
            auto it = func_map.find(func.name);
            if (it == func_map.end())
                throw std::runtime_error("function not found");

            std::string s = (it->second)(data, func.arg_ids);
            data[func.return_id] = std::move(s);
        }
    }

private:
    std::unordered_map<std::string, Function> func_map;
};

} // namespace remote

#endif // REMOTE_CALL_FUNCTION_MANAGER_H
