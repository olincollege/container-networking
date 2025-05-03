# If you have a program that's just a .c file and it has a main method, define
# an executable. If it has both a .c and .h file, make it a library. Also, link
# libraries appropriately here. Targets should be added in dependency order - if
# executable bar needs library foo, then you should do
# add_library(foo foo.c foo.h)
# add_executable(bar bar.c)
# target_link_libraries(bar PRIVATE foo)
# or similar. The PRIVATE/INTERFACE/PUBLIC keyword will depend on whether the
# library is used only in function bodies (PRIVATE), only in function
# signatures/types (INTERFACE), or both (PUBLIC).

add_library(utils utils.c utils.h)

add_library(cgroups cgroups.c cgroups.h)
target_link_libraries(cgroups PRIVATE utils)

add_library(syscalls syscalls.c syscalls.h)
target_link_libraries(syscalls PRIVATE seccomp cap)

add_library(namespaces namespaces.c namespaces.h)
target_link_libraries(namespaces PRIVATE cgroups syscalls)

add_executable(contained contained.c)
target_link_libraries(contained PRIVATE namespaces cgroups syscalls)
