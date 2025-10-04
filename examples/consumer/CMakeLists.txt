cmake_minimum_required(VERSION 3.18)
project(leo_consumer)

# Method 1: find_package (after install)
find_package(leo REQUIRED)

add_executable(my_game main.c)
target_link_libraries(my_game PRIVATE Leo::Runtime)

# Method 2: FetchContent (direct from source)
# include(FetchContent)
# FetchContent_Declare(
#     leo
#     GIT_REPOSITORY https://github.com/bluesentinelsec/leo-engine.git
#     GIT_TAG main
# )
# FetchContent_MakeAvailable(leo)
# target_link_libraries(my_game PRIVATE Leo::Runtime)
