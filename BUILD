package(default_visibility = ["//visibility:public"])

cc_library(
    name = "remote",
    srcs = [
        "src/remote_client.cpp",
    ],
    hdrs = [
        "include/remote/common.h",
        "include/remote/command_builder.h",
        "include/remote/function_manager.h",
        "include/remote/task.h",
        "include/remote/client.h",
        "include/remote/server.h",
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

# virtual target to build all binary
cc_library(
    name = "all_binary",
    deps = [
        ":client",
        ":server",
    ]
)
