project(pl_discord_bot-main)

add_executable(${PROJECT_NAME}
        source/main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE include)
target_link_libraries(${PROJECT_NAME} PRIVATE dpp libpl fmt::fmt)