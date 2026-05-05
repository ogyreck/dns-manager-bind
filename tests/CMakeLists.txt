include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Библиотека парсеров (без Qt Widgets) для тестов
add_library(dns_parsers STATIC
    ${CMAKE_SOURCE_DIR}/src/parser/NamedConfParser.cpp
    ${CMAKE_SOURCE_DIR}/src/parser/ZoneFileParser.cpp
)
target_include_directories(dns_parsers PUBLIC ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(dns_parsers Qt5::Core)

add_executable(dns_parser_tests
    main.cpp
    parser/test_named_conf_parser.cpp
    parser/test_zone_file_parser.cpp
)

target_include_directories(dns_parser_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(dns_parser_tests
    GTest::gtest
    dns_parsers
    Qt5::Core
)

# Путь к fixture-файлам передаём через define
target_compile_definitions(dns_parser_tests PRIVATE
    FIXTURES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

include(GoogleTest)
add_test(NAME ParserTests COMMAND dns_parser_tests)
