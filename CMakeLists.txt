cmake_minimum_required(VERSION 3.27)
project(chatRoomServer)

# C++标准
set(CMAKE_CXX_STANDARD 17)


#添加可执行文件
add_executable(chatRoomServer main.cpp
        Server.cpp
        Server.h
        global.h
        DBManager.cpp
        DBManager.h)


#使用jsoncpp
set(jsoncpp_lib libjsoncpp.a)
target_link_libraries(chatRoomServer ${jsoncpp_lib})


#链接SQLite库
target_link_libraries(chatRoomServer sqlite3)
