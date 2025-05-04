add_library(util util.c util.h)

add_library(client client.c client.h)
target_link_libraries(client PRIVATE util)

add_executable(run_client run_client.c)
target_link_libraries(run_client PRIVATE client util)

add_library(server server.c server.h)
target_link_libraries(server PRIVATE util)

add_executable(run_server run_server.c)
target_link_libraries(run_server PRIVATE server util)
