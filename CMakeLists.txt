cmake_minimum_required(VERSION 2.8)

project(CppPhoenixChannels)
set (CMAKE_CXX_STANDARD 11)
set(HEADERS channel.h socket.h push.h phoenix.h)
set(SOURCES channel.cpp  socket.cpp  push.cpp channel_message.cpp main.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "./websocketpp")
FIND_PACKAGE( Boost COMPONENTS thread system REQUIRED )
FIND_PACKAGE(OpenSSL)
TARGET_LINK_LIBRARIES( ${PROJECT_NAME} LINK_PUBLIC ${Boost_LIBRARIES} )
TARGET_LINK_LIBRARIES (${PROJECT_NAME} ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})

