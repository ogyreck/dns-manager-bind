cmake_minimum_required(VERSION 3.10)
project(dns-manager-admin VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS src/ui)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

add_compile_options(-Wall -Wextra)

set(SOURCES
    src/main.cpp
    src/ui/MainWindow.h
    src/ui/MainWindow.cpp
    src/ui/mainwindow.ui
    src/parser/NamedConfParser.h
    src/parser/NamedConfParser.cpp
    src/parser/ZoneFileParser.h
    src/parser/ZoneFileParser.cpp
    src/dns/BindManager.h
    src/dns/BindManager.cpp
    src/model/Zone.h
    src/model/ResourceRecord.h
)

add_executable(dns-manager-admin ${SOURCES})

target_include_directories(dns-manager-admin PRIVATE src)

target_link_libraries(dns-manager-admin Qt5::Widgets)

enable_testing()
add_subdirectory(tests)
