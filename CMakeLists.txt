cmake_minimum_required(VERSION 3.19)
project(LogAnalyzer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Concurrent)

qt_standard_project_setup()

qt_add_executable(LogAnalyzer
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    LogEntry.h
)

target_link_libraries(LogAnalyzer
    PRIVATE
        Qt::Core
        Qt::Widgets
        Qt::Concurrent
)

install(TARGETS LogAnalyzer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET LogAnalyzer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
