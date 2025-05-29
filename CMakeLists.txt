cmake_minimum_required(VERSION 3.19)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(FlexbarPlugin-Qt LANGUAGES CXX)

# ========================================
# Dependencies
# ========================================

# Node-API header from git submodule
set(NODE_API_DIR "extra/node-api-headers/include")

# Qt5 package
find_package(Qt5 REQUIRED COMPONENTS Core Widgets WebSockets)


# ========================================
# Targets
# ========================================

# PluginCore: where all the logic lives
add_library(PluginCore STATIC
        app.cpp
        app.h
)
target_link_libraries(PluginCore PUBLIC
        Qt::Core
        Qt::Widgets
        Qt::WebSockets
)
set_target_properties(PluginCore PROPERTIES
        AUTOMOC ON
)

# Plugin: "plugin.node" DLL file for Node.js invocation
add_library(Plugin MODULE "extra/node_entry.cpp")
target_link_libraries(Plugin PluginCore)

target_compile_definitions(Plugin PRIVATE NAPI_VERSION=3)
target_include_directories(Plugin PRIVATE ${NODE_API_DIR})
set_target_properties(Plugin PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/dist/backend>"
        OUTPUT_NAME "plugin"
        PREFIX ""
        SUFFIX ".node"
)

# PluginDebug: standalone executable for debugging
add_executable(PluginDebug "extra/debug_entry.cpp")
target_link_libraries(PluginDebug PluginCore)
set_target_properties(PluginDebug PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/dist/backend>"
)

# Create dist directories
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/dist")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/dist/backend")

# Copy extra/data/ to dist/ on core build
add_custom_command(TARGET PluginCore
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/extra/data" "${CMAKE_BINARY_DIR}/dist"
)

if (WIN32)
    # Retrieve the absolute path to qmake and then use that path to find
    # the windeployqt executable
    get_target_property(QMAKE_EXE Qt::qmake IMPORTED_LOCATION)
    get_filename_component(QT_BIN_DIR "${QMAKE_EXE}" DIRECTORY)

    find_program(WINDEPLOYQT_ENV_SETUP qtenv2.bat HINTS "${QT_BIN_DIR}")
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${QT_BIN_DIR}")

    # Run windeployqt immediately after build
    add_custom_command(TARGET Plugin
            POST_BUILD
            COMMAND "${WINDEPLOYQT_ENV_SETUP}" && "${WINDEPLOYQT_EXECUTABLE}" \"${CMAKE_BINARY_DIR}/dist/backend/$<TARGET_FILE_NAME:Plugin>\"
    )
    add_custom_command(TARGET PluginDebug
            POST_BUILD
            COMMAND "${WINDEPLOYQT_ENV_SETUP}" && "${WINDEPLOYQT_EXECUTABLE}" \"${CMAKE_BINARY_DIR}/dist/backend/$<TARGET_FILE_NAME:PluginDebug>\"
    )
endif ()
