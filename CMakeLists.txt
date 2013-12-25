project(btc-arb)
cmake_minimum_required (VERSION 2.8)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11 -fomit-frame-pointer -Wall -Wcast-align -D_WEBSOCKETPP_CPP11_STL_ -g")

find_package(Glog REQUIRED)
find_package(
  Boost 1.53
    COMPONENTS
      program_options
      system
    REQUIRED
)
find_package(JsonCpp REQUIRED)


# find_package(Websocketspp REQUIRED)

include_directories(
  ${Boost_INCLUDE_DIR}
  ${GLOG_INCLUDE_DIR}
  ${JSONCPP_INCLUDE_DIR}
)

add_executable(
  collect
    collect.cpp
)
target_link_libraries(
  collect
    ${Boost_LIBRARIES}
    ${GLOG_LIBRARY}
    pthread
    z
    leveldb
    snappy
)

add_executable(
  main
    main.cpp
    ticker_plant.hpp
    ticker_plant.cpp
    market_logger.hpp
    market_logger.cpp
    log_reporter.hpp
    log_reporter.cpp
)
target_link_libraries(
  main
    ${Boost_LIBRARIES}
    ${GLOG_LIBRARY}
    ${JSONCPP_LIBRARY}
    pthread
    z
    leveldb
    snappy
)
