cmake_minimum_required(VERSION 3.22)
project(chatRoomServer)

# C++标准
set(CMAKE_CXX_STANDARD 17)

# 添加可执行文件
add_executable(${PROJECT_NAME} main.cpp
        Server.cpp
        Server.h
        global.h
        DBManager.cpp
        DBManager.h
)

# 添加第三方库头文件
include_directories(ThirdParty/json)
# 指定第三方库文件的地址
link_directories(${CMAKE_SOURCE_DIR}/lib)
# jsoncpp库
set(JSON_LIBRARY ${CMAKE_SOURCE_DIR}/lib/libjsoncpp.so)
# hiredis库
set(HIREDIS_LIBRARY ${CMAKE_SOURCE_DIR}/lib/libhiredis.so)



message(STATUS "JSON_LIBRARY: ${JSON_LIBRARY}" )
message(STATUS "HIREDIS_LIBRARY: ${HIREDIS_LIBRARY}" )

# 动态库的链接
target_link_libraries(${PROJECT_NAME} PRIVATE
        sqlite3
        ${JSON_LIBRARY}
        ${HIREDIS_LIBRARY}
)

