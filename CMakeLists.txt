
cmake_minimum_required(VERSION 3.10)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DASIO_STANDALONE -pthread")

include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${CMAKE_SOURCE_DIR}/lib/asio/asio/include)

add_subdirectory(src)
message("SOURCE FILES ARE: ${SOURCEFILES}")
add_executable(cliente_t2_redes ${SOURCEFILES})
