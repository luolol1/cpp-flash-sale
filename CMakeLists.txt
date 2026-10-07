cmake_minimum_required(VERSION 3.20)

project(cpp_flash_sale VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(flash_sale
    src/main.cpp
    src/http_server.cpp
)

target_include_directories(flash_sale PRIVATE include)

if(MSVC)
    target_compile_options(flash_sale PRIVATE /W4 /permissive-)
    target_link_libraries(flash_sale PRIVATE ws2_32)
else()
    target_compile_options(flash_sale PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)

