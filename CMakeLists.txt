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
    src/ui/ZoneDialog.h
    src/ui/ZoneDialog.cpp
    src/ui/RecordDialog.h
    src/ui/RecordDialog.cpp
    src/ui/SettingsDialog.h
    src/ui/SettingsDialog.cpp
    src/ui/ZoneWizard.h
    src/ui/ZoneWizard.cpp
    src/parser/NamedConfParser.h
    src/parser/NamedConfParser.cpp
    src/parser/ZoneFileParser.h
    src/parser/ZoneFileParser.cpp
    src/parser/ConfigWriter.h
    src/parser/ConfigWriter.cpp
    src/dns/BindManager.h
    src/dns/BindManager.cpp
    src/dns/ConfigValidator.h
    src/dns/ConfigValidator.cpp
    src/model/Zone.h
    src/model/ResourceRecord.h
    src/model/ServerConfig.h
)

add_executable(dns-manager-admin ${SOURCES})

target_include_directories(dns-manager-admin PRIVATE src)

target_link_libraries(dns-manager-admin Qt5::Widgets)

include(GNUInstallDirs)
install(TARGETS dns-manager-admin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_NAME "dns-manager-admin")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Графический менеджер DNS BIND9 для Astra Linux")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ogyreck")
set(CPACK_DEBIAN_PACKAGE_SECTION "net")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5widgets5 (>= 5.15), libqt5core5a (>= 5.15), bind9utils")
include(CPack)

enable_testing()
add_subdirectory(tests)
