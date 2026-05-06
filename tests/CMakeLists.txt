include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Библиотека dns/ + parser/ (без Qt Widgets) для тестов
add_library(dns_layer STATIC
    ${CMAKE_SOURCE_DIR}/src/parser/NamedConfParser.cpp
    ${CMAKE_SOURCE_DIR}/src/parser/ZoneFileParser.cpp
    ${CMAKE_SOURCE_DIR}/src/parser/ConfigWriter.cpp
    ${CMAKE_SOURCE_DIR}/src/dns/BindManager.cpp
    ${CMAKE_SOURCE_DIR}/src/dns/ConfigValidator.cpp
)
target_include_directories(dns_layer PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(dns_layer Qt5::Core)

add_executable(dns_parser_tests
    main.cpp
    parser/test_named_conf_parser.cpp
    parser/test_zone_file_parser.cpp
    dns/test_bind_manager.cpp
    dns/test_config_validator.cpp
    dns/test_config_writer.cpp
)

target_include_directories(dns_parser_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(dns_parser_tests
    GTest::gtest
    dns_layer
    Qt5::Core
)

# Путь к fixture-файлам передаём через define
target_compile_definitions(dns_parser_tests PRIVATE
    FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

include(GoogleTest)
add_test(NAME ParserTests COMMAND dns_parser_tests)
