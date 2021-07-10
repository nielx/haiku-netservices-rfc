cmake_minimum_required(VERSION 3.20)

project(HaikuNetservicesRfc)

add_subdirectory(src)

add_executable(Tests test/main.cpp)
target_link_libraries(Tests PUBLIC -lbe netservices_rfc)
