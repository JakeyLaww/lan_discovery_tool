# Unit tests for lan_mdns / lan_discovery (scanner is built from root CMakeLists.txt).

function(lan_add_mdns_test ctest_name exe source)
    add_executable(${exe} ${source})
    target_link_libraries(${exe} PRIVATE lan_mdns)
    target_include_directories(${exe} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    add_test(NAME ${ctest_name} COMMAND ${exe})
endfunction()

function(lan_add_discovery_test ctest_name exe source)
    add_executable(${exe} ${source})
    target_link_libraries(${exe} PRIVATE lan_discovery lan_mdns)
    target_include_directories(${exe} PRIVATE ${CMAKE_SOURCE_DIR}/include)
    add_test(NAME ${ctest_name} COMMAND ${exe})
endfunction()

lan_add_mdns_test(ParserTests test_parser test_parser.cpp)
lan_add_discovery_test(InterpreterTests test_interpreter test_interpreter.cpp)
lan_add_discovery_test(ProbePlannerTests test_probe_planner test_probe_planner.cpp)
lan_add_discovery_test(DeviceRegistryTests test_device_registry test_device_registry.cpp)
lan_add_discovery_test(DiscoveryJsonTests test_discovery_json test_discovery_json.cpp)
lan_add_discovery_test(MdnsNamesTests test_mdns_names test_mdns_names.cpp)
lan_add_discovery_test(RecordLabelsTests test_record_labels test_record_labels.cpp)
lan_add_discovery_test(MacResolverTests test_mac_resolver test_mac_resolver.cpp)
lan_add_discovery_test(DeviceStateStoreTests test_device_state_store
    test_device_state_store.cpp)

add_executable(test_api_config test_api_config.cpp ../src/scanner/ApiConfig.cpp
    ../src/scanner/CliArgs.cpp)
target_include_directories(test_api_config PRIVATE ${CMAKE_SOURCE_DIR}/include)
add_test(NAME ApiConfigTests COMMAND test_api_config)
