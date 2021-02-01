cmake_minimum_required(VERSION 2.8)

project(CppPhoenixChannels)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_VERBOSE_MAKEFILE ON)
set(HEADERS channel.h socket.h push.h phoenix.h)
set(SOURCES channel.cpp  socket.cpp  push.cpp channel_message.cpp examples/main.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "./websocketpp")
FIND_PACKAGE(OpenSSL)

link_directories("/opt/boost_1_72_0/build/lib")
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} LINK_PUBLIC boost_date_time boost_system -lpthread )
TARGET_LINK_LIBRARIES (${PROJECT_NAME} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})

