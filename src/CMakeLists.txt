# ZxFS
cmake_minimum_required(VERSION 3.25)

# Project Name
project(Zut_ZxFS)

# Export Symbols
if(BUILD_SHARED_LIBS)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

# Set Source File Path
list(APPEND "${PROJECT_NAME}_SRC_FILE" 
    "src/ZxFS/ZxFS.cpp")

# Set Library
add_library("${PROJECT_NAME}" "${${PROJECT_NAME}_SRC_FILE}")
add_library(Zut::ZxFS ALIAS "${PROJECT_NAME}")
target_include_directories("${PROJECT_NAME}" PUBLIC "include")
target_compile_features("${PROJECT_NAME}" PRIVATE cxx_std_23)
target_compile_options("${PROJECT_NAME}" PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# Warning
if(MSVC)
    target_compile_options("${PROJECT_NAME}" PRIVATE /W4)
else()
    target_compile_options("${PROJECT_NAME}" PRIVATE -Wall -Wextra)
endif()
