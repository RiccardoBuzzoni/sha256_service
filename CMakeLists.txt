cmake_minimum_required(VERSION 3.10)
project(SHA256Service C)

set(CMAKE_C_STANDARD 99)

find_package(OpenSSL REQUIRED)
if (NOT OPENSSL_FOUND)
    message(FATAL_ERROR "OpenSSL non trovata. Installa libssl-dev o equivalente.")
endif()

include_directories(include ${OPENSSL_INCLUDE_DIR})
link_libraries(${OPENSSL_LIBRARIES})

add_executable(server src/server.c src/utils.c)
add_executable(client src/client.c src/utils.c)
add_executable(admin src/admin_client.c src/utils.c)

target_link_libraries(server pthread)
target_link_libraries(client)
target_link_libraries(admin)