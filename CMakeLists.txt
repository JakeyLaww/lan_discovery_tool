cmake_minimum_required(VERSION 3.5)
project(lan_discovery_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(scanner
    src/main.cpp
    src/NetworkSocket.cpp
    src/DiscoveryEngine.cpp
)

target_include_directories(scanner PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Simple build type default
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
