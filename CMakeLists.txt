cmake_minimum_required(VERSION 3.25)
project(leo_consumer C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Bring in the runtime from the sibling directory
# Adjust the relative path if your layout differs
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/runtime)

add_executable(leo_consumer main.c)

# Because the leo target PUBLIC-exposes its include/, we don’t need to add includes here
target_link_libraries(leo_consumer PRIVATE Leo::Runtime)

# On Windows, copy the built DLL next to the executable so it runs immediately
if (WIN32)
  add_custom_command(TARGET leo_consumer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:leo>
            $<TARGET_FILE_DIR:leo_consumer>)
endif()
