package(default_visibility = ["//visibility:public"])

cc_library(
    name = "remote",
    srcs = [
        "src/remote_client.cpp",
    ],
    hdrs = [
        "include/remote/remote_common.h",
        "include/remote/remote_manager.h",
        "include/remote/function_manager.h",
        "include/remote/remote_task.h",
        "include/remote/remote_client.h",
        "include/remote/remote_server.h",
    ],
    includes = ["include"],
    deps = [
        "@coke//:net",
    ]
)

cc_binary(
    name = "client",
    srcs = ["example/client.cpp"],
    deps = [
        "//:remote",
        "@coke//:tools",
    ]
)

cc_binary(
    name = "server",
    srcs = ["example/server.cpp"],
    deps = [
        "//:remote",
        "@coke//:tools",
    ]
)
