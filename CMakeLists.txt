cmake_minimum_required(VERSION 3.10)
project(SHA256_IPC C)

set(CMAKE_C_STANDARD 11)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror") 

# Include directories
include_directories(include)

# Add executable
add_executable(server src/server.c src/sha256.c)
add_executable(client src/client.c src/sha256.c)
add_executable(monitor src/monitor.c)

# Link with required libraries
target_link_libraries(server m pthread)
target_link_libraries(client m pthread)
target_link_libraries(monitor)