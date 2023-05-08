cmake_minimum_required(VERSION 3.23)
project(AdvanceCode)

set(CMAKE_CXX_STANDARD 17)

add_executable(AdvanceCode main.cpp threadpool/threadpool.h threadpool/sqlconnpool.h threadpool/sqlconnpool.cpp threadpool/sqlconnRAII.h buffer/buffer.h buffer/buffer.cpp log/blockqueue.h log/log.h log/log.cpp)
