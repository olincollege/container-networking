# Some compilers warn that the return value of cr_assert gets converted from an
# int to a unsigned char:1. To avoid this error in those compilers, turn off
# conversion warnings for test code.
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-conversion")

list(APPEND CMAKE_CTEST_ARGUMENTS --verbose)

add_executable(test_cgroups test_cgroups.c)
target_link_libraries(test_cgroups
    PRIVATE cgroups
    PUBLIC ${CRITERION}
)
add_test(
    NAME test_cgroups
    COMMAND test_cgroups ${CRITERION_FLAGS}
)

add_executable(test_namespaces test_namespaces.c)
target_link_libraries(test_namespaces
    PRIVATE namespaces
    PUBLIC ${CRITERION}
)
target_link_options(test_namespaces
    PRIVATE
    "-Wl,--wrap=open,--wrap=dprintf,--wrap=write,--wrap=read"
)
add_test(
    NAME test_namespaces
    COMMAND test_namespaces ${CRITERION_FLAGS}
)
