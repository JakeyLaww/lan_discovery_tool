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
    src/discovery/MdnsNames.cpp
    src/discovery/RecordLabels.cpp
    src/discovery/DiscoveryRecord.cpp
    src/discovery/DiscoveryJson.cpp
    src/discovery/MdnsProbePlanner.cpp
    src/discovery/DeviceRegistry.cpp
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
    src/util/InterfaceInfo.cpp
    src/event_sink/StdoutEventSink.cpp
    src/event_sink/ChangeFilteredEventSink.cpp
    src/event_sink/DeviceSummaryEventSink.cpp
)

target_link_libraries(scanner PRIVATE lan_discovery lan_mdns)
target_include_directories(scanner PRIVATE ${CMAKE_SOURCE_DIR}/include)

enable_testing()

add_executable(test_parser
    tests/test_parser.cpp
)

target_link_libraries(test_parser PRIVATE lan_mdns)
target_include_directories(test_parser PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ParserTests COMMAND test_parser)

add_executable(test_interpreter
    tests/test_interpreter.cpp
)

target_link_libraries(test_interpreter PRIVATE lan_discovery lan_mdns)
target_include_directories(test_interpreter PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME InterpreterTests COMMAND test_interpreter)

add_executable(test_probe_planner
    tests/test_probe_planner.cpp
)

target_link_libraries(test_probe_planner PRIVATE lan_discovery lan_mdns)
target_include_directories(test_probe_planner PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ProbePlannerTests COMMAND test_probe_planner)

add_executable(test_device_registry
    tests/test_device_registry.cpp
)

target_link_libraries(test_device_registry PRIVATE lan_discovery lan_mdns)
target_include_directories(test_device_registry PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DeviceRegistryTests COMMAND test_device_registry)

add_executable(test_discovery_json
    tests/test_discovery_json.cpp
)

target_link_libraries(test_discovery_json PRIVATE lan_discovery lan_mdns)
target_include_directories(test_discovery_json PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME DiscoveryJsonTests COMMAND test_discovery_json)

add_executable(test_mdns_names
    tests/test_mdns_names.cpp
)

target_link_libraries(test_mdns_names PRIVATE lan_discovery lan_mdns)
target_include_directories(test_mdns_names PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME MdnsNamesTests COMMAND test_mdns_names)

add_executable(test_record_labels
    tests/test_record_labels.cpp
)

target_link_libraries(test_record_labels PRIVATE lan_discovery lan_mdns)
target_include_directories(test_record_labels PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME RecordLabelsTests COMMAND test_record_labels)
