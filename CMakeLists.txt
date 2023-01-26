cmake_minimum_required(VERSION 3.10)

project(pl_discord_bot VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 23)
add_compile_options("-Wno-unknown-pragmas")

set(LIBPL_ENABLE_CLI ON)
set(LIBPL_SHARED_LIBRARY ON)
add_subdirectory(external/dpp)
add_subdirectory(external/pattern_language)

add_subdirectory(main)
