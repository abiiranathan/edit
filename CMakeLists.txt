cmake_minimum_required(VERSION 3.16)

project(edit VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Tell CMake to run moc when necessary
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(EXPORT_COMPILE_COMMANDS ON)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Specify the sources
set(SOURCES
    main.cpp
    highlight.cpp
    editor.cpp
)

# Create the executable
add_executable(edit ${SOURCES})

# Link the required Qt6 libraries
target_link_libraries(edit PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Install the executable
install(TARGETS edit
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Add a custom target for running CMake with Qt Creator
add_custom_target(cmake_run
    COMMAND ${CMAKE_COMMAND} ${CMAKE_SOURCE_DIR}
    DEPENDS ${CMAKE_SOURCE_DIR}/CMakeLists.txt
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running CMake for edit"
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Remove or replace GCC-specific flags for Clang
    list(REMOVE_ITEM CMAKE_CXX_FLAGS "-mno-direct-extern-access")
endif()
