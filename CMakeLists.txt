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
    src/mdns/DnsTypes.cpp
    src/util/BufferBoundsValidator.cpp
)

add_library(lan_mdns STATIC ${LAN_MDNS_SOURCES})
target_include_directories(lan_mdns PUBLIC ${CMAKE_SOURCE_DIR}/include)

set(LAN_DISCOVERY_SOURCES
    src/discovery/MdnsNames.cpp
    src/discovery/RecordLabels.cpp
    src/discovery/DiscoveryRecord.cpp
    src/discovery/DiscoveryJson.cpp
    src/discovery/MacResolver.cpp
    src/discovery/MdnsProbePlanner.cpp
    src/discovery/DeviceRegistry.cpp
    src/discovery/RecordFilter.cpp
    src/core/MdnsPacketInterpreter.cpp
    src/core/DeviceStateStore.cpp
    src/core/PacketReceiveQueue.cpp
    src/util/HexDump.cpp
)

add_library(lan_discovery STATIC ${LAN_DISCOVERY_SOURCES})
target_link_libraries(lan_discovery PUBLIC lan_mdns)
target_include_directories(lan_discovery PUBLIC ${CMAKE_SOURCE_DIR}/include)

find_package(CURL QUIET)
if(CURL_FOUND)
  set(LAN_HTTP_BACKEND "curl")
else()
  set(LAN_HTTP_BACKEND "httplib")
endif()

add_executable(scanner
    src/main.cpp
    src/NetworkSocket.cpp
    src/DiscoveryEngine.cpp
    src/util/InterfaceInfo.cpp
    src/scanner/ApiConfig.cpp
    src/event_sink/StdoutEventSink.cpp
    src/event_sink/ChangeFilteredEventSink.cpp
    src/event_sink/DeviceSummaryEventSink.cpp
    src/event_sink/CompositeEventSink.cpp
    src/event_sink/HttpEventSink.cpp
)

target_link_libraries(scanner PRIVATE lan_discovery lan_mdns)
if(LAN_HTTP_BACKEND STREQUAL "curl")
  target_link_libraries(scanner PRIVATE CURL::libcurl)
  target_compile_definitions(scanner PRIVATE LAN_HTTP_USE_CURL=1)
else()
  target_include_directories(scanner PRIVATE ${CMAKE_SOURCE_DIR}/third_party)
  target_compile_definitions(scanner PRIVATE LAN_HTTP_USE_HTTPLIB=1)
  target_link_libraries(scanner PRIVATE pthread)
endif()
target_include_directories(scanner PRIVATE ${CMAKE_SOURCE_DIR}/include)

enable_testing()
add_subdirectory(tests)
