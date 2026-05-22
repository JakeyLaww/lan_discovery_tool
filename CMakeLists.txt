cmake_minimum_required(VERSION 3.16)
project(lan_discovery_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra -Wpedantic)

set(LAN_MDNS_SOURCES
    src/mdns/Parser.cpp
    src/mdns/MdnsMessageDecoder.cpp
    src/mdns/RDataFormatter.cpp
    src/mdns/QueryBuilder.cpp
    src/util/BufferBoundsValidator.cpp
)

add_library(lan_mdns STATIC ${LAN_MDNS_SOURCES})
target_include_directories(lan_mdns PUBLIC ${CMAKE_SOURCE_DIR}/include)

set(LAN_DISCOVERY_SOURCES
    src/discovery/RecordLabels.cpp
    src/discovery/DiscoveryJson.cpp
    src/core/MdnsPacketInterpreter.cpp
    src/core/DeviceStateStore.cpp
    src/util/HexDump.cpp
)

add_library(lan_discovery STATIC ${LAN_DISCOVERY_SOURCES})
target_link_libraries(lan_discovery PUBLIC lan_mdns)
target_include_directories(lan_discovery PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_executable(scanner
    src/main.cpp
    src/NetworkSocket.cpp
    src/DiscoveryEngine.cpp
    src/event_sink/StdoutEventSink.cpp
    src/event_sink/ChangeFilteredEventSink.cpp
)

target_link_libraries(scanner PRIVATE lan_discovery lan_mdns)
target_include_directories(scanner PRIVATE ${CMAKE_SOURCE_DIR}/include)

enable_testing()

add_executable(test_parser
    tests/test_parser.cpp
)

target_link_libraries(test_parser PRIVATE lan_mdns)
target_include_directories(test_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(test_parser PRIVATE -U_NDEBUG)
endif()
add_test(NAME ParserTests COMMAND test_parser)

add_executable(test_interpreter
    tests/test_interpreter.cpp
)

target_link_libraries(test_interpreter PRIVATE lan_discovery lan_mdns)
target_include_directories(test_interpreter PRIVATE ${CMAKE_SOURCE_DIR}/include)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(test_interpreter PRIVATE -U_NDEBUG)
endif()
add_test(NAME InterpreterTests COMMAND test_interpreter)
