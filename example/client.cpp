#include <iostream>

#include "remote/client.h"
#include "coke/coke.h"

using Arg = remote::Arg;

coke::Task<void> set_value(remote::Client &cli) {
    remote::CommandBuilder m;

    m.remote("kv/set", "a", "2000");
    m.remote("kv/set", "b", "25");

    auto [state, error] = co_await cli.call(m);

    if (state != coke::STATE_SUCCESS) {
        std::cerr << "Error: " << state << ' ' << error << std::endl;
    }
    else {
        std::cout << "Set value success" << std::endl;
    }

    co_return;
}

coke::Task<void> add_value(remote::Client &cli) {
    remote::CommandBuilder m;

    Arg arg_sum = m.remote("kedixa/add",
        m.remote("kedixa/to_int", m.remote("kv/get", "a")),
        m.remote("kedixa/to_int", m.remote("kv/get", "b"))
    );

    m.remote("kv/set", "sum", m.remote("kedixa/to_string", arg_sum));
    m.set_return_args(arg_sum);

    auto [state, error] = co_await cli.call(m);

    if (state != coke::STATE_SUCCESS) {
        std::cerr << "Error: " << state << ' ' << error << std::endl;
    }
    else {
        int sum = m.get_return_value<int>(arg_sum);
        std::cout << "Add value success, sum is " << sum << std::endl;
    }

    co_return;
}

coke::Task<void> append(remote::Client &cli) {
    remote::CommandBuilder m;

    Arg arg_ref = m.arg("the sum is ");
    m.remote("kedixa/append", arg_ref, m.remote("kv/get", "sum"));
    m.set_return_args(arg_ref);

    auto [state, error] = co_await cli.call(m);

    if (state != coke::STATE_SUCCESS) {
        std::cerr << "Error: " << state << ' ' << error << std::endl;
    }
    else {
        auto ref = m.get_return_value<std::string>(arg_ref);
        std::cout << "Append success, ref " << std::quoted(ref) << std::endl;
    }
}

coke::Task<void> no_param(remote::Client &cli) {
    remote::CommandBuilder m;

    Arg arg_id = m.remote("kedixa/next_id");
    m.set_return_args(arg_id);

    auto [state, error] = co_await cli.call(m);

    if (state != coke::STATE_SUCCESS) {
        std::cerr << "Error: " << state << ' ' << error << std::endl;
    }
    else {
        std::size_t id = m.get_return_value<std::size_t>(arg_id);
        std::cout << "next id is " << id << std::endl;
    }
}

coke::Task<void> loop(remote::Client &cli) {
    remote::CommandBuilder m;

    Arg arg_a = m.arg(0);
    Arg arg_b = m.arg(0);
    Arg arg_c = m.arg(11);

    Arg flag = m.remote("kedixa/integer_less", arg_a, arg_c);
    m.remote_while(flag, [&] {
        arg_a = m.remote("kedixa/add", arg_a, 2);
        flag = m.remote("kedixa/integer_less", arg_a, arg_c);
    });

    m.remote_while([&] {
        return m.remote("kedixa/integer_less", arg_b, arg_c);
    }, [&] {
        arg_b = m.remote("kedixa/add", arg_b, 5);
    });

    m.set_return_args(arg_a, arg_b);

    auto [state, error] = co_await cli.call(m);
    if (state != coke::STATE_SUCCESS) {
        std::cerr << "Error: " << state << ' ' << error << std::endl;
    }
    else {
        int a = m.get_return_value<int>(arg_a);
        int b = m.get_return_value<int>(arg_b);
        std::cout << "loop success, a = " << a << ", b = " << b << std::endl;
    }
}

coke::Task<void> call_remote(remote::Client &cli) {
    co_await set_value(cli);
    co_await add_value(cli);
    co_await append(cli);

    co_await no_param(cli);
    co_await no_param(cli);

    co_await loop(cli);
}

int main() {
    remote::ClientParams params {
        .host = "127.0.0.1",
    };

    remote::Client cli(params);

    coke::sync_wait(call_remote(cli));

    return 0;
}
